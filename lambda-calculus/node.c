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
  int len = check(emitf(s, "{node_kind}", node->kind));

  if (node->kind == NodeKind_VAR) {
    len += check(emitf(s, "({sv})", node->name));
  }

  return len;
}

static int emit_ast(const sink s, const Node *node, str_builder *sb, const bool last) {
  int len = check(emitf(s, "%s%s{node}\n", sb->buf, last ? "└─" : "├─", node));

  const int truncate_to = sb->size;
  sb_append(sb, last ? "  " : "│ ");

  if (node->kind == NodeKind_VAR) {}

  else if (node->kind == NodeKind_FUN) {
    len += check(emit_ast(s, node->var, sb, false));
    if (node->body) len += check(emit_ast(s, node->body, sb, true));
    else len += check(emitf(s, "%s  └─?\n", sb->buf));
  }

  else if (node->kind == NodeKind_APP) {
    len += check(emit_ast(s, node->fun, sb, false));
    len += check(emit_ast(s, node->arg, sb, true));
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
  assert(node->kind == NodeKind_FUN,
         "bad invocation: fmt_scope({node})", node);

  int len = check(emitf(s, "{{"));

  while (node) {
    if (node->var->name.len)
      len += check(emitf(s, "{sv}", node->var->name));
    else
      len += check(emitf(s, "*"));

    if ((node = node->par))
      len += check(emitf(s, ", "));
  }

  len += check(emitf(s, "}}"));

  return len;
}

static int emit_lambda(const sink s,
                       const Node *node,
                       const bool ext,
                       const bool lhs,
                       const bool nested_fun,
                       const bool right_assoc_app) {
  int len = 0;

  if (node->kind == NodeKind_VAR) {
    len += check(emitf(s, "{sv}", node->name));
  }

  else if (node->kind == NodeKind_FUN) {
    if (!nested_fun) {
      if (lhs)
        len += check(emitf(s, "("));
      len += check(emitf(s, "\\"));
    }

    len += check(emit_lambda(s, node->var, ext, false, false, false));

    if (!node->body) {
      len += check(emitf(s, ".?"));
    }

    else if (ext && node->body->kind == NodeKind_FUN) {
      len += check(emitf(s, " "));
      len += check(emit_lambda(s, node->body, ext, false, true, false));
    }

    else {
      len += check(emitf(s, "."));
      len += check(emit_lambda(s, node->body, ext, false, false, false));
    }

    if (!nested_fun && lhs)
      len += check(emitf(s, ")"));
  }

  else if (node->kind == NodeKind_APP) {
    if (right_assoc_app)
      len += check(emitf(s, "("));
    len += check(emit_lambda(s, node->fun, ext, true, false, false));
    len += check(emitf(s, " "));
    len += check(emit_lambda(s, node->arg, ext, false, false, node->arg->kind == NodeKind_APP));
    if (right_assoc_app)
      len += check(emitf(s, ")"));
  }

  else fail("{node_kind}", node->kind);

  return len;
}

int fmt_lambda(const sink s, va_list ap) {
  const Node *node = va_arg(ap, Node *);
  const bool ext = va_arg(ap, int);
  return emit_lambda(s, node, ext, false, false, false);
}

Node *new_node(const NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

Node *new_var(const str_view name, Node *ref) {
  Node *node = new_node(NodeKind_VAR);
  node->name = name;
  node->ref = ref ? ref : node;
  node->lexeme = name;
  return node;
}

Node *get_var(const str_view name, Node *scope) {
  Node *prev_scope = NULL;
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

Node *new_fun(Node *scope, Node *var, Node *body) {
  Node *node = new_node(NodeKind_FUN);
  node->par = scope;
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

Node *copy_node(Node *node, Node *scope) {
  if (node->kind == NodeKind_VAR) {
    // free variable, make direct reference to avoid capturing
    if (node->ref == node)
      return new_var(node->name, node);
    // if (scope)
    //   debug("looking up {sv} in {lambda} (scope={scope})\n",
    //         node->lexeme, scope, true, scope);
    Node *ret = get_var(node->name, scope);
    assert(ret->ref != ret);
    return ret;
  }

  else if (node->kind == NodeKind_FUN) {
    Node *ret = new_node(NodeKind_FUN);
    ret->par = scope;
    ret->var = new_var(node->var->name, NULL);
    // debug("copying lambda {lambda} (scope={scope})\n",
    //       node, true, scope);
    ret->body = copy_node(node->body, ret);
    ret->lexeme = node->lexeme;
    return ret;
  }

  else if (node->kind == NodeKind_APP) {
    Node *ret = new_node(NodeKind_APP);
    ret->fun = copy_node(node->fun, scope);
    ret->arg = copy_node(node->arg, scope);
    ret->lexeme = node->lexeme;
    return ret;
  }

  else error("{node_kind}", node->kind);
}
