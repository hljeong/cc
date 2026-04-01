#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

// todo: stack trace / event emitter

typedef struct StrConsumer StrConsumer;
typedef struct StrFormatter StrFormatter;
typedef struct StringView StringView;
typedef struct StringBuilder StringBuilder;
typedef enum TokenKind TokenKind;
typedef struct Token Token;
typedef enum NodeKind NodeKind;
typedef struct Node Node;
typedef enum SymbolKind SymbolKind;
typedef struct Symbol Symbol;
typedef struct Scope Scope;
typedef enum TypeKind TypeKind;
typedef struct Type Type;

#define BUF_LEN     (4096)
#define SB_INIT_LEN (256)


// string view

struct StringView {
  const char *loc;
  int len;
};

StringView sv_create(const char *loc, const int len);
bool       sv_eq    (const StringView s, const StringView t);
bool       sv_eq_s  (const StringView s, const char *t);


// string builder

struct StringBuilder {
  char *buf;
  int capacity;
  int size;
};

StringBuilder sb_create  ();
void          sb_free    (StringBuilder *sb);
void          sb_clear   (StringBuilder *sb);
void          sb_append  (StringBuilder *sb, const char *fmt, ...);
void          sb_truncate(StringBuilder *sb, const int to);


// strings

typedef void (*ConsumeStr)(const char *s, void *);
struct StrConsumer {
  ConsumeStr consume;
  void *ctx;
};

typedef void (*FormatArg)(const StrConsumer, va_list);
typedef void *(*FormatPtr)(const StrConsumer, void *);
struct StrFormatter {
  const char *spec;
  FormatArg fmt_arg;
  FormatPtr fmt_ptr;
};

extern StrFormatter FORMATTERS[];

void consume_v(const StrConsumer c, const char *fmt, va_list ap);
void consume_f(const StrConsumer c, const char *fmt, ...);

void fmt_arg_at_loc      (const StrConsumer c, va_list ap);
void fmt_arg_at_cur_loc  (const StrConsumer c, va_list ap);
void fmt_arg_at_tok      (const StrConsumer c, va_list ap);
void fmt_arg_cur_tok     (const StrConsumer c, va_list ap);
void fmt_arg_at_node     (const StrConsumer c, va_list ap);
void fmt_arg_token_kind  (const StrConsumer c, va_list ap);
void fmt_arg_token       (const StrConsumer c, va_list ap);
void fmt_arg_node_kind   (const StrConsumer c, va_list ap);
void fmt_arg_node        (const StrConsumer c, va_list ap);
void fmt_arg_ast         (const StrConsumer c, va_list ap);
void fmt_arg_type_kind   (const StrConsumer c, va_list ap);
void fmt_arg_type        (const StrConsumer c, va_list ap);
void fmt_arg_symbol_kind (const StrConsumer c, va_list ap);
void fmt_arg_symbol      (const StrConsumer c, va_list ap);

void *fmt_ptr_token (const StrConsumer c, void *ptr);
void *fmt_ptr_node  (const StrConsumer c, void *ptr);
void *fmt_ptr_type  (const StrConsumer c, void *ptr);
void *fmt_ptr_symbol(const StrConsumer c, void *ptr);


// io

void _print(const char *fmt, ...);
#define print(fmt, ...) _print(fmt, ##__VA_ARGS__)

void _debug(const char *fmt, ...);
#define debug(fmt, ...) _debug(fmt, ##__VA_ARGS__)

[[noreturn]]
void _error(const char *fmt, ...);
#define error(fmt, ...) _error(fmt, ##__VA_ARGS__)

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
  TokenKind_LBRACKET,
  TokenKind_RBRACKET,
  TokenKind_EQ,
  TokenKind_DEQ,
  TokenKind_NEQ,
  TokenKind_LEQ,
  TokenKind_GEQ,
  TokenKind_LT,
  TokenKind_GT,
  TokenKind_SEMICOLON,
  TokenKind_COMMA,
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
    struct {
      int value;
    } num;

    // TokenKind_IDENT
    struct {
      StringView name;
    } ident;
  };
  Token *prev, *next;
};

Token *new_token(const TokenKind kind, const int len);

Token *link(Token *tok, Token *next);


// node

enum NodeKind {
  NodeKind_NUM,
  NodeKind_REF,
  NodeKind_DECL,
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
  NodeKind_CALL,
  NodeKind_EXPR_STMT,
  NodeKind_DECL_STMT,
  NodeKind_VAR_DECL,
  NodeKind_RETURN,
  NodeKind_BLOCK,
  NodeKind_IF,
  NodeKind_FOR,
  NodeKind_FUN_DECL,
  NodeKind_PROG,
};

struct Node {
  NodeKind kind;
  StringView lexeme;
  union {
    // NodeKind_NUM
    struct {
      int value;
    } num;

    // NodeKind_REF
    struct {
      StringView name;
      Symbol *symbol;     // populated at analysis time
    } ref;

    // NodeKind_DECL
    struct {
      StringView name;
      Node      *dims;    // list<NodeKind_NUM>, for arrays
      Node      *params;  // list<NodeKind_DECL>, for functions
      Symbol    *symbol;  // populated at analysis time
    } decl;

    // node_kind_is_unop()
    struct {
      Node *opr;          // expr
    } unop;

    // node_kind_is_binop()
    struct {
      Node *lhs;          // expr
      Node *rhs;          // expr
    } binop;

    // node_kind_is_list()
    struct {
      Node *head;
    } list;

    // NodeKind_PROG
    struct {
      Node *head;         // list<NodeKind_FUN_DECL>
    } prog;

    // NodeKind_FUN_DECL
    struct {
      Node   *decl;       // NodeKind_DECL
      Node   *body;       // NodeKind_BLOCK
      Symbol *fun;        // populated at analysis time
    } fun_decl;

    // NodeKind_EXPR_STMT
    struct {
      Node *expr;         // expr
    } expr_stmt;

    // NodeKind_IF
    struct {
      Node *cond;         // expr
      Node *then;         // stmt
      Node *else_;        // stmt
    } if_;

    // NodeKind_FOR
    struct {
      // todo: what about `for (int i = 0;;)`?
      Node *init;         // NULL | expr
      Node *cond;         // NULL | expr
      Node *inc;          // NULL | expr
      Node *body;         // stmt
    } for_;

    // NodeKind_VAR_DECL
    struct {
      Node *decl;         // NodeKind_DECL
      Node *init;         // NodeKind_ASSIGN
    } var_decl;

    // NodeKind_CALL
    struct {
      Node *fun;          // NodeKind_REF
      Node *args;         // list<expr>
    } call;
  };
  Type *type;
  Node *next;
};

bool node_kind_is_variant(const NodeKind kind);
bool node_kind_is_unop   (const NodeKind kind);
bool node_kind_is_binop  (const NodeKind kind);
bool node_kind_is_list   (const NodeKind kind);

Node *new_node      (const NodeKind kind);
Node *new_num_node  (const int value);
Node *new_ref_node  (const StringView name);
Node *new_unop_node (const NodeKind kind, Node *operand);
Node *new_binop_node(const NodeKind kind, Node *lhs, Node *rhs);
Node *new_list_node (const NodeKind kind, Node *head);


// symbol

struct Scope {
  Symbol *symbols;
  Scope *par;
};

enum SymbolKind {
  SymbolKind_VAR,
  SymbolKind_FUN,
};

struct Symbol {
  SymbolKind kind;
  StringView name;
  Type *type;
  Node *decl;
  union {
    // SymbolKind_VAR
    struct {
      int offset;
    } var;

    // SymbolKind_FUN
    struct {
      Scope scope;
      Symbol *params;
      int stack_size;
      int label;
    } fun;
  };
  Symbol *next;
};

Symbol *new_symbol(Node *decl, const SymbolKind kind);
Symbol *lookup_symbol(Node *ref);


// type

enum TypeKind {
  TypeKind_INT,
  TypeKind_PTR,
  TypeKind_ARR,
  TypeKind_FUN,
};

struct Type {
  TypeKind kind;
  int size;
  union {
    // TypeKind_PTR
    struct {
      Type *referenced;
    } ptr;

    // TypeKind_PTR
    struct {
      Type *base;
      int len;
    } arr;

    // TypeKind_FUN
    struct {
      Type *returns;
      Type *params;
    } fun;
  };
  Type *next;
};

typedef struct {
  Type int_;
} Types;

extern Types t;

bool type_eq(const Type *t, const Type *u);
Type *type_copy(const Type *type);
bool type_is_ptr(const Type *type);

Type *new_type    (const TypeKind kind);
Type *new_ptr_type(Type *referenced);
Type *new_fun_type(Type *returns, Node *params /* list<NodeKind_DECL> */);
Type *new_arr_type(Type *base, const int len);


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
  Scope globals;

  struct {
    const char *loc;
  } lexer;

  struct {
    const Token *tok;
  } parser;

  struct {
    Scope *scope;
    Symbol *fun;
  } analyzer;

  struct {
    int depth;
    Scope *scope;
    Symbol *fun;
  } codegen;
} Context;

extern Context ctx;
