#include "lc.h"

#include <stdarg.h>

static void vdebugf(const char *fmt, va_list ap) { vfprintf(stderr, fmt, ap); }

void debugf(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  vdebugf(fmt, ap);
}

static void vdebugf_at(const char *loc, const char *fmt, va_list ap) {
  const int col = loc - ctx.src;
  if (!(0 <= col && col <= ctx.src_len))
    failf("invalid loc: %d, src_len=%d",
          col, ctx.src_len);

  debugf("%s\n", ctx.src);
  debugf("%*s^ ", col, "");
  vdebugf(fmt, ap); debugf("\n");
}

static void vdebugf_span(const StringView span, const char *fmt, va_list ap) {
  const int col = span.loc - ctx.src;
  if (!(0 <= col && col <= ctx.src_len))
    failf("invalid loc: %d, src_len=%d",
          col, ctx.src_len);

  debugf("%s\n", ctx.src);
  debugf("%*s^", col, "");
  for (int i = 0; i < span.len - 1; i++)
    debugf("%c", '~');
  debugf(" ");
  vdebugf(fmt, ap); debugf("\n");
}

void debugf_span(const StringView span, const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  vdebugf_span(span, fmt, ap);
}

void debugf_at_loc(const char *loc, const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  vdebugf_at(loc, fmt, ap);
}

void debugf_loc(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  vdebugf_at(ctx.lexer.loc, fmt, ap);
}

void debugf_at_tok(const Token *tok, const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  vdebugf_span(tok->lexeme, fmt, ap);
}

void debugf_tok(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  vdebugf_span(ctx.parser.tok->lexeme, fmt, ap);
}

void debugf_at_node(const Node *node, const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  vdebugf_span(node->lexeme, fmt, ap);
}

[[noreturn]]
static void _vfailf(const char *file, const int line,
                    const char *fmt, va_list ap) {
  debugf("%s:%d: ", file, line);
  vdebugf(fmt, ap); debugf("\n");
  exit(1);
}

void _failf(const char *file, const int line,
            const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  _vfailf(file, line, fmt, ap);
}

void errorf(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  vdebugf(fmt, ap); debugf("\n");
  exit(1);
}

void errorf_at_loc(const char *loc, const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  vdebugf_at(loc, fmt, ap);
  exit(1);
}

void errorf_loc(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  vdebugf_at(ctx.lexer.loc, fmt, ap);
  exit(1);
}

void errorf_at_tok(const Token *tok, const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  vdebugf_span(tok->lexeme, fmt, ap);
  exit(1);
}

void errorf_tok(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  vdebugf_span(ctx.parser.tok->lexeme, fmt, ap);
  exit(1);
}

void errorf_at_node(const Node *node, const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  vdebugf_span(node->lexeme, fmt, ap);
  exit(1);
}
