#include "cc.h"

#include <stdio.h>
#include <stdlib.h>

static void print_consume(const char *s, void *ctx) {
  assert(s);
  printf("%s", s);
}

static StrConsumer PRINT = { .consume = print_consume };

void _print(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  consume_v(PRINT, fmt, ap);
  consume_f(PRINT, "\n");
  va_end(ap);
}

static void debug_consume(const char *s, void *ctx) {
  assert(s);
  fprintf(stderr, "%s", s);
}

static StrConsumer DEBUG = { .consume = debug_consume };

void _debug(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  consume_v(DEBUG, fmt, ap);
  consume_f(DEBUG, "\n");
  va_end(ap);
}

void _error(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  consume_v(DEBUG, fmt, ap);
  consume_f(DEBUG, "\n");
  va_end(ap);
  exit(1);
}

static void consume_at_loc(const StrConsumer c, const char *loc) {
  const int col = loc - ctx.src.loc;
  assert(0 <= col && col <= ctx.src.len,
         "invalid loc: %d, src.len=%d",
         col, ctx.src.len);

  consume_f(c, "%s\n", ctx.src);
  consume_f(c, "%*s^", col, "");
}

void fmt_at_loc(const StrConsumer c, va_list ap) {
  consume_at_loc(c, va_arg(ap, const char *));
}

void fmt_at_cur_loc(const StrConsumer c, va_list ap) {
  consume_at_loc(c, ctx.lexer.loc);
}

static void consume_at_span(const StrConsumer c, const StringView span) {
  const int col = span.loc - ctx.src.loc;
  assert(0 <= col && col + span.len <= ctx.src.len,
         "invalid span: (%d, %d), src.len=%d",
         col, span.len, ctx.src.len);

  consume_f(c, "%s\n", ctx.src);
  consume_f(c, "%*s", col, "");
  for (int i = 0; i < span.len; i++)
    consume_f(c, "%c", '~');
}

void fmt_at_tok(const StrConsumer c, va_list ap) {
  const Token *tok = va_arg(ap, const Token *);
  consume_at_span(c, tok->lexeme);
}

void fmt_at_cur_tok(const StrConsumer c, va_list ap) {
  consume_at_span(c, ctx.parser.tok->lexeme);
}

void fmt_at_node(const StrConsumer c, va_list ap) {
  const Node *node = va_arg(ap, const Node *);
  consume_at_span(c, node->lexeme);
}

void _assert(const char *file, const int line, const char *func, const char *cond,
             const char *fmt,...) {
  va_list ap; va_start(ap, fmt);

  consume_f(DEBUG, "%s:%d: assert(%s) failed in %s()", file, line, cond, func);
  if (fmt) {
    consume_f(DEBUG, ": ");
    consume_v(DEBUG, fmt, ap);
  }
  consume_f(DEBUG, "\n");

  va_end(ap);
  exit(1);
}

void _fail(const char *file, const int line, const char *func,
           const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);

  consume_f(DEBUG, "%s:%d: failed in %s()", file, line, func);
  if (fmt) {
    consume_f(DEBUG, ": ");
    consume_v(DEBUG, fmt, ap);
  }
  consume_f(DEBUG, "\n");

  va_end(ap);
  exit(1);
}
