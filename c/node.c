#include "cc.h"

#include <stdlib.h>

static void consume_node_kind(const StrConsumer c, const NodeKind kind) {
  if      (kind == NodeKind_NUM)       consume_f(c, "num");
  else if (kind == NodeKind_REF)       consume_f(c, "ref");
  else if (kind == NodeKind_DECL)      consume_f(c, "decl");
  else if (kind == NodeKind_ADD)       consume_f(c, "+");
  else if (kind == NodeKind_SUB)       consume_f(c, "-");
  else if (kind == NodeKind_MUL)       consume_f(c, "*");
  else if (kind == NodeKind_DIV)       consume_f(c, "/");
  else if (kind == NodeKind_NEG)       consume_f(c, "-");
  else if (kind == NodeKind_ADDR)      consume_f(c, "addr");
  else if (kind == NodeKind_DEREF)     consume_f(c, "deref");
  else if (kind == NodeKind_EQ)        consume_f(c, "==");
  else if (kind == NodeKind_NEQ)       consume_f(c, "!=");
  else if (kind == NodeKind_LT)        consume_f(c, "<");
  else if (kind == NodeKind_LEQ)       consume_f(c, "<=");
  else if (kind == NodeKind_ASSIGN)    consume_f(c, "assign");
  else if (kind == NodeKind_CALL)      consume_f(c, "call");
  else if (kind == NodeKind_EXPR_STMT) consume_f(c, "expr-stmt");
  else if (kind == NodeKind_DECL_STMT) consume_f(c, "decl-stmt");
  else if (kind == NodeKind_VAR_DECL)  consume_f(c, "var-decl");
  else if (kind == NodeKind_RETURN)    consume_f(c, "return");
  else if (kind == NodeKind_BLOCK)     consume_f(c, "block");
  else if (kind == NodeKind_IF)        consume_f(c, "if");
  else if (kind == NodeKind_FOR)       consume_f(c, "for");
  else if (kind == NodeKind_FUN_DECL)  consume_f(c, "fun_decl");
  else if (kind == NodeKind_PROG)      consume_f(c, "prog");
  else                                 fail("unexpected node kind: %d", kind);
}

void fmt_arg_node_kind(const StrConsumer c, va_list ap) {
  consume_node_kind(c, va_arg(ap, const NodeKind));
}

static void consume_node(const StrConsumer c, const Node *node) {
  consume_f(c, "%{node_kind}", node->kind);

  if (node->kind == NodeKind_NUM)  consume_f(c, "(%d)",    node->num.value);

  else if (node->kind == NodeKind_REF) {
    // references are not annotated with type at parse time
    if (node->type)
      consume_f(c, "(%{sv}: %{type})",
                node->var.name, node->type);
    else
      consume_f(c, "(%{sv})", node->var.name);
  }

  else if (node->kind == NodeKind_DECL) {
    // declarations always have a type
    consume_f(c, "(%{sv}: %{type})",
              node->var.name, node->type);
  }

  // show type if applicable
  else if (node->type) consume_f(c, ": %{type}", node->type);
}

void *fmt_ptr_node(const StrConsumer c, void *ptr) {
  consume_node(c, ptr);
  return ((const Node *) ptr)->next;
}

void fmt_arg_node(const StrConsumer c, va_list ap) {
  consume_node(c, va_arg(ap, const Node *));
}

void _consume_ast(const StrConsumer c, const Node *node, StringBuilder *sb, const bool last) {
  if (!node) return;

  consume_f(c, "%s%s%{node}\n",
            sb->buf, last ? "└─" : "├─", node, "\n");

  const int truncate_to = sb->size;
  sb_append(sb, last ? "  " : "│ ");

  if      (node->kind == NodeKind_NUM)       {}
  else if (node->kind == NodeKind_REF)       {}

  else if (node->kind == NodeKind_PROG) {
    Node *child = node->prog.head;
    while (child) {
      _consume_ast(c, child, sb, !(child->next));
      child = child->next;
    }
  }

  else if (node->kind == NodeKind_DECL) {
    Node *param = node->decl.params;
    while (param) {
      _consume_ast(c, param, sb, !(param->next));
      param = param->next;
    }
  }

  else if (node->kind == NodeKind_FUN_DECL) {
    _consume_ast(c, node->fun_decl.decl, sb, false);
    _consume_ast(c, node->fun_decl.body, sb, true);
  }

  else if (node->kind == NodeKind_VAR_DECL) {
    _consume_ast(c, node->var_decl.decl,  sb, !node->var_decl.init);
    _consume_ast(c, node->var_decl.init, sb, true);
  }

  else if (node->kind == NodeKind_EXPR_STMT) {
    _consume_ast(c, node->expr_stmt.expr, sb, true);
  }

  else if (node->kind == NodeKind_IF) {
    _consume_ast(c, node->if_.cond,  sb, false);
    _consume_ast(c, node->if_.then,  sb, !node->if_.else_);
    _consume_ast(c, node->if_.else_, sb, true);
  }

  else if (node->kind == NodeKind_FOR) {
    _consume_ast(c, node->for_.init, sb, false);
    _consume_ast(c, node->for_.cond, sb, false);
    _consume_ast(c, node->for_.inc,  sb, false);
    _consume_ast(c, node->for_.body, sb, true);
  }

  else if (node->kind == NodeKind_CALL) {
    _consume_ast(c, node->call.fun, sb, !node->call.args);
    Node *arg = node->call.args;
    while (arg) {
      _consume_ast(c, arg, sb, !arg->next);
      arg = arg->next;
    }
  }

  else if (node_kind_is_unop(node->kind)) {
    _consume_ast(c, node->unop.opr, sb, true);
  }

  else if (node_kind_is_binop(node->kind)) {
    _consume_ast(c, node->binop.lhs, sb, false);
    _consume_ast(c, node->binop.rhs, sb, true);
  }

  else if (node_kind_is_list(node->kind)) {
    Node *child = node->list.head;
    while (child) {
      _consume_ast(c, child, sb, !(child->next));
      child = child->next;
    }
  }

  else fail("unexpected node kind: {%node_kind}",
            node->kind);

   sb_truncate(sb, truncate_to);
}

static void consume_ast(const StrConsumer c, const Node *node) {
  StringBuilder sb = sb_create();
  _consume_ast(c, node, &sb, true);
  sb_free(&sb);
}

void fmt_arg_ast(const StrConsumer c, va_list ap) {
  consume_ast(c, va_arg(ap, const Node *));
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
  return (kind == NodeKind_BLOCK) ||
         (kind == NodeKind_DECL_STMT);
}

Node *new_node(const NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

Node *new_num_node(const int value) {
  Node *node = new_node(NodeKind_NUM);
  node->num.value = value;
  return node;
}

Node *new_var_node(const StringView name) {
  Node *node = new_node(NodeKind_REF);
  node->var.name = name;
  return node;
}

Node *new_unop_node(const NodeKind kind, Node *opr) {
  assert(node_kind_is_unop(kind), "not an unop: %{node_kind}", kind);
  Node *node = new_node(kind);
  node->unop.opr = opr;
  return node;
}

Node *new_binop_node(const NodeKind kind, Node *lhs, Node *rhs) {
  assert(node_kind_is_binop(kind), "not a binop: %{node_kind}", kind);
  Node *node = new_node(kind);
  node->binop.lhs = lhs;
  node->binop.rhs = rhs;
  return node;
}

Node *new_list_node(const NodeKind kind, Node *head) {
  assert(node_kind_is_list(kind), "not a list: %{node_kind}", kind);
  Node *node = new_node(kind);
  node->list.head = head;
  return node;
}
