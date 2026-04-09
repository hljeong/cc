#include "cc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

StringView sv_create(const char *loc, const int len) {
  return (StringView) { .loc = loc, .len = len };
}

bool sv_eq(const StringView s, const StringView t) {
  return (s.len == t.len) && !strncmp(s.loc, t.loc, s.len);
}

bool sv_eq_s(const StringView s, const char *t) {
  const int len = strlen(t);
  return (s.len == len) && !strncmp(s.loc, t, len);
}

static void fmt_arg_sv(const StrConsumer c, va_list ap) {
  const StringView sv = va_arg(ap, StringView);
  consume_f(c, "%.*s", sv.len, sv.loc);
}

StringBuilder sb_create() {
  char *buf = (char *) calloc(SB_INIT_LEN, sizeof(char));
  return (StringBuilder) { .buf = buf, .capacity = BUF_LEN, .size = 0 };
}

void sb_free(StringBuilder *sb) {
  free(sb->buf);
}

void sb_clear(StringBuilder *sb) {
  sb_truncate(sb, 0);
}

static void sb_append_s(StringBuilder *sb, const char *s) {
  const int s_len = strlen(s);
  const int end_size = sb->size + s_len;
  // make sure to leave space for null terminator
  if (end_size + 1 > sb->capacity) {
    int new_capacity = sb->capacity;
    while (end_size + 1 > new_capacity)
      new_capacity = new_capacity * 3 / 2;
    char *new_buf = realloc(sb->buf, new_capacity * sizeof(char));
    if (!new_buf) {
      free(sb->buf);
      fail("failed to grow string buffer to %d bytes", end_size + 1);
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

void sb_append(StringBuilder *sb, const char *fmt, ...) {
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

static void fmt_arg_list(const StrConsumer c, va_list ap) {
  const char *spec = va_arg(ap, const char *);
  StrFormatter *formatter = &FORMATTERS[0];
  while (formatter->spec) {
    if (!strcmp(spec, formatter->spec)) break;
    formatter++;
  }
  assert(formatter->spec, "unknown spec: %s", spec);

  void *ptr = va_arg(ap, void *);
  consume_s(c, "[");
  while (ptr) {
    ptr = formatter->fmt_ptr(c, ptr);
    if (ptr) consume_s(c, ", ");
  }
  consume_s(c, "]");
}

StrFormatter FORMATTERS[] = {
  (StrFormatter) { .spec = "list",         .fmt_arg = fmt_arg_list,         .fmt_ptr = NULL },
  (StrFormatter) { .spec = "@loc",         .fmt_arg = fmt_arg_at_loc,       .fmt_ptr = NULL },
  (StrFormatter) { .spec = "@cur_loc",     .fmt_arg = fmt_arg_at_cur_loc,   .fmt_ptr = NULL },
  (StrFormatter) { .spec = "@tok",         .fmt_arg = fmt_arg_at_tok,       .fmt_ptr = NULL },
  (StrFormatter) { .spec = "@cur_tok",     .fmt_arg = fmt_arg_cur_tok,      .fmt_ptr = NULL },
  (StrFormatter) { .spec = "@node",        .fmt_arg = fmt_arg_at_node,      .fmt_ptr = NULL },
  (StrFormatter) { .spec = "sv",           .fmt_arg = fmt_arg_sv,           .fmt_ptr = NULL },
  (StrFormatter) { .spec = "token_kind",   .fmt_arg = fmt_arg_token_kind,   .fmt_ptr = NULL },
  (StrFormatter) { .spec = "token",        .fmt_arg = fmt_arg_token,        .fmt_ptr = fmt_ptr_token, },
  (StrFormatter) { .spec = "node_kind",    .fmt_arg = fmt_arg_node_kind,    .fmt_ptr = NULL },
  (StrFormatter) { .spec = "node",         .fmt_arg = fmt_arg_node,         .fmt_ptr = fmt_ptr_node },
  (StrFormatter) { .spec = "ast",          .fmt_arg = fmt_arg_ast,          .fmt_ptr = NULL },
  (StrFormatter) { .spec = "type_kind",    .fmt_arg = fmt_arg_type_kind,    .fmt_ptr = NULL },
  (StrFormatter) { .spec = "type",         .fmt_arg = fmt_arg_type,         .fmt_ptr = fmt_ptr_type },
  (StrFormatter) { .spec = "symbol_kind",  .fmt_arg = fmt_arg_symbol_kind,  .fmt_ptr = NULL },
  (StrFormatter) { .spec = "symbol",       .fmt_arg = fmt_arg_symbol,       .fmt_ptr = fmt_ptr_symbol },
  (StrFormatter) { .spec = NULL,           .fmt_arg = NULL,                 .fmt_ptr = NULL },
};

void consume_v(const StrConsumer c, const char *fmt, va_list ap) {
  char buf[BUF_LEN];
  const char *seg = fmt;

  while (*fmt) {
    // not a specifier, skip
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
      StrFormatter *formatter = &FORMATTERS[0];
      while (formatter->spec) {
        if (!strcmp(spec, formatter->spec)) {
          formatter->fmt_arg(c, ap);
          break;
        }
        formatter++;
      }
      assert(formatter->spec, "unknown spec: %s", spec);
    }

    // start new segment
    seg = fmt;
  }

  // flush remaining segment
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
