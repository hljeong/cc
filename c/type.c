#include "cc.h"

Types t = {
  .int_ = { .kind = TypeKind_INT },
};

StrEmitter str_type_kind(const TypeKind kind) {
  if      (kind == TypeKind_INT) return str_f("int");
  else if (kind == TypeKind_PTR) return str_f("ptr");
  else                           fail(str_int(kind));
}

static void emit_type(const StrConsumer c, void *data) {
  const Type *type = *((const Type **) data);

  if (type->kind == TypeKind_PTR) {
    emit_e(c, str_type(type->referenced));
    emit_s(c,"*");
  }

  else emit_e(c, str_type_kind(type->kind));

  free(data);
}

StrEmitter str_type(const Type *type) {
  const Type **type_ptr = calloc(1, sizeof(const Type *));
  *type_ptr = type;
  return (StrEmitter) { .emit = emit_type, .data = type_ptr };
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
