#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Consumer Consumer;
typedef struct StringView StringView;
typedef struct StringBuilder StringBuilder;
typedef struct Token Token;
typedef struct Node Node;
typedef struct Var Var;
typedef struct Type Type;


// string view

struct StringView {
  const char *loc;
  int len;
};

StringView sv_create(const char *loc, const int len);
int        sv_eq    (const StringView s, const StringView t);

#define    sv_fmt     "%.*s"
#define    sv_arg(sv) (sv).len, (sv).loc


// string builder

struct StringBuilder {
  char *buf;
  int capacity;
  int size;
};

StringBuilder sb_create   (const int capacity);
void          sb_free     (StringBuilder *sb);
void          sb_clear    (StringBuilder *sb);
int           sb_appendv  (StringBuilder *sb, const char *fmt, va_list ap);
int           sb_appendf  (StringBuilder *sb, const char *fmt, ...);
void          sb_backspace(StringBuilder *sb, const int len);

// debug

// print to debug
void debugf(const char *fmt, ...);

// print to debug and abort
void errorf(const char *fmt, ...);

struct Consumer {
  void *(*consume)(void *arg, void *ctx);
  void *ctx;
};

static inline void *emit(Consumer consumer, void *arg) {
  return consumer.consume(arg, consumer.ctx);
}

// print consumed string to debug
void *consume_debug(void *arg, void *ctx);
extern Consumer DEBUG;

// print consumed string to debug and abort
void *consume_error(void *arg, void *ctx);
extern Consumer ERROR;

// append consumed string to string builder
void *consume_append(void *arg, void *ctx);
static inline Consumer APPEND(StringBuilder *sb) {
  return (Consumer) { .consume = consume_append, .ctx = sb };
}

// show message at cursor location
void at_loc(const Consumer consumer, const char *loc, const char *fmt, ...);

// show message at token lexeme
void at_tok(const Consumer consumer, const Token *tok, const char *fmt, ...);

// show message at node lexeme
void at_node(const Consumer consumer, const Node *node, const char *fmt, ...);

// show message at current cursor location
void this_loc(const Consumer consumer, const char *fmt, ...);

// show message at current token lexeme
void this_tok(const Consumer consumer, const char *fmt, ...);

// stringify token stream
void token_stream(const Consumer consumer, const Token *tok);

// stringify ast
void ast(const Consumer consumer, const Node *node);


// assertion

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

[[noreturn]]
void _failf(const char *file, const int line,
            const char *fmt, ...);

#define failf(fmt, ...) _failf(__FILE__, __LINE__, fmt, __VA_ARGS__)

#define fail(msg) _failf(__FILE__, __LINE__, msg)


// token

typedef enum {
  TokenKind_NUM,
  TokenKind_IDENT,
  TokenKind_PLUS,
  TokenKind_MINUS,
  TokenKind_STAR,
  TokenKind_SLASH,
  TokenKind_AND,
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
  TokenKind_RETURN,
  TokenKind_LBRACE,
  TokenKind_RBRACE,
  TokenKind_IF,
  TokenKind_ELSE,
  TokenKind_FOR,
  TokenKind_WHILE,
  TokenKind_EOF,
} TokenKind;

struct Token {
  TokenKind kind;
  StringView lexeme;
  union {
    // TokenKind_NUM
    int num;

    // TokenKind_IDENT
    StringView ident;
  };
  Token *prev, *next;
};

const char *token_kind_to_str(const TokenKind kind);

const char *token_to_str(const Token *tok);

Token *new_token(const TokenKind kind, const int len);

Token *link(Token *tok, Token *next);


// node

typedef enum {
  NodeKind_NUM,
  NodeKind_VAR,
  NodeKind_ADD,
  NodeKind_SUB,
  NodeKind_MUL,
  NodeKind_DIV,
  NodeKind_NEG,
  NodeKind_ADDR,
  NodeKind_DEREF,
  NodeKind_EQ,
  NodeKind_NEQ,
  NodeKind_LT,
  NodeKind_LEQ,
  NodeKind_ASSIGN,
  NodeKind_EXPR_STMT,
  NodeKind_RETURN,
  NodeKind_BLOCK,
  NodeKind_IF,
  NodeKind_FOR,
  NodeKind_FUN_DECL,  // todo
  NodeKind_PROG,
} NodeKind;

struct Node {
  NodeKind kind;
  StringView lexeme;
  union {
    int num;                     // NodeKind_NUM
    struct {
      StringView name;           // NodeKind_VAR
      Var *var;
    };
    Node *operand;               // node_kind_id_unop()
    struct { Node *lhs, *rhs; }; // node_kind_is_binop()
    Node *head;                  // node_kind_is_list()
    struct {                     // NodeKind_FUN_DECL
      Node *body;
      Var *locals;
    };
    Node *expr;                  // NodeKind_EXPR_STMT
    struct {                     // NodeKind_IF
      Node *cond;
      Node *then;
      Node *else_;
    };
    struct {                     // NodeKind_FOR
      Node *init;
      Node *loop_cond;           // tragic
      Node *inc;
      Node *loop_body;           // tragic
    };
  };
  Type *type;
  Node *next;
};

const char *node_kind_to_str(const NodeKind kind);

const char *node_to_str(const Node *node);

bool node_kind_is_variant(const NodeKind kind);

bool node_kind_is_unop(const NodeKind kind);

bool node_kind_is_binop(const NodeKind kind);

bool node_kind_is_list(const NodeKind kind);

Node *new_node(const NodeKind kind);

Node *new_num_node(const int num);

Node *new_var_node(const StringView name);

Node *new_unop_node(const NodeKind kind, Node *operand);

Node *new_binop_node(const NodeKind kind, Node *lhs, Node *rhs);

Node *new_list_node(const NodeKind kind, Node *head);


// var

struct Var {
  StringView name;
  Type *type;
  Node *decl;
  int offset;
  Var *next;
};

Var *new_var(Node *decl);

Var *lookup_or_new_var(Var *locals, Node *node);


// type

typedef enum {
  TypeKind_INT,
  TypeKind_PTR,
} TypeKind;

struct Type {
  TypeKind kind;
  union {
    Type *referenced;  // TypeKind_PTR
  };
};

typedef struct {
  Type int_;
} Types;

extern Types t;

const char *type_kind_to_str(const TypeKind kind);

const char *type_to_str(const Type *type);

bool type_eq(const Type *t, const Type *u);

Type *new_type(const TypeKind kind);

Type *new_pointer_type(Type *referenced);


// action

Token *lex();

Node *parse();

void analyze();

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

  Node *ast;

  struct {
    Var locals;
  } analyzer;

  struct {
    int depth;
    int label;
  } codegen;
} Context;

extern Context ctx;
