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

void sb_appendv(StringBuilder *sb, const char *fmt, va_list ap) {
  const int space = sb->capacity - sb->size;
  const int len = vsnprintf(sb->buf + sb->size, space, fmt, ap);
  assertf(len < space, "not enough space: len=%d, space=%d",
          len, space);
  sb->size += len;
}

void sb_appendf(StringBuilder *sb, const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  sb_appendv(sb, fmt, ap); va_end(ap);
}
