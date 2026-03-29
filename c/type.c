#include "cc.h"

#include <stdlib.h>

Types t = {
  .int_ = { .kind = TypeKind_INT },
};

static StrEmitter str_type_kind(const TypeKind kind) {
  if      (kind == TypeKind_INT) return str_f("int");
  else if (kind == TypeKind_PTR) return str_f("ptr");
  else                           fail_f("unexpected type kind: %d", kind);
}

void fmt_type_kind(const StrConsumer c, va_list ap) {
  const TypeKind type_kind = va_arg(ap, TypeKind);
  consume_e(c, str_type_kind(type_kind));
}

static void emit_type(const StrConsumer c, void *data) {
  const Type *type = *((const Type **) data);
  if (type->kind == TypeKind_PTR) consume_f(c, "%{type}*", type->referenced);
  else                            consume_f(c, "%{type_kind}", type->kind);
  free(data);
}

static StrEmitter str_type(const Type *type) {
  const Type **type_ptr = calloc(1, sizeof(const Type *));
  *type_ptr = type;
  return (StrEmitter) { .emit = emit_type, .data = type_ptr };
}

void fmt_type(const StrConsumer c, va_list ap) {
  const Type *type = va_arg(ap, const Type *);
  consume_e(c, str_type(type));
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
