#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "cfmt.h"


typedef struct Token Token;
typedef struct Node Node;


// debug

void register_formatters(void);

// print debug message
int debug(const char *fmt, ...);

[[noreturn]]
void _fail(const char *file, const int line,
           const char *fmt, ...);

// internal error
#define fail(...) _fail(__FILE__, __LINE__, ##__VA_ARGS__, NULL)

// print compile error message
[[noreturn]]
void error(const char *fmt, ...);

// todo: assert()
// todo: fail immediately on emitf() returning <0
// todo: token.c, node.c

// token

int fmt_token_kind  (const sink s, va_list ap);
int fmt_token       (const sink s, va_list ap);
int fmt_token_stream(const sink s, va_list ap);


// node

int fmt_node_kind(const sink s, va_list ap);
int fmt_node     (const sink s, va_list ap);
int fmt_ast      (const sink s, va_list ap);
int fmt_scope    (const sink s, va_list ap);
int fmt_lambda   (const sink s, va_list ap);


// lexer

typedef enum {
  TokenKind_IDENT,
  TokenKind_BACKSLASH,
  TokenKind_DOT,
  TokenKind_LPAREN,
  TokenKind_RPAREN,
  TokenKind_EOF,
} TokenKind;

struct Token {
  TokenKind kind;
  Token *next;
  str_view lexeme;
};

const char *token_kind_to_str(const TokenKind kind);

const char *token_to_str(const Token *tok);

// print token stream to debug
void debug_token_stream(const Token *tok);

// parse the token stream
Token *lex();


// parser

typedef enum {
  NodeKind_VAR,
  NodeKind_FUN,
  NodeKind_APP,
} NodeKind;

typedef struct Node Node;
struct Node {
  NodeKind kind;
  union {
    // NodeKind_VAR
    // `ref` stores the reference to the declaration
    // var node for this node. the cases are:
    // - for lambda binding sites, this is a self reference
    // - for free variables, this points to the first occurence
    // - for bound variables, this points to the binding site
    //
    // the first case will be referred to as a "binding var node",
    // and the latter two will be referred to as "reference var nodes"
    // or "references"
    struct {
      str_view name;
      Node *ref;
    };

    // NodeKind_FUN
    // this node represents scope in addition to storing
    // ast structure. `par` points to the semantic parent scope
    // of the function this node represents. if this node
    // represents a top-level function, `par` points to *the*
    // dummy node. the dummy node is a delimiter for free
    // variables. when a variable reference searches up the
    // entire scope chain and does not find a declaration,
    // it will create a fictitious function node at the tail
    // of the scope chain to "bind" the free variable.
    // consider the expression `(\x.\y.w z) z (\x.z)`:
    //
    // the scope chain (actually a tree) is as follows:
    //   ┌─ (\z.???) binds `z`
    //   ├─ (\w.???) binds `w`
    //   * (dummy)
    //   ├─ (\x.\y.w z) binds `x`
    //   │  └─ (\y.w z) binds `y`
    //   └─ (\x.z) binds `x`
    //
    // the "free" part of the scope chain is deliberately
    // depicted as an upside-down linked list *above* the dummy.
    // the effective topology is more apparent when drawn in
    // the conventional manner:
    //   (\z.???) binds `z`
    //   └─ (\w.???) binds `w`
    //      └─ * (dummy)
    //         ├─ (\x.\y.w z) binds `x`
    //         │  └─ (\y.w z) binds `y`
    //         └─ (\x.z) binds `x`
    //
    // this way we are effectively operating on
    // `\z.\w.((\x.\y.w z) z (\x.z)), with no free variables
    struct {
      Node *par;
      Node *var;
      Node *body;
    };

    // NodeKind_APP
    struct {
      Node *fun;
      Node *arg;
    };
  };
  str_view lexeme;
};

Node *new_fun(Node *var, Node *body);

Node *new_app(Node *fun, Node *arg);

// print unparsed ast to debug
void debug_unparse(const Node *node, const bool ext);

// print unparsed ast to stdout
void print_unparse(const Node *node, const bool ext);

// parse the ast
Node *parse();


// reduction

typedef enum {
  NormalForm_WEAK_HEAD,
  NormalForm_BETA,
  NormalForm_BETA_ETA,
} NormalForm;

// perform one step of reduction towards `nf`
//
// a new node is created for each reduction. the
// original `node` is returned if no reduction
// could be done. the reduction retains the original
// ast as much as possible. this design is inspired by
// persistent data structures
// consider the divergence operator Ω = `(\x.x x)(\x.x x)`:
// the ast transforms as follows:
//      app#1     ───step()───>  app#2      ───step()───>  app#3
//      ├─ fun#1          ┌───>  ├─ fun#2           ┌───>  ├─ fun#2
//      │  ├─ var(x)      │      │  ├─ var(x)       │      │  ├─ var(x)
//      │  └─ app         │      │  └─ app          │      │  └─ app
//      │     ├─ var(x)   │      │     ├─ var(x)    │      │     ├─ var(x)
//      │     └─ var(x)   │      │     └─ var(x)    │      │     └─ var(x)
//      └─ fun#2 ─────────┴───>  └─ fun#2 ──────────┴───>  └─ fun#2
//         ├─ var(x)                ├─ var(x)                 ├─ var(x)
//         └─ app                   └─ app                    └─ app
//            ├─ var(x)                ├─ var(x)                 ├─ var(x)
//            └─ var(x)                └─ var(x)                 └─ var(x)
//
// note that since in each step the top `app` node performs
// a reduction, a new `app` node is created
Node *step(Node *node, const NormalForm nf);


// global context

typedef struct {
  const char *src;
  int src_len;

  struct {
    const char *loc;
  } lexer;

  struct {
    const Token *tok;
    Node *scope;
  } parser;
} Context;

extern Context ctx;
