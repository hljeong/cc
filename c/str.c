#include "cc.h"

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
  sb->buf[0] = '\0';
  sb->size = 0;
}

int sb_append_v(StringBuilder *sb, const char *fmt, va_list ap) {
  const int space = sb->capacity - sb->size;
  const int len = vsnprintf(sb->buf + sb->size, space, fmt, ap);
  assert_f(len < space,
           "not enough space: len=%d, space=%d",
            len, space);
  sb->size += len;
  return len;
}

int sb_append_f(StringBuilder *sb, const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  const int len = sb_append_v(sb, fmt, ap);
  va_end(ap);
  return len;
}

void sb_backspace(StringBuilder *sb, const int len) {
  assert_f(len <= sb->size,
           "len=%d, sb->size=%d",
           len, sb->size);
  sb->size -= len;
  sb->buf[sb->size] = '\0';
}

static void emit_halt(const StrConsumer c, void *data) {
  c.consume(NULL, c.ctx);
}

const StrEmitter HALT = { .emit = emit_halt };

// todo: delete old version and rename
void emit_v2(const StrConsumer c, const char *fmt, va_list ap) {
  char buf[256];
  const char *seg = fmt;

  while (*fmt) {
    // not a specifier
    if (*fmt++ != '%') continue;

    // standard specifier, skip
    if (*fmt++ != '{') continue;

    // flush current segment up to right before '%'
    {
      char seg_fmt[256];
      const int seg_len = (fmt - 2) - seg;
      if (seg_len) {
        assert(seg_len < sizeof(seg_fmt));
        memcpy(seg_fmt, seg, seg_len);
        seg_fmt[seg_len] = '\0';

        const int len = vsnprintf(buf, sizeof(buf), seg_fmt, ap);
        assert(len < sizeof(buf));
        emit_s(c, buf);
      }
    }

    // process custom format specifier
    {
      // parse format specifier
      char spec[256];
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
        emit_e(c, e);
      }

      else if (!strcmp(spec, "sv")) {
        const StringView sv = va_arg(ap, StringView);
        emit_f(c, sv_fmt, sv_arg(sv));
      }

      else if (!strcmp(spec, "token_kind")) {
        const TokenKind kind = va_arg(ap, TokenKind);
        emit_e(c, str_token_kind(kind));
      }

      else if (!strcmp(spec, "token")) {
        const Token *tok = va_arg(ap, const Token *);
        emit_e(c, str_token(tok));
      }

      else if (!strcmp(spec, "token_stream")) {
        const Token *tok = va_arg(ap, const Token *);
        emit_e(c, str_token_stream(tok));
      }

      else if (!strcmp(spec, "node_kind")) {
        const NodeKind node_kind = va_arg(ap, NodeKind);
        emit_e(c, str_node_kind(node_kind));
      }

      else if (!strcmp(spec, "node")) {
        const Node *node = va_arg(ap, const Node *);
        emit_e(c, str_node(node));
      }

      else if (!strcmp(spec, "ast")) {
        const Node *node = va_arg(ap, const Node *);
        emit_e(c, str_ast(node));
      }

      else if (!strcmp(spec, "type_kind")) {
        const TypeKind type_kind = va_arg(ap, TypeKind);
        emit_e(c, str_type_kind(type_kind));
      }

      else if (!strcmp(spec, "type")) {
        const Type *type = va_arg(ap, const Type *);
        emit_e(c, str_type(type));
      }

      else fail_f("unknown spec: %s", spec);
    }

    // start new segment
    seg = fmt;
  }

  if (*seg) {
      const int len = vsnprintf(buf, sizeof(buf), seg, ap);
      assert(len < sizeof(buf));
      emit_s(c, buf);
  }
}

void emit_f2(const StrConsumer c, const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  emit_v2(c, fmt, ap);
  va_end(ap);
}

void emit_s(const StrConsumer c, const char *s) {
  c.consume(s, c.ctx);
}

void emit_v(const StrConsumer c, const char *fmt, va_list ap) {
  StringBuilder sb = sb_create(256);
  sb_append_v(&sb, fmt, ap);
  emit_s(c, sb.buf);
  sb_free(&sb);
}

void emit_f(const StrConsumer c, const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  emit_v(c, fmt, ap);
  va_end(ap);
}

void emit_e(const StrConsumer c, const StrEmitter emitter) {
  emitter.emit(c, emitter.data);
}

void emit_all_v(const StrConsumer c, va_list ap) {
  while (true) {
    const StrEmitter emitter = va_arg(ap, StrEmitter);
    assert(emitter.emit);
    emit_e(c, emitter);
    // send halt to consumer before breaking
    if (emitter.emit == emit_halt) break;
  }
}

void _emit_all(const StrConsumer c, ...) {
  va_list ap; va_start(ap, c);
  emit_all_v(c, ap);
  va_end(ap);
}

static void emit_sb(const StrConsumer c, void *data) {
  StringBuilder *sb = (StringBuilder *) data;
  emit_s(c, sb->buf);
  sb_free(sb);
  free(data);
}

StrEmitter str_v(const char *fmt, va_list ap) {
  StringBuilder *sb = calloc(1, sizeof(StringBuilder));
  *sb = sb_create(256);
  sb_append_v(sb, fmt, ap);
  return (StrEmitter) { .emit = emit_sb, .data = sb };
}

StrEmitter str_f(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  StrEmitter emitter = str_v(fmt, ap);
  va_end(ap);
  return emitter;
}

static void emit_int(const StrConsumer c, void *data) {
  int value = *((int *) data);
  emit_f(c, "%d", value);
  free(data);
}

StrEmitter str_int(const int value) {
  int *int_ptr = calloc(1, sizeof(int *));
  *int_ptr = value;
  return (StrEmitter) { .emit = emit_int, .data = int_ptr };
}
