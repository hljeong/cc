#include "cc.h"

#include <stdlib.h>

Types t = {
  .int_ = { .kind = TypeKind_INT },
};

static void consume_type_kind(const StrConsumer c, const TypeKind kind) {
  if      (kind == TypeKind_INT) consume_f(c, "int");
  else if (kind == TypeKind_PTR) consume_f(c, "ptr");
  else if (kind == TypeKind_FUN) consume_f(c, "fun");
  else                           fail("unexpected type kind: %d", kind);
}

void fmt_type_kind(const StrConsumer c, va_list ap) {
  consume_type_kind(c, va_arg(ap, const TypeKind));
}

static void consume_type(const StrConsumer c, const Type *type) {
  if      (type->kind == TypeKind_FUN) consume_f(c, "() -> %{type}", type->returns);
  else if (type->kind == TypeKind_PTR) consume_f(c, "%{type}*",      type->referenced);
  else                                 consume_f(c, "%{type_kind}",  type->kind);
}

void fmt_type(const StrConsumer c, va_list ap) {
  consume_type(c, va_arg(ap, const Type *));
}

bool type_eq(const Type *t, const Type *u) {
  if (t->kind != u->kind) return false;

  else if (t->kind == TypeKind_PTR) {
    return type_eq(t->referenced, u->referenced);
  }

  return true;
}

Type *new_type(const TypeKind kind) {
  Type *type = calloc(1, sizeof(Type));
  type->kind = kind;
  return type;
}

Type *new_pointer_type(Type *referenced) {
  Type *type = new_type(TypeKind_PTR);
  type->referenced = referenced;
  return type;
}

Type *new_fun_type(Type *returns) {
  Type *type = new_type(TypeKind_FUN);
  type->returns = returns;
  return type;
}
