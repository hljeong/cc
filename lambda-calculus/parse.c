#include "lc.h"

int fmt_node_kind(const sink s, va_list ap) {
  const NodeKind kind = va_arg(ap, NodeKind);
  if      (kind == NodeKind_VAR) return emitf(s, "var");
  else if (kind == NodeKind_FUN) return emitf(s, "fun");
  else if (kind == NodeKind_APP) return emitf(s, "app");
  else                           fail("unexpected node kind: %d", kind);
}

int fmt_node(const sink s, va_list ap) {
  const Node *node = va_arg(ap, Node *);
  int len = 0;
  {
    const int ret = emitf(s, "{node_kind}", node->kind);
    if (ret < 0) return ret;
    len += ret;
  }

  if (node->kind == NodeKind_VAR) {
    const int ret = emitf(s, "({sv})", node->name);
    if (ret < 0) return ret;
    len += ret;
  }

  return len;
}

static int emit_ast(const sink s, const Node *node, str_builder *sb, const bool last) {
  int len = 0;
  {
    const int ret = emitf(s, "%s%s{node}\n", sb->buf, last ? "└─" : "├─", node);
    if (ret < 0) return ret;
    len += ret;
  }

  const int truncate_to = sb->size;
  sb_append(sb, last ? "  " : "│ ");

  if (node->kind == NodeKind_VAR) {}

  else if (node->kind == NodeKind_FUN) {
    int ret = -1;
    if (node->body) ret = emit_ast(s, node->body, sb, true);
    else ret = debug("%s  └─...\n", sb->buf);
    if (ret < 0) return ret;
    len += ret;
  }

  else if (node->kind == NodeKind_APP) {
    {
      const int ret = emit_ast(s, node->fun, sb, false);
      if (ret < 0) return ret;
      len += ret;
    }
    {
      const int ret = len += emit_ast(s, node->arg, sb, true);
      if (ret < 0) return ret;
      len += ret;
    }
  }

  else fail("{node_kind}", node->kind);

  sb_truncate(sb, truncate_to);
  return len;
}

int fmt_ast(const sink s, va_list ap) {
  str_builder sb = sb_create(256);
  return emit_ast(s, va_arg(ap, Node *), &sb, true);
}

// print all variables climbing up scope hierarchy.
// free variables delimited by '*'
int fmt_scope(const sink s, va_list ap) {
  const Node *node = va_arg(ap, Node *);
  if (node->kind != NodeKind_FUN)
    fail("bad invocation: fmt_scope({node})", node);

    int len = 0;
  {
    const int ret = emitf(s, "{");
    if (ret < 0) return ret;
    len += ret;
  }


  while (node) {
    if (node->var->name.len) {
      const int ret = emitf(s, "{sv}", node->var->name);
      if (ret < 0) return ret;
      len += ret;
    }
    else {
      const int ret = emitf(s, "*");
      if (ret < 0) return ret;
      len += ret;
    }

    if ((node = node->par)) {
      const int ret = emitf(s, ", ");
      if (ret < 0) return ret;
      len += ret;
    }
  }

  {
    const int ret = emitf(s, "}");
    if (ret < 0) return ret;
    len += ret;
  }

  return len;
}

static int _fmt_lambda(const sink s, const Node *node, const bool ext, const bool nested_fun) {
  int len = 0;

  if (node->kind == NodeKind_VAR) {
    const int ret = emitf(s, "{sv}", node->name);
    if (ret < 0) return ret;
    len += ret;
  }

  else if (node->kind == NodeKind_FUN) {
    if (!nested_fun) {
      const int ret = emitf(s, "(\\");
      if (ret < 0) return ret;
      len += ret;
    }

    {
      const int ret = _fmt_lambda(s, node->var, ext, false);
      if (ret < 0) return ret;
      len += ret;
    }

    if (ext && node->body->kind == NodeKind_FUN) {
      {
        const int ret = emitf(s, " ");
        if (ret < 0) return ret;
        len += ret;
      }

      {
        const int ret = _fmt_lambda(s, node->body, ext, true);
        if (ret < 0) return ret;
        len += ret;
      }
    }

    else {
      {
        const int ret = emitf(s, ".");
        if (ret < 0) return ret;
        len += ret;
      }

      {
        const int ret = _fmt_lambda(s, node->body, ext, false);
        if (ret < 0) return ret;
        len += ret;
      }
    }

    if (!nested_fun) {
      const int ret = emitf(s, ")");
      if (ret < 0) return ret;
      len += ret;
    }
  }

  else if (node->kind == NodeKind_APP) {
    {
      const int ret = emitf(s, "(");
      if (ret < 0) return ret;
      len += ret;
    }

    {
      const int ret = _fmt_lambda(s, node->fun, ext, false);
      if (ret < 0) return ret;
      len += ret;
    }

    {
      const int ret = emitf(s, " ");
      if (ret < 0) return ret;
      len += ret;
    }

    {
      const int ret = _fmt_lambda(s, node->arg, ext, false);
      if (ret < 0) return ret;
      len += ret;
    }

    {
      const int ret = emitf(s, ")");
      if (ret < 0) return ret;
      len += ret;
    }
  }

  else fail("not implemented: %u", (uint32_t) node->kind);

  return len;
}

int fmt_lambda(const sink s, va_list ap) {
  const Node *node = va_arg(ap, Node *);
  const bool ext = va_arg(ap, int);
  return _fmt_lambda(s, node, ext, false);
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

static Node *new_var(const str_view name, Node *ref) {
  Node *node = new_node(NodeKind_VAR);
  node->name = name;
  node->ref = ref ? ref : node;
  node->lexeme = name;
  return node;
}

static Node *get_var(const str_view name) {
  Node *scope = ctx.parser.scope, *prev_scope = NULL;
  while (scope) {
    if (!sv_cmp(scope->var->name, name))
      return new_var(name, scope->var);  // found binding var node,
                                         // create a reference
    scope = (prev_scope = scope)->par;
  }

  // exhausted all ancestor scopes and did not find the bind site.
  // create a free variable binding var node
  scope = new_node(NodeKind_FUN);
  scope->var = new_var(name, NULL);
  scope->lexeme = name;
  prev_scope->par = scope;

  return scope->var;
}

Node *new_fun(Node *var, Node *body) {
  Node *node = new_node(NodeKind_FUN);
  node->var = var;
  node->body = body;
  return node;
}

Node *new_app(Node *fun, Node *arg) {
  Node *node = new_node(NodeKind_APP);
  node->fun = fun;
  node->arg = arg;
  return node;
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

// atom ::= "(" expr ")" | fun | var
static Node *atom(void) {
  const char *start = ctx.parser.tok->lexeme.loc;
  if (consume(TokenKind_LPAREN)) {
    Node *node = expr();
    node->lexeme = sv_create(start, sv_end(expect(TokenKind_RPAREN)->lexeme) - start);
    return node;
  }
  else if (consume(TokenKind_BACKSLASH)) return fun();
  else if (match(TokenKind_IDENT))       return var(/*binding=*/ false);
  else                                   error("{@cur_tok} expected expression, got {token}",
                                               ctx.parser.tok);
}

// fun ::= ("\") var+ "." expr
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
    if (!(0 <= col && col <= ctx.src_len))
      fail("invalid loc: %d, src_len=%d",
            col, ctx.src_len);

    debug("%*s", col, "");
    for (int i = 0; i < left.len; i++) debug("%c", '~');
  }

  {
    const int col = right.loc - ctx.src;
    if (!(0 <= col && col <= ctx.src_len))
      fail("invalid loc: %d, src_len=%d",
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
