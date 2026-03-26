#include "cc.h"

// todo: comments

static void addr(const Node *node) {
  if (node->kind == NodeKind_VAR) {
    int offset = -8;
    for (Var *var = ctx.analyzer.locals.next; !sv_eq(var->name, node->name); (offset -= 8), (var = var->next));
    printf("  lea   %d(%%rbp), %%rax\n", offset);
    return;
  }

  else errorf("not an lvalue: %s",
              node_kind_to_str(node->kind));
}

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

static void fun_decl(const Node *node) {
  assertf(node->kind == NodeKind_FUN_DECL,
          "bad invocation: fun_decl(%s)",
          node_kind_to_str(node->kind));

  printf("  .globl main\n");
  printf("main:\n");

  printf("  push  %%rbp\n");
  printf("  mov   %%rsp, %%rbp\n");

  int n_locals = 0;
  for (Var *local = ctx.analyzer.locals.next; local; n_locals++, local = local->next);

  printf("  sub   $%d,  %%rsp\n", n_locals * 8);

  Node *cur = node->body->head;
  while (cur) {
    stmt(cur);
    cur = cur->next;
  }

  printf("  mov   %%rbp, %%rsp\n");
  printf("  pop   %%rbp\n");
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

  else if (node->kind == NodeKind_VAR) {
    addr(node);
    printf("  mov   (%%rax), %%rax\n");
  }

  else if (node->kind == NodeKind_ASSIGN) {
    addr(node->lhs);
    push();
    expr(node->rhs);
    pop("%rdi");
    printf("  mov   %%rax, (%%rdi)\n");
  }

  else if (node->kind == NodeKind_NEG) {
    expr(node->operand);
    printf("  neg   %%rax\n");
  }

  else if (node_kind_is_binop(node->kind)) {
    expr(node->rhs);
    push();
    expr(node->lhs);
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
  fun_decl(ctx.ast);
}
