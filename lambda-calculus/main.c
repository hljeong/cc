#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef int bool;
const int true = 1;
const int false = 0;


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

static Token *lex();


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
    StringView name;              // NodeKind_VAR
    struct { Node *var, *expr; }; // NodeKind_FUN
    struct { Node *fun, *val; };  // NodeKind_APP
  };
  Node *next;
};

static void _debug_ast(const Node *node, const char *prefix, const bool last) {
  char child_prefix[256];
  snprintf(child_prefix, sizeof(child_prefix), "%s%s",
           prefix, last ? "  " : "│ ");

  const char *branch = last ? "└─" : "├─";

  if (node->kind == NodeKind_VAR) {
    debugf("%s%svar("sv_fmt")\n", prefix, branch, sv_arg(node->name));
  }

  else if (node->kind == NodeKind_FUN) {
    debugf("%s%sfun\n", prefix, branch);
    _debug_ast(node->var, child_prefix, false);
    _debug_ast(node->expr, child_prefix, true);
  }

  else if (node->kind == NodeKind_APP) {
    debugf("%s%sapp\n", prefix, branch);
    _debug_ast(node->fun, child_prefix, false);
    _debug_ast(node->val, child_prefix, true);
  }

  else {
    failf("not implemented: %u", (uint32_t) node->kind);
  }
}

static void debug_ast(const Node *node) {
  _debug_ast(node, "", true);
}

static Node *parse();

// global context

struct {
  const char *src;

  struct {
    const char *loc;
  } lexer;

  struct {
    const Token *tok;
  } parser;
} ctx;


// lexer implementation

// lex_consume_*(): if current loc matches,
// - advance current loc by len matched and
// - return len matched,
// otherwise return 0

// would've preferred `bool (*pred)(const char)` but `isspace()`, etc. are int (*)(int)
static int lex_consume_pred(int (*pred)(int)) {
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

static int lex_consume_ident() {
  int len = 0;
  if (!(len = lex_consume_pred(is_ident_first))) return 0;
  return (len += lex_consume_pred(is_ident_rest));
}

static int lex_consume_ch(const char c) {
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
    if      ((len = lex_consume_pred(isspace))) ;  // skip whitespace
    else if ((len = lex_consume_ident()))       cur = (cur->next = new_token(TokenKind_IDENT, len));
    else if ((len = lex_consume_ch('\\')))      cur = (cur->next = new_token(TokenKind_BACKSLASH, len));
    else if ((len = lex_consume_ch('.')))       cur = (cur->next = new_token(TokenKind_DOT,       len));
    else if ((len = lex_consume_ch('(')))       cur = (cur->next = new_token(TokenKind_LPAREN,    len));
    else if ((len = lex_consume_ch(')')))       cur = (cur->next = new_token(TokenKind_RPAREN,    len));
    else                                        errorf("invalid token");  // todo: error context
  }

  cur = (cur->next = new_token(TokenKind_EOF, 0));
  return head.next;
}


// parser implementation

static Node *new_node(const NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

static Node *new_app(Node *fun, Node *val) {
  Node *node = new_node(NodeKind_APP);
  node->fun = fun;
  node->val = val;
  return node;
}

static int parse_match(const TokenKind kind) {
  return ctx.parser.tok->kind == kind;
}

static const Token *parse_advance() {
  const Token *tok = ctx.parser.tok;
  ctx.parser.tok = ctx.parser.tok->next;
  return tok;
}

static const Token *parse_consume(const TokenKind kind) {
  return parse_match(kind) ? parse_advance() : NULL;
}

static const Token *parse_expect(const TokenKind kind) {
  const Token *tok = parse_consume(kind);
  if (!tok) errorf("expected %s, got: %s",
                   token_kind_to_str(kind),
                   token_to_str(ctx.parser.tok));
  return tok;
}

static Node *expr();
static Node *fun();
static Node *var();

// expr ::= "(" expr ")" | fun | var | expr expr
static Node *expr() {
  Node *node = NULL;
  if (parse_consume(TokenKind_LPAREN)) {
    node = expr();
    parse_expect(TokenKind_RPAREN);
  }
  else if (parse_match(TokenKind_BACKSLASH)) node = fun();
  else if (parse_match(TokenKind_IDENT))     node = var();
  else                                       errorf("expected expression, got %s",
                                                    token_to_str(ctx.parser.tok));

  return parse_match(TokenKind_EOF) || parse_match(TokenKind_RPAREN) ? node
                                                                     : new_app(node, expr());
}

// fun ::= "\" var "." expr
static Node *fun() {
  Node *node = new_node(NodeKind_FUN);
  parse_expect(TokenKind_BACKSLASH);
  node->var = var();
  parse_expect(TokenKind_DOT);
  node->expr = expr();
  return node;
}

// var ::= ident
static Node *var() {
  Node *node = new_node(NodeKind_VAR);
  node->name = parse_expect(TokenKind_IDENT)->lexeme;
  return node;
}

static Node *parse() {
  Node *node = expr();
  if (!parse_match(TokenKind_EOF))
    errorf("extra token");
  return node;
}

// entry point

int main(int argc, char **argv ) {
  if (argc != 2) errorf("usage: %s <expression>", argv[0]);

  ctx.src = argv[1];

  debugf("src: %s\n", ctx.lexer.loc = ctx.src);

  debug_token_stream(ctx.parser.tok = lex());

  debug_ast(parse());
}
