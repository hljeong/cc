#include "rc.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static Node *new_binop_node(const NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

static Node *new_repeat_node(Node *expr) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = NodeKind_REPEAT;
  node->expr = expr;
  return node;
}

static Node *new_literal_node(const char c) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = NodeKind_LITERAL;
  node->c = c;
  return node;
}

static bool match(const TokenKind kind) {
  return ctx.parser.tok->kind == kind;
}

static char consume(const TokenKind kind) {
  if (!match(kind)) return '\0';
  const char c = ctx.parser.tok->c;
  ctx.parser.tok = ctx.parser.tok->next;
  return c;
}

static char expect(const TokenKind kind) {
  const char c = consume(kind);
  if (!c) {
    fprintf(stderr, "unexpected token\n");
    exit(1);
  }
  return c;
}

static Node *union_expr();
static Node *concat_expr();
static Node *repeat_expr();
static Node *base_expr();

// regex ::= union_expr
static Node *regex() {
  return union_expr();
}

// union_expr ::= concat_expr ("|" union_expr)?
static Node *union_expr() {
  Node *node = concat_expr();
  return consume(TokenKind_PIPE) ? new_binop_node(NodeKind_UNION,
                                                  node,
                                                  union_expr())
                                 : node;
}

// concat_expr ::= repeat_expr concat_expr?
static Node *concat_expr() {
  Node *node = repeat_expr();
  return match(TokenKind_EOF) ? node
                              : new_binop_node(NodeKind_CONCAT,
                                               node,
                                               concat_expr());
}

// repeat_expr ::= base_epxr "*"
static Node *repeat_expr() {
  Node *node = base_expr();
  return consume(TokenKind_STAR) ? new_repeat_node(node)
                                 : node;
}

// base_expr ::= "(" union_expr ")" | literal
static Node *base_expr() {
  char c;

  if (consume(TokenKind_LPAREN)) {
    Node *node = union_expr();
    expect(TokenKind_RPAREN);
    return node;
  }

  else if ((c = consume(TokenKind_LITERAL))) {
    return new_literal_node(c);
  }

  else {
    fprintf(stderr, "unexpected token\n");
    exit(1);
  }
}

void parse() {
  ctx.parser.tok = ctx.toks;
  ctx.ast = regex();
}
