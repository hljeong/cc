#include "cc.h"

#include <stdio.h>
#include <stdlib.h>

static void debug_consume(const char *s, void *ctx) {
  if (s) fprintf(stderr, "%s", s);
  else   fprintf(stderr, "\n");
}

static StrConsumer DEBUG = { .consume = debug_consume };

void _debug_f(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  consume_v(DEBUG, fmt, ap);
  va_end(ap);
  halt(DEBUG);
}

void _error_f(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  consume_v(DEBUG, fmt, ap);
  va_end(ap);
  halt(DEBUG);
  exit(1);
}

static void emit_at_loc(const StrConsumer c, void *data) {
  const char *loc = *((const char **) data);
  const int col = loc - ctx.src;
  assert_f(0 <= col && col <= ctx.src_len,
           "invalid loc: %d, src_len=%d",
            col, ctx.src_len);

  consume_f(c, "%s\n", ctx.src);

  consume_f(c, "%*s^", col, "");

  free(data);
}

static StrEmitter at_loc(const char *loc) {
  const char **loc_ptr = calloc(1, sizeof(const char *));
  *loc_ptr = loc;
  return (StrEmitter) { .emit = emit_at_loc, .data = loc_ptr };
}

void fmt_at_loc(const StrConsumer c, va_list ap) {
  const char *loc = va_arg(ap, const char *);
  consume_e(c, at_loc(loc));
}

static StrEmitter at_cur_loc() {
  return at_loc(ctx.lexer.loc);
}

void fmt_at_cur_loc(const StrConsumer c, va_list ap) {
  consume_e(c, at_cur_loc());
}

static void emit_at_span(const StrConsumer c, void *data) {
  const StringView span = *((const StringView *) data);

  const int col = span.loc - ctx.src;
  assert_f(0 <= col && col + span.len <= ctx.src_len,
           "invalid span: (%d, %d), src_len=%d",
           col, span.len, ctx.src_len);
  consume_f(c, "%s\n", ctx.src);

  consume_f(c, "%*s", col, "");
  for (int i = 0; i < span.len; i++)
    consume_f(c, "%c", '~');

  free(data);
}

static StrEmitter at_span(const StringView span) {
  StringView *span_ptr = calloc(1, sizeof(StringView));
  *span_ptr = span;
  return (StrEmitter) { .emit = emit_at_span, .data = span_ptr };
}

static StrEmitter at_tok(const Token *tok) {
  return at_span(tok->lexeme);
}

void fmt_at_tok(const StrConsumer c, va_list ap) {
  const Token *tok = va_arg(ap, const Token *);
  consume_e(c, at_tok(tok));
}

static StrEmitter at_cur_tok() {
  return at_tok(ctx.parser.tok);
}

void fmt_at_cur_tok(const StrConsumer c, va_list ap) {
  consume_e(c, at_cur_tok());
}

static StrEmitter at_node(const Node *node) {
  return at_span(node->lexeme);
}

void fmt_at_node(const StrConsumer c, va_list ap) {
  const Node *node = va_arg(ap, const Node *);
  consume_e(c, at_node(node));
}

void _assert(const char *file, const int line, const char *cond) {
  consume_f(DEBUG, "%s:%d: assert(%s) failed", file, line, cond);
  halt(DEBUG);
  exit(1);
}

void _assert_f(const char *file, const int line, const char *cond,
               const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  consume_f(DEBUG, "%s:%d: assert(%s) failed: ", file, line, cond);
  consume_v(DEBUG, fmt, ap);
  va_end(ap);
  halt(DEBUG);
  exit(1);
}

void _fail(const char *file, const int line) {
  consume_f(DEBUG, "%s:%d", file, line);
  halt(DEBUG);
  exit(1);
}

void _fail_f(const char *file, const int line,
             const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  consume_f(DEBUG, "%s:%d: failed: ", file, line);
  consume_v(DEBUG, fmt, ap);
  va_end(ap);
  halt(DEBUG);
  exit(1);
}
