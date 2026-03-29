#include "cc.h"

StringView sv_create(const char *loc, const int len) {
  return (StringView) { .loc = loc, .len = len };
}

int sv_eq(const StringView s, const StringView t) {
  return (s.len == t.len) && !strncmp(s.loc, t.loc, s.len);
}

StringBuilder sb_create(const int capacity) {
  assert(capacity, "todo: allow raw asserts");
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
  assert(len < space,
         str_f("not enough space: len=%d, space=%d",
               len, space));
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
  assert(len <= sb->size,
         str_f("len=%d, sb->size=%d",
               len, sb->size));
  sb->size -= len;
  sb->buf[sb->size] = '\0';
}

void halt(const StrConsumer consumer) {
  consumer.consume(NULL, consumer.ctx);
}

void emit_s(const StrConsumer consumer, const char *s) {
  consumer.consume(s, consumer.ctx);
}

void emit_v(const StrConsumer consumer, const char *fmt, va_list ap) {
  StringBuilder sb = sb_create(256);
  sb_append_v(&sb, fmt, ap);
  emit_s(consumer, sb.buf);
  sb_free(&sb);
}

void emit_f(const StrConsumer consumer, const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  emit_v(consumer, fmt, ap);
  va_end(ap);
}

void emit_e(const StrConsumer consumer, const StrEmitter emitter) {
  emitter.emit(consumer, emitter.data);
}

const StrEmitter HALT = { .emit = NULL };

void emit_all_v(const StrConsumer consumer, va_list ap) {
  while (true) {
    const StrEmitter emitter = va_arg(ap, StrEmitter);
    if (!emitter.emit) break;
    emit_e(consumer, emitter);
  }
  // todo: does this make sense?
  halt(consumer);
}

void _emit_all(const StrConsumer consumer, ...) {
  va_list ap; va_start(ap, consumer);
  emit_all_v(consumer, ap);
  va_end(ap);
}

static void emit_sb(const StrConsumer consumer, void *data) {
  StringBuilder *sb = (StringBuilder *) data;
  emit_s(consumer, sb->buf);
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

static void emit_int(const StrConsumer consumer, void *data) {
  int value = *((int *) data);
  emit_f(consumer, "%d", value);
  free(data);
}

StrEmitter str_int(const int value) {
  int *int_ptr = calloc(1, sizeof(int *));
  *int_ptr = value;
  return (StrEmitter) { .emit = emit_int, .data = int_ptr };
}
