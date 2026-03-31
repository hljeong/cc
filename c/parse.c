#include "cc.h"

#include <stdint.h>
#include <stdlib.h>
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
  if (!tok) error("%{@cur_tok} expected %{token_kind}, got: %{token}",
                  kind, ctx.parser.tok);
  return tok;
}

// These cannot be `const` due to the intrusive linked list
static Node *prog();
static Node *fun_decl();
static Node *stmt();
static Type *decl_spec();
static Node *declarator(Type *base_type);
static Node *var_decl(Type *base_type);
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
  assert(src_stack, "empty src stack");
  assert(ctx.parser.tok->prev, "no tokens consumed before %{token}", ctx.parser.tok);
  const StringView last = ctx.parser.tok->prev->lexeme;
  const char *start = src_stack->loc;
  return (StringView) {
    .loc = start,
    .len = last.loc + last.len - start,
  };
}

static void src_pop() {
  assert(src_stack, "empty src stack");
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

static bool is_type(const Token *tok) {
  assert(tok->kind == TokenKind_IDENT);
  return sv_eq_s(tok->lexeme, "int");
}

// todo: parser combinator

// prog ::= fun_decl*
static Node *prog() {
  if (debug_parse) debug("%{@cur_tok} parsing prog");
  src_push();
  Node *node = new_node(NodeKind_PROG);
  Node head = {};
  Node *cur = &head;
  while (!match(TokenKind_EOF)) {
    cur = (cur->next = fun_decl());
  }
  node->list.head = head.next;
  return pop_lexeme(node);
}

// fun_decl ::= decl_spec declarator "(" ")" "{" stmt* "}"
static Node *fun_decl() {
  if (debug_parse) debug("%{@cur_tok} parsing fun_decl");
  src_push();
  Node *node = new_node(NodeKind_FUN_DECL);
  {
    Type *type = decl_spec();
    node->fun_decl.decl = declarator(type);
  }
  {
    src_push();
    Node head = {};
    Node *cur = &head;
    expect(TokenKind_LBRACE);
    while (!consume(TokenKind_RBRACE)) {
      cur = (cur->next = stmt());
    }
    node->fun_decl.body = pop_lexeme(new_list_node(NodeKind_BLOCK, head.next));
  }
  return pop_lexeme(node);
}

// stmt ::= "return" expr ";"
//        | "{" stmt* "}"
//        | "if" "(" expr ")" stmt ("else" stmt)?
//        | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//        | "while" "(" expr ")" stmt
//        | declspec (declaration? ("," declaration)*)? ";"
//        | expr? ";"
static Node *stmt() {
  if (debug_parse) debug("%{@cur_tok} parsing stmt");
  src_push();

  // ";"
  if (consume(TokenKind_SEMICOLON))
  {
    // cool trick from chibicc
    return pop_lexeme(new_list_node(NodeKind_BLOCK, NULL));
  }

  // "{" stmt* "}"
  else if (consume(TokenKind_LBRACE)) {
    Node head = {};
    Node *cur = &head;
    while (!consume(TokenKind_RBRACE))
      cur = (cur->next = stmt());
    return pop_lexeme(new_list_node(NodeKind_BLOCK, head.next));
  }

  // "return" expr ";"
  else if (consume(TokenKind_RETURN)) {
    Node *node = new_unop_node(NodeKind_RETURN, expr());
    expect(TokenKind_SEMICOLON);
    return pop_lexeme(node);
  }

  // "if" "(" expr ")" stmt ("else" stmt)?
  else if (consume(TokenKind_IF)) {
    Node *node = new_node(NodeKind_IF);
    expect(TokenKind_LPAREN);
    node->if_.cond = expr();
    expect(TokenKind_RPAREN);
    node->if_.then = stmt();
    if (consume(TokenKind_ELSE))
      node->if_.else_ = stmt();
    return pop_lexeme(node);
  }

  // "for" "(" expr? ";" expr? ";" expr? ")" stmt
  else if (consume(TokenKind_FOR)) {
    // wow for loops have horrible syntax...
    Node *node = new_node(NodeKind_FOR);
    expect(TokenKind_LPAREN);
    if (!consume(TokenKind_SEMICOLON)) {
      node->for_.init = expr();
      expect(TokenKind_SEMICOLON);
    }
    if (!consume(TokenKind_SEMICOLON)) {
      node->for_.cond = expr();
      expect(TokenKind_SEMICOLON);
    }
    if (!consume(TokenKind_RPAREN)) {
      node->for_.inc = expr();
      expect(TokenKind_RPAREN);
    }
    node->for_.body = stmt();
    return pop_lexeme(node);
  }

  // "while" "(" expr ")" stmt
  else if (consume(TokenKind_WHILE)) {
    Node *node = new_node(NodeKind_FOR);
    expect(TokenKind_LPAREN);
    node->for_.cond = expr();
    expect(TokenKind_RPAREN);
    node->for_.body = stmt();
    return pop_lexeme(node);
  }

  // decl_spec (var_decl ("," var_decl)*)? ";"
  else if (match(TokenKind_IDENT) && is_type(ctx.parser.tok)) {
    Type *base_type = decl_spec();

    Node head = {};
    Node *cur = &head;
    while (true) {
      cur = (cur->next = var_decl(base_type));
      if (consume(TokenKind_SEMICOLON)) break;
      expect(TokenKind_COMMA);
    }

    Node *node = new_node(NodeKind_DECL_STMT);
    node->list.head = head.next;
    return pop_lexeme(node);
  }

  // expr? ";"
  else {
    Node *node = new_node(NodeKind_EXPR_STMT);
    node->expr_stmt.expr = expr();
    expect(TokenKind_SEMICOLON);
    return pop_lexeme(node);
  }
}

// decl_spec ::= "int"
static Type *decl_spec() {
  const Token *tok = expect(TokenKind_IDENT);
  if (strncmp(tok->lexeme.loc, "int", tok->lexeme.len))
    error("%{@tok} not a type", tok);
  return &t.int_;
}

// var_decl ::= declarator ("=" expr)?
static Node *var_decl(Type *base_type) {
  src_push();
  Node *node = new_node(NodeKind_VAR_DECL);

  node->var_decl.decl = declarator(base_type);

  if (consume(TokenKind_EQ)) {
    Node *var_ref = new_var_node(node->var_decl.decl->decl.name);
    var_ref->lexeme = node->var_decl.decl->lexeme;
    node->var_decl.init = add_lexeme(new_binop_node(NodeKind_ASSIGN, var_ref, expr()));
  }

  return pop_lexeme(node);
}

// declarator ::= "*"* ident ("(" params? ")")?
// params ::= param ("," param)*
// param ::= decl_spec declarator
static Node *declarator(Type *base_type) {
  src_push();
  Node *node = new_node(NodeKind_DECL);

  node->type = base_type;
  while (consume(TokenKind_STAR))
    node->type = new_ptr_type(node->type);

  node->decl.name = expect(TokenKind_IDENT)->lexeme;

  if (consume(TokenKind_LPAREN)) {
    if (!consume(TokenKind_RPAREN)){
      Node head = {};
      Node *cur = &head;
      {
        Type *type = decl_spec();
        cur = (cur->next = declarator(type));
      }
      while (!consume(TokenKind_RPAREN)) {
        expect(TokenKind_COMMA);
        Type *type = decl_spec();
        cur = (cur->next = declarator(type));
      }
      node->decl.params = head.next;
    }
    node->type = new_fun_type(node->type, node->decl.params);
  }

  return pop_lexeme(node);
}

// expr ::= assign
static Node *expr() {
  if (debug_parse) debug("%{@cur_tok} parsing expr");
  return assign();
}

// note: right-associative
// assign ::= equality ("=" assign)?
static Node *assign() {
  if (debug_parse) debug("%{@cur_tok} parsing assign");
  src_push();
  Node *node = equality();
  return consume(TokenKind_EQ) ? pop_lexeme(new_binop_node(NodeKind_ASSIGN, node, assign()))
                               : (src_pop(), node);
}

// equality ::= relational ("==" relational | "!=" relational)*
static Node *equality() {
  if (debug_parse) debug("%{@cur_tok} parsing equality");
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
  if (debug_parse) debug("%{@cur_tok} parsing relational");
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
  if (debug_parse) debug("%{@cur_tok} parsing add");
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
  if (debug_parse) debug("%{@cur_tok} parsing mul");
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
  if (debug_parse) debug("%{@cur_tok} parsing unary");
  src_push();
  const Token *tok = NULL;
  if      ((tok = consume(TokenKind_PLUS)))  return src_pop(), unary();
  else if ((tok = consume(TokenKind_MINUS))) return pop_lexeme(new_unop_node(NodeKind_NEG, unary()));
  else if ((tok = consume(TokenKind_AND)))   return pop_lexeme(new_unop_node(NodeKind_ADDR, unary()));
  else if ((tok = consume(TokenKind_STAR)))  return pop_lexeme(new_unop_node(NodeKind_DEREF, unary()));
  else                                       return src_pop(), primary();
}

// primary ::= "(" expr ")" | ident args? | num
// args ::= "(" (expr ("," expr)*)? ")"
static Node *primary() {
  if (debug_parse) debug("%{@cur_tok} parsing primary");
  src_push();
  const Token *tok = NULL;

  if (consume(TokenKind_LPAREN)) {
    Node *node = expr();
    expect(TokenKind_RPAREN);
    return src_pop(), node;
  }

  else if ((tok = consume(TokenKind_NUM))) {
    return pop_lexeme(new_num_node(tok->num.value));
  }

  else if ((tok = consume(TokenKind_IDENT))) {
    Node *node = add_lexeme(new_var_node(tok->ident.name));
    if (consume(TokenKind_LPAREN)) {
      Node *call = new_node(NodeKind_CALL);
      call->call.fun = node;
      node = call;
      Node head = {};
      Node *cur = &head;
      if (!consume(TokenKind_RPAREN)) {
        cur = (cur->next = expr());
        while (!consume(TokenKind_RPAREN)) {
          expect(TokenKind_COMMA);
          cur = (cur->next = expr());
        }
      }
      node->call.args = head.next;
      add_lexeme(node);
    }
    return src_pop(), node;
  }

  else error("%{@cur_tok} expected expression");
}

void parse() {
  ctx.parser.tok = ctx.toks;
  Node *node = prog();
  if (src_stack) {
    int size = 0;
    SourceStack *cur = src_stack;
    while (cur) {
      size++;
      cur = cur->last;
    }
    fail("non-empty src stack: %d", size);
  }
  ctx.ast = node;
}
