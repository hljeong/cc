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
static Node *sub(Node *node, Node *var, Node *subval, Node *scope) {
  if (var->kind != NodeKind_VAR)
    fail("bad invocation: sub(*, {node_kind}, *)", var->kind);

  if (node->kind == NodeKind_VAR) {
    // `node->ref` points to the exact node that
    // binds the variable so pointer equality is used,
    // effectively using the pointer value as a handle
    // of some kind
    return node->ref == var ? copy_node(subval, scope) : node;
  }

  else if (node->kind == NodeKind_FUN) {
    Node *body = sub(node->body, var, subval, node);
    // if substitution occurs in the child node,
    // a new node must be created
    if (body != node->body) return new_fun(scope, node->var, body);

    return node;
  }

  else if (node->kind == NodeKind_APP) {
    Node *fun = sub(node->fun, var, subval, scope);
    Node *arg = sub(node->arg, var, subval, scope);
    // if substitution occurs in the child node,
    // a new node must be created
    if (fun != node->fun || arg != node->arg) return new_app(fun, arg);

    return node;
  }

  else fail("{node_kind}", node->kind);
}

//perform beta-reduction on the given application node
// return `node` as is if there is nothing to be done
static Node *beta(Node *node, Node *scope) {
  assert(node->kind == NodeKind_APP,
         "bad invocation: beta({node_kind})", node->kind);
  assert(node->fun->kind == NodeKind_FUN,
         "bad invocation: beta(app({node_kind}, *))", node->fun->kind);

  // debug("substituting {sv} in ({lambda}) for ({lambda}), scope={scope}\n",
  //       node->fun->var->name, node->fun->body, true, node->arg, true, scope);
  return sub(node->fun->body, node->fun->var, node->arg, scope);
}

// return `var` occurs free in `node`
static bool is_free(Node *var, Node *node) {
  assert(var->kind == NodeKind_VAR,
         "bad invocation: is_free({node_kind}, *)", var->kind);

  if      (node->kind == NodeKind_VAR) return node->ref == var;
  else if (node->kind == NodeKind_FUN) return is_free(var, node->body);
  else if (node->kind == NodeKind_APP) return is_free(var, node->fun) || is_free(var, node->arg);
  else                                 fail("{node_kind}", node->kind);
}

// a lambda term is eta-reducible if it's in the form of
// (\x.f x) where `x` is not a free variable in `f`. this
// can be reduced to simply `f`
static bool eta_reducible(Node *node) {
    return node->kind == NodeKind_FUN &&
           node->body->kind == NodeKind_APP &&
           node->body->arg->kind == NodeKind_VAR &&
           node->body->arg->ref == node->var &&
           !is_free(node->var, node->body->fun);
}

Node *_step(Node *node, const NormalForm nf, Node *scope) {
  if (node->kind == NodeKind_VAR) return node;

  else if (node->kind == NodeKind_FUN) {
    // weak head normal form does not reduce inside lambdas
    if (nf == NormalForm_WEAK_HEAD) return node;

    if ((nf == NormalForm_BETA_ETA) && eta_reducible(node))
      return node->body->fun;

    Node *body = _step(node->body, nf, node);
    // if beta-reduction occurs in the child node,
    // a new node must be created
    if (body != node->body) {
      return new_fun(scope, node->var, body);
    }

    return node;
  }

  else if (node->kind == NodeKind_APP) {
    // immediately beta-reduce outermost applications. this is called
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
    if (node->fun->kind == NodeKind_FUN) return beta(node, scope);

    // weak head normal form does not reduce applications whose
    // lhs is not a function
    if (nf == NormalForm_WEAK_HEAD) return node;

    Node *fun = _step(node->fun, nf, scope);
    // if beta-reduction occurs in the child node,
    // a new node must be created
    if (fun != node->fun) return new_app(fun, node->arg);

    Node *arg = _step(node->arg, nf, scope);
    if (arg != node->arg) return new_app(node->fun, arg);

    return node;
  }

  else fail("{node_kind}", node->kind);
}

Node *step(Node *node, const NormalForm nf) {
  return _step(node, nf, ctx.parser.scope);
}
