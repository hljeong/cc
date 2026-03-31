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

void fmt_arg_type_kind(const StrConsumer c, va_list ap) {
  consume_type_kind(c, va_arg(ap, const TypeKind));
}

static void consume_type(const StrConsumer c, const Type *type) {
  if (type->kind == TypeKind_FUN) {
    consume_f(c, "(");
    for (Type *param = type->fun.params; param; param = param->next) {
      consume_f(c, "%{type}", param);
      if (param->next)
        consume_f(c, ", ");
    }
    consume_f(c, ") -> %{type}", type->fun.returns);
  }

  else if (type->kind == TypeKind_PTR) consume_f(c, "%{type}*",      type->ptr.referenced);
  else                                 consume_f(c, "%{type_kind}",  type->kind);
}

void *fmt_ptr_type(const StrConsumer c, void *ptr) {
  consume_type(c, ptr);
  return ((const Type *) ptr)->next;
}

void fmt_arg_type(const StrConsumer c, va_list ap) {
  consume_type(c, va_arg(ap, const Type *));
}

bool type_eq(const Type *t, const Type *u) {
  if (t->kind != u->kind) return false;

  else if (t->kind == TypeKind_PTR) {
    return type_eq(t->ptr.referenced, u->ptr.referenced);
  }

  return true;
}

// shallow copy, just to free up the next pointer
Type *type_copy(const Type *type) {
  Type *copy = new_type(type->kind);
  if (type->kind == TypeKind_FUN) {
    Type head = {};
    Type *cur = &head;
    for (Type *param = type->fun.params; param; param = param->next) {
      cur = (cur->next = param);
    }
    copy->fun.params = head.next;
    copy->fun.returns = type->fun.returns;
  }

  else if (type->kind == TypeKind_PTR) copy->ptr.referenced = type->ptr.referenced;

  return copy;
}

Type *new_type(const TypeKind kind) {
  Type *type = calloc(1, sizeof(Type));
  type->kind = kind;
  return type;
}

Type *new_ptr_type(Type *referenced) {
  Type *type = new_type(TypeKind_PTR);
  type->ptr.referenced = referenced;
  return type;
}

Type *new_fun_type(Type *returns, Node *params) {
  Type *type = new_type(TypeKind_FUN);
  {
    Type head = {};
    Type *cur = &head;
    for (Node *param = params; param; param = param->next)
      cur = (cur->next = type_copy(param->type));
    type->fun.params = head.next;
  }
  type->fun.returns = returns;
  return type;
}
