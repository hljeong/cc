#include "cc.h"

const char *node_kind_to_str(const NodeKind kind) {
  if      (kind == NodeKind_NUM)       return "num";
  else if (kind == NodeKind_VAR)       return "var";
  else if (kind == NodeKind_ADD)       return "+";
  else if (kind == NodeKind_SUB)       return "-";
  else if (kind == NodeKind_MUL)       return "*";
  else if (kind == NodeKind_DIV)       return "/";
  else if (kind == NodeKind_NEG)       return "-";
  else if (kind == NodeKind_ADDR)      return "addr";
  else if (kind == NodeKind_DEREF)     return "deref";
  else if (kind == NodeKind_EQ)        return "==";
  else if (kind == NodeKind_NEQ)       return "!=";
  else if (kind == NodeKind_LT)        return "<";
  else if (kind == NodeKind_LEQ)       return "<=";
  else if (kind == NodeKind_ASSIGN)    return "assign";
  else if (kind == NodeKind_EXPR_STMT) return "expr-stmt";
  else if (kind == NodeKind_RETURN)    return "return";
  else if (kind == NodeKind_BLOCK)     return "block";
  else if (kind == NodeKind_IF)        return "if";
  else if (kind == NodeKind_FOR)       return "for";
  else if (kind == NodeKind_FUN_DECL)  return "fun_decl";
  else if (kind == NodeKind_PROG)      return "prog";
  else                                 failf("%d", kind);
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
  assertf(node_kind_is_unop(kind),
          "%s", node_kind_to_str(kind));
  Node *node = new_node(kind);
  node->operand = operand;
  return node;
}

Node *new_binop_node(const NodeKind kind, Node *lhs, Node *rhs) {
  assertf(node_kind_is_binop(kind),
          "%s", node_kind_to_str(kind));
  Node *node = new_node(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_list_node(const NodeKind kind, Node *head) {
  assertf(node_kind_is_list(kind),
          "%s", node_kind_to_str(kind));
  Node *node = new_node(kind);
  node->head = head;
  return node;
}

static void _debug_ast(const Node *node, const char *prefix, const bool last) {
  if (!node) return;

  char child_prefix[256];
  snprintf(child_prefix, sizeof(child_prefix), "%s%s",
           prefix, last ? "  " : "│ ");

  const char *branch = last ? "└─" : "├─";

  if (node->kind == NodeKind_NUM) {
    debugf("%s%snum(%d)\n", prefix, branch, node->num);
  }

  else if (node->kind == NodeKind_VAR) {
    debugf("%s%svar("sv_fmt")\n", prefix, branch, sv_arg(node->name));
  }

  else if (node->kind == NodeKind_FUN_DECL) {
    debugf("%s%sfn\n", prefix, branch);
    _debug_ast(node->body, child_prefix, true);
  }

  else if (node->kind == NodeKind_EXPR_STMT) {
    debugf("%s%sexpr-stmt\n", prefix, branch);
    _debug_ast(node->expr, child_prefix, true);
  }

  else if (node->kind == NodeKind_IF) {
    debugf("%s%sif\n", prefix, branch);
    _debug_ast(node->cond, child_prefix, false);
    _debug_ast(node->body, child_prefix, true);
  }

  else if (node->kind == NodeKind_FOR) {
    debugf("%s%sfor\n", prefix, branch);
    _debug_ast(node->init, child_prefix, false);
    _debug_ast(node->loop_cond, child_prefix, false);
    _debug_ast(node->inc, child_prefix, false);
    _debug_ast(node->loop_body, child_prefix, true);
  }

  else if (node_kind_is_unop(node->kind)) {
    if (node->type) debugf("%s%s%s: %s\n",
                           prefix, branch,
                           node_kind_to_str(node->kind),
                           type_to_str(node->type));

    else            debugf("%s%s%s\n",
                           prefix, branch,
                           node_kind_to_str(node->kind));

    _debug_ast(node->operand, child_prefix, true);
  }

  else if (node_kind_is_binop(node->kind)) {
    if (node->type) debugf("%s%s%s: %s\n",
                           prefix, branch,
                           node_kind_to_str(node->kind),
                           type_to_str(node->type));

    else            debugf("%s%s%s\n",
                           prefix, branch,
                           node_kind_to_str(node->kind));

    _debug_ast(node->lhs, child_prefix, false);
    _debug_ast(node->rhs, child_prefix, true);
  }

  else if (node_kind_is_list(node->kind)) {
    debugf("%s%s%s\n", prefix, branch, node_kind_to_str(node->kind));
    Node *child = node->head;
    while (child) {
      _debug_ast(child, child_prefix, !(child->next));
      child = child->next;
    }
  }

  else failf("%s", node_kind_to_str(node->kind));
}

void debug_ast(const Node *node) {
  _debug_ast(node, "", true);
}
