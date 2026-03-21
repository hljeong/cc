#include "cc.h"

#include <stdint.h>
#include <string.h>

const char *node_kind_to_str(const NodeKind kind) {
  if      (kind == NodeKind_NUM)    return "num";
  else if (kind == NodeKind_VAR)    return "var";
  else if (kind == NodeKind_ADD)    return "+";
  else if (kind == NodeKind_SUB)    return "-";
  else if (kind == NodeKind_MUL)    return "*";
  else if (kind == NodeKind_DIV)    return "/";
  else if (kind == NodeKind_NEG)    return "-";
  else if (kind == NodeKind_EQ)     return "==";
  else if (kind == NodeKind_NEQ)    return "!=";
  else if (kind == NodeKind_LT)     return "<";
  else if (kind == NodeKind_LEQ)    return "<=";
  else if (kind == NodeKind_ASSIGN) return "assign";
  else if (kind == NodeKind_EXPR)   return "expr";
  else if (kind == NodeKind_PROG)   return "prog";
  else                              failf("not implemented: %u",
                                          (uint32_t) kind);
}

static void _debug_ast(const Node *node, const char *prefix, const bool last) {
  char child_prefix[256];
  snprintf(child_prefix, sizeof(child_prefix), "%s%s",
           prefix, last ? "  " : "│ ");

  const char *branch = last ? "└─" : "├─";

  if (node->kind == NodeKind_NUM) {
    debugf("%s%snum(%d)\n", prefix, branch, node->num);
  }

  else if (node->kind == NodeKind_VAR) {
    debugf("%s%snum("sv_fmt")\n", prefix, branch, sv_arg(node->name));
  }

  else if (node_kind_is_variant(node->kind)) {
    // todo: this, except show variant after branch?
    // _debug_ast(node->variant, prefix, last);
    debugf("%s%s%s\n", prefix, branch, node_kind_to_str(node->kind));
    _debug_ast(node->variant, child_prefix, true);
  }

  else if (node_kind_is_unop(node->kind)) {
    debugf("%s%s%s\n", prefix, branch, node_kind_to_str(node->kind));
    _debug_ast(node->operand, child_prefix, true);
  }

  else if (node_kind_is_binop(node->kind)) {
    debugf("%s%s%s\n", prefix, branch, node_kind_to_str(node->kind));
    _debug_ast(node->binop.lhs, child_prefix, false);
    _debug_ast(node->binop.rhs, child_prefix, true);
  }

  else if (node_kind_is_list(node->kind)) {
    debugf("%s%s%s\n", prefix, branch, node_kind_to_str(node->kind));
    Node *child = node->head;
    while (child) {
      _debug_ast(child, child_prefix, !(child->next));
      child = child->next;
    }
  }

  else {
    failf("not implemented: %s", node_kind_to_str(node->kind));
  }
}

void debug_ast(const Node *node) {
  _debug_ast(node, "", true);
}

bool node_kind_is_variant(const NodeKind kind) {
  return (kind == NodeKind_EXPR);
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
         (kind == NodeKind_LT)  ||
         (kind == NodeKind_ASSIGN);
}

bool node_kind_is_list(const NodeKind kind) {
  return (kind == NodeKind_PROG);
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

static Node *new_num(const int num) {
  Node *node = new_node(NodeKind_NUM);
  node->num = num;
  return node;
}

static Node *new_var(const StringView name) {
  Node *node = new_node(NodeKind_VAR);
  node->name = name;
  return node;
}

static Node *new_variant(const NodeKind kind, Node *variant) {
  assertf(node_kind_is_variant(kind),
          "bad invocation: new_variant(%s)", node_kind_to_str(kind));
  Node *node = new_node(kind);
  node->variant = variant;
  return node;
}

static Node *new_unop(const NodeKind kind, Node *operand) {
  assertf(node_kind_is_unop(kind),
          "bad invocation: new_unop(%s)", node_kind_to_str(kind));
  Node *node = new_node(kind);
  node->operand = operand;
  return node;
}

static Node *new_binop(const NodeKind kind, Node *lhs, Node *rhs) {
  assertf(node_kind_is_binop(kind),
          "bad invocation: new_binop(%s)", node_kind_to_str(kind));
  Node *node = new_node(kind);
  node->binop.lhs = lhs;
  node->binop.rhs = rhs;
  return node;
}

static Node *new_list(const NodeKind kind, Node *head) {
  assertf(node_kind_is_list(kind),
          "bad invocation: new_list(%s)", node_kind_to_str(kind));
  Node *node = new_node(kind);
  node->head = head;
  return node;
}

// These cannot be `const` due to the intrusive linked list
static Node *prog();
static Node *stmt();
static Node *expr();
static Node *assign();
static Node *equality();
static Node *relational();
static Node *add();
static Node *mul();
static Node *unary();
static Node *primary();

// prog ::= stmt*
static Node *prog() {
  Node head = {};
  Node *cur = &head;
  while (!match(TokenKind_EOF)) {
    cur = (cur->next = stmt());
  }
  return new_list(NodeKind_PROG, head.next);
}

// stmt ::= expr ";"
static Node *stmt() {
  Node *node = new_variant(NodeKind_EXPR, expr());
  expect(TokenKind_SEMICOLON);
  return node;
}

// expr ::= assign
static Node *expr() { return assign(); }

// note: right-associative
// assign ::= equality ("=" assign)?
static Node *assign() {
  Node *node = equality();
  return consume(TokenKind_EQ) ? new_binop(NodeKind_ASSIGN, node, assign()) : node;
}

// equality ::= relational ("==" relational | "!=" relational)*
static Node *equality() {
  Node *node = relational();
  const Token *tok = NULL;
  while (true) {
    if      ((tok = consume(TokenKind_DEQ))) node = new_binop(NodeKind_EQ,  node, relational());
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
    if      ((tok = consume(TokenKind_PLUS)))  node = new_binop(NodeKind_ADD, node, mul());
    else if ((tok = consume(TokenKind_MINUS))) node = new_binop(NodeKind_SUB, node, mul());
    else                                       break;
  }
  return node;
}

// mul ::= unary ("*" unary | "/" unary)*
static Node *mul() {
  Node *node = unary();
  const Token *tok = NULL;
  while (true) {
    if      ((tok = consume(TokenKind_STAR)))  node = new_binop(NodeKind_MUL, node, unary());
    else if ((tok = consume(TokenKind_SLASH))) node = new_binop(NodeKind_DIV, node, unary());
    else                                       break;
  }
  return node;
}

// unary ::= ("+" | "-") unary
//         | primary
static Node *unary() {
  const Token *tok = NULL;
  if      ((tok = consume(TokenKind_PLUS)))  return unary();
  else if ((tok = consume(TokenKind_MINUS))) return new_unop(NodeKind_NEG, unary());
  else                                       return primary();
}

// primary ::= "(" expr ")" | ident | num
static Node *primary() {
  const Token *tok = NULL;
  if (consume(TokenKind_LPAREN)) {
    Node *node = expr();
    expect(TokenKind_RPAREN);
    return node;
  }
  else if ((tok = consume(TokenKind_IDENT))) return new_var(tok->ident);
  else if ((tok = consume(TokenKind_NUM)))   return new_num(tok->num);
  else                                       errorf_tok("expected expression");
}

Node *parse() {
  Node *node = prog();
  if (ctx.parser.tok->kind != TokenKind_EOF)
    errorf_tok("extra token");
  return node;
}
