#include "cc.h"

#include <stdio.h>

// todo: comment

static void visit(Node *node);

static void addr(const Node *node) {
  if (node->kind == NodeKind_VAR) {
    printf("  lea   %d(%%rbp), %%rax\n", node->var->offset);
  }

  else if (node->kind == NodeKind_DEREF) {
    visit(node->operand);
  }

  else error_f("%{} not an lvalue: %{node_kind}",
               at_node(node), node->kind);
}

static void push(void) {
  printf("  push  %%rax\n");
  ctx.codegen.depth++;
}

static void pop(const char *arg) {
  printf("  pop   %s\n", arg);
  ctx.codegen.depth--;
}

static void visit(Node *node) {
  if (!node) return;

  if (node->kind == NodeKind_FUN_DECL) {
    printf("  .globl main\n");
    printf("main:\n");

    printf("  push  %%rbp\n");
    printf("  mov   %%rsp, %%rbp\n");

    int offset = 0;
    for (Var *local = ctx.analyzer.locals.next; local; local = local->next) {
      offset += 8;
      // todo: why is this negative?
      local->offset = -offset;
    }
    printf("  sub   $%d,  %%rsp\n", (offset + 15) / 16 * 16);

    Node *cur = node->body->head;
    while (cur) {
      visit(cur);
      cur = cur->next;
    }

    printf(".L.return:\n");
    printf("  mov   %%rbp, %%rsp\n");
    printf("  pop   %%rbp\n");
    printf("  ret\n");
  }

  else if (node->kind == NodeKind_BLOCK) {
    Node *cur = node->head;
    while (cur) {
      visit(cur);
      cur = cur->next;
    }
  }

  else if (node->kind == NodeKind_IF) {
      const int label = ctx.codegen.label++;
      visit(node->cond);
      printf("  cmp   $0, %%rax\n");
      printf("  je    .L.%d.else\n", label);
      visit(node->then);
      printf("  je    .L.%d.end\n", label);
      printf(".L.%d.else:\n", label);
      visit(node->else_);
      printf(".L.%d.end:\n", label);
  }

  else if (node->kind == NodeKind_FOR) {
      const int label = ctx.codegen.label++;
      visit(node->init);
      printf(".L.%d.cond:\n", label);
      if (node->loop_cond) {
        visit(node->loop_cond);
        printf("  cmp   $0, %%rax\n");
        printf("  je    .L.%d.end\n", label);
      }
      visit(node->loop_body);
      visit(node->inc);
      printf("  jmp   .L.%d.cond\n", label);
      printf(".L.%d.end:\n", label);
  }

  else if (node->kind == NodeKind_RETURN) {
      visit(node->operand);
      printf("  jmp .L.return\n");
  }

  else if (node->kind == NodeKind_EXPR_STMT) {
    visit(node->expr);
    assert(ctx.codegen.depth == 0);
  }

  else if (node->kind == NodeKind_NUM) {
    printf("  mov   $%d, %%rax\n", node->num);
  }

  else if (node->kind == NodeKind_VAR) {
    addr(node);
    printf("  mov   (%%rax), %%rax\n");
  }

  else if (node->kind == NodeKind_ASSIGN) {
    addr(node->lhs);
    push();
    visit(node->rhs);
    pop("%rdi");
    printf("  mov   %%rax, (%%rdi)\n");
  }

  else if (node->kind == NodeKind_NEG) {
    visit(node->operand);
    printf("  neg   %%rax\n");
  }

  else if (node->kind == NodeKind_ADDR) {
    addr(node->operand);
  }

  else if (node->kind == NodeKind_DEREF) {
    visit(node->operand);
    printf("  mov   (%%rax), %%rax\n");
  }

  else if (node_kind_is_binop(node->kind)) {
    visit(node->rhs);
    push();
    visit(node->lhs);
    pop("%rdi");

    switch (node->kind) {
      case NodeKind_ADD: { printf("  add   %%rdi, %%rax\n"); break; }
      case NodeKind_SUB: { printf("  sub   %%rdi, %%rax\n"); break; }
      case NodeKind_MUL: { printf("  imul  %%rdi, %%rax\n"); break; }
      case NodeKind_DIV: {
        printf("  cqo\n");
        printf("  idiv  %%rdi, %%rax\n");
        break;
      }

      case NodeKind_EQ:
      case NodeKind_NEQ:
      case NodeKind_LT:
      case NodeKind_LEQ: {
        printf("  cmp   %%rdi, %%rax\n");
        switch (node->kind) {
          case NodeKind_EQ:  { printf("  sete  %%al\n"); break; }
          case NodeKind_NEQ: { printf("  setne %%al\n"); break; }
          case NodeKind_LT:  { printf("  setl  %%al\n"); break; }
          case NodeKind_LEQ: { printf("  setle %%al\n"); break; }
          default:           fail_f("unexpected comparison: %{node_kind}",
                                    node->kind);
        }
        printf("  movzb %%al, %%rax\n");
        break;
      }

      default: fail_f("unexpected binop: %{node_kind}",
                      node->kind);
    }
  }

  else fail_f("unexpected node kind: %{node_kind}",
              node->kind);
}

void codegen() {
  visit(ctx.ast);
}
