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

Consumer DEBUG = { .consume = consume_debug };
Consumer ERROR = { .consume = consume_error };

void *consume_debug(void *arg, void *ctx) {
  if (arg) {
    debugf("%s", *((const char **) arg));
    return NULL;
  }

  else {
    debugf("\n");
    return NULL;
  }
}

void *consume_error(void *arg, void *ctx) {
  consume_debug(arg, ctx);
  if (!arg) exit(1);
  return NULL;
}

void *consume_append(void *arg, void *ctx) {
  if (!arg) return NULL;

  StringBuilder *sb = (StringBuilder *) ctx;
  sb_appendf(sb, *((const char **) arg));
  return NULL;
}

static void at_locv(const Consumer consumer, const char *loc, const char *fmt, va_list ap) {
  const int col = loc - ctx.src;
  assertf(0 <= col && col <= ctx.src_len,
          "invalid loc: %d, src_len=%d",
          col, ctx.src_len);

  emit(consumer, &ctx.src);

  StringBuilder sb = sb_create(256);
  sb_appendf(&sb, "\n%*s^ ", col, "");
  sb_appendv(&sb, fmt, ap);
  emit(consumer, &sb.buf);
  sb_free(&sb);

  emit(consumer, NULL);
}

void at_loc(const Consumer consumer, const char *loc, const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  at_locv(consumer, loc, fmt, ap);
  va_end(ap);
}

void this_loc(const Consumer consumer, const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  at_locv(consumer, ctx.lexer.loc, fmt, ap);
  va_end(ap);
}

static void at_spanv(const Consumer consumer, const StringView span, const char *fmt, va_list ap) {
  const int col = span.loc - ctx.src;
  assertf(0 <= col && col + span.len <= ctx.src_len,
          "invalid span: (%d, %d), src_len=%d",
          col, span.len, ctx.src_len);


  emit(consumer, &ctx.src);

  StringBuilder sb = sb_create(256);
  sb_appendf(&sb, "\n%*s", col, "");
  for (int i = 0; i < span.len; i++) sb_appendf(&sb, "%c", '~');
  sb_appendf(&sb, " ");
  sb_appendv(&sb, fmt, ap);
  emit(consumer, &sb.buf);
  sb_free(&sb);

  emit(consumer, NULL);
}

void at_tok(const Consumer consumer, const Token *tok, const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  at_spanv(consumer, tok->lexeme, fmt, ap);
  va_end(ap);
}

void this_tok(const Consumer consumer, const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  at_spanv(consumer, ctx.parser.tok->lexeme, fmt, ap);
  va_end(ap);
}

void at_node(const Consumer consumer, const Node *node, const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  at_spanv(consumer, node->lexeme, fmt, ap);
  va_end(ap);
}

void _assert(const char *file, const int line, const char *cond) {
  debugf("%s:%d: assert(%s) failed\n", file, line, cond);
  exit(2);
}

void _assertf(const char *file, const int line, const char *cond,
              const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  debugf("%s:%d: assert(%s) failed: ", file, line, cond);
  debugv(fmt, ap);
  debugf("\n");
  exit(2);
}

void _failf(const char *file, const int line,
            const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  debugf("%s:%d: ", file, line);
  debugv(fmt, ap);
  debugf("\n");
  exit(2);
}
