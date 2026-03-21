#include "cc.h"

static void push(void) {
  printf("  push  %%rax\n");
  ctx.codegen.depth++;
}

static void pop(const char *arg) {
  printf("  pop   %s\n", arg);
  ctx.codegen.depth--;
}

static void stmt(const Node *node);
static void expr(const Node *node);

static void prog(const Node *node) {
  assertf(node->kind == NodeKind_PROG,
          "bad invocation: prog(%s)", node_kind_to_str(node->kind));

  printf("  .globl main\n");
  printf("main:\n");

  Node *cur = node->head;
  while (cur) {
    stmt(cur);
    cur = cur->next;
  }

  printf("  ret\n");
}

static void stmt(const Node *node) {
  switch (node->kind) {
    case NodeKind_EXPR: { expr(node->variant); break; }
    default:            assertf(node->kind == NodeKind_EXPR,
                                "bad invocation: stmt(%s)",
                                node_kind_to_str(node->kind));
  }
  assert(ctx.codegen.depth == 0);
}

static void expr(const Node *node) {
  if (node->kind == NodeKind_NUM) {
    printf("  mov   $%d, %%rax\n", node->num);
  }

  else if (node->kind == NodeKind_NEG) {
    expr(node->operand);
    printf("  neg   %%rax\n");
  }

  else if (node_kind_is_binop(node->kind)) {
    expr(node->binop.rhs);
    push();
    expr(node->binop.lhs);
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
          default:           failf("comparison not implemented: %s",
                                   node_kind_to_str(node->kind));
        }
        printf("  movzb %%al, %%rax\n");
        break;
      }

      default: failf("binop not implemented: %s",
                     node_kind_to_str(node->kind));
    }
  }

  else errorf("invalid expression");
}

void codegen() {
  prog(ctx.codegen.node);
}
