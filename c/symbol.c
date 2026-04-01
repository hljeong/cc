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

Symbol *new_symbol(Node *decl, const SymbolKind kind) {
  assert(decl->kind == NodeKind_DECL,
         "%{node_kind}", decl->kind);

  Scope *scope = ctx.analyzer.scope;
  for (Symbol *symbol = scope->symbols; symbol; symbol = symbol->next) {
    if (sv_eq(decl->ref.name, symbol->name))
      error("%{@node} already declared", decl);
  }

  while ((scope = scope->par)) {
    for (Symbol *symbol = scope->symbols; symbol; symbol = symbol->next) {
      if (sv_eq(decl->ref.name, symbol->name)) {
        debug("%{@node} declares previous declaration\n\n%{@node}",
              decl, symbol->decl);
      }
    }
  }

  Symbol *symbol = calloc(1, sizeof(Symbol));
  symbol->kind = kind;
  symbol->name = decl->ref.name;
  symbol->type = decl->type;
  symbol->decl = decl;
  symbol->next = ctx.analyzer.scope->symbols;
  if (kind == SymbolKind_FUN) {
    symbol->fun.scope.par = ctx.analyzer.scope;
  }
  return (ctx.analyzer.scope->symbols = symbol);
}

Symbol *lookup_symbol(Node *ref) {
  assert(ref->kind == NodeKind_REF,
         "%{node_kind}", ref->kind);

  Scope *scope = ctx.analyzer.scope;
  while (scope) {
    for (Symbol *symbol = scope->symbols; symbol; symbol = symbol->next) {
      if (sv_eq(ref->ref.name, symbol->name))
        return symbol;
    }
    scope = scope->par;
  }
  error("%{@node} undeclared symbol", ref);
}
