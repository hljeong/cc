#include "lc.h"

Node *sub(Node *node, Node *var, Node *subval) {
  if (var->kind != NodeKind_VAR)
    failf("bad invocation: sub(*, %s, *)", node_kind_to_str(var->kind));

  if (node->kind == NodeKind_VAR) {
    return sv_eq(node->name, var->name) ? subval
                                        : node;
  }

  else if (node->kind == NodeKind_FUN) {
    Node *expr = sub(node->expr, var, subval);
    if (expr != node->expr) return new_fun(node->var, expr);

    return node;
  }

  else if (node->kind == NodeKind_APP) {
    Node *fun = sub(node->fun, var, subval);
    Node *val = sub(node->val, var, subval);
    if (fun != node->fun || val != node->val) return new_app(fun, val);

    return node;
  }

  else failf("not implemented: %u", (uint32_t) node->kind);
}

Node *beta(Node *node) {
  if (node->kind != NodeKind_APP)
    failf("bad invocation: beta(%s)", node_kind_to_str(node->kind));

  return node->fun->kind == NodeKind_FUN ? sub(node->fun->expr,
                                                 node->fun->var,
                                                 node->val)
                                           : node;
}

Node *step(Node *node, bool whnf) {
  if      (node->kind == NodeKind_VAR) return node;

  else if (node->kind == NodeKind_FUN) {
    if (whnf) return node;

    Node *expr = step(node->expr, false);
    if (expr != node->expr) return new_fun(node->var, expr);

    return node;
  }

  else if (node->kind == NodeKind_APP) {
    if (node->fun->kind == NodeKind_FUN) return beta(node);

    Node *fun = step(node->fun, whnf);
    if (fun != node->fun) return new_app(fun, node->val);

    return node;
  }

  else failf("not implemented: %u", (uint32_t) node->kind);
}
