#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Token Token;

static struct {
  const char *src;
  const char *cursor;
  const Token *tok;
} g;

static void vdebug(const char *fmt, va_list ap) {
  vfprintf(stderr, fmt, ap);
}

static void debug(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  vdebug(fmt, ap);
}

[[noreturn]] static void error(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  vdebug(fmt, ap);
  exit(1);
}

[[noreturn]] static void verror_at(const char *loc, const char * fmt, va_list ap) {
  debug("%s\n", g.src);
  const int col = loc - g.src;
  debug("%*s^ ", col, ""); vdebug(fmt, ap); debug("\n");
  exit(1);
}

[[noreturn]] static void error_at(const char *loc, const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  verror_at(loc, fmt, ap);
}

[[noreturn]] static void error_at_cursor(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  verror_at(g.cursor, fmt, ap);
}

typedef enum {
  T_OP,
  T_NUM,
  T_EOF,
} TokenKind;

static const char *token_kind_to_str(const TokenKind kind) {
  if (kind == T_OP) return "operator";
  else if (kind == T_NUM) return "number";
  else if (kind == T_EOF) return "eof";
  else error("unknown token kind: %d\n", (uint32_t) kind);
}

typedef enum {
  BINOP_ADD,
  BINOP_SUB,
} BinOp;

static const char *binop_to_str(const BinOp binop) {
  if (binop == BINOP_ADD) return "+";
  else if (binop == BINOP_SUB) return "-";
  else error("unknown binary operator kind: %d\n", (uint32_t) binop);
}

typedef struct {
  const char *loc;
  size_t len;
} StringView;

static StringView sv(const char *loc, const size_t len) {
  return (StringView) { .loc = loc, .len = len };
}

#define SV_FMT "%.*s"
#define SV_ARG(sv) (sv).len, (sv).loc

typedef union {
  BinOp binop;
  int num;
} TokenValue;

struct Token {
  TokenKind kind;
  Token *next;
  TokenValue value;
  StringView lexeme;
};

static const char *token_to_str(const Token *tok) {
  static char buf[100] = {0};
  const int off = snprintf(buf, sizeof(buf), "%s", token_kind_to_str(tok->kind));
  if      (tok->kind == T_OP)  snprintf(buf + off, sizeof(buf) - off, "(%s)", binop_to_str(tok->value.binop));
  else if (tok->kind == T_NUM) snprintf(buf + off, sizeof(buf) - off, "(%d)", tok->value.num);
  else                         ;
  return buf;
}

[[noreturn]] static void error_at_tok(const Token *tok, const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  verror_at(tok->lexeme.loc, fmt, ap);
}

[[noreturn]] static void error_at_cursor_tok(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  verror_at(g.tok->lexeme.loc, fmt, ap);
}

static Token *make_token(const TokenKind kind, const size_t len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->next = NULL;
  tok->lexeme = sv(g.cursor, len);
  return tok;
}

static Token *lex() {
  Token head = {};
  Token *cur = &head;

  char ch;
  while ((ch = *g.cursor)) {
    if (isspace(ch)) g.cursor++;

    else if (isdigit(ch)) {
      cur = (cur->next = make_token(T_NUM, 0));
      const char *start = g.cursor;
      // todo: why is this cast needed?
      cur->value.num = strtoul(g.cursor, (char **) &g.cursor, 10);
      cur->lexeme.len = g.cursor - start;
    }

    else if (ch == '+' || ch == '-') {
      cur = (cur->next = make_token(T_OP, 1));
      cur->value.binop = (ch == '+') ? BINOP_ADD : BINOP_SUB;
      g.cursor++;
    }

    else error_at_cursor("invalid token");
  }

  cur = (cur->next = make_token(T_EOF, 0));
  return head.next;
}

TokenValue consume(const TokenKind kind) {
  if (g.tok->kind != kind) {
    error_at_tok(g.tok, "expected %s, got: %s", token_kind_to_str(kind), token_to_str(g.tok));
  }

  const Token *tok = g.tok;
  g.tok = g.tok->next;
  return tok->value;
}

int main(int argc, char **argv) {
  if (argc != 2) error("usage: %s <expression>\n", argv[0]);

  g.src = argv[1];
  g.cursor = g.src;
  g.tok = lex();

  {
    const Token *tok_ = g.tok;
    debug("%s", token_to_str(tok_));
    while ((tok_ = tok_->next)) {
      debug(" %s", token_to_str(tok_));
    }
    debug("\n");
  }

  printf("  .globl main\n");
  printf("main:\n");

  printf("  mov $%d, %%rax\n", consume(T_NUM).num);
  while (g.tok->kind != T_EOF) {
    const BinOp op = consume(T_OP).binop;
    if      (op == BINOP_ADD) printf("  add $%d, %%rax\n", consume(T_NUM).num);
    else if (op == BINOP_SUB) printf("  sub $%d, %%rax\n", consume(T_NUM).num);
    else                      error_at_cursor_tok("unknown binop: %s", binop_to_str(op));
  }
  printf("  ret\n");

  return 0;
}
