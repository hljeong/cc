#include "cc.h"

// todo: comment

static void visit(Node *node);

static void addr(const Node *node) {
  if (node->kind == NodeKind_VAR) {
    print("  lea   %d(%%rbp), %%rax", node->var->offset);
  }

  else if (node->kind == NodeKind_DEREF) {
    visit(node->operand);
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

  if (node->kind == NodeKind_FUN_DECL) {
    print("  .globl main");
    print("main:");

    print("  push  %%rbp");
    print("  mov   %%rsp, %%rbp");

    int offset = 0;
    for (Var *local = ctx.analyzer.locals.next; local; local = local->next) {
      offset += 8;
      // todo: why is this negative?
      local->offset = -offset;
    }
    print("  sub   $%d,  %%rsp", (offset + 15) / 16 * 16);

    Node *cur = node->body->head;
    while (cur) {
      visit(cur);
      cur = cur->next;
    }

    print(".L.return:");
    print("  mov   %%rbp, %%rsp");
    print("  pop   %%rbp");
    print("  ret");
  }

  else if (node->kind == NodeKind_BLOCK) {
    Node *cur = node->head;
    while (cur) {
      visit(cur);
      cur = cur->next;
    }
  }

  else if (node->kind == NodeKind_DECL_STMT) {
    Node *cur = node->head;
    while (cur) {
      visit(cur);
      cur = cur->next;
    }
  }

  else if (node->kind ==NodeKind_VAR_DECL) {
    visit(node->var_init);
  }

  else if (node->kind == NodeKind_IF) {
      const int label = ctx.codegen.label++;
      visit(node->cond);
      print("  cmp   $0, %%rax");
      print("  je    .L.%d.else", label);
      visit(node->then);
      print("  je    .L.%d.end", label);
      print(".L.%d.else:", label);
      visit(node->else_);
      print(".L.%d.end:", label);
  }

  else if (node->kind == NodeKind_FOR) {
      const int label = ctx.codegen.label++;
      visit(node->init);
      print(".L.%d.cond:", label);
      if (node->loop_cond) {
        visit(node->loop_cond);
        print("  cmp   $0, %%rax");
        print("  je    .L.%d.end", label);
      }
      visit(node->loop_body);
      visit(node->inc);
      print("  jmp   .L.%d.cond", label);
      print(".L.%d.end:", label);
  }

  else if (node->kind == NodeKind_RETURN) {
      visit(node->operand);
      print("  jmp .L.return");
  }

  else if (node->kind == NodeKind_EXPR_STMT) {
    visit(node->expr);
    assert(ctx.codegen.depth == 0);
  }

  else if (node->kind == NodeKind_NUM) {
    print("  mov   $%d, %%rax", node->num);
  }

  else if (node->kind == NodeKind_VAR) {
    addr(node);
    print("  mov   (%%rax), %%rax");
  }

  else if (node->kind == NodeKind_ASSIGN) {
    addr(node->lhs);
    push();
    visit(node->rhs);
    pop("%rdi");
    print("  mov   %%rax, (%%rdi)");
  }

  else if (node->kind == NodeKind_NEG) {
    visit(node->operand);
    print("  neg   %%rax");
  }

  else if (node->kind == NodeKind_ADDR) {
    addr(node->operand);
  }

  else if (node->kind == NodeKind_DEREF) {
    visit(node->operand);
    print("  mov   (%%rax), %%rax");
  }

  else if (node_kind_is_binop(node->kind)) {
    visit(node->rhs);
    push();
    visit(node->lhs);
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
  visit(ctx.ast);
}
