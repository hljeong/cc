#include "cc.h"

Var *new_var(Node *decl) {
  assert(decl->kind == NodeKind_VAR);
  Var *var = calloc(1, sizeof(Var));
  var->name = decl->name;
  var->decl = decl;
  return var;
}

Var *lookup_or_new_var(Var *locals, Node *node) {
  assert(node->kind == NodeKind_VAR);
  if (sv_eq(locals->name, node->name)) return locals;
  else if (locals->next) return lookup_or_new_var(locals->next, node);
  else return (locals->next = new_var(node));
}
