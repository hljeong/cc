#include "cc.h"

#include <stdint.h>
#include <string.h>

const bool debug_parse = false;

const char *node_kind_to_str(const NodeKind kind) {
  if      (kind == NodeKind_NUM)       return "num";
  else if (kind == NodeKind_VAR)       return "var";
  else if (kind == NodeKind_ADD)       return "+";
  else if (kind == NodeKind_SUB)       return "-";
  else if (kind == NodeKind_MUL)       return "*";
  else if (kind == NodeKind_DIV)       return "/";
  else if (kind == NodeKind_NEG)       return "-";
  else if (kind == NodeKind_ADDR)      return "addr";
  else if (kind == NodeKind_DEREF)     return "deref";
  else if (kind == NodeKind_EQ)        return "==";
  else if (kind == NodeKind_NEQ)       return "!=";
  else if (kind == NodeKind_LT)        return "<";
  else if (kind == NodeKind_LEQ)       return "<=";
  else if (kind == NodeKind_ASSIGN)    return "assign";
  else if (kind == NodeKind_EXPR_STMT) return "expr-stmt";
  else if (kind == NodeKind_RETURN)    return "return";
  else if (kind == NodeKind_BLOCK)     return "block";
  else if (kind == NodeKind_IF)        return "if";
  else if (kind == NodeKind_FOR)       return "for";
  else if (kind == NodeKind_FUN_DECL)  return "fun_decl";
  else if (kind == NodeKind_PROG)      return "prog";
  else                                 failf("%u", (uint32_t) kind);
}

static void _debug_ast(const Node *node, const char *prefix, const bool last) {
  if (!node) return;

  char child_prefix[256];
  snprintf(child_prefix, sizeof(child_prefix), "%s%s",
           prefix, last ? "  " : "│ ");

  const char *branch = last ? "└─" : "├─";

  if (node->kind == NodeKind_NUM) {
    debugf("%s%snum(%d)\n", prefix, branch, node->num);
  }

  else if (node->kind == NodeKind_VAR) {
    debugf("%s%svar("sv_fmt")\n", prefix, branch, sv_arg(node->name));
  }

  else if (node->kind == NodeKind_FUN_DECL) {
    debugf("%s%sfn\n", prefix, branch);
    _debug_ast(node->body, child_prefix, true);
  }

  else if (node->kind == NodeKind_IF) {
    debugf("%s%sif\n", prefix, branch);
    _debug_ast(node->cond, child_prefix, false);
    _debug_ast(node->body, child_prefix, true);
  }

  else if (node->kind == NodeKind_FOR) {
    debugf("%s%sfor\n", prefix, branch);
    _debug_ast(node->init, child_prefix, false);
    _debug_ast(node->loop_cond, child_prefix, false);
    _debug_ast(node->inc, child_prefix, false);
    _debug_ast(node->loop_body, child_prefix, true);
  }

  else if (node_kind_is_unop(node->kind)) {
    debugf("%s%s%s\n", prefix, branch, node_kind_to_str(node->kind));
    _debug_ast(node->operand, child_prefix, true);
  }

  else if (node_kind_is_binop(node->kind)) {
    debugf("%s%s%s\n", prefix, branch, node_kind_to_str(node->kind));
    _debug_ast(node->lhs, child_prefix, false);
    _debug_ast(node->rhs, child_prefix, true);
  }

  else if (node_kind_is_list(node->kind)) {
    debugf("%s%s%s\n", prefix, branch, node_kind_to_str(node->kind));
    Node *child = node->head;
    while (child) {
      _debug_ast(child, child_prefix, !(child->next));
      child = child->next;
    }
  }

  else failf("%s", node_kind_to_str(node->kind));
}

void debug_ast(const Node *node) {
  _debug_ast(node, "", true);
}

bool node_kind_is_unop(const NodeKind kind) {
  return (kind == NodeKind_NEG)       ||
         (kind == NodeKind_ADDR)      ||
         (kind == NodeKind_DEREF)     ||
         (kind == NodeKind_EXPR_STMT) ||
         (kind == NodeKind_RETURN);
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
  return (kind == NodeKind_BLOCK);
}

static bool match(const TokenKind kind) {
  return ctx.parser.tok->kind == kind;
}

static const Token *advance() {
  const Token *tok = ctx.parser.tok;
  ctx.parser.tok = ctx.parser.tok->next;
  return tok;
}

static const Token *consume(const TokenKind kind) {
  return match(kind) ? advance() : NULL;
}

static const Token *expect(const TokenKind kind) {
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
  node->lhs = lhs;
  node->rhs = rhs;
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
static Node *fun_decl();
static Node *stmt();
static Node *block();
static Node *expr();
static Node *assign();
static Node *equality();
static Node *relational();
static Node *add();
static Node *mul();
static Node *unary();
static Node *primary();

typedef struct SourceStack SourceStack;
struct SourceStack {
  const char *loc;
  SourceStack *last;
};

static SourceStack *src_stack = NULL;

static void src_push() {
  SourceStack *new_src_stack = calloc(1, sizeof(SourceStack));
  new_src_stack->loc = ctx.parser.tok->lexeme.loc;
  new_src_stack->last = src_stack;
  src_stack = new_src_stack;
}

static StringView src_peek() {
  assertm(src_stack, "empty src stack");
  assertf(ctx.parser.tok->prev, "no tokens consumed before %s", token_to_str(ctx.parser.tok));
  const StringView last = ctx.parser.tok->prev->lexeme;
  const char *start = src_stack->loc;
  return (StringView) {
    .loc = start,
    .len = last.loc + last.len - start,
  };
}

static void src_pop() {
  assertm(src_stack, "empty src stack");
  SourceStack *last_src_stack = src_stack->last;
  free(src_stack);
  src_stack = last_src_stack;
}

static Node *pop_lexeme(Node *node) {
  node->lexeme = src_peek();
  src_pop();
  return node;
}

static Node *add_lexeme(Node *node) {
  node->lexeme = src_peek();
  return node;
}

// todo: temp
// prog ::= fun_decl
static Node *prog() {
  if (debug_parse) debugf_tok("parsing prog");
  return fun_decl();
}

// todo: temp
// decl ::= block
static Node *fun_decl() {
  if (debug_parse) debugf_tok("parsing fun_decl");
  src_push();
  Node *node = new_node(NodeKind_FUN_DECL);
  node->body = block();
  return pop_lexeme(node);
}

// stmt ::= "return" expr ";"
//        | block
//        | "if" "(" expr ")" stmt ("else" stmt)?
//        | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//        | "while" "(" expr ")" stmt
//        | expr? ";"
static Node *stmt() {
  if (debug_parse) debugf_tok("parsing stmt");
  src_push();
  Node *node = NULL;

  if (consume(TokenKind_SEMICOLON))
  {
    // cool trick from chibicc
    return pop_lexeme(new_list(NodeKind_BLOCK, NULL));
  }

  else if (consume(TokenKind_RETURN)) {
    node = new_unop(NodeKind_RETURN, expr());
    expect(TokenKind_SEMICOLON);
    return pop_lexeme(node);
  }

  else if (consume(TokenKind_IF)) {
    Node *node = new_node(NodeKind_IF);
    expect(TokenKind_LPAREN);
    node->cond = expr();
    expect(TokenKind_RPAREN);
    node->then = stmt();
    if (consume(TokenKind_ELSE)) {
      node->else_ = stmt();
    }
    return pop_lexeme(node);
  }

  else if (consume(TokenKind_FOR)) {
    // wow for loops have horrible syntax...
    Node *node = new_node(NodeKind_FOR);
    expect(TokenKind_LPAREN);
    if (!consume(TokenKind_SEMICOLON)) {
      node->init = expr();
      expect(TokenKind_SEMICOLON);
    }
    if (!consume(TokenKind_SEMICOLON)) {
      node->loop_cond = expr();
      expect(TokenKind_SEMICOLON);
    }
    if (!consume(TokenKind_RPAREN)) {
      node->inc = expr();
      expect(TokenKind_RPAREN);
    }
    node->loop_body = stmt();
    return pop_lexeme(node);
  }

  else if (consume(TokenKind_WHILE)) {
    Node *node = new_node(NodeKind_FOR);
    expect(TokenKind_LPAREN);
    node->loop_cond = expr();
    expect(TokenKind_RPAREN);
    node->loop_body = stmt();
    return pop_lexeme(node);
  }

  else if (match(TokenKind_LBRACE)) {
    src_pop();
    return block();
  }

  else {
    node = new_unop(NodeKind_EXPR_STMT, expr());
    expect(TokenKind_SEMICOLON);
    return pop_lexeme(node);
  }
}

// block ::= stmt*
static Node *block() {
  if (debug_parse) debugf_tok("parsing block");
  src_push();
  expect(TokenKind_LBRACE);
  Node head = {};
  Node *cur = &head;
  while (!consume(TokenKind_RBRACE)) {
    cur = (cur->next = stmt());
  }
  return pop_lexeme(new_list(NodeKind_BLOCK, head.next));
}

// expr ::= assign
static Node *expr() {
  if (debug_parse) debugf_tok("parsing expr");
  return assign();
}

// note: right-associative
// assign ::= equality ("=" assign)?
static Node *assign() {
  if (debug_parse) debugf_tok("parsing assign");
  src_push();
  Node *node = equality();
  return consume(TokenKind_EQ) ? pop_lexeme(new_binop(NodeKind_ASSIGN, node, assign()))
                               : (src_pop(), node);
}

// equality ::= relational ("==" relational | "!=" relational)*
static Node *equality() {
  if (debug_parse) debugf_tok("parsing equality");
  src_push();
  Node *node = relational();
  const Token *tok = NULL;
  while (true) {
    if      ((tok = consume(TokenKind_DEQ))) node = add_lexeme(new_binop(NodeKind_EQ,  node, relational()));
    else if ((tok = consume(TokenKind_NEQ))) node = add_lexeme(new_binop(NodeKind_NEQ, node, relational()));
    else                                     break;
  }
  src_pop();
  return node;
}

// relational ::= add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational() {
  if (debug_parse) debugf_tok("parsing relational");
  src_push();
  Node *node = add();
  const Token *tok = NULL;
  while (true) {
    if      ((tok = consume(TokenKind_LT)))  node = add_lexeme(new_binop(NodeKind_LT,  node, add()));
    else if ((tok = consume(TokenKind_GT)))  node = add_lexeme(new_binop(NodeKind_LT,  add(), node));
    else if ((tok = consume(TokenKind_LEQ))) node = add_lexeme(new_binop(NodeKind_LEQ, node, add()));
    else if ((tok = consume(TokenKind_GEQ))) node = add_lexeme(new_binop(NodeKind_LEQ, add(), node));
    else                                     break;
  }
  src_pop();
  return node;
}

// add ::= mul ("+" mul | "-" mul)*
static Node *add() {
  if (debug_parse) debugf_tok("parsing add");
  src_push();
  Node *node = mul();
  const Token *tok = NULL;
  while (true) {
    if      ((tok = consume(TokenKind_PLUS)))  node = add_lexeme(new_binop(NodeKind_ADD, node, mul()));
    else if ((tok = consume(TokenKind_MINUS))) node = add_lexeme(new_binop(NodeKind_SUB, node, mul()));
    else                                       break;
  }
  return node;
}

// mul ::= unary ("*" unary | "/" unary)*
static Node *mul() {
  if (debug_parse) debugf_tok("parsing mul");
  src_push();
  Node *node = unary();
  const Token *tok = NULL;
  while (true) {
    if      ((tok = consume(TokenKind_STAR)))  node = add_lexeme(new_binop(NodeKind_MUL, node, unary()));
    else if ((tok = consume(TokenKind_SLASH))) node = add_lexeme(new_binop(NodeKind_DIV, node, unary()));
    else                                       break;
  }
  src_pop();
  return node;
}

// unary ::= ("+" | "-" | "&" | "*") unary
//         | primary
static Node *unary() {
  if (debug_parse) debugf_tok("parsing unary");
  src_push();
  const Token *tok = NULL;
  if      ((tok = consume(TokenKind_PLUS)))  return src_pop(), unary();
  else if ((tok = consume(TokenKind_MINUS))) return pop_lexeme(new_unop(NodeKind_NEG, unary()));
  else if ((tok = consume(TokenKind_AND)))   return pop_lexeme(new_unop(NodeKind_ADDR, unary()));
  else if ((tok = consume(TokenKind_STAR)))  return pop_lexeme(new_unop(NodeKind_DEREF, unary()));
  else                                       return src_pop(), primary();
}

// primary ::= "(" expr ")" | ident | num
static Node *primary() {
  if (debug_parse) debugf_tok("parsing primary");
  src_push();
  const Token *tok = NULL;
  if (consume(TokenKind_LPAREN)) {
    Node *node = expr();
    expect(TokenKind_RPAREN);
    return node;
  }
  else if ((tok = consume(TokenKind_IDENT))) return pop_lexeme(new_var(tok->ident));
  else if ((tok = consume(TokenKind_NUM)))   return pop_lexeme(new_num(tok->num));
  else                                       errorf_tok("expected expression");
}

Node *parse() {
  Node *node = prog();
  if (!match(TokenKind_EOF))
    errorf_tok("extra token");
  return node;
}
