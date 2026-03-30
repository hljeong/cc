#include "cc.h"

#include <stdlib.h>

Var *new_var(Var *locals, Node *declr) {
  assert(declr->kind == NodeKind_VAR,
         "%{node_kind}", declr->kind);
  assert(declr->var.is_decl);

  if (sv_eq(locals->name, declr->var.name))
    error("%{@node} already declared", declr);
  while (locals->next) {
    locals = locals->next;
    if (sv_eq(locals->name, declr->var.name))
      error("%{@node} already declared", declr);
  }

  Var *var = calloc(1, sizeof(Var));
  var->name = declr->var.name;
  var->type = declr->type;
  var->decl = declr;
  return (locals->next = var);
}

Var *lookup_var(Var *locals, Node *node) {
  Var *var = locals;
  while (!sv_eq(var->name, node->var.name)) {
    var = var->next;
    if (!var)
      error("%{@node} undeclared variable", node);
  }
  return var;
}

static void consume_var(const StrConsumer c, const Var *var) {
  consume_f(c, "%{sv}: %{type}", var->name, var->type);
}

void fmt_var(const StrConsumer c, va_list ap) {
  consume_var(c, va_arg(ap, const Var *));
}

void fmt_vars(const StrConsumer c, va_list ap) {
  const Var *vars = va_arg(ap, const Var *);
  vars = vars->next;

  if (!vars) consume_f(c, "{}");
  else {
    consume_f(c, "{ ");
    while (vars) {
      consume_var(c, vars);
      if ((vars = vars->next))
        consume_f(c, ", ");
    }
    consume_f(c, " }");
  }
}
