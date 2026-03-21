#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>


// debug

// debug print
void debugf(const char *fmt, ...);

// internal assertion
[[noreturn]]
void _assert(const char *file, const int line, const char *cond);

[[noreturn]]
void _assertf(const char *file, const int line,
              const char *cond, const char *fmt, ...);

#define assert(cond)                      \
  do {                                    \
    if (!(cond))                          \
      _assert(__FILE__, __LINE__, #cond); \
  } while (0)

#define assertm(cond, msg)         \
  do {                             \
    if (!(cond))                   \
      _assertf(__FILE__, __LINE__, \
               #cond, msg);        \
  } while (0)

#define assertf(cond, fmt, ...)          \
  do {                                   \
    if (!(cond))                         \
      _assertf(__FILE__, __LINE__,       \
               #cond, fmt, __VA_ARGS__); \
  } while (0)

// internal failure
[[noreturn]]
void _failf(const char *file, const int line,
            const char *fmt, ...);

#define failf(fmt, ...) _failf(__FILE__, __LINE__, fmt, __VA_ARGS__)

#define fail(msg) _failf(__FILE__, __LINE__, msg)

// compile error
[[noreturn]] void errorf(const char *fmt, ...);


// lexer

typedef struct {
  const char *loc;
  int len;
} StringView;

StringView sv(const char *loc, const int len);

#define sv_fmt "%.*s"
#define sv_arg(sv) (sv).len, (sv).loc

typedef enum {
  TokenKind_NUM,
  TokenKind_IDENT,
  TokenKind_PLUS,
  TokenKind_MINUS,
  TokenKind_STAR,
  TokenKind_SLASH,
  TokenKind_LPAREN,
  TokenKind_RPAREN,
  TokenKind_EQ,
  TokenKind_DEQ,
  TokenKind_NEQ,
  TokenKind_LEQ,
  TokenKind_GEQ,
  TokenKind_LT,
  TokenKind_GT,
  TokenKind_SEMICOLON,
  TokenKind_EOF,
} TokenKind;

typedef struct Token Token;
struct Token {
  TokenKind kind;
  Token *next;
  union {
    int num;
    StringView ident;
  };
  StringView lexeme;
};

// debug print at given loc
void debugf_at(const char *loc, const char *fmt, ...);

// debug print at current loc
void debugf_loc(const char *fmt, ...);

// lex error at given loc
[[noreturn]] void errorf_at(const char *loc, const char *fmt, ...);

// lex error at current loc
[[noreturn]] void errorf_loc(const char *fmt, ...);

const char *token_kind_to_str(const TokenKind kind);

const char *token_to_str(const Token *tok);

void debug_token_stream(const Token *tok);

Token *lex();


// parser

typedef enum {
  NodeKind_NUM,
  NodeKind_VAR,
  NodeKind_ADD,
  NodeKind_SUB,
  NodeKind_MUL,
  NodeKind_DIV,
  NodeKind_NEG,
  NodeKind_EQ,
  NodeKind_NEQ,
  NodeKind_LT,
  NodeKind_LEQ,
  NodeKind_ASSIGN,
  NodeKind_EXPR,
  NodeKind_PROG,
} NodeKind;

typedef struct Node Node;
struct Node {
  NodeKind kind;
  union {
    int                         num;
    StringView                  name;
    Node *                      variant;
    Node *                      operand;
    struct { Node *lhs, *rhs; } binop;
    Node *                      head;
  };
  Node *next;
};

// debug log at given token
void debugf_at_tok(const Token *tok, const char *fmt, ...);

// debug log at current tok
void debugf_tok(const char *fmt, ...);

// parse error at given token
[[noreturn]] void errorf_at_tok(const Token *tok, const char *fmt, ...);

// parse error at current tok
[[noreturn]] void errorf_tok(const char *fmt, ...);

const char *node_kind_to_str(const NodeKind kind);

void debug_ast(const Node *node);

bool node_kind_is_variant(const NodeKind kind);

bool node_kind_is_unop(const NodeKind kind);

bool node_kind_is_binop(const NodeKind kind);

bool node_kind_is_list(const NodeKind kind);

Node *parse();


// code generator

// todo: debugf_at_node(), debugf_node()

void codegen();


// global context

typedef struct {
  const char *src;
  int src_len;

  struct {
    const char *loc;
  } lexer;

  struct {
    const Token *tok;
  } parser;

  struct {
    const Node *node;
    int depth;
  } codegen;
} Context;

extern Context ctx;
