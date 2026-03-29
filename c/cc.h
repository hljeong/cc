#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct StrConsumer StrConsumer;
typedef struct StrFormatter StrFormatter;
typedef struct StringView StringView;
typedef struct StringBuilder StringBuilder;
typedef enum TokenKind TokenKind;
typedef struct Token Token;
typedef enum NodeKind NodeKind;
typedef struct Node Node;
typedef struct Var Var;
typedef enum TypeKind TypeKind;
typedef struct Type Type;

#define BUF_LEN (256)


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

StringBuilder sb_create  ();
void          sb_free    (StringBuilder *sb);
void          sb_clear   (StringBuilder *sb);
void          sb_append_s(StringBuilder *sb, const char *s);
void          sb_append_f(StringBuilder *sb, const char *fmt, ...);
void          sb_truncate(StringBuilder *sb, const int to);


// strings

typedef void (*ConsumeStr)(const char *s, void *);
struct StrConsumer {
  ConsumeStr consume;
  void *ctx;
};

typedef void (*FormatStr)(const StrConsumer, va_list);
struct StrFormatter {
  const char *spec;
  FormatStr fmt;
};

extern StrFormatter FORMATTERS[];

// todo: allow format strings?
void consume_v(const StrConsumer c, const char *fmt, va_list ap);
void consume_f(const StrConsumer c, const char *fmt, ...);

void fmt_at_loc      (const StrConsumer c, va_list ap);
void fmt_at_cur_loc  (const StrConsumer c, va_list ap);
void fmt_at_tok      (const StrConsumer c, va_list ap);
void fmt_at_cur_tok  (const StrConsumer c, va_list ap);
void fmt_at_node     (const StrConsumer c, va_list ap);
void fmt_token_kind  (const StrConsumer c, va_list ap);
void fmt_token       (const StrConsumer c, va_list ap);
void fmt_token_stream(const StrConsumer c, va_list ap);
void fmt_node_kind   (const StrConsumer c, va_list ap);
void fmt_node        (const StrConsumer c, va_list ap);
void fmt_ast         (const StrConsumer c, va_list ap);
void fmt_type_kind   (const StrConsumer c, va_list ap);
void fmt_type        (const StrConsumer c, va_list ap);


// debug

void _debug(const char *fmt, ...);
#define debug(fmt, ...) _debug(fmt, ##__VA_ARGS__)

[[noreturn]]
void _error(const char *fmt, ...);
#define error(fmt, ...) _error(fmt, ##__VA_ARGS__)

// assertion

[[noreturn]]
void _assert(const char *file, const int line, const char *func, const char *cond,
             const char *fmt, ...);
#define assert(cond, ...)                          \
  do {                                             \
    if (!(cond))                                   \
      _assert(__FILE__, __LINE__, __func__, #cond, \
              ##__VA_ARGS__, NULL);                \
  } while (0)

[[noreturn]]
void _fail(const char *file, const int line, const char *func,
           const char *fmt, ...);
#define fail(...) _fail(__FILE__, __LINE__, __func__, ##__VA_ARGS__, NULL)


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
bool node_kind_is_unop   (const NodeKind kind);
bool node_kind_is_binop  (const NodeKind kind);
bool node_kind_is_list   (const NodeKind kind);

Node *new_node      (const NodeKind kind);
Node *new_num_node  (const int num);
Node *new_var_node  (const StringView name);
Node *new_unop_node (const NodeKind kind, Node *operand);
Node *new_binop_node(const NodeKind kind, Node *lhs, Node *rhs);
Node *new_list_node (const NodeKind kind, Node *head);


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

Type *new_type        (const TypeKind kind);
Type *new_pointer_type(Type *referenced);


// action

void lex();
void parse();
void analyze();
void codegen();


// global context

typedef struct {
  StringView src;
  const Token *toks;
  Node *ast;

  struct {
    const char *loc;
  } lexer;

  struct {
    const Token *tok;
  } parser;

  struct {
    Var locals;
  } analyzer;

  struct {
    int depth;
    int label;
  } codegen;
} Context;

extern Context ctx;
