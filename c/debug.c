#include "cc.h"

static void debugv(const char *fmt, va_list ap) {
  vfprintf(stderr, fmt, ap);
}

void debugf(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  debugv(fmt, ap);
  va_end(ap);
}

void errorf(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  debugv(fmt, ap);
  va_end(ap);
  exit(1);
}

StrConsumer DEBUG = { .consume = debug_consume };
StrConsumer ERROR = { .consume = error_consume };

void debug_consume(const char *s, void *ctx) {
  if (s) debugf("%s", s);
  else   debugf("\n");
}

void error_consume(const char *s, void *ctx) {
  debug_consume(s, ctx);
  if (!s) exit(1);
}

void append_consume(const char *s, void *ctx) {
  if (!s) return;
  StringBuilder *sb = (StringBuilder *) ctx;
  sb_appendf(sb, s);
}

static void at_locv(const StrConsumer consumer, const char *loc, const char *fmt, va_list ap) {
  const int col = loc - ctx.src;
  assertf(0 <= col && col <= ctx.src_len,
          "invalid loc: %d, src_len=%d",
          col, ctx.src_len);

  emit_f(consumer, "%s\n", ctx.src);

  emit_f(consumer, "%*s^ ", col, "");
  emit_v(consumer, fmt, ap);

  halt(consumer);
}

void at_loc(const StrConsumer consumer, const char *loc, const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  at_locv(consumer, loc, fmt, ap);
  va_end(ap);
}

void this_loc(const StrConsumer consumer, const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  at_locv(consumer, ctx.lexer.loc, fmt, ap);
  va_end(ap);
}

static void at_spanv(const StrConsumer consumer, const StringView span, const char *fmt, va_list ap) {
  const int col = span.loc - ctx.src;
  assertf(0 <= col && col + span.len <= ctx.src_len,
          "invalid span: (%d, %d), src_len=%d",
          col, span.len, ctx.src_len);


  emit_f(consumer, "%s\n", ctx.src);

  emit_f(consumer, "%*s", col, "");
  for (int i = 0; i < span.len; i++)
    emit_s(consumer, "~");
  emit_s(consumer, " ");
  emit_v(consumer, fmt, ap);

  halt(consumer);
}

void at_tok(const StrConsumer consumer, const Token *tok, const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  at_spanv(consumer, tok->lexeme, fmt, ap);
  va_end(ap);
}

void this_tok(const StrConsumer consumer, const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  at_spanv(consumer, ctx.parser.tok->lexeme, fmt, ap);
  va_end(ap);
}

void at_node(const StrConsumer consumer, const Node *node, const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  at_spanv(consumer, node->lexeme, fmt, ap);
  va_end(ap);
}

void token_stream(const StrConsumer consumer, const Token *tok) {
  assert(tok);

  emit_s(consumer, "[[");

  do emit_f(consumer, " %s", token_to_str(tok));
  while ((tok = tok->next));

  emit_s(consumer, " ]]");

  halt(consumer);
}

void _ast(const StrConsumer consumer, const Node *node, StringBuilder *sb, const bool last) {
  if (!node) return;

  // emit string representation for this node
  {
    const int len = sb_appendf(sb, "%s%s\n", last ? "└─" : "├─", node_to_str(node));
    emit_s(consumer, sb->buf);
    sb_backspace(sb, len);
  }

  // recursively emit children representation
  const int prefix_len = sb_appendf(sb, last ? "  " : "│ ");

  if      (node->kind == NodeKind_NUM) {}
  else if (node->kind == NodeKind_VAR) {}

  else if (node->kind == NodeKind_FUN_DECL) {
    _ast(consumer, node->body, sb, true);
  }

  else if (node->kind == NodeKind_EXPR_STMT) {
    _ast(consumer, node->expr, sb, true);
  }

  else if (node->kind == NodeKind_IF) {
    _ast(consumer, node->cond, sb, true);
    _ast(consumer, node->body, sb, true);
  }

  else if (node->kind == NodeKind_FOR) {
    _ast(consumer, node->init, sb, true);
    _ast(consumer, node->loop_cond, sb, true);
    _ast(consumer, node->inc, sb, true);
    _ast(consumer, node->loop_body, sb, true);
  }

  else if (node_kind_is_unop(node->kind)) {
    _ast(consumer, node->operand, sb, true);
  }

  else if (node_kind_is_binop(node->kind)) {
    _ast(consumer, node->lhs, sb, false);
    _ast(consumer, node->rhs, sb, true);
  }

  else if (node_kind_is_list(node->kind)) {
    Node *child = node->head;
    while (child) {
      _ast(consumer, child, sb, !(child->next));
      child = child->next;
    }
  }

  else failf("%s", node_kind_to_str(node->kind));

  sb_backspace(sb, prefix_len);
}

void ast(const StrConsumer consumer, const Node *node) {
  assert(node);
  StringBuilder sb = sb_create(256);
  _ast(consumer, node, &sb, true);
  sb_free(&sb);
  halt(consumer);
}

void _assert(const char *file, const int line, const char *cond) {
  debugf("%s:%d: assert(%s) failed\n", file, line, cond);
  exit(2);
}

void _assertf(const char *file, const int line, const char *cond,
              const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  debugf("%s:%d: assert(%s) failed: ", file, line, cond);
  debugv(fmt, ap);
  debugf("\n");
  exit(2);
}

void _failf(const char *file, const int line,
            const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  debugf("%s:%d: ", file, line);
  debugv(fmt, ap);
  debugf("\n");
  exit(2);
}
