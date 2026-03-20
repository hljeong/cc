#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Token Token;

static struct {
  const char *src;

  struct {
    const char *loc;
  } lexer;

  struct {
    const Token *tok;
  } parser;

  struct {
    int depth;
  } codegen;
} g = {};

static void vdebug(const char *fmt, va_list ap) {
  vfprintf(stderr, fmt, ap);
}

static void debug(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  vdebug(fmt, ap);
}

[[noreturn]] static void vassert_abort(const char *file, const int line, const char *condition, const char *fmt, va_list ap) {
  debug("%s:%d: assert(%s) failed: ", file, line, condition);
  vdebug(fmt, ap);
  abort();
}

[[noreturn]] static void assert_abort(const char *file, const int line, const char *condition, const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  vassert_abort(file, line, condition, fmt, ap);
}

#define assertf(condition, fmt, ...) do { if (!(condition)) assert_abort(__FILE__, __LINE__, #condition, fmt, __VA_ARGS__); } while (0);
#define assert(condition, msg) do { if (!(condition)) assert_abort(__FILE__, __LINE__, #condition, msg); } while (0);

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

[[noreturn]] static void error_at_loc(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  verror_at(g.lexer.loc, fmt, ap);
}

typedef union {
  int num;
} TokenValue;

typedef enum {
  NodeKind_ADD,
  NodeKind_SUB,
  NodeKind_MUL,
  NodeKind_DIV,
  NodeKind_NUM,
} NodeKind;

static const char *node_kind_to_str(const NodeKind kind) {
  if      (kind == NodeKind_ADD) return "+";
  else if (kind == NodeKind_SUB) return "-";
  else if (kind == NodeKind_MUL) return "*";
  else if (kind == NodeKind_DIV) return "/";
  else if (kind == NodeKind_NUM) return "number";
  else                           error("unknown node kind: %d\n", (uint32_t) kind);
}

static bool node_kind_is_binop(const NodeKind kind) {
  return (kind == NodeKind_ADD) ||
         (kind == NodeKind_SUB) ||
         (kind == NodeKind_MUL) ||
         (kind == NodeKind_DIV);
}

typedef struct Node Node;

typedef union {
  struct {
    Node *lhs;
    Node *rhs;
  } binop;
} NodeTopology;

struct Node {
  NodeKind kind;
  NodeTopology topo;
  TokenValue value;
};

typedef enum {
  TokenKind_ADD,
  TokenKind_SUB,
  TokenKind_MUL,
  TokenKind_DIV,
  TokenKind_LPAREN,
  TokenKind_RPAREN,
  TokenKind_NUM,
  TokenKind_EOF,
} TokenKind;

static const char *token_kind_to_str(const TokenKind kind) {
  if      (kind == TokenKind_ADD)    return "+";
  else if (kind == TokenKind_SUB)    return "-";
  else if (kind == TokenKind_MUL)    return "*";
  else if (kind == TokenKind_DIV)    return "/";
  else if (kind == TokenKind_LPAREN) return "(";
  else if (kind == TokenKind_RPAREN) return ")";
  else if (kind == TokenKind_NUM)    return "number";
  else if (kind == TokenKind_EOF)    return "eof";
  else                               assertf(false, "token_kind_to_str not implemented for token kind: %d\n", (uint32_t) kind);
}

static bool token_kind_is_binop(const TokenKind kind) {
  return (kind == TokenKind_ADD) ||
         (kind == TokenKind_SUB) ||
         (kind == TokenKind_MUL) ||
         (kind == TokenKind_DIV);
}

typedef struct {
  const char *loc;
  int len;
} StringView;

static StringView sv(const char *loc, const int len) {
  return (StringView) { .loc = loc, .len = len };
}

#define SV_FMT "%.*s"
#define SV_ARG(sv) (sv).len, (sv).loc

struct Token {
  TokenKind kind;
  Token *next;
  TokenValue value;
  StringView lexeme;
};

static const char *token_to_str(const Token *tok) {
  static char buf[100] = {0};
  const int off = snprintf(buf, sizeof(buf), "%s", token_kind_to_str(tok->kind));
  if      (tok->kind == TokenKind_ADD)    ;
  else if (tok->kind == TokenKind_SUB)    ;
  else if (tok->kind == TokenKind_MUL)    ;
  else if (tok->kind == TokenKind_DIV)    ;
  else if (tok->kind == TokenKind_LPAREN) ;
  else if (tok->kind == TokenKind_RPAREN) ;
  else if (tok->kind == TokenKind_NUM)    snprintf(buf + off, sizeof(buf) - off, "(%d)", tok->value.num);
  else                                    ;
  return buf;
}

[[noreturn]] static void error_at_tok(const Token *tok, const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  verror_at(tok->lexeme.loc, fmt, ap);
}

[[noreturn]] static void error_at_cur_tok(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  verror_at(g.parser.tok->lexeme.loc, fmt, ap);
}

static Token *new_token(const TokenKind kind, const int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->lexeme = sv(g.lexer.loc, len);
  return tok;
}

static Token *lex() {
  Token head = {};
  Token *cur = &head;

  char ch;
  while ((ch = *g.lexer.loc)) {
    if (isspace(ch)) g.lexer.loc++;

    else if (isdigit(ch)) {
      cur = (cur->next = new_token(TokenKind_NUM, 0));
      const char *start = g.lexer.loc;
      // todo: why is this cast needed?
      cur->value.num = strtoul(g.lexer.loc, (char **) &g.lexer.loc, 10);
      cur->lexeme.len = g.lexer.loc - start;
    }

    else if (ch == '+') { cur = (cur->next = new_token(TokenKind_ADD,    1)); g.lexer.loc++; }
    else if (ch == '-') { cur = (cur->next = new_token(TokenKind_SUB,    1)); g.lexer.loc++; }
    else if (ch == '*') { cur = (cur->next = new_token(TokenKind_MUL,    1)); g.lexer.loc++; }
    else if (ch == '/') { cur = (cur->next = new_token(TokenKind_DIV,    1)); g.lexer.loc++; }
    else if (ch == '(') { cur = (cur->next = new_token(TokenKind_LPAREN, 1)); g.lexer.loc++; }
    else if (ch == ')') { cur = (cur->next = new_token(TokenKind_RPAREN, 1)); g.lexer.loc++; }
    else                error_at_loc("invalid token");
  }

  cur = (cur->next = new_token(TokenKind_EOF, 0));
  return head.next;
}

static bool match(const TokenKind kind) {
  return g.parser.tok->kind == kind;
}

static const Token *advance() {
  const Token *tok = g.parser.tok;
  g.parser.tok = g.parser.tok->next;
  return tok;
}

const Token *consume(const TokenKind kind) {
  return match(kind) ? advance() : NULL;
}

const Token *expect(const TokenKind kind) {
  const Token *tok = consume(kind);
  if (!tok) error_at_cur_tok("expected %s, got: %s", token_kind_to_str(kind), token_to_str(g.parser.tok));
  return tok;
}

static Node *new_node(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

static Node *new_binop_node(NodeKind kind, Node *lhs, Node *rhs) {
  assertf(node_kind_is_binop(kind), "bad invocation: new_binop_node(%s)", node_kind_to_str(kind));
  Node *node = new_node(kind);
  node->topo.binop.lhs = lhs;
  node->topo.binop.rhs = rhs;
  return node;
}

static Node *new_num_node(int value) {
  Node *node = new_node(NodeKind_NUM);
  node->value.num = value;
  return node;
}

static Node *parse_expr();
static Node *parse_mul();
static Node *parse_primary();

// expr ::= mul ("+" mul | "-" mul)*
static Node *parse_expr() {
  Node *node = parse_mul();
  const Token *tok = NULL;
  while (true) {
    if      ((tok = consume(TokenKind_ADD))) node = new_binop_node(NodeKind_ADD, node, parse_mul());
    else if ((tok = consume(TokenKind_SUB))) node = new_binop_node(NodeKind_SUB, node, parse_mul());
    else                                     break;
  }
  return node;
}

// mul ::= primary ("*" primary | "/" primary)*
static Node *parse_mul() {
  Node *node = parse_primary();
  const Token *tok = NULL;
  while (true) {
    if      ((tok = consume(TokenKind_MUL))) node = new_binop_node(NodeKind_MUL, node, parse_primary());
    else if ((tok = consume(TokenKind_DIV))) node = new_binop_node(NodeKind_DIV, node, parse_primary());
    else                                     break;
  }
  return node;
}

// primary ::= "(" expr ")" | num
static Node *parse_primary() {
  const Token *tok = NULL;
  if (consume(TokenKind_LPAREN)) {
    Node *node = parse_expr();
    expect(TokenKind_RPAREN);
    return node;
  }
  else if ((tok = consume(TokenKind_NUM))) return new_num_node(tok->value.num);
  else                                     error_at_cur_tok("expected expression");
}

static void push(void) {
  printf("  push %%rax\n");
  g.codegen.depth++;
}

static void pop(const char *arg) {
  printf("  pop %s\n", arg);
  g.codegen.depth--;
}

static void gen_expr(const Node *node) {
  if (node->kind == NodeKind_NUM) {
    printf("  mov $%d, %%rax\n", node->value.num);
  }

  else if (node_kind_is_binop(node->kind)) {
    gen_expr(node->topo.binop.lhs);
    push();
    gen_expr(node->topo.binop.rhs);
    pop("%rdi");

    switch (node->kind) {
      case NodeKind_ADD: { printf("  add %%rdi, %%rax\n"); break; }
      case NodeKind_SUB: { printf("  sub %%rdi, %%rax\n"); break; }
      case NodeKind_MUL: { printf("  imul %%rdi, %%rax\n"); break; }
      case NodeKind_DIV: {
        printf("  cqo\n");
        printf("  idiv %%rdi, %%rax\n");
        break;
      }
      default: assertf(false, "unaccounted-for binop node kind: %s", node_kind_to_str(node->kind));
    }
  }

  else error("invalid expression");
}

int main(int argc, char **argv) {
  if (argc != 2) error("usage: %s <expression>\n", argv[0]);

  g.src = argv[1];
  g.lexer.loc = g.src;
  g.parser.tok = lex();

  {
    const Token *tok_ = g.parser.tok;
    debug("tokens: %s", token_to_str(tok_));
    while ((tok_ = tok_->next)) {
      debug(" %s", token_to_str(tok_));
    }
    debug("\n");
  }

  Node *node = parse_expr();
  if (g.parser.tok->kind != TokenKind_EOF) error_at_cur_tok("extra token");

  printf("  .globl main\n");
  printf("main:\n");
  gen_expr(node);
  printf("  ret\n");

  assertf(g.codegen.depth == 0, "nonzero codegen depth: %d", g.codegen.depth);
  return 0;
}
