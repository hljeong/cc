#include "lc.h"

static bool is_atom_first(const TokenKind kind) {
  return (kind == TokenKind_LPAREN) ||
         (kind == TokenKind_LET) ||
         (kind == TokenKind_BACKSLASH) ||
         (kind == TokenKind_IDENT);
}

// helper semantics:
// - match():    check whether the next token satifies some condition
// - advance():  return the next token and advance the cursor
// - consume():  advance() if the next token match()'s, otherwise return NULL
// - expect():   consume() and assert that the token has been consumed

static bool match_pred(bool (*pred)(const TokenKind)) {
  return pred(ctx.parser.tok->kind);
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
  if (!tok) error("{@cur_tok} expected {token_kind}, got: {token}",
                   kind, ctx.parser.tok);
  return tok;
}

static Node *expr(void);
static Node *atom(void);
static Node *decl(void);
static Node *fun(void);
static Node *var(const bool binding);

// expr ::= expr atom | atom
static Node *expr(void) {
  const char *start = ctx.parser.tok->lexeme.loc;
  Node *node = atom();
  while (match_pred(is_atom_first)) {
    node = new_app(node, atom());
    node->lexeme = sv_create(start, sv_end(node->arg->lexeme) - start);
  }
  return node;
}

// atom ::= "(" expr ")" | "let" decl | "\" fun | var
static Node *atom(void) {
  const char *start = ctx.parser.tok->lexeme.loc;
  if (consume(TokenKind_LPAREN)) {
    Node *node = expr();
    node->lexeme = sv_create(start, sv_end(expect(TokenKind_RPAREN)->lexeme) - start);
    return node;
  }
  else if (consume(TokenKind_LET))       return decl();
  else if (consume(TokenKind_BACKSLASH)) return fun();
  else if (match(TokenKind_IDENT))       return var(/*binding=*/ false);
  else                                   error("{@cur_tok} expected expression, got {token}",
                                               ctx.parser.tok);
}

// decl ::= var ":=" expr ";" expr
static Node *decl(void) {
  const char *start = ctx.parser.tok->lexeme.loc;
  Node *node = new_node(NodeKind_APP);
  node->fun = new_node(NodeKind_FUN);
  node->fun->par = ctx.parser.scope;
  node->fun->var = var(/*binding=*/ true);
  expect(TokenKind_WALRUS);
  node->arg = expr();
  expect(TokenKind_SEMICOLON);
  ctx.parser.scope = node->fun;
  node->fun->body = expr();
  ctx.parser.scope = node->fun->par;
  node->lexeme = sv_create(start, sv_end(node->arg->lexeme) - start);
  node->fun->lexeme = node->lexeme;
  return node;
}

// fun ::= var (fun | "." expr)
static Node *fun(void) {
  const char *start = ctx.parser.tok->lexeme.loc;
  Node *node = new_node(NodeKind_FUN);

  // set parent scope to current parser scope
  node->par = ctx.parser.scope;
  // create a binding var node at parent scope
  node->var = var(/*binding=*/ true);
  // push current scope onto stack
  ctx.parser.scope = node;

  if (consume(TokenKind_DOT)) {
    node->body = expr();

    // populate lexeme
    node->lexeme = sv_create(start, sv_end(node->body->lexeme) - start);
  }

  else {
    node->body = fun();

    // populate lexeme
    node->lexeme = sv_create(start, sv_end(node->body->lexeme) - start);
  }

  // pop current scope off of stack
  // (back to parent scope)
  ctx.parser.scope = node->par;

  return node;
}

// var ::= ident
static Node *var(const bool binding) {
  const str_view name = expect(TokenKind_IDENT)->lexeme;
  if (binding) return new_var(name, NULL); // create a binding var node
  else         return get_var(name);       // get a reference var node to an existing
                                           // bound or free var node. if the variable
                                           // does not exist, create a binding var node
                                           // for the free variable
}

static void debug_shadow(const str_view shadower,
                         const str_view shadowee,
                         const char *desc) {
  // todo: colors to emphasize which one shadows which
  const bool shadower_first = shadower.loc < shadowee.loc;
  const str_view left =  shadower_first ? shadower : shadowee;
  const str_view right = shadower_first ? shadowee : shadower;

  debug("%s\n", ctx.src);
  {
    const int col = left.loc - ctx.src;
    assert(0 <= col && col <= ctx.src_len,
           "invalid loc: %d, src_len=%d",
           col, ctx.src_len);

    debug("%*s", col, "");
    for (int i = 0; i < left.len; i++) debug("%c", '~');
  }

  {
    const int col = right.loc - ctx.src;
    assert(0 <= col && col <= ctx.src_len,
           "invalid loc: %d, src_len=%d",
           col, ctx.src_len);

    const int d_col = col - (sv_end(left) - ctx.src);
    for (int i = 0; i < d_col; i++) debug("%c", '.');
    for (int i = 0; i < right.len; i++) debug("%c", '~');
    debug("\n");
  }

  {
    const int col = shadower.loc - ctx.src;
    debug("%*s^ warning: \"{sv}\" shadows %s variable\n", col, "",
           shadower, desc);
  }

  debug("\n");
}

// print warnings to shadowing variable bindings, i.e.,
// a binding that has the same name as:
// - some variable that exists in the current scope or
// - some free variable whose existence is implied
//   by the rest of the expression
//
// the shadowing and the shadowed binding site are underlined,
// with the warning message positioned underneath the shadowing
// binding site. caveat: for free variables that are implied by
// a reference, the first such reference is underlined instead
//
// some examples:
// $ lc '(\foo.\bar.\foo.bar)'
// (\foo.\bar.\foo.bar)
//   ~~~.......~~~
//             ^ warning: "foo" shadows bound variable
//
//
// $ lc '(\foo.foo) foo bar (\bar.bar)'
// (\foo.foo) foo bar (\bar.bar)
//   ~~~......~~~
//   ^ warning: "foo" shadows free variable
//
// (\foo.foo) foo bar (\bar.bar)
//                ~~~...~~~
//                      ^ warning: "bar" shadows free variable
//
//
// $ lc '(\foo.bar) (\bar.foo)'
// (\foo.bar) (\bar.foo)
//   ~~~............~~~
//   ^ warning: "foo" shadows free variable
//
// (\foo.bar) (\bar.foo)
//       ~~~....~~~
//              ^ warning: "bar" shadows free variable
//
// note that `(\foo.bar)` implies the existence of free variable `bar`
// and `(\bar.foo)` implies the existence of free variable `foo`
//
// this is done by walking the ast and checking for every binding
// whether there exists a variable with the same name up the scope
// chain
static void warn_shadows(const Node *node) {
  if (node->kind == NodeKind_VAR) return;

  else if (node->kind == NodeKind_FUN) {
    const Node *cur = node;
    const char *desc = "bound";
    while ((cur = cur->par)) {
      // hit dummy scope -- variables up the chain are free
      if (!cur->var->name.len) {
        desc = "free";
        continue;
      }

      if (!sv_cmp(cur->var->name, node->var->name)) {
        debug_shadow(node->var->lexeme, cur->var->lexeme, desc);
        break;
      }
    }

    warn_shadows(node->body);
  }

  else if (node->kind == NodeKind_APP) {
    warn_shadows(node->fun);
    warn_shadows(node->arg);
  }

  else fail("{node_kind}", node->kind);
}

Node *parse(void) {
  // *please* only ever call this *once*
  static Node dummy_scope = {
    .kind = NodeKind_FUN,
    .par = NULL,
  };
  dummy_scope.lexeme = sv_create(ctx.lexer.loc, 0);
  // 0-length variable name guarantees nothing
  // will ever match the dummy var
  dummy_scope.var = new_var(dummy_scope.lexeme, NULL);
  ctx.parser.scope = &dummy_scope;
  Node *node = expr();
  if (!match(TokenKind_EOF))
    error("{@cur_tok} extra token");
  warn_shadows(node);
  return node;
}
