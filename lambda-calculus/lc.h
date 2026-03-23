#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct StringView StringView;
typedef struct Token Token;
typedef struct Node Node;


// debug

// print debug message
void debugf(const char *fmt, ...);

// print debug message while underlining src over `span`
void debugf_span(const StringView span, const char *fmt, ...);

// print debug message with cursor pointing at given loc in src
void debugf_at_loc(const char *loc, const char *fmt, ...);

// print debug message with cursor pointing at current lexing loc
void debugf_loc(const char *fmt, ...);

// print debug message while underlining given tok in src
void debugf_at_tok(const Token *tok, const char *fmt, ...);

// print debug message while underlining current parsing tok
void debugf_tok(const char *fmt, ...);

// print debug message while underlining given node in src
void debugf_at_node(const Node *node, const char*fmt, ...);

[[noreturn]]
void _failf(const char *file, const int line,
            const char *fmt, ...);

// internal error
#define failf(fmt, ...) _failf(__FILE__, __LINE__, fmt, __VA_ARGS__)
#define fail(msg) _failf(__FILE__, __LINE__, msg)

// print compile error message
[[noreturn]]
void errorf(const char *fmt, ...);

// see debugf_... equivalent
[[noreturn]]
void errorf_at_loc(const char *loc, const char *fmt, ...);

// see debugf_... equivalent
[[noreturn]]
void errorf_loc(const char *fmt, ...);

// see debugf_... equivalent
[[noreturn]]
void errorf_at_tok(const Token *tok, const char *fmt, ...);

// see debugf_... equivalent
[[noreturn]]
void errorf_tok(const char *fmt, ...);

// see debugf_... equivalent
[[noreturn]]
void errorf_at_node(const Node *node, const char *fmt, ...);


// lexer

typedef struct StringView StringView;
struct StringView {
  const char *loc;
  int len;
};

static inline StringView sv(const char *loc, const int len) {
  return (StringView) { .loc = loc, .len = len };
}

static inline int sv_eq(const StringView s, const StringView t) {
  return (s.len == t.len) && !strncmp(s.loc, t.loc, s.len);
}

static inline const char *sv_end(const StringView s) {
  return (s.loc + s.len);
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

const char *node_kind_to_str(const NodeKind kind);

typedef struct Node Node;
struct Node {
  NodeKind kind;
  union {
    struct { StringView name; Node *ref; };  // NodeKind_VAR
                                             // `ref` stores the reference to the declaration
                                             // var node for this node. the cases are:
                                             // - for lambda binding sites, this is a self reference
                                             // - for free variables, this points to the first occurence
                                             // - for bound variables, this points to the binding site
                                             //
                                             // the first case will be referred to as a "binding var node",
                                             // and the latter two will be referred to as "reference var nodes"
                                             // or "references"

    struct { Node *par, *var, *body; };      // NodeKind_FUN
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

    struct { Node *fun, *arg; };             // NodeKind_APP - application node
  };
  StringView lexeme;
};

Node *new_fun(Node *var, Node *body);

Node *new_app(Node *fun, Node *arg);

// print abstract syntax tree to debug
void debug_ast(const Node *node);

// print unparsed ast to debug
void debug_unparse(const Node *node);

// print unparsed ast to stdout
void print_unparse(const Node *node);

// parse the ast
Node *parse();


// reduction

// perform one step of beta-reduction towards:
// - weak head normal form if `whnf` is true, and
// - normal form otherwise
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
// a beta-reduction, a new `app` node is created
Node *step(Node *node, bool whnf);


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
