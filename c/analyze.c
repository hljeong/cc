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

static Type *new_type(const TypeKind kind) {
  Type *type = calloc(1, sizeof(Type));
  type->kind = kind;
  return type;
}

static Type *new_pointer_type(Type *referenced) {
  Type *type = new_type(TypeKind_PTR);
  type->referenced = referenced;
  return type;
}

static Var *new_var(Node *decl) {
  assert(decl->kind == NodeKind_VAR);
  Var *var = calloc(1, sizeof(Var));
  var->name = decl->name;
  var->decl = decl;
  return var;
}

static Var *lookup_or_new_var(Var *locals, Node *node) {
  assert(node->kind == NodeKind_VAR);
  if (sv_eq(locals->name, node->name)) return locals;
  else if (locals->next) return lookup_or_new_var(locals->next, node);
  else return (locals->next = new_var(node));
}

static void visit(Node **node_ptr);
static void visit(Node **node_ptr) {
  Node *node = *node_ptr;
  if (!node) {}

  else if (node->kind == NodeKind_FUN_DECL) {
    visit(&node->body);
  }

  else if (node->kind == NodeKind_EXPR_STMT) {
    visit(&node->expr);
  }

  else if (node->kind == NodeKind_IF) {
    visit(&node->cond);
    visit(&node->then);
    visit(&node->else_);
  }

  else if (node->kind == NodeKind_FOR) {
    visit(&node->init);
    visit(&node->loop_cond);
    visit(&node->inc);
    visit(&node->loop_body);
  }

  else if (node_kind_is_unop(node->kind)) {
    visit(&node->operand);

    if      (node->kind == NodeKind_NEG)   node->type = node->operand->type;
    else if (node->kind == NodeKind_ADDR)  node->type = new_pointer_type(node->operand->type);
    else if (node->kind == NodeKind_DEREF) {
      if (node->operand->type->kind != TypeKind_PTR)
        errorf_at_node(node, "not an lvalue: %s",
                       type_to_str(node->operand->type));
      node->type = node->operand->type->referenced;
    }
  }

  else if (node->kind == NodeKind_ASSIGN) {
    visit(&node->rhs);

    // todo: temp
    if (node->lhs->kind == NodeKind_VAR) {
      Var *var = lookup_or_new_var(&ctx.analyzer.locals, node->lhs);
      if (!var->type) {
        var->type = node->rhs->type;
        debugf_at_node(node, "declaring "sv_fmt": %s",
                       sv_arg(var->name), type_to_str(var->type));
      }
    }

    visit(&node->lhs);

    // todo: how to check if lhs is an lvalue?
    if (!type_eq(node->lhs->type, node->rhs->type)) {
      errorf_at_node(node, "cannot assign ("sv_fmt": %s) to ("sv_fmt": %s)",
                     sv_arg(node->rhs->lexeme), type_to_str(node->rhs->type),
                     sv_arg(node->lhs->lexeme), type_to_str(node->lhs->type));
    }

    node->type = node->lhs->type;
  }

  else if (node->kind == NodeKind_ADD) {
    visit(&node->lhs);
    visit(&node->rhs);

    const Type *t_lhs = node->lhs->type;
    const Type *t_rhs = node->rhs->type;

    // todo: type_is_integral()
    // `int + int`
    if (t_lhs->kind == TypeKind_INT && t_rhs->kind == TypeKind_INT) {
      node->type = node->lhs->type;
    }

    // `ptr + ptr`
    else if (t_lhs->kind == TypeKind_PTR && t_rhs->kind == TypeKind_PTR) {
      errorf_at_node(node, "cannot add ("sv_fmt": %s) to ("sv_fmt" %s)",
                     sv_arg(node->rhs->lexeme), type_to_str(node->rhs->type),
                     sv_arg(node->lhs->lexeme), type_to_str(node->lhs->type));
    }

    // todo: dangerous catch-all
    // `int + ptr` or `ptr + int`
    else {
      // canonicalize 'int + ptr' to `ptr + int`
      if (t_lhs->kind == TypeKind_INT) {
        Node *tmp = node->lhs;
        node->lhs = node->rhs;
        node->rhs = node->lhs;
      }

      Node *stride = new_num(8);
      stride->lexeme = node->rhs->lexeme;
      node->rhs = new_binop(NodeKind_MUL, stride, node->rhs);

      node->type = node->lhs->type;
    }
  }

  else if (node->kind == NodeKind_SUB) {
    visit(&node->lhs);
    visit(&node->rhs);

    const Type *t_lhs = node->lhs->type;
    const Type *t_rhs = node->rhs->type;

    // `int - int`
    if (t_lhs->kind == TypeKind_INT && t_rhs->kind == TypeKind_INT) {
      node->type = node->lhs->type;
    }

    // `ptr - ptr`
    else if (t_lhs->kind == TypeKind_PTR && t_rhs->kind == TypeKind_PTR) {
      if (!type_eq(t_lhs->referenced, t_rhs->referenced))
        errorf_at_node(node, "cannot subtract ("sv_fmt": %s) from ("sv_fmt" %s)",
                       sv_arg(node->rhs->lexeme), type_to_str(node->rhs->type),
                       sv_arg(node->lhs->lexeme), type_to_str(node->lhs->type));

      node->type = &t.int_;

      Node *stride = new_num(8);
      stride->lexeme = node->lexeme;
      // todo: return this
      Node *new_node = new_binop(NodeKind_DIV, node, stride);
      new_node->type = &t.int_;
      new_node->lexeme = node->lexeme;
      *node_ptr = new_node;
    }

    // `ptr - num`
    else if (t_lhs->kind == TypeKind_PTR && t_rhs->kind == TypeKind_INT) {
      Node *stride = new_num(8);
      stride->lexeme = node->rhs->lexeme;
      node->rhs = new_binop(NodeKind_MUL, stride, node->rhs);

      node->type = node->lhs->type;
    }

    // `num - ptr`
    else if (t_lhs->kind == TypeKind_INT && t_rhs->kind == TypeKind_PTR) {
      errorf_at_node(node, "cannot subtract ("sv_fmt": %s) from ("sv_fmt" %s)",
                     sv_arg(node->rhs->lexeme), type_to_str(node->rhs->type),
                     sv_arg(node->lhs->lexeme), type_to_str(node->lhs->type));
    }

    else failf("("sv_fmt": %s) - ("sv_fmt" %s)",
               sv_arg(node->lhs->lexeme), type_to_str(node->lhs->type),
               sv_arg(node->rhs->lexeme), type_to_str(node->rhs->type));
  }

  else if (node_kind_is_binop(node->kind)) {
    visit(&node->lhs);
    visit(&node->rhs);
    node->type = node->lhs->type;
  }

  else if (node_kind_is_list(node->kind)) {
    for (Node *cur = node->head; cur; cur = cur->next) {
      visit(&cur);
    }
  }

  else if (node->kind == NodeKind_VAR) {
    node->var = lookup_or_new_var(&ctx.analyzer.locals, node);
    if (!node->var->type) {
      node->type = &t.int_;
      debugf_at_node(node, "undeclared variable");
    }
    node->type = node->var->type;
  }

  else if (node->kind == NodeKind_NUM) {
    node->type = &t.int_;
  }

  else failf("%s", node_kind_to_str(node->kind));
}

void analyze() {
  visit(&ctx.ast);
}
