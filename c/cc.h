#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct StrConsumer StrConsumer;
typedef struct StrEmitter StrEmitter;
typedef struct StringView StringView;
typedef struct StringBuilder StringBuilder;
typedef enum TokenKind TokenKind;
typedef struct Token Token;
typedef enum NodeKind NodeKind;
typedef struct Node Node;
typedef struct Var Var;
typedef enum TypeKind TypeKind;
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
int           sb_append_v (StringBuilder *sb, const char *fmt, va_list ap);
int           sb_append_f (StringBuilder *sb, const char *fmt, ...);
void          sb_backspace(StringBuilder *sb, const int len);


// strings

struct StrConsumer {
  void (*consume)(const char *s, void *ctx);
  void *ctx;
};

struct StrEmitter {
  void (*emit)(const StrConsumer, void *data);
  void *data;
};

extern const StrEmitter HALT;

void emit_s(const StrConsumer consumer, const char *s);
void emit_v(const StrConsumer consumer, const char *fmt, va_list ap);
void emit_f(const StrConsumer consumer, const char *fmt, ...);
void emit_e(const StrConsumer consumer, const StrEmitter emitter);

void emit_all_v(const StrConsumer consumer, va_list ap);
void _emit_all(const StrConsumer consumer, ...);
#define emit_all(consumer, ...) _emit_all(consumer, __VA_ARGS__, HALT)

StrEmitter str_f(const char *fmt, ...);
StrEmitter str_int(const int value);


// debug

// print to debug
void debugf(const char *fmt, ...);

// print to debug and abort
void errorf(const char *fmt, ...);

// print consumed string to debug
void _debug(void *_, ...);  // c mandates named parameter before ellipsis
#define debug(...) _debug(NULL, __VA_ARGS__, HALT)

// print consumed string to debug and abort
[[noreturn]]
void _error(void *_, ...);
#define error(...) _error(NULL, __VA_ARGS__, HALT)

// append consumed string to string builder
void _sb_append(StringBuilder *sb, ...);
#define sb_append(sb, ...) _sb_append(sb, __VA_ARGS__, HALT)

// show message at cursor location
StrEmitter at_loc(const char *loc);

// show message at token lexeme
StrEmitter at_tok(const Token *tok);

// show message at node lexeme
StrEmitter at_node(const Node *node);

// show message at current cursor location
StrEmitter this_loc();

// show message at current token lexeme
StrEmitter this_tok();

// stringify
StrEmitter str_token_kind  (const TokenKind kind);
StrEmitter str_token       (const Token *tok);
StrEmitter str_token_stream(const Token *tok);
StrEmitter str_node_kind   (const NodeKind kind);
StrEmitter str_node        (const Node *node);
StrEmitter str_ast         (const Node *node);
StrEmitter str_type_kind   (const TypeKind kind);
StrEmitter str_type        (const Type *type);


// assertion

// todo: optional args (##__VA_ARGS__ or separate macro?)
[[noreturn]]
void _assert(const char *file, const int line, const char *cond, ...);
#define assert(cond, ...)                 \
  do {                                    \
    if (!(cond))                          \
      _assert(__FILE__, __LINE__,  #cond, \
              __VA_ARGS__, HALT);         \
  } while (0)

// print consumed string to debug
[[noreturn]]
void _fail(const char *file, const int line, ...);
#define fail(...) _fail(__FILE__, __LINE__, __VA_ARGS__, HALT);


// token

enum TokenKind {
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
};

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

Token *new_token(const TokenKind kind, const int len);

Token *link(Token *tok, Token *next);


// node

enum NodeKind {
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
};

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

enum TypeKind {
  TypeKind_INT,
  TypeKind_PTR,
};

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
