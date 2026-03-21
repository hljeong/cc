#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


// debug

// debug print
static void _vdebugf(const char *fmt, va_list ap) { vfprintf(stderr, fmt, ap); }

static void debugf(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  _vdebugf(fmt, ap);
}

// internal error
[[noreturn]]
static void _vfailf(const char *file, const int line,
                    const char *fmt, va_list ap) {
  debugf("%s:%d: ", file, line);
  _vdebugf(fmt, ap); debugf("\n");
  exit(1);
}

[[noreturn]]
void _failf(const char *file, const int line,
            const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  _vfailf(file, line, fmt, ap);
}

#define failf(fmt, ...) _failf(__FILE__, __LINE__, fmt, __VA_ARGS__)
#define fail(msg) _failf(__FILE__, __LINE__, msg)

// compile error
[[noreturn]]
static void errorf(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  _vdebugf(fmt, ap); debugf("\n");
  exit(1);
}


// lexer

typedef struct {
  const char *loc;
  int len;
} StringView;

static StringView sv(const char *loc, const int len) {
  return (StringView) { .loc = loc, .len = len };
}

#define sv_fmt "%.*s"
#define sv_arg(sv) (sv).len, (sv).loc

typedef enum {
  TokenKind_IDENT,
  TokenKind_BACKSLASH,
  TokenKind_DOT,
  TokenKind_LPAREN,
  TokenKind_RPAREN,
  TokenKind_EOF,
} TokenKind;

typedef struct Token Token;
struct Token {
  TokenKind kind;
  Token *next;
  StringView lexeme;
};

const char *token_kind_to_str(const TokenKind kind) {
  if      (kind == TokenKind_IDENT)     return "ident";
  else if (kind == TokenKind_BACKSLASH) return "\\";
  else if (kind == TokenKind_DOT)       return ".";
  else if (kind == TokenKind_LPAREN)    return "(";
  else if (kind == TokenKind_RPAREN)    return ")";
  else if (kind == TokenKind_EOF)       return "eof";
  else                                  failf("not implemented: %u",
                                              (uint32_t) kind);
}

const char *token_to_str(const Token *tok) {
  static char buf[256] = {0};
  const int off = snprintf(buf, sizeof(buf), "%s", token_kind_to_str(tok->kind));
  if (tok->kind == TokenKind_IDENT) {
    snprintf(buf + off, sizeof(buf) - off, "("sv_fmt")", sv_arg(tok->lexeme));
  }
  return buf;
}

static void debug_token_stream(const Token *tok) {
  debugf("tokens: %s", token_to_str(tok));
  while ((tok = tok->next)) {
    debugf(" %s", token_to_str(tok));
  }
  debugf("\n");
}

// would've preferred `bool (*pred)(const char)` but `isspace()`, etc. are int (*)(int)
static int consume_pred(int (*pred)(int));

static Token *new_token(const TokenKind kind, const int len);

static Token *lex();


// global context

struct {
  const char *src;

  struct {
    const char *loc;
  } lexer;
} ctx;


// implementation

// consume_*(): if current loc matches,
// - advance current loc by len matched and
// - return len matched,
// otherwise return 0
static int consume_pred(int (*pred)(int)) {
  if (pred('\0')) fail("predicate accepts eof");
  const char *start = ctx.lexer.loc;
  while (pred(*ctx.lexer.loc)) ctx.lexer.loc++;
  return ctx.lexer.loc - start;
}

// identifiers: (letter or underscore) followed by
//              0 or more (letter or digit or underscore)
static int is_ident_first(int c) {
  return isalpha(c) || (c == '_');
}

static int is_ident_rest(int c) {
  return isalnum(c) || (c == '_');
}

static int consume_ident() {
  int len = 0;
  if (!(len = consume_pred(is_ident_first))) return 0;
  return (len += consume_pred(is_ident_rest));
}

static int consume_ch(const char c) {
  if (*ctx.lexer.loc == c) {
    ctx.lexer.loc++; return 1;
  }
  return 0;
}

static Token *new_token(const TokenKind kind, const int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->lexeme = sv(ctx.lexer.loc - len, len);
  return tok;
}

Token *lex() {
  Token head = {};
  Token *cur = &head;

  int len = 0;
  char ch = '\0';
  while ((ch = *ctx.lexer.loc)) {
    if      ((len = consume_pred(isspace))) ;  // skip whitespace
    else if ((len = consume_ident()))       cur = (cur->next = new_token(TokenKind_IDENT, len));
    else if ((len = consume_ch('\\')))      cur = (cur->next = new_token(TokenKind_BACKSLASH, len));
    else if ((len = consume_ch('.')))       cur = (cur->next = new_token(TokenKind_DOT,       len));
    else if ((len = consume_ch('(')))       cur = (cur->next = new_token(TokenKind_LPAREN,    len));
    else if ((len = consume_ch(')')))       cur = (cur->next = new_token(TokenKind_RPAREN,    len));
    else                                    errorf("invalid token");  // todo: error context
  }

  cur = (cur->next = new_token(TokenKind_EOF, 0));
  return head.next;
}


// entry point

int main(int argc, char **argv ) {
  if (argc != 2) errorf("usage: %s <expression>", argv[0]);

  ctx.src = argv[1];

  debugf("src: %s\n", ctx.lexer.loc = ctx.src);

  debug_token_stream(lex());
}
