#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// debug

void debugf(const char *fmt, ...);

[[noreturn]]
void _failf(const char *file, const int line,
            const char *fmt, ...);

#define failf(fmt, ...) _failf(__FILE__, __LINE__, fmt, __VA_ARGS__)
#define fail(msg) _failf(__FILE__, __LINE__, msg)

[[noreturn]]
void errorf(const char *fmt, ...);


// lexer

typedef struct {
  const char *loc;
  int len;
} StringView;

static inline StringView sv(const char *loc, const int len) {
  return (StringView) { .loc = loc, .len = len };
}

static inline int sv_eq(const StringView s, const StringView t) {
  return (s.len == t.len) && !strncmp(s.loc, t.loc, s.len);
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

const char *token_kind_to_str(const TokenKind kind);

const char *token_to_str(const Token *tok);

void debug_token_stream(const Token *tok);

Token *lex();


// parser

typedef enum {
  NodeKind_VAR,
  NodeKind_FUN,
  NodeKind_APP,
} NodeKind;

const char *node_kind_to_str(const NodeKind kind);

typedef struct Node Node;
struct Node {
  NodeKind kind;
  union {
    StringView name;               // NodeKind_VAR
    struct { Node *var, *expr; };  // NodeKind_FUN
    struct { Node *fun, *val; };   // NodeKind_APP
  };
  Node *next;
};

Node *new_fun(Node *var, Node *expr);

Node *new_app(Node *fun, Node *val);

void debug_ast(const Node *node);

void debug_unparse(const Node *node);

void print_unparse(const Node *node);

Node *parse();


// reduction

Node *step(Node *node, bool whnf);


// global context

typedef struct {
  const char *src;

  struct {
    const char *loc;
  } lexer;

  struct {
    const Token *tok;
  } parser;
} Context;

extern Context ctx;
