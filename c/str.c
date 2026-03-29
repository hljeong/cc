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

static void fmt_sv(const StrConsumer c, va_list ap) {
  const StringView sv = va_arg(ap, StringView);
  consume_f(c, sv_fmt, sv_arg(sv));
}

StringBuilder sb_create() {
  char *buf = (char *) calloc(BUF_LEN, sizeof(char));
  return (StringBuilder) { .buf = buf, .capacity = BUF_LEN, .size = 0 };
}

void sb_free(StringBuilder *sb) {
  free(sb->buf);
}

void sb_clear(StringBuilder *sb) {
  sb_truncate(sb, 0);
}

void sb_append_s(StringBuilder *sb, const char *s) {
  const int s_len = strlen(s);
  const int end_size = sb->size + s_len;
  // make sure to leave sace for null terminator
  if (end_size + 1 > sb->capacity) {
    int new_capacity = sb->capacity;
    while (end_size + 1 > new_capacity)
      new_capacity = new_capacity * 3 / 2;
    char *new_buf = realloc(sb->buf, new_capacity * sizeof(char));
    if (!new_buf) {
      free(sb->buf);
      fail("failed to grow string buffer to %d bytes", sb->capacity);
    }
    sb->buf = new_buf;
    sb->capacity = new_capacity;
  }
  // include null terminator
  memcpy(sb->buf + sb->size, s, s_len + 1);
  sb->size += s_len;
}

static void sb_append_consume(const char *s, void *ctx) {
  assert(s);
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
}

void sb_truncate(StringBuilder *sb, const int to) {
  assert(0 <= to && to < sb->size,
         "to=%d, sb->size=%d",
         to, sb->size);
  sb->size = to;
  sb->buf[sb->size] = '\0';
}

static void consume_s(const StrConsumer c, const char *s) {
  c.consume(s, c.ctx);
}

StrFormatter FORMATTERS[] = {
  (StrFormatter) { .spec = "@loc",         .fmt = fmt_at_loc },
  (StrFormatter) { .spec = "@cur_loc",     .fmt = fmt_at_cur_loc },
  (StrFormatter) { .spec = "@tok",         .fmt = fmt_at_tok },
  (StrFormatter) { .spec = "@cur_tok",     .fmt = fmt_at_cur_tok },
  (StrFormatter) { .spec = "@node",        .fmt = fmt_at_node },
  (StrFormatter) { .spec = "sv",           .fmt = fmt_sv },
  (StrFormatter) { .spec = "token_kind",   .fmt = fmt_token_kind },
  (StrFormatter) { .spec = "token",        .fmt = fmt_token },
  (StrFormatter) { .spec = "token_stream", .fmt = fmt_token_stream },
  (StrFormatter) { .spec = "node_kind",    .fmt = fmt_node_kind },
  (StrFormatter) { .spec = "node",         .fmt = fmt_node },
  (StrFormatter) { .spec = "ast",          .fmt = fmt_ast },
  (StrFormatter) { .spec = "type_kind",    .fmt = fmt_type_kind },
  (StrFormatter) { .spec = "type",         .fmt = fmt_type },
  (StrFormatter) { .spec = NULL,           .fmt = NULL },
};

void consume_v(const StrConsumer c, const char *fmt, va_list ap) {
  char buf[BUF_LEN];
  const char *seg = fmt;

  while (*fmt) {
    // not a specifier
    if (*fmt++ != '%') continue;

    // standard specifier, skip
    if (*fmt++ != '{') continue;

    // flush current segment up to right before '%'
    // todo: or when we hit some run-length?
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
      StrFormatter *formatter = &FORMATTERS[0];
      while (formatter->spec) {
        if (!strcmp(spec, formatter->spec)) {
          formatter->fmt(c, ap);
          break;
        }
        formatter++;
      }
      assert(formatter->spec, "unknown spec: %s", spec);
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
