#include "cc.h"

// todo: comment

static const char *arg_reg[] = { "%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9" };

static void visit(Node *node);

static void addr(const Node *node) {
  if (node->kind == NodeKind_REF) {
    print("  lea   %d(%%rbp), %%rax", node->ref.symbol->var.offset);
  }

  else if (node->kind == NodeKind_DEREF) {
    visit(node->unop.opr);
  }

  else error("%{@node} not an lvalue: %{node_kind}",
             node, node->kind);
}

static void push(void) {
  print("  push  %%rax");
  ctx.codegen.depth++;
}

static void pop(const char *arg) {
  print("  pop   %s", arg);
  ctx.codegen.depth--;
}

static void visit(Node *node) {
  if (!node) return;

  if (node->kind == NodeKind_PROG) {
    Node *cur = node->list.head;
    while (cur) {
      visit(cur);
      cur = cur->next;
    }
  }

  else if (node->kind == NodeKind_FUN_DECL) {
    Symbol *fun = (ctx.codegen.fun = node->fun_decl.fun);
    ctx.codegen.scope = &fun->fun.scope;
    const StringView fun_name = fun->name;

    print("  .globl %{sv}", fun_name);
    print("%{sv}:", fun_name);

    print("  push  %%rbp");
    print("  mov   %%rsp, %%rbp");
    print("  sub   $%d,  %%rsp", fun->fun.stack_size);

    // push pass-by-register arguments to stack
    {
      int i = 0;
      for (Symbol *param = fun->fun.params; param; param = param->next)
        print("  mov   %s, %d(%%rbp)", arg_reg[i++], param->var.offset);
    }

    Node *cur = node->fun_decl.body->list.head;
    while (cur) {
      visit(cur);
      cur = cur->next;
    }

    print(".L.%{sv}.return:", fun->name);
    print("  mov   %%rbp, %%rsp");
    print("  pop   %%rbp");
    print("  ret");

    ctx.codegen.scope = ctx.codegen.scope->par;
    ctx.codegen.fun = NULL;
  }

  else if (node->kind == NodeKind_BLOCK) {
    Node *cur = node->list.head;
    while (cur) {
      visit(cur);
      cur = cur->next;
    }
  }

  else if (node->kind == NodeKind_DECL_STMT) {
    Node *cur = node->list.head;
    while (cur) {
      visit(cur);
      cur = cur->next;
    }
  }

  else if (node->kind == NodeKind_VAR_DECL) {
    visit(node->var_decl.init);
  }

  else if (node->kind == NodeKind_IF) {
    const StringView fun_name = ctx.codegen.fun->name;
    const int label = ctx.codegen.fun->fun.label++;
    visit(node->if_.cond);
    print("  cmp   $0, %%rax");
    print("  je    .L.%{sv}.%d.else", fun_name, label);
    visit(node->if_.then);
    print("  je    .L.%{sv}.%d.end", fun_name, label);
    print(".L.%{sv}.%d.else:", fun_name,label);
    visit(node->if_.else_);
    print(".L.%{sv}.%d.end:", fun_name, label);
  }

  else if (node->kind == NodeKind_FOR) {
    const StringView fun_name = ctx.codegen.fun->name;
    const int label = ctx.codegen.fun->fun.label++;
    visit(node->for_.init);
    print(".L.%{sv}.%d.cond:", fun_name, label);
    if (node->for_.cond) {
      visit(node->for_.cond);
      print("  cmp   $0, %%rax");
      print("  je    .L.%{sv}.%d.end", fun_name, label);
    }
    visit(node->for_.body);
    visit(node->for_.inc);
    print("  jmp   .L.%{sv}.%d.cond", fun_name, label);
    print(".L.%{sv}.%d.end:", fun_name, label);
  }

  else if (node->kind == NodeKind_RETURN) {
    visit(node->unop.opr);
    print("  jmp .L.%{sv}.return", ctx.codegen.fun->name);
  }

  else if (node->kind == NodeKind_EXPR_STMT) {
    visit(node->expr_stmt.expr);
    assert(ctx.codegen.depth == 0);
  }

  else if (node->kind == NodeKind_NUM) {
    print("  mov   $%d, %%rax", node->num.value);
  }

  else if (node->kind == NodeKind_REF) {
    addr(node);
    print("  mov   (%%rax), %%rax");
  }

  else if (node->kind == NodeKind_ASSIGN) {
    addr(node->binop.lhs);
    push();
    visit(node->binop.rhs);
    pop("%rdi");
    print("  mov   %%rax, (%%rdi)");
  }

  else if (node->kind == NodeKind_CALL) {
    int n_args = 0;
    for (Node *arg = node->call.args; arg; arg = arg->next) {
      visit(arg);
      push();
      n_args++;
    }

    assert(n_args <= 6);

    for (int i = n_args - 1; i >= 0; i--)
      pop(arg_reg[i]);

    print("  mov   $0, %%rax");
    print("  call  %{sv}", node->unop.opr->ref.name);
  }

  else if (node->kind == NodeKind_NEG) {
    visit(node->unop.opr);
    print("  neg   %%rax");
  }

  else if (node->kind == NodeKind_ADDR) {
    addr(node->unop.opr);
  }

  else if (node->kind == NodeKind_DEREF) {
    visit(node->unop.opr);
    print("  mov   (%%rax), %%rax");
  }

  else if (node_kind_is_binop(node->kind)) {
    visit(node->binop.rhs);
    push();
    visit(node->binop.lhs);
    pop("%rdi");

    switch (node->kind) {
      case NodeKind_ADD: { print("  add   %%rdi, %%rax"); break; }
      case NodeKind_SUB: { print("  sub   %%rdi, %%rax"); break; }
      case NodeKind_MUL: { print("  imul  %%rdi, %%rax"); break; }
      case NodeKind_DIV: {
        print("  cqo");
        print("  idiv  %%rdi, %%rax");
        break;
      }

      case NodeKind_EQ:
      case NodeKind_NEQ:
      case NodeKind_LT:
      case NodeKind_LEQ: {
        print("  cmp   %%rdi, %%rax");
        switch (node->kind) {
          case NodeKind_EQ:  { print("  sete  %%al"); break; }
          case NodeKind_NEQ: { print("  setne %%al"); break; }
          case NodeKind_LT:  { print("  setl  %%al"); break; }
          case NodeKind_LEQ: { print("  setle %%al"); break; }
          default:           fail("unexpected comparison: %{node_kind}",
                                  node->kind);
        }
        print("  movzb %%al, %%rax");
        break;
      }

      default: fail("unexpected binop: %{node_kind}",
                    node->kind);
    }
  }

  else fail("unexpected node kind: %{node_kind}",
            node->kind);
}

void codegen() {
  ctx.codegen.scope = &ctx.globals;
  visit(ctx.ast);
}
