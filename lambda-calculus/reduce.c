#include "lc.h"

// given any node, a binding var node, and a value to substitute
// (also an arbitrary node):
// if `node` contains references to `var`, return a new node
// that is structurely identical to `node` except every occurrence
// of a reference var node to `var` is replaced with `subval`
//
// if `node` does not contain any references to `var`, return
// `node` as is
//
// note that `node`, `var`, and `subval` must not overlap
Node *sub(Node *node, Node *var, Node *subval) {
  if (var->kind != NodeKind_VAR)
    failf("bad invocation: sub(*, %s, *)", node_kind_to_str(var->kind));

  if (node->kind == NodeKind_VAR) {
    // `node->ref` points to the exact node that
    // binds the variable so pointer equality is used,
    // effectively using the pointer value as a handle
    // of some kind
    return node->ref == var ? subval
                            : node;
  }

  else if (node->kind == NodeKind_FUN) {
    Node *expr = sub(node->expr, var, subval);
    // if substitution occurs in the child node,
    // a new node must be created
    if (expr != node->expr) return new_fun(node->var, expr);

    return node;
  }

  else if (node->kind == NodeKind_APP) {
    Node *fun = sub(node->fun, var, subval);
    Node *val = sub(node->val, var, subval);
    // if substitution occurs in the child node,
    // a new node must be created
    if (fun != node->fun || val != node->val) return new_app(fun, val);

    return node;
  }

  else failf("%s", node_kind_to_str(node->kind));
}

// perform beta-reduction on the given application node
// return `node` as is if there is nothing to be done
Node *beta(Node *node) {
  if (node->kind != NodeKind_APP)
    failf("bad invocation: beta(%s)", node_kind_to_str(node->kind));

  return node->fun->kind == NodeKind_FUN ? sub(node->fun->expr,
                                               node->fun->var,
                                               node->val)
                                         : node;
}

Node *step(Node *node, bool whnf) {
  if (node->kind == NodeKind_VAR) return node;

  else if (node->kind == NodeKind_FUN) {
    // weak head normal form does not reduce inside lambdas
    if (whnf) return node;

    Node *expr = step(node->expr, false);
    // if beta-reduction occurs in the child node,
    // a new node must be created
    if (expr != node->expr) return new_fun(node->var, expr);

    return node;
  }

  else if (node->kind == NodeKind_APP) {
    // immediately beta-reduce outermost lambdas. this is called
    // the normal order (reduce leftmost, outermost redex ("reducible
    // expression") first), which guarantees reduction to normal
    // form if it exists (per church-rosser theorem). applicative
    // order (reduce leftmost, innermost redex first) may lead to
    // infinite loops even if a normal form exists.
    // consider `(\x.\y.y)((\z.z z)(\z.z z))`. applicative
    // order attempts to reduce `(\z.z z)(\z.z z)` first, which simply
    // yields itself, immediately leading to an infinite loop.
    // normal order reduces the entire expression in one step to
    // `\y.y` since `x` is not used in the function body of `(\x.\y.y)`
    if (node->fun->kind == NodeKind_FUN) return beta(node);

    // weak head normal form does not reduce applications whose
    // lhs is not a function
    if (whnf) return node;

    Node *fun = step(node->fun, whnf);
    // if beta-reduction occurs in the child node,
    // a new node must be created
    if (fun != node->fun) return new_app(fun, node->val);

    Node *val = step(node->val, whnf);
    if (val != node->val) return new_app(node->fun, val);

    return node;
  }

  else failf("%s", node_kind_to_str(node->kind));
}
