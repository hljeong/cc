#include "cc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

StringView sv_create(const char *loc, const int len) {
  return (StringView) { .loc = loc, .len = len };
}

int sv_eq(const StringView s, const StringView t) {
  return (s.len == t.len) && !strncmp(s.loc, t.loc, s.len);
}

StringBuilder sb_create(const int capacity) {
  assert(capacity);
  char *buf = (char *) calloc(capacity, sizeof(char));
  return (StringBuilder) { .buf = buf, .capacity = capacity, .size = 0 };
}

void sb_free(StringBuilder *sb) {
  free(sb->buf);
}

void sb_clear(StringBuilder *sb) {
  sb_truncate(sb, 0);
}

void sb_append_s(StringBuilder *sb, const char *s) {
  const int space = sb->capacity - sb->size;
  const int len = snprintf(sb->buf + sb->size, space, "%s", s);
  assert_f(len < space,
           "not enough space: len=%d, space=%d, sb->size=%d, sb->capacity=%d",
           len, space, sb->size, sb->capacity);
  sb->size += len;
}

static void sb_append_consume(const char *s, void *ctx) {
  if (!s) return;
  StringBuilder *sb = (StringBuilder *) ctx;
  sb_append_s(sb, s);
}

static StrConsumer SB_APPEND(StringBuilder *sb) {
  return (StrConsumer) { .consume = sb_append_consume, .ctx = sb };
}

void sb_append_f(StringBuilder *sb, const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  StrConsumer sb_append = SB_APPEND(sb);
  consume_v(sb_append, fmt, ap);
  va_end(ap);
  halt(sb_append);
}

void sb_truncate(StringBuilder *sb, const int to) {
  assert_f(0 <= to && to < sb->size,
           "to=%d, sb->size=%d",
           to, sb->size);
  sb->size = to;
  sb->buf[sb->size] = '\0';
}

static void emit_halt(const StrConsumer c, void *data) {
  c.consume(NULL, c.ctx);
}

const StrEmitter HALT = { .emit = emit_halt };

static void consume_e(const StrConsumer c, const StrEmitter e) {
  e.emit(c, e.data);
}

void halt(const StrConsumer c) {
  consume_e(c, HALT);
}

static void consume_s(const StrConsumer c, const char *s) {
  c.consume(s, c.ctx);
}

void consume_v(const StrConsumer c, const char *fmt, va_list ap) {
  char buf[BUF_LEN];
  const char *seg = fmt;

  while (*fmt) {
    // not a specifier
    if (*fmt++ != '%') continue;

    // standard specifier, skip
    if (*fmt++ != '{') continue;

    // flush current segment up to right before '%'
    {
      char seg_fmt[BUF_LEN];
      const int seg_len = (fmt - 2) - seg;
      if (seg_len) {
        assert(seg_len < sizeof(seg_fmt));
        memcpy(seg_fmt, seg, seg_len);
        seg_fmt[seg_len] = '\0';

        const int len = vsnprintf(buf, sizeof(buf), seg_fmt, ap);
        assert(len < sizeof(buf));
        consume_s(c, buf);
      }
    }

    // process custom format specifier
    {
      // parse format specifier
      char spec[BUF_LEN];
      const char *spec_start = fmt;
      while (*fmt && *fmt != '}') fmt++;
      assert(*fmt == '}');
      const int spec_len = fmt - spec_start;
      assert(spec_len < sizeof(spec));
      memcpy(spec, spec_start, spec_len);
      spec[spec_len] = '\0';
      fmt++;  // skip '}'

      // dispatch
      if (!strcmp(spec, "")) {
        const StrEmitter e = va_arg(ap, StrEmitter);
        consume_e(c, e);
      }

      else if (!strcmp(spec, "sv")) {
        const StringView sv = va_arg(ap, StringView);
        consume_f(c, sv_fmt, sv_arg(sv));
      }

      else if (!strcmp(spec, "token_kind")) {
        const TokenKind kind = va_arg(ap, TokenKind);
        consume_e(c, str_token_kind(kind));
      }

      else if (!strcmp(spec, "token")) {
        const Token *tok = va_arg(ap, const Token *);
        consume_e(c, str_token(tok));
      }

      else if (!strcmp(spec, "token_stream")) {
        const Token *tok = va_arg(ap, const Token *);
        consume_e(c, str_token_stream(tok));
      }

      else if (!strcmp(spec, "node_kind")) {
        const NodeKind node_kind = va_arg(ap, NodeKind);
        consume_e(c, str_node_kind(node_kind));
      }

      else if (!strcmp(spec, "node")) {
        const Node *node = va_arg(ap, const Node *);
        consume_e(c, str_node(node));
      }

      else if (!strcmp(spec, "ast")) {
        const Node *node = va_arg(ap, const Node *);
        consume_e(c, str_ast(node));
      }

      else if (!strcmp(spec, "type_kind")) {
        const TypeKind type_kind = va_arg(ap, TypeKind);
        consume_e(c, str_type_kind(type_kind));
      }

      else if (!strcmp(spec, "type")) {
        const Type *type = va_arg(ap, const Type *);
        consume_e(c, str_type(type));
      }

      else fail_f("unknown spec: %s", spec);
    }

    // start new segment
    seg = fmt;
  }

  if (*seg) {
      const int len = vsnprintf(buf, sizeof(buf), seg, ap);
      assert(len < sizeof(buf));
      consume_s(c, buf);
  }
}

void consume_f(const StrConsumer c, const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  consume_v(c, fmt, ap);
  va_end(ap);
}

static void emit_str(const StrConsumer c, void *data) {
  const char *s = (const char *) data;
  consume_s(c, s);
  free(data);
}

StrEmitter str_f(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  char *s = calloc(BUF_LEN, sizeof(char));
  const int len = vsnprintf(s, BUF_LEN, fmt, ap);
  assert(len < BUF_LEN);
  StrEmitter e = (StrEmitter) { .emit = emit_str, .data = s };
  va_end(ap);
  return e;
}
