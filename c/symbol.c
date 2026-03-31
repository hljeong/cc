#include "cc.h"

#include <stdlib.h>

static void consume_symbol_kind(const StrConsumer c, const SymbolKind kind) {
  if      (kind == SymbolKind_VAR) consume_f(c, "var");
  else if (kind == SymbolKind_FUN) consume_f(c, "fun");
  else                             fail("unexpected symbol kind: %d", kind);
}

void fmt_arg_symbol_kind(const StrConsumer c, va_list ap) {
  consume_symbol_kind(c, va_arg(ap, const SymbolKind));
}

static void consume_symbol(const StrConsumer c, const Symbol *symbol) {
  consume_f(c, "%{node_kind}(%{sv}: %{type})",
            symbol->kind, symbol->name, symbol->type);
}

void *fmt_ptr_symbol(const StrConsumer c, void *ptr) {
  consume_symbol(c, ptr);
  return ((const Symbol *) ptr)->next;
}

void fmt_arg_symbol(const StrConsumer c, va_list ap) {
  consume_symbol(c, va_arg(ap, const Symbol *));
}

Symbol *new_var(Node *decl) {
  assert(decl->kind == NodeKind_DECL,
         "%{node_kind}", decl->kind);

  for (Symbol *local = ctx.analyzer.fun->fun.locals; local; local = local->next) {
    assert(local->kind == SymbolKind_VAR);
    if (sv_eq(decl->ref.name, local->name))
      error("%{@node} already declared", decl);
  }

  Symbol *var = calloc(1, sizeof(Symbol));
  var->kind = SymbolKind_VAR;
  var->name = decl->ref.name;
  var->type = decl->type;
  var->decl = decl;
  var->next = ctx.analyzer.fun->fun.locals;
  return (ctx.analyzer.fun->fun.locals = var);
}

// todo: awfully similar to new_var2()!!!!! i smell refactor :d
Symbol *new_fun(Node *decl) {
  assert(decl->kind == NodeKind_DECL,
         "%{node_kind}", decl->kind);

  for (Symbol *fun = ctx.globals; fun; fun = fun->next) {
    assert(fun->kind == SymbolKind_FUN);
    if (sv_eq(fun->name, decl->fun_decl.decl->decl.name))
      error("%{@node} already declared", decl);
  }

  Symbol *fun = calloc(1, sizeof(Symbol));
  fun->kind = SymbolKind_FUN;
  fun->name = decl->ref.name;
  fun->type = decl->type;
  fun->decl = decl;
  fun->next = ctx.globals;
  return (ctx.globals = fun);
}

Symbol *lookup_var(Node *var) {
  for (Symbol *local = ctx.analyzer.fun->fun.locals; local; local = local->next) {
    if (sv_eq(local->name, var->ref.name))
      return local;
  }
  error("%{@node} undeclared variable", var);
}

Symbol *lookup_fun(Node *fun) {
  for (Symbol *fun_ = ctx.globals; fun_; fun_ = fun_->next) {
    if (sv_eq(fun_->name, fun->ref.name))
      return fun_;
  }
  error("%{@node} undeclared function", fun);
}
