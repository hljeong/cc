#include "lc.h"

#include <stdarg.h>

static int vdebug(const char *fmt, va_list ap) { return vfprint(stderr, fmt, ap); }

int debug(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  const int ret = vdebug(fmt, ap);
  va_end(ap);
  return ret;
}

static int emit_at_loc(const sink s, const char *loc) {
  const int col = loc - ctx.src;
  assert(0 <= col && col <= ctx.src_len,
         "invalid loc: %d, src_len=%d",
         col, ctx.src_len);

  int len = check(emitf(s, "%s\n", ctx.src));
  len += check(emitf(s, "%*s^", col, ""));
  return len;
}

static int fmt_at_loc(const sink s, va_list ap) {
  return emit_at_loc(s, va_arg(ap, char *));
}

static int fmt_at_cur_loc(const sink s, va_list ap) {
  return emit_at_loc(s, ctx.lexer.loc);
}

static int emit_at_span(const sink s, const str_view span) {
  const int col = span.loc - ctx.src;
  assert(0 <= col && col <= ctx.src_len,
         "invalid loc: %d, src_len=%d",
         col, ctx.src_len);

  int len = check(emitf(s, "%s\n", ctx.src));
  len += check(emitf(s, "%*s", col, ""));
  for (int i = 0; i < span.len; i++) {
    len += check(emitf(s, "%c", '~'));
  }
  len += check(emitf(s, " "));
  return len;
}

int fmt_at_tok(const sink s, va_list ap) {
  return emit_at_span(s, va_arg(ap, Token *)->lexeme);
}

int fmt_at_cur_tok(const sink s, va_list ap) {
  return emit_at_span(s, ctx.parser.tok->lexeme);
}

int fmt_at_node(const sink s, va_list ap) {
  return emit_at_span(s, va_arg(ap, Node *)->lexeme);
}

void error(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vdebug(fmt, ap);
  debug("\n");
  va_end(ap);
  exit(1);
}

void _assert(const char *file, const int line, const char *cond,
             const char *fmt, ...) {
  debug("%s:%d: assert(%s) failed", file, line, cond);
  if (fmt) {
    va_list ap;
    va_start(ap, fmt);
    debug(": ");
    vdebug(fmt, ap);
    va_end(ap);
  }
  debug("\n");

  exit(1);
}


void _fail(const char *file, const int line,
           const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  debug("%s%d: ", file, line);
  if (fmt) {
    debug(": ");
    vdebug(fmt, ap);
  }
  debug("\n");
  va_end(ap);
  exit(1);
}

int check(const int ret) {
  assert(ret >= 0, "ret=%d", ret);
  return ret;
}

void register_formatters() {
  add_formatter("@loc",         fmt_at_loc);
  add_formatter("@cur_loc",     fmt_at_cur_loc);
  add_formatter("@tok",         fmt_at_tok);
  add_formatter("@cur_tok",     fmt_at_cur_tok);
  add_formatter("@node",        fmt_at_node);
  add_formatter("token_kind",   fmt_token_kind);
  add_formatter("token",        fmt_token);
  add_formatter("token_stream", fmt_token_stream);
  add_formatter("node_kind",    fmt_node_kind);
  add_formatter("node",         fmt_node);
  add_formatter("ast",          fmt_ast);
  add_formatter("lambda",       fmt_lambda);
}
