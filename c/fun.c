#include "cc.h"

#include <stdlib.h>

Fun *new_fun(Node *decl) {
  assert(decl->kind == NodeKind_FUN_DECL,
         "%{node_kind}", decl->kind);

  for (Fun *fun = ctx.analyzer.prog->funs; fun; fun = fun->next) {
    if (sv_eq(fun->name, decl->fun_decl.var->var.name))
      error("%{@node} already declared", decl);
  }

  Fun *fun = calloc(1, sizeof(Fun));
  fun->name = decl->fun_decl.var->var.name;
  fun->type = decl->fun_decl.var->type;
  fun->decl = decl;
  fun->next = ctx.analyzer.prog->funs;
  return (ctx.analyzer.prog->funs = fun);
}
