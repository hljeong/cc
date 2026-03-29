#include "cc.h"

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
        error(at_node(node),
              str_f("not an lvalue: "), str_type(node->operand->type));
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
        debug(at_node(node),
              str_f("declaring "sv_fmt": ", sv_arg(var->name)),
              str_type(var->type));
      }
    }

    visit(&node->lhs);

    // todo: how to check if lhs is an lvalue?
    if (!type_eq(node->lhs->type, node->rhs->type)) {
      // todo: context
      error(at_node(node), str_f("cannot assign"));
      // error(at_node(node), str_f("cannot assign ("sv_fmt": %s) to ("sv_fmt": %s)",
      //         sv_arg(node->rhs->lexeme), type_to_str(node->rhs->type),
      //         sv_arg(node->lhs->lexeme), type_to_str(node->lhs->type)));
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
      // todo: context
      error(at_node(node), str_f("cannot add"));
      // error(at_node(node), str_f("cannot add ("sv_fmt": %s) to ("sv_fmt": %s)",
      //         sv_arg(node->rhs->lexeme), type_to_str(node->rhs->type),
      //         sv_arg(node->lhs->lexeme), type_to_str(node->lhs->type)));
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

      Node *stride = new_num_node(8);
      stride->lexeme = node->rhs->lexeme;
      node->rhs = new_binop_node(NodeKind_MUL, stride, node->rhs);

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
      if (!type_eq(t_lhs->referenced, t_rhs->referenced)) {
        // todo: context
        error(at_node(node), "cannot subtract");
        // error(at_node(node), str_f("cannot subtract ("sv_fmt": %s) from ("sv_fmt": %s)",
        //         sv_arg(node->rhs->lexeme), type_to_str(node->rhs->type),
        //         sv_arg(node->lhs->lexeme), type_to_str(node->lhs->type)));
      }

      node->type = &t.int_;

      Node *stride = new_num_node(8);
      stride->lexeme = node->lexeme;
      Node *new_node = new_binop_node(NodeKind_DIV, node, stride);
      new_node->type = &t.int_;
      new_node->lexeme = node->lexeme;
      *node_ptr = new_node;
    }

    // `ptr - num`
    else if (t_lhs->kind == TypeKind_PTR && t_rhs->kind == TypeKind_INT) {
      Node *stride = new_num_node(8);
      stride->lexeme = node->rhs->lexeme;
      node->rhs = new_binop_node(NodeKind_MUL, stride, node->rhs);

      node->type = node->lhs->type;
    }

    // `num - ptr`
    else if (t_lhs->kind == TypeKind_INT && t_rhs->kind == TypeKind_PTR) {
      // todo: no one can tell whats happening here :(
      //       implement smth like:
      //         errorf(at_node(), "cannot subtract (%{sv}: %{type}) from (%{sv} : %{type})");
      error(at_node(node),
            str_f("cannot subtract ("sv_fmt": ", sv_arg(node->rhs->lexeme)),
            str_type(node->rhs->type),
            str_f(") from ("sv_fmt": ", sv_arg(node->lhs->lexeme)),
            str_type(node->lhs->type),
            str_f(")"));
    }

    else fail(str_f("("sv_fmt": ", sv_arg(node->lhs->lexeme)),
              str_type(node->lhs->type),
              str_f(") - ("sv_fmt": ", sv_arg(node->rhs->lexeme)),
              str_type(node->rhs->type),
              str_f(")"));
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
      debug(at_node(node), str_f("undeclared variable"));
    }
    node->type = node->var->type;
  }

  else if (node->kind == NodeKind_NUM) {
    node->type = &t.int_;
  }

  else fail(str_node_kind(node->kind));
}

void analyze() {
  visit(&ctx.ast);
}
