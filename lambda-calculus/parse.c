#include "lc.h"

const char *node_kind_to_str(const NodeKind kind) {
  if      (kind == NodeKind_VAR) return "var";
  else if (kind == NodeKind_FUN) return "fun";
  else if (kind == NodeKind_APP) return "app";
  else                           failf("not implemented: %u",
                                       (uint32_t) kind);
}

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

  else failf("not implemented: %u", (uint32_t) node->kind);
}

void debug_ast(const Node *node) {
  _debug_ast(node, "", true);
}

static void _debug_unparse(const Node *node) {
  if (node->kind == NodeKind_VAR) {
    debugf(sv_fmt, sv_arg(node->name));
  }

  else if (node->kind == NodeKind_FUN) {
    debugf("(\\");
    _debug_unparse(node->var);
    debugf(".");
    _debug_unparse(node->expr);
    debugf(")");
  }

  else if (node->kind == NodeKind_APP) {
    debugf("(");
    _debug_unparse(node->fun);
    debugf(" ");
    _debug_unparse(node->val);
    debugf(")");
  }

  else failf("not implemented: %u", (uint32_t) node->kind);
}

void debug_unparse(const Node *node) {
  _debug_unparse(node); debugf("\n");
}

// todo: unlucky code duplication due to bad structure
static void _print_unparse(const Node *node) {
  if (node->kind == NodeKind_VAR) {
    printf(sv_fmt, sv_arg(node->name));
  }

  else if (node->kind == NodeKind_FUN) {
    printf("(\\");
    _print_unparse(node->var);
    printf(".");
    _print_unparse(node->expr);
    printf(")");
  }

  else if (node->kind == NodeKind_APP) {
    printf("(");
    _print_unparse(node->fun);
    printf(" ");
    _print_unparse(node->val);
    printf(")");
  }

  else failf("not implemented: %u", (uint32_t) node->kind);
}

void print_unparse(const Node *node) {
  _print_unparse(node); printf("\n");
}

static bool is_atom_first(const TokenKind kind) {
  return (kind == TokenKind_LPAREN) ||
         (kind == TokenKind_BACKSLASH) ||
         (kind == TokenKind_IDENT);
}

static Node *new_node(const NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

static Node *new_var(const StringView name) {
  Node *node = new_node(NodeKind_VAR);
  node->name = name;
  return node;
}

Node *new_fun(Node *var, Node *expr) {
  Node *node = new_node(NodeKind_FUN);
  node->var = var;
  node->expr = expr;
  return node;
}

Node *new_app(Node *fun, Node *val) {
  Node *node = new_node(NodeKind_APP);
  node->fun = fun;
  node->val = val;
  return node;
}

static int parse_match_pred(bool (*pred)(const TokenKind)) {
  return pred(ctx.parser.tok->kind);
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

static Node *expr(void);
static Node *atom(void);
static Node *fun(void);
static Node *var(void);

// expr ::= expr atom | atom
static Node *expr(void) {
  Node *node = atom();
  while (parse_match_pred(is_atom_first)) {
    Node *val = atom();
    node = new_app(node, val);
  }
  return node;
}

// atom ::= "(" expr ")" | fun | var
static Node *atom(void) {
  if (parse_consume(TokenKind_LPAREN)) {
    Node *node = expr();
    parse_expect(TokenKind_RPAREN);
    return node;
  }
  else if (parse_match(TokenKind_BACKSLASH)) return fun();
  else if (parse_match(TokenKind_IDENT))     return var();
  else                                       errorf("expected expression, got %s",
                                                    token_to_str(ctx.parser.tok));
}

// fun ::= "\" var "." expr
static Node *fun(void) {
  Node *node = new_node(NodeKind_FUN);
  parse_expect(TokenKind_BACKSLASH);
  node->var = var();
  parse_expect(TokenKind_DOT);
  node->expr = expr();
  return node;
}

// var ::= ident
static Node *var(void) {
  return new_var(parse_expect(TokenKind_IDENT)->lexeme);
}

Node *parse(void) {
  Node *node = expr();
  if (!parse_match(TokenKind_EOF))
    errorf("extra token");
  return node;
}
