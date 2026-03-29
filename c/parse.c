#include "cc.h"

#include <stdint.h>
#include <string.h>

const bool debug_parse = false;

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
  if (!tok) error_f("%{} expected %{token_kind}, got: %{token}",
                    this_tok(), kind, ctx.parser.tok);
  return tok;
}

// These cannot be `const` due to the intrusive linked list
static Node *prog();
static Node *fun_decl();
static Node *stmt();
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
  // todo: ##__VA_ARGS__?
  assert_f(src_stack, "empty src stack", NULL);
  assert_f(ctx.parser.tok->prev, "no tokens consumed before %{token}", ctx.parser.tok);
  const StringView last = ctx.parser.tok->prev->lexeme;
  const char *start = src_stack->loc;
  return (StringView) {
    .loc = start,
    .len = last.loc + last.len - start,
  };
}

static void src_pop() {
  // todo: ##__VA_ARGS__?
  assert_f(src_stack, "empty src stack", NULL);
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
  if (debug_parse) debug_f("%{} parsing prog", this_tok());
  return fun_decl();
}

// todo: temp
// decl ::= "{" stmt* "}"
static Node *fun_decl() {
  if (debug_parse) debug_f("%{} parsing fun_decl", this_tok());
  src_push();
  Node *node = new_node(NodeKind_FUN_DECL);
  {
    src_push();
    Node head = {};
    Node *cur = &head;
    expect(TokenKind_LBRACE);
    while (!consume(TokenKind_RBRACE)) {
      cur = (cur->next = stmt());
    }
    node->body = pop_lexeme(new_list_node(NodeKind_BLOCK, head.next));
  }
  return pop_lexeme(node);
}

// stmt ::= "return" expr ";"
//        | "{" stmt* "}"
//        | "if" "(" expr ")" stmt ("else" stmt)?
//        | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//        | "while" "(" expr ")" stmt
//        | expr? ";"
static Node *stmt() {
  if (debug_parse) debug_f("%{} parsing stmt", this_tok());
  src_push();
  Node *node = NULL;

  if (consume(TokenKind_SEMICOLON))
  {
    // cool trick from chibicc
    return pop_lexeme(new_list_node(NodeKind_BLOCK, NULL));
  }

  else if (consume(TokenKind_RETURN)) {
    node = new_unop_node(NodeKind_RETURN, expr());
    expect(TokenKind_SEMICOLON);
    return pop_lexeme(node);
  }

  else if (consume(TokenKind_IF)) {
    Node *node = new_node(NodeKind_IF);
    expect(TokenKind_LPAREN);
    node->cond = expr();
    expect(TokenKind_RPAREN);
    node->then = stmt();
    if (consume(TokenKind_ELSE))
      node->else_ = stmt();
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

  else if (consume(TokenKind_LBRACE)) {
    Node head = {};
    Node *cur = &head;
    while (!consume(TokenKind_RBRACE)) {
      cur = (cur->next = stmt());
    }
    return pop_lexeme(new_list_node(NodeKind_BLOCK, head.next));
  }

  else {
    node = new_node(NodeKind_EXPR_STMT);
    node->expr = expr();
    expect(TokenKind_SEMICOLON);
    return pop_lexeme(node);
  }
}

// expr ::= assign
static Node *expr() {
  if (debug_parse) debug_f("%{} parsing expr", this_tok());
  // if (debug_parse) this_tok(DEBUG, "parsing expr");
  return assign();
}

// note: right-associative
// assign ::= equality ("=" assign)?
static Node *assign() {
  if (debug_parse) debug_f("%{} parsing assign", this_tok());
  src_push();
  Node *node = equality();
  return consume(TokenKind_EQ) ? pop_lexeme(new_binop_node(NodeKind_ASSIGN, node, assign()))
                               : (src_pop(), node);
}

// equality ::= relational ("==" relational | "!=" relational)*
static Node *equality() {
  if (debug_parse) debug_f("%{} parsing equality", this_tok());
  src_push();
  Node *node = relational();
  const Token *tok = NULL;
  while (true) {
    if      ((tok = consume(TokenKind_DEQ))) node = add_lexeme(new_binop_node(NodeKind_EQ,  node, relational()));
    else if ((tok = consume(TokenKind_NEQ))) node = add_lexeme(new_binop_node(NodeKind_NEQ, node, relational()));
    else                                     break;
  }
  src_pop();
  return node;
}

// relational ::= add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational() {
  if (debug_parse) debug_f("%{} parsing relational", this_tok());
  src_push();
  Node *node = add();
  const Token *tok = NULL;
  while (true) {
    if      ((tok = consume(TokenKind_LT)))  node = add_lexeme(new_binop_node(NodeKind_LT,  node, add()));
    else if ((tok = consume(TokenKind_GT)))  node = add_lexeme(new_binop_node(NodeKind_LT,  add(), node));
    else if ((tok = consume(TokenKind_LEQ))) node = add_lexeme(new_binop_node(NodeKind_LEQ, node, add()));
    else if ((tok = consume(TokenKind_GEQ))) node = add_lexeme(new_binop_node(NodeKind_LEQ, add(), node));
    else                                     break;
  }
  src_pop();
  return node;
}

// add ::= mul ("+" mul | "-" mul)*
static Node *add() {
  if (debug_parse) debug_f("%{} parsing add", this_tok());
  src_push();
  Node *node = mul();
  const Token *tok = NULL;
  while (true) {
    if      ((tok = consume(TokenKind_PLUS)))  node = add_lexeme(new_binop_node(NodeKind_ADD, node, mul()));
    else if ((tok = consume(TokenKind_MINUS))) node = add_lexeme(new_binop_node(NodeKind_SUB, node, mul()));
    else                                       break;
  }
  return src_pop(), node;
}

// mul ::= unary ("*" unary | "/" unary)*
static Node *mul() {
  if (debug_parse) debug_f("%{} parsing mul", this_tok());
  src_push();
  Node *node = unary();
  const Token *tok = NULL;
  while (true) {
    if      ((tok = consume(TokenKind_STAR)))  node = add_lexeme(new_binop_node(NodeKind_MUL, node, unary()));
    else if ((tok = consume(TokenKind_SLASH))) node = add_lexeme(new_binop_node(NodeKind_DIV, node, unary()));
    else                                       break;
  }
  return src_pop(), node;
}

// unary ::= ("+" | "-" | "&" | "*") unary
//         | primary
static Node *unary() {
  if (debug_parse) debug_f("%{} parsing unary", this_tok());
  src_push();
  const Token *tok = NULL;
  if      ((tok = consume(TokenKind_PLUS)))  return src_pop(), unary();
  else if ((tok = consume(TokenKind_MINUS))) return pop_lexeme(new_unop_node(NodeKind_NEG, unary()));
  else if ((tok = consume(TokenKind_AND)))   return pop_lexeme(new_unop_node(NodeKind_ADDR, unary()));
  else if ((tok = consume(TokenKind_STAR)))  return pop_lexeme(new_unop_node(NodeKind_DEREF, unary()));
  else                                       return src_pop(), primary();
}

// primary ::= "(" expr ")" | ident | num
static Node *primary() {
  if (debug_parse) debug_f("%{} parsing primary", this_tok());
  src_push();
  const Token *tok = NULL;
  if (consume(TokenKind_LPAREN)) {
    Node *node = expr();
    expect(TokenKind_RPAREN);
    return src_pop(), node;
  }
  else if ((tok = consume(TokenKind_IDENT))) return pop_lexeme(new_var_node(tok->ident));
  else if ((tok = consume(TokenKind_NUM)))   return pop_lexeme(new_num_node(tok->num));
  else                                       error_f("%{} expected expression", this_tok());
}

Node *parse() {
  Node *node = prog();
  if (src_stack) {
    int size = 0;
    SourceStack *cur = src_stack;
    while (cur) {
      size++;
      cur = cur->last;
    }
    fail_f("non-empty src stack: %d", size);
  }
  if (!match(TokenKind_EOF))
    error_f("%{} extra token", this_tok());
  return node;
}
