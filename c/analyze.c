#include "cc.h"

static Var *new_var(const StringView name) {
  Var *var = calloc(1, sizeof(Var));
  var->name = name;
  return var;

}

static Var *lookup_or_new(Var *locals, const StringView name) {
  if (sv_eq(locals->name, name)) return locals;
  else if (locals->next) return lookup_or_new(locals->next, name);
  else return (locals->next = new_var(name));
}

static void visit(Node *node);

static void visit(Node *node) {
  if (!node) {}

  else if (node->kind == NodeKind_FUN_DECL) {
    visit(node->body);
  }

  else if (node->kind == NodeKind_IF) {
    visit(node->cond);
    visit(node->then);
    visit(node->else_);
  }

  else if (node_kind_is_unop(node->kind)) {
    visit(node->operand);
  }

  else if (node_kind_is_binop(node->kind)) {
    visit(node->lhs);
    visit(node->rhs);
  }

  else if (node_kind_is_list(node->kind)) {
    for (Node *cur = node->head; cur; cur = cur->next) {
      visit(cur);
    }
  }

  else if (node->kind == NodeKind_VAR) {
    lookup_or_new(&ctx.analyzer.locals, node->name);
  }

  else if (node->kind == NodeKind_NUM);

  else failf("%s", node_kind_to_str(node->kind));
}

void analyze() {
  visit(ctx.ast);
}
