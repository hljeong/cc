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
