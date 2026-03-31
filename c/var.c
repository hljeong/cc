#include "cc.h"

#include <stdlib.h>

Var *new_var(Node *declr) {
  assert(declr->kind == NodeKind_VAR,
         "%{node_kind}", declr->kind);
  assert(declr->var.is_decl);

  for (Var *local = ctx.analyzer.fun->locals; local; local = local->next) {
    if (sv_eq(declr->var.name, local->name))
      error("%{@node} already declared", declr);
  }

  Var *var = calloc(1, sizeof(Var));
  var->name = declr->var.name;
  var->type = declr->type;
  var->decl = declr;
  var->next = ctx.analyzer.fun->locals;
  return (ctx.analyzer.fun->locals = var);
}

Var *lookup_var(Node *node) {
  for (Var *local = ctx.analyzer.fun->locals; local; local = local->next) {
    if (sv_eq(local->name, node->var.name))
      return local;
  }
  error("%{@node} undeclared variable", node);
}

static void consume_var(const StrConsumer c, const Var *var) {
  consume_f(c, "%{sv}: %{type}", var->name, var->type);
}

void fmt_arg_var(const StrConsumer c, va_list ap) {
  consume_var(c, va_arg(ap, const Var *));
}

void fmt_arg_vars(const StrConsumer c, va_list ap) {
  const Var *vars = va_arg(ap, const Var *);

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
