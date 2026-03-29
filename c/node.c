#include "cc.h"

StrEmitter str_node_kind(const NodeKind kind) {
  if      (kind == NodeKind_NUM)       return str_f("num");
  else if (kind == NodeKind_VAR)       return str_f("var");
  else if (kind == NodeKind_ADD)       return str_f("+");
  else if (kind == NodeKind_SUB)       return str_f("-");
  else if (kind == NodeKind_MUL)       return str_f("*");
  else if (kind == NodeKind_DIV)       return str_f("/");
  else if (kind == NodeKind_NEG)       return str_f("-");
  else if (kind == NodeKind_ADDR)      return str_f("addr");
  else if (kind == NodeKind_DEREF)     return str_f("deref");
  else if (kind == NodeKind_EQ)        return str_f("==");
  else if (kind == NodeKind_NEQ)       return str_f("!=");
  else if (kind == NodeKind_LT)        return str_f("<");
  else if (kind == NodeKind_LEQ)       return str_f("<=");
  else if (kind == NodeKind_ASSIGN)    return str_f("assign");
  else if (kind == NodeKind_EXPR_STMT) return str_f("expr-stmt");
  else if (kind == NodeKind_RETURN)    return str_f("return");
  else if (kind == NodeKind_BLOCK)     return str_f("block");
  else if (kind == NodeKind_IF)        return str_f("if");
  else if (kind == NodeKind_FOR)       return str_f("for");
  else if (kind == NodeKind_FUN_DECL)  return str_f("fun_decl");
  else if (kind == NodeKind_PROG)      return str_f("prog");
  else                                 fail(str_int(kind));
}

static void emit_node(const StrConsumer c, void *data) {
  const Node *node = *((const Node **) data);

  emit_e(c, str_node_kind(node->kind));

  if      (node->kind == NodeKind_NUM) emit_f(c, "(%d)", node->num);
  else if (node->kind == NodeKind_VAR) emit_f(c, "("sv_fmt")", sv_arg(node->name));

  // show type if applicable
  if (node->type) {
    emit_s(c, ": ");
    emit_e(c, str_type(node->type));
  }

  free(data);
}

StrEmitter str_node(const Node *node) {
  const Node **node_ptr = calloc(1, sizeof(const Node *));
  *node_ptr = node;
  return (StrEmitter) { .emit = emit_node, .data = node_ptr };
}

void _emit_ast(const StrConsumer c, const Node *node, StringBuilder *sb, const bool last) {
  if (!node) return;

  // emit string representation for this node
  {
    emit_s(c, sb->buf);
    emit_s(c, last ? "└─" : "├─");
    emit_e(c, str_node(node));
    emit_s(c, "\n");
  }

  // recursively emit children representation
  const int prefix_len = sb_append_f(sb, last ? "  " : "│ ");

  if      (node->kind == NodeKind_NUM) {}
  else if (node->kind == NodeKind_VAR) {}

  else if (node->kind == NodeKind_FUN_DECL) {
    _emit_ast(c, node->body, sb, true);
  }

  else if (node->kind == NodeKind_EXPR_STMT) {
    _emit_ast(c, node->expr, sb, true);
  }

  else if (node->kind == NodeKind_IF) {
    _emit_ast(c, node->cond, sb, false);
    _emit_ast(c, node->body, sb, true);
  }

  else if (node->kind == NodeKind_FOR) {
    _emit_ast(c, node->init,      sb, false);
    _emit_ast(c, node->loop_cond, sb, false);
    _emit_ast(c, node->inc,       sb, false);
    _emit_ast(c, node->loop_body, sb, true);
  }

  else if (node_kind_is_unop(node->kind)) {
    _emit_ast(c, node->operand, sb, true);
  }

  else if (node_kind_is_binop(node->kind)) {
    _emit_ast(c, node->lhs, sb, false);
    _emit_ast(c, node->rhs, sb, true);
  }

  else if (node_kind_is_list(node->kind)) {
    Node *child = node->head;
    while (child) {
      _emit_ast(c, child, sb, !(child->next));
      child = child->next;
    }
  }

  else fail(str_node_kind(node->kind));

   sb_backspace(sb, prefix_len);
}

static void emit_ast(const StrConsumer c, void *data) {
  const Node *node = *((const Node **) data);
  StringBuilder sb = sb_create(256);
  _emit_ast(c, node, &sb, true);
  sb_free(&sb);
}

StrEmitter str_ast(const Node *node) {
  assert(node, "todo: allow raw asserts");
  const Node **node_ptr = calloc(1, sizeof(const Node **));
  *node_ptr = node;
  return (StrEmitter) { .emit = emit_ast, .data = node_ptr };
}

bool node_kind_is_unop(const NodeKind kind) {
  return (kind == NodeKind_NEG)       ||
         (kind == NodeKind_ADDR)      ||
         (kind == NodeKind_DEREF)     ||
         (kind == NodeKind_RETURN);
}

bool node_kind_is_binop(const NodeKind kind) {
  return (kind == NodeKind_ADD) ||
         (kind == NodeKind_SUB) ||
         (kind == NodeKind_MUL) ||
         (kind == NodeKind_DIV) ||
         (kind == NodeKind_EQ)  ||
         (kind == NodeKind_NEQ) ||
         (kind == NodeKind_LEQ) ||
         (kind == NodeKind_LT)  ||
         (kind == NodeKind_ASSIGN);
}

bool node_kind_is_list(const NodeKind kind) {
  return (kind == NodeKind_BLOCK);
}

Node *new_node(const NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

Node *new_num_node(const int num) {
  Node *node = new_node(NodeKind_NUM);
  node->num = num;
  return node;
}

Node *new_var_node(const StringView name) {
  Node *node = new_node(NodeKind_VAR);
  node->name = name;
  return node;
}

Node *new_unop_node(const NodeKind kind, Node *operand) {
  assert(node_kind_is_unop(kind), str_node_kind(kind));
  Node *node = new_node(kind);
  node->operand = operand;
  return node;
}

Node *new_binop_node(const NodeKind kind, Node *lhs, Node *rhs) {
  assert(node_kind_is_binop(kind), str_node_kind(kind));
  Node *node = new_node(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_list_node(const NodeKind kind, Node *head) {
  assert(node_kind_is_list(kind), str_node_kind(kind));
  Node *node = new_node(kind);
  node->head = head;
  return node;
}
