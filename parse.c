#include "cc.h"

#include <stdint.h>

const char *node_kind_to_str(const NodeKind kind) {
  if      (kind == NodeKind_ADD) return "+";
  else if (kind == NodeKind_SUB) return "-";
  else if (kind == NodeKind_MUL) return "*";
  else if (kind == NodeKind_DIV) return "/";
  else if (kind == NodeKind_NEG) return "-";
  else if (kind == NodeKind_EQ)  return "==";
  else if (kind == NodeKind_NEQ) return "!=";
  else if (kind == NodeKind_LT)  return "<";
  else if (kind == NodeKind_LEQ) return "<=";
  else if (kind == NodeKind_NUM) return "number";
  else                           failf("not implemented: %u",
                                       (uint32_t) kind);
}

void debug_ast(const Node *node) {
  // todo
  debugf("debug_ast() not implemented\n");
}

bool node_kind_is_unop(const NodeKind kind) {
  return (kind == NodeKind_NEG);
}

bool node_kind_is_binop(const NodeKind kind) {
  return (kind == NodeKind_ADD) ||
         (kind == NodeKind_SUB) ||
         (kind == NodeKind_MUL) ||
         (kind == NodeKind_DIV) ||
         (kind == NodeKind_EQ)  ||
         (kind == NodeKind_NEQ) ||
         (kind == NodeKind_LEQ) ||
         (kind == NodeKind_LT);
}

static bool match(const TokenKind kind) {
  return ctx.parser.tok->kind == kind;
}

static const Token *advance() {
  const Token *tok = ctx.parser.tok;
  ctx.parser.tok = ctx.parser.tok->next;
  return tok;
}

const Token *consume(const TokenKind kind) {
  return match(kind) ? advance() : NULL;
}

const Token *expect(const TokenKind kind) {
  const Token *tok = consume(kind);
  if (!tok) errorf_tok("expected %s, got: %s",
                       token_kind_to_str(kind),
                       token_to_str(ctx.parser.tok));
  return tok;
}

static Node *new_node(const NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

static Node *new_unop(const NodeKind kind, Node *operand) {
  assertf(node_kind_is_unop(kind),
          "bad invocation: new_unop_node(%s)", node_kind_to_str(kind));
  Node *node = new_node(kind);
  node->unop.operand = operand;
  return node;
}

static Node *new_binop(const NodeKind kind, Node *lhs, Node *rhs) {
  assertf(node_kind_is_binop(kind),
          "bad invocation: new_binop_node(%s)", node_kind_to_str(kind));
  Node *node = new_node(kind);
  node->binop.lhs = lhs;
  node->binop.rhs = rhs;
  return node;
}

static Node *new_num(const int num) {
  Node *node = new_node(NodeKind_NUM);
  node->num = num;
  return node;
}

static Node *expr();
static Node *equality();
static Node *relational();
static Node *add();
static Node *mul();
static Node *unary();
static Node *primary();

// expr ::= equality
static Node *expr() { return equality(); }

// equality ::= relational ("==" relational | "!=" relational)*
static Node *equality() {
  Node *node = relational();
  const Token *tok = NULL;
  while (true) {
    if      ((tok = consume(TokenKind_EEQ))) node = new_binop(NodeKind_EQ,  node, relational());
    else if ((tok = consume(TokenKind_NEQ))) node = new_binop(NodeKind_NEQ, node, relational());
    else                                     break;
  }
  return node;
}

// relational ::= add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational() {
  Node *node = add();
  const Token *tok = NULL;
  while (true) {
    if      ((tok = consume(TokenKind_LT)))  node = new_binop(NodeKind_LT,  node, add());
    else if ((tok = consume(TokenKind_GT)))  node = new_binop(NodeKind_LT,  add(), node);
    else if ((tok = consume(TokenKind_LEQ))) node = new_binop(NodeKind_LEQ, node, add());
    else if ((tok = consume(TokenKind_GEQ))) node = new_binop(NodeKind_LEQ, add(), node);
    else                                     break;
  }
  return node;
}

// add ::= mul ("+" mul | "-" mul)*
static Node *add() {
  Node *node = mul();
  const Token *tok = NULL;
  while (true) {
    if      ((tok = consume(TokenKind_ADD))) node = new_binop(NodeKind_ADD, node, mul());
    else if ((tok = consume(TokenKind_SUB))) node = new_binop(NodeKind_SUB, node, mul());
    else                                     break;
  }
  return node;
}

// mul ::= unary ("*" unary | "/" unary)*
static Node *mul() {
  Node *node = unary();
  const Token *tok = NULL;
  while (true) {
    if      ((tok = consume(TokenKind_MUL))) node = new_binop(NodeKind_MUL, node, unary());
    else if ((tok = consume(TokenKind_DIV))) node = new_binop(NodeKind_DIV, node, unary());
    else                                     break;
  }
  return node;
}

// unary ::= ("+" | "-") unary
//         | primary
static Node *unary() {
  const Token *tok = NULL;
  if      ((tok = consume(TokenKind_ADD))) return unary();
  else if ((tok = consume(TokenKind_SUB))) return new_unop(NodeKind_NEG, unary());
  else                                     return primary();
}

// primary ::= "(" expr ")" | num
static Node *primary() {
  const Token *tok = NULL;
  if (consume(TokenKind_LPAREN)) {
    Node *node = expr();
    expect(TokenKind_RPAREN);
    return node;
  }
  else if ((tok = consume(TokenKind_NUM))) return new_num(tok->num);
  else                                     errorf_tok("expected expression");
}

Node *parse() {
  Node *node = expr();
  if (ctx.parser.tok->kind != TokenKind_EOF)
    errorf_tok("extra token");
  return node;
}
