#include "cc.h"

static void debugv(const char *fmt, va_list ap) {
  vfprintf(stderr, fmt, ap);
}

void debugf(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  debugv(fmt, ap);
  va_end(ap);
}

void errorf(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  debugv(fmt, ap);
  va_end(ap);
  exit(1);
}

static void debug_consume(const char *s, void *ctx) {
  if (s) debugf("%s", s);
  else   debugf("\n");
}

static StrConsumer DEBUG = { .consume = debug_consume };

void _debug(void *_, ...) {
  va_list ap; va_start(ap, _);
  emit_all_v(DEBUG, ap);
  va_end(ap);
}

void _debug_f(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  emit_v2(DEBUG, fmt, ap);
  va_end(ap);
  emit_e(DEBUG, HALT);
}

void _error(void *_, ...) {
  va_list ap; va_start(ap, _);
  emit_all_v(DEBUG, ap);
  va_end(ap);
  exit(1);
}

void _error_f(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  emit_v2(DEBUG, fmt, ap);
  va_end(ap);
  emit_e(DEBUG, HALT);
  exit(1);
}

static void append_consume(const char *s, void *ctx) {
  if (!s) return;
  StringBuilder *sb = (StringBuilder *) ctx;
  sb_append_f(sb, s);
}

static StrConsumer SB_APPEND(StringBuilder *sb) {
  return (StrConsumer) { .consume = append_consume, .ctx = sb };
}

void _sb_append(StringBuilder *sb, ...) {
  va_list ap; va_start(ap, sb);
  emit_all_v(SB_APPEND(sb), ap);
  va_end(ap);
}

static void emit_at_loc(const StrConsumer c, void *data) {
  const char *loc = *((const char **) data);
  const int col = loc - ctx.src;
  assert_f(0 <= col && col <= ctx.src_len,
           "invalid loc: %d, src_len=%d",
            col, ctx.src_len);

  emit_f(c, "%s\n", ctx.src);

  emit_f(c, "%*s^", col, "");

  free(data);
}

StrEmitter at_loc(const char *loc) {
  const char **loc_ptr = calloc(1, sizeof(const char *));
  *loc_ptr = loc;
  return (StrEmitter) { .emit = emit_at_loc, .data = loc_ptr };
}

StrEmitter this_loc() {
  return at_loc(ctx.lexer.loc);
}

static void emit_at_span(const StrConsumer c, void *data) {
  const StringView span = *((const StringView *) data);

  const int col = span.loc - ctx.src;
  assert_f(0 <= col && col + span.len <= ctx.src_len,
           "invalid span: (%d, %d), src_len=%d",
           col, span.len, ctx.src_len);
  emit_f(c, "%s\n", ctx.src);

  emit_f(c, "%*s", col, "");
  for (int i = 0; i < span.len; i++)
    emit_s(c, "~");

  free(data);
}

static StrEmitter at_span(const StringView span) {
  StringView *span_ptr = calloc(1, sizeof(StringView));
  *span_ptr = span;
  return (StrEmitter) { .emit = emit_at_span, .data = span_ptr };
}

StrEmitter at_tok(const Token *tok) {
  return at_span(tok->lexeme);
}

StrEmitter this_tok() {
  return at_tok(ctx.parser.tok);
}

StrEmitter at_node(const Node *node) {
  return at_span(node->lexeme);
}

void _assert(const char *file, const int line, const char *cond) {
  emit_f2(DEBUG, "%s:%d: assert(%s) failed", file, line, cond);
  emit_e(DEBUG, HALT);
  exit(1);
}

void _assert_f(const char *file, const int line, const char *cond,
               const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  emit_f2(DEBUG, "%s:%d: assert(%s) failed: ", file, line, cond);
  emit_v2(DEBUG, fmt, ap);
  va_end(ap);
  emit_e(DEBUG, HALT);
  exit(1);
}

void _fail(const char *file, const int line) {
  emit_f(DEBUG, "%s:%d", file, line);
  emit_e(DEBUG, HALT);
  exit(1);
}

void _fail_f(const char *file, const int line,
             const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  emit_f2(DEBUG, "%s:%d: failed: ", file, line);
  emit_v2(DEBUG, fmt, ap);
  va_end(ap);
  emit_e(DEBUG, HALT);
  exit(1);
}
