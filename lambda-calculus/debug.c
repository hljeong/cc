#include "lc.h"

#include <stdarg.h>

static void vdebugf(const char *fmt, va_list ap) { vfprintf(stderr, fmt, ap); }

void debugf(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  vdebugf(fmt, ap);
}

static void vdebugf_at(const char *loc, const char *fmt, va_list ap) {
  const int col = loc - ctx.src;
  if (0 <= col && col <= ctx.src_len) {
    debugf("%s\n", ctx.src);
    debugf("%*s^ ", col, "");
  }
  vdebugf(fmt, ap); debugf("\n");
}

void debugf_loc(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  vdebugf_at(ctx.lexer.loc, fmt, ap);
}

void debugf_tok(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  vdebugf_at(ctx.parser.tok->lexeme.loc, fmt, ap);
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

void errorf_loc(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  vdebugf_at(ctx.lexer.loc, fmt, ap);
  exit(1);
}

void errorf_tok(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  vdebugf_at(ctx.parser.tok->lexeme.loc, fmt, ap);
  exit(1);
}
