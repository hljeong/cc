#include "cc.h"

Types t = {
  .int_ = { .kind = TypeKind_INT },
};

const char *type_kind_to_str(const TypeKind kind) {
  if      (kind == TypeKind_INT) return "int";
  else if (kind == TypeKind_PTR) return "ptr";
  else                           failf("%d\n", kind);
}

const char *type_to_str(const Type *type) {
  if (type->kind == TypeKind_PTR) {
    // todo: figure out mem mgmt
    const int BUFLEN = 256;
    char *buf = calloc(BUFLEN, sizeof(char));
    const int len = snprintf(buf, BUFLEN, "%s", type_to_str(type->referenced));
    snprintf(buf + len, BUFLEN - len, "*");
    return buf;
  }

  else return type_kind_to_str(type->kind);
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
