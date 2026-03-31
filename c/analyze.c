#include "cc.h"

static void visit(Node **node_ptr);
static void visit(Node **node_ptr) {
  Node *node = *node_ptr;

  if (!node) {}

  else if (node->kind == NodeKind_PROG) {
    for (Node *cur = node->prog.head; cur; cur = cur->next)
      visit(&cur);
  }

  else if (node->kind == NodeKind_FUN_DECL) {
    // declare function first before visiting body, to allow recursion
    ctx.analyzer.fun = (node->fun_decl.fun = new_fun(node->fun_decl.decl));

    {
      Node *param = node->fun_decl.decl->decl.params;
      while (param) {
        new_var(param);
        param = param->next;
      }

      // right now params are in reverse order. reverse the linked list
      if (ctx.analyzer.fun->fun.locals) {
        Symbol *prev = NULL;
        while (ctx.analyzer.fun->fun.locals) {
          Symbol *next = ctx.analyzer.fun->fun.locals->next;
          ctx.analyzer.fun->fun.locals->next = prev;
          prev = ctx.analyzer.fun->fun.locals;
          ctx.analyzer.fun->fun.locals = next;
        }
        ctx.analyzer.fun->fun.locals = prev;
      }
      ctx.analyzer.fun->fun.params = ctx.analyzer.fun->fun.locals;
    }

    visit(&node->fun_decl.body);

    // allocate local vars
    int offset = 0;
    for (Symbol *var = ctx.analyzer.fun->fun.locals; var; var = var->next) {
      assert(var->kind == SymbolKind_VAR);
      offset += 8;
      // todo: why is this negative?
      var->var.offset = -offset;
    }
    // align to 16-byte boundary
    // todo: why?
    ctx.analyzer.fun->fun.stack_size = (offset + 15) / 16 * 16;
  }

  else if (node->kind == NodeKind_EXPR_STMT) {
    visit(&node->expr_stmt.expr);
  }

  else if (node->kind == NodeKind_IF) {
    visit(&node->if_.cond);
    visit(&node->if_.then);
    visit(&node->if_.else_);
  }

  else if (node->kind == NodeKind_FOR) {
    visit(&node->for_.init);
    visit(&node->for_.cond);
    visit(&node->for_.inc);
    visit(&node->for_.body);
  }

  else if (node_kind_is_unop(node->kind)) {
    visit(&node->unop.opr);

    if      (node->kind == NodeKind_NEG)  node->type = node->unop.opr->type;
    else if (node->kind == NodeKind_ADDR) node->type = new_ptr_type(node->unop.opr->type);

    else if (node->kind == NodeKind_DEREF) {
      if (node->unop.opr->type->kind != TypeKind_PTR)
        error("%{@node} not an pointer: %{type}",
              node, node->unop.opr->type);
      node->type = node->unop.opr->type->ptr.referenced;
    }

    else if (node->kind == NodeKind_RETURN) {
      if (!type_eq(ctx.analyzer.fun->type->fun.returns, node->unop.opr->type))
        error("%{@node} expected to return %{type}, got: %{type}",
              node->unop.opr,
              ctx.analyzer.fun->type->fun.returns,
              node->unop.opr->type);
    }

    else fail("unexpected unop: %{node_kind}", node->kind);
  }

  else if (node->kind == NodeKind_CALL) {
    node->ref.symbol = lookup_fun(node->call.fun);
    node->type = node->ref.symbol->type;
    Symbol *param = node->ref.symbol->fun.params;
    Node *arg = node->call.args;
    for (; arg && param; (arg = arg->next), (param = param->next)) {
      visit(&arg);

      if (!type_eq(arg->type, param->type))
        error("%{@node} expected %{type}, got (%{sv}: %{type})",
              arg, param->type, arg->lexeme, arg->type);
    }

    if (arg)
      error("%{@node} extra argument", arg);

    if (param)
      error("%{@loc} expected argument (%{sv}: %{type})",
            node->lexeme.loc + node->lexeme.len - 1,
            param->name, param->type);

    node->type = node->ref.symbol->type->fun.returns;
  }

  else if (node->kind == NodeKind_ASSIGN) {
    visit(&node->binop.rhs);
    visit(&node->binop.lhs);

    // todo: how to check if lhs is an lvalue?
    if (!type_eq(node->binop.lhs->type, node->binop.rhs->type)) {
      error("%{@node} cannot assign (%{sv}: %{type}) to (%{sv}: %{type})",
            node,
            node->binop.rhs->lexeme, node->binop.rhs->type,
            node->binop.lhs->lexeme, node->binop.lhs->type);
    }

    node->type = node->binop.lhs->type;
  }

  else if (node->kind == NodeKind_ADD) {
    visit(&node->binop.lhs);
    visit(&node->binop.rhs);

    const Type *t_lhs = node->binop.lhs->type;
    const Type *t_rhs = node->binop.rhs->type;

    // todo: type_is_integral()
    // `int + int`
    if (t_lhs->kind == TypeKind_INT && t_rhs->kind == TypeKind_INT) {
      node->type = node->binop.lhs->type;
    }

    // `ptr + ptr`
    else if (t_lhs->kind == TypeKind_PTR && t_rhs->kind == TypeKind_PTR) {
      error("%{@node} cannot assign (%{sv}: %{type}) to (%{sv}: %{type})",
            node,
            node->binop.rhs->lexeme, node->binop.rhs->type,
            node->binop.lhs->lexeme, node->binop.lhs->type);
    }

    // todo: dangerous catch-all
    // `int + ptr` or `ptr + int`
    else {
      // canonicalize 'int + ptr' to `ptr + int`
      if (t_lhs->kind == TypeKind_INT) {
        Node *tmp = node->binop.lhs;
        node->binop.lhs = node->binop.rhs;
        node->binop.rhs = node->binop.lhs;
      }

      Node *stride = new_num_node(8);
      stride->lexeme = node->binop.rhs->lexeme;
      node->binop.rhs = new_binop_node(NodeKind_MUL, stride, node->binop.rhs);

      node->type = node->binop.lhs->type;
    }
  }

  else if (node->kind == NodeKind_SUB) {
    visit(&node->binop.lhs);
    visit(&node->binop.rhs);

    const Type *t_lhs = node->binop.lhs->type;
    const Type *t_rhs = node->binop.rhs->type;

    // `int - int`
    if (t_lhs->kind == TypeKind_INT && t_rhs->kind == TypeKind_INT) {
      node->type = node->binop.lhs->type;
    }

    // `ptr - ptr`
    else if (t_lhs->kind == TypeKind_PTR && t_rhs->kind == TypeKind_PTR) {
      if (!type_eq(t_lhs->ptr.referenced, t_rhs->ptr.referenced)) {
        error("%{@node} cannot subtract (%{sv}: %{type}) from (%{sv}: %{type})",
              node,
              node->binop.rhs->lexeme, node->binop.rhs->type,
              node->binop.lhs->lexeme, node->binop.lhs->type);
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
      stride->lexeme = node->binop.rhs->lexeme;
      node->binop.rhs = new_binop_node(NodeKind_MUL, stride, node->binop.rhs);

      node->type = node->binop.lhs->type;
    }

    // `num - ptr`
    else if (t_lhs->kind == TypeKind_INT && t_rhs->kind == TypeKind_PTR) {
      error("%{@node} cannot subtract (%{sv}: %{type}) from (%{sv}: %{type})",
            node,
            node->binop.rhs->lexeme, node->binop.rhs->type,
            node->binop.lhs->lexeme, node->binop.lhs->type);
    }

    else fail("unexpected type check: (%{sv}: %{type}) - (%{sv}: %{type})",
              node->binop.lhs->lexeme, node->binop.lhs->type,
              node->binop.rhs->lexeme, node->binop.rhs->type);
  }

  else if (node_kind_is_binop(node->kind)) {
    visit(&node->binop.lhs);
    visit(&node->binop.rhs);
    node->type = node->binop.lhs->type;
  }

  else if (node_kind_is_list(node->kind)) {
    for (Node *cur = node->list.head; cur; cur = cur->next) {
      visit(&cur);
    }
  }

  else if (node->kind == NodeKind_VAR_DECL) {
    // visit init value before declaring var
    if (node->var_decl.init)
      visit(&node->var_decl.init->binop.rhs);

    // declare var
    node->var_decl.decl->decl.symbol = new_var(node->var_decl.decl);

    // type check
    if (node->var_decl.init)
      visit(&node->var_decl.init);
  }

  else if (node->kind == NodeKind_REF) {
    node->ref.symbol = lookup_var(node);
    node->type = node->ref.symbol->type;
  }

  else if (node->kind == NodeKind_NUM) {
    node->type = &t.int_;
  }

  else fail("unexpected node kind: %{node_kind}", node->kind);
}

void analyze() {
  visit(&ctx.ast);
}
