#include "lc.h"

const char *node_kind_to_str(const NodeKind kind) {
  if      (kind == NodeKind_VAR) return "var";
  else if (kind == NodeKind_FUN) return "fun";
  else if (kind == NodeKind_APP) return "app";
  else                           failf("node_kind_to_str(%u)",
                                       (uint32_t) kind);
}

static void _debug_ast(const Node *node, const char *prefix, const bool last) {
  char child_prefix[256];
  snprintf(child_prefix, sizeof(child_prefix), "%s%s",
           prefix, last ? "  " : "│ ");

  const char *branch = last ? "└─" : "├─";

  if (node->kind == NodeKind_VAR) {
    debugf("%s%svar("sv_fmt")\n", prefix, branch, sv_arg(node->name));
    // debugf_at_node(node, "var("sv_fmt")", sv_arg(node->name));
  }

  else if (node->kind == NodeKind_FUN) {
    debugf("%s%sfun\n", prefix, branch);
    // debugf_at_node(node, "fun");
    _debug_ast(node->var, child_prefix, false);
    if (node->body) _debug_ast(node->body, child_prefix, true);
    else debugf("%s  └─...\n", prefix);
  }

  else if (node->kind == NodeKind_APP) {
    debugf("%s%sapp\n", prefix, branch);
    // debugf_at_node(node, "app");
    _debug_ast(node->fun, child_prefix, false);
    _debug_ast(node->arg, child_prefix, true);
  }

  else failf("%s", node_kind_to_str(node->kind));
}

void debug_ast(const Node *node) {
  _debug_ast(node, "", true);
}

// print all variables climbing up scope hierarchy.
// free variables delimited by '*'
void debug_fun_scope(const Node *node) {
  if (node->kind != NodeKind_FUN)
    failf("bad invocation: debug_fun_scope(%s)", node_kind_to_str(node->kind));

  debugf("debug_fun_scope(): ");
  const Node *cur = node;
  if (cur->var->name.len) debugf(sv_fmt, sv_arg(cur->var->name));
  else                    debugf("*");
  while ((cur = cur->par)) {
    debugf(", ");
    if (cur->var->name.len) debugf(sv_fmt, sv_arg(cur->var->name));
    else                    debugf("*");
  }
  debugf("\n");
}

static void _debug_unparse(const Node *node) {
  if (node->kind == NodeKind_VAR) {
    debugf(sv_fmt, sv_arg(node->name));
  }

  else if (node->kind == NodeKind_FUN) {
    debugf("(\\");
    _debug_unparse(node->var);
    debugf(".");
    _debug_unparse(node->body);
    debugf(")");
  }

  else if (node->kind == NodeKind_APP) {
    debugf("(");
    _debug_unparse(node->fun);
    debugf(" ");
    _debug_unparse(node->arg);
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
    _print_unparse(node->body);
    printf(")");
  }

  else if (node->kind == NodeKind_APP) {
    printf("(");
    _print_unparse(node->fun);
    printf(" ");
    _print_unparse(node->arg);
    printf(")");
  }

  else failf("%u", (uint32_t) node->kind);
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

static Node *new_var(const StringView name, Node *ref) {
  Node *node = new_node(NodeKind_VAR);
  node->name = name;
  node->ref = ref ? ref : node;
  node->lexeme = name;
  return node;
}

static Node *get_var(const StringView name) {
  Node *scope = ctx.parser.scope, *prev_scope = NULL;
  while (scope) {
    if (sv_eq(scope->var->name, name))
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

static int match_pred(bool (*pred)(const TokenKind)) {
  return pred(ctx.parser.tok->kind);
}

static int match(const TokenKind kind) {
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

static Node *expr(void);
static Node *atom(void);
static Node *fun(void);
static Node *var(const bool lookup);

// expr ::= expr atom | atom
static Node *expr(void) {
  const char *start = ctx.parser.tok->lexeme.loc;
  Node *node = atom();
  while (match_pred(is_atom_first)) {
    Node *arg = atom();
    node = new_app(node, arg);
    node->lexeme = sv(start, sv_end(node->arg->lexeme) - start);
  }
  return node;
}

// atom ::= "(" expr ")" | fun | var
static Node *atom(void) {
  const char *start = ctx.parser.tok->lexeme.loc;
  if (consume(TokenKind_LPAREN)) {
    Node *node = expr();
    node->lexeme = sv(start, sv_end(expect(TokenKind_RPAREN)->lexeme) - start);
    return node;
  }
  else if (match(TokenKind_BACKSLASH)) return fun();
  else if (match(TokenKind_IDENT))     return var(/*binding=*/ false);
  else                                 errorf_tok("expected expression, got %s",
                                                  token_to_str(ctx.parser.tok));
}

// fun ::= "\" var "." expr
static Node *fun(void) {
  const char *start = ctx.parser.tok->lexeme.loc;
  Node *node = new_node(NodeKind_FUN);

  expect(TokenKind_BACKSLASH);

  // set parent scope to current parser scope
  node->par = ctx.parser.scope;
  // create a binding var node at parent scope
  node->var = var(/*binding=*/ true);
  // push current scope onto stack
  ctx.parser.scope = node;

  expect(TokenKind_DOT);
  node->body = expr();

  // populate lexeme
  node->lexeme = sv(start, sv_end(node->body->lexeme) - start);

  // pop current scope off of stack
  // (back to parent scope)
  ctx.parser.scope = node->par;

  return node;
}

// var ::= ident
static Node *var(const bool binding) {
  const StringView name = expect(TokenKind_IDENT)->lexeme;
  if (binding) return new_var(name, NULL); // create a binding var node
  else         return get_var(name);       // get a reference var node to an existing
                                           // bound or free var node. if the variable
                                           // does not exist, create a binding var node
                                           // for the free variable
}

static void debug_shadow(const StringView shadower,
                         const StringView shadowee,
                         const char *desc) {
  // todo: colors to emphasize which one shadows which
  const bool shadower_first = shadower.loc < shadowee.loc;
  const StringView left =  shadower_first ? shadower : shadowee;
  const StringView right = shadower_first ? shadowee : shadower;

  debugf("%s\n", ctx.src);
  {
    const int col = left.loc - ctx.src;
    if (!(0 <= col && col <= ctx.src_len))
      failf("invalid loc: %d, src_len=%d",
            col, ctx.src_len);

    debugf("%*s", col, "");
    for (int i = 0; i < left.len; i++) debugf("%c", '~');
  }

  {
    const int col = right.loc - ctx.src;
    if (!(0 <= col && col <= ctx.src_len))
      failf("invalid loc: %d, src_len=%d",
            col, ctx.src_len);

    const int d_col = col - (sv_end(left) - ctx.src);
    for (int i = 0; i < d_col; i++) debugf("%c", '.');
    for (int i = 0; i < right.len; i++) debugf("%c", '~');
    debugf("\n");
  }

  {
    const int col = shadower.loc - ctx.src;
    debugf("%*s^ warning: \""sv_fmt"\" shadows %s variable\n", col, "",
           sv_arg(shadower), desc);
  }

  debugf("\n");
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

      if (sv_eq(cur->var->name, node->var->name)) {
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

  else failf("%s", node_kind_to_str(node->kind));
}

Node *parse(void) {
  // *please* only ever call this *once*
  static Node dummy_scope = {
    .kind = NodeKind_FUN,
    .par = NULL,
  };
  dummy_scope.lexeme = sv(ctx.lexer.loc, 0);
  // 0-length variable name guarantees nothing
  // will ever match the dummy var
  dummy_scope.var = new_var(dummy_scope.lexeme, NULL);
  ctx.parser.scope = &dummy_scope;
  Node *node = expr();
  if (!match(TokenKind_EOF))
    errorf_tok("extra token");
  warn_shadows(node);
  return node;
}
