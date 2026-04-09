// mit license - copyright (c) 2026

#ifndef CFMT_H
#define CFMT_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CFMT_SEG_BUF_LEN  (4096)
#define CFMT_SPEC_BUF_LEN (256)

typedef struct {
  void *self;
  void (*emit)(void *self, const char *s);
} sink;

typedef struct formatter formatter;
struct formatter {
  const char *spec;
  int (*fmt)(const sink s, va_list);
  formatter *next;
};

extern formatter FORMATTERS;

// `spec` must be string literal
formatter *add_formatter(const char *spec, int (*fmt)(const sink s, va_list));

enum {
  OK             =  0,
  OVERFLOW       = -727,
  SINGLE_RBRACE  = -728,
  MISSING_RBRACE = -729,
  UNKNOWN_SPEC   = -730,
};

// supposedly passing va_list by value is not portable
// but vsnprintf only receives the va_list by value,
// so thats a whole load of bullshit im not about to
// deal with
int  emitf(const sink s, const char *fmt, ...);
int vemitf(const sink s, const char *fmt, va_list ap);

int    print(const char *fmt, ...);
int   vprint(const char *fmt, va_list ap);
int   fprint(FILE *file, const char *fmt, ...);
int  vfprint(FILE *file, const char *fmt, va_list ap);
int  snprint(char *buf, const size_t size, const char *fmt, ...);
int vsnprint(char *buf, const size_t size, const char *fmt, va_list ap);


typedef struct {
  const char *loc;
  int len;
} str_view;

str_view    sv_create(const char *loc, const int len);
const char *sv_end   (const str_view s);
int         sv_cmp   (const str_view s, const str_view t);
int         sv_cmp_s (const str_view s, const char *t);


typedef struct {
  char *buf;
  int capacity;
  int size;
} str_builder;

str_builder sb_create  (int capacity);
void        sb_free    (str_builder *self);
void        sb_append  (str_builder *self, const char *fmt, ...);
void        sb_truncate(str_builder *self, const int to);

#endif

// instantiate the implementation with #define CFMT_IMPL
#ifdef CFMT_IMPL

formatter *add_formatter(const char *spec, int (*fmt)(const sink s, va_list)) {
  formatter *f = calloc(1, sizeof(formatter));
  f->spec = spec;
  f->fmt = fmt;
  f->next = FORMATTERS.next;
  return (FORMATTERS.next = f);
}

int vemitf(const sink s, const char *fmt, va_list ap) {
  char buf[CFMT_SEG_BUF_LEN];
  int total_len = 0;

  const char *seg = fmt;
  while (*fmt) {
    const char c = *fmt++;
    if (c != '{' && c != '}') continue;

    // emit current segment up to right before `c`
    {
      char seg_fmt[CFMT_SEG_BUF_LEN];
      const size_t seg_len = (fmt - 1) - seg;
      if (seg_len) {
        if (seg_len >= sizeof(seg_fmt)) return OVERFLOW;
        memcpy(seg_fmt, seg, seg_len);
        seg_fmt[seg_len] = '\0';

        // delegate formatting standard format specifiers to vsnprintf
        const int ret = vsnprintf(buf, sizeof(buf), seg_fmt, ap);
        if (ret < 0) return ret;
        if ((size_t) ret >= sizeof(buf)) return OVERFLOW;
        s.emit(s.self, buf);
        total_len += ret;
      }
    }

    if (c == '}') {
      // "}}" -> "}"
      if (*fmt++ != '}') return SINGLE_RBRACE;
       s.emit(s.self, "}");
      total_len++;
      seg = fmt;
    }

    // "{{" -> "{"
    else if (*fmt == '{') {
      fmt++;
       s.emit(s.self, "{");
      total_len++;
      seg = fmt;
    }

    // custom formatter
    else {
      // parse format specifier
      char spec_buf[CFMT_SPEC_BUF_LEN];
      const char *spec_start = fmt;
      while (*fmt && *fmt != '}') fmt++;
      if (*fmt != '}') return MISSING_RBRACE;
      const size_t spec_len = fmt - spec_start;
      if (spec_len >= sizeof(spec_buf)) return OVERFLOW;
      memcpy(spec_buf, spec_start, spec_len);
      spec_buf[spec_len] = '\0';
      fmt++;

      // dispatch formatter
      formatter *f = FORMATTERS.next;
      while (f) {
        if (!strcmp(spec_buf, f->spec)) {
          const int ret = f->fmt(s, ap);
          if (ret < 0) return ret;
          total_len += ret;
          break;
        }
        f = f->next;
      }
      if (!f) return UNKNOWN_SPEC;

      seg = fmt;
    }
  }

  // emit last segment
  if (*seg) {
    // delegate formatting standard format specifiers to vsnprintf
    const int ret = vsnprintf(buf, sizeof(buf), seg, ap);
    if (ret < 0) return ret;
    if ((size_t) ret >= sizeof(buf)) return OVERFLOW;
    s.emit(s.self, buf);
    total_len += ret;
  }

  return total_len;
}

int emitf(const sink s, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  const int ret = vemitf(s, fmt, ap);
  va_end(ap);
  return ret;
}

typedef struct { FILE *file; } _cfmt_fprint_sink;
typedef struct { char *buf; size_t n; } _cfmt_snprint_sink;

static void _cfmt_print(void *self, const char *s) {
  (void) self;
  printf("%s", s);
}

int vprint(const char *fmt, va_list ap) {
  return vemitf((sink) { .emit = _cfmt_print }, fmt, ap);
}

int print(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  const int ret = vprint(fmt, ap);
  va_end(ap);
  return ret;
}

static void _cfmt_fprint(void *self, const char *s) {
  fprintf(((_cfmt_fprint_sink *) self)->file, "%s", s);
}

int vfprint(FILE *file, const char *fmt, va_list ap) {
  _cfmt_fprint_sink s = { .file = file };
  return vemitf((sink) { .self = &s, .emit = _cfmt_fprint }, fmt, ap);
}

int fprint(FILE *file, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  const int ret = vfprint(file, fmt, ap);
  va_end(ap);
  return ret;
}

static void _cfmt_snprint(void *self, const char *s) {
  _cfmt_snprint_sink *sink = self;
  const int ret = snprintf(sink->buf, sink->n, "%s", s);
  const int advance = (size_t) ret >= sink->n ? (int) sink->n : ret;
  sink->buf += advance;
  sink->n -= advance;
}

int vsnprint(char *buf, const size_t n, const char *fmt, va_list ap) {
  _cfmt_snprint_sink s = { .buf = buf, .n = n };
  return vemitf((sink) { .self = &s, .emit = _cfmt_snprint }, fmt, ap);
}

int snprint(char *buf, const size_t n, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  const int ret = vsnprint(buf, n, fmt, ap);
  va_end(ap);
  return ret;
}


static int _cfmt_fmt_sv(const sink s, va_list ap) {
  const str_view sv = va_arg(ap, str_view);
  return emitf(s, "%.*s", sv.len, sv.loc);
}

formatter _cfmt_sv_formatter = { .spec = "sv", .fmt = _cfmt_fmt_sv };

str_view sv_create(const char *loc, const int len) {
  return (str_view) { .loc = loc, .len = len };
}

const char *sv_end(const str_view s) {
  return s.loc + s.len;
}

int sv_cmp(const str_view s, const str_view t) {
  const int min_len = s.len < t.len ? s.len : t.len;
  const int cmp = strncmp(s.loc, t.loc, min_len);
  return cmp ? cmp : (s.len - t.len);
}

int sv_cmp_s(const str_view s, const char *t) {
  return sv_cmp(s, sv_create(t, strlen(t)));
}


str_builder sb_create(int capacity) {
  return (str_builder) {
    .buf = calloc(capacity, sizeof(char)),
    .capacity = capacity,
    .size = 0,
  };
}

void sb_free(str_builder *self) {
  free(self->buf);
}

static void _cfmt_sb_append(void *self, const char *s) {
  str_builder *sb = self;
  const int s_len = strlen(s);
  // make sure to leave space for null terminator
  const int end_size = sb->size + s_len + 1;
  if (end_size > sb->capacity) {
    int new_capacity = sb->capacity;
    while (end_size > new_capacity)
      new_capacity = new_capacity * 3 / 2;
    char *new_buf = realloc(sb->buf, new_capacity * sizeof(char));
    if (!new_buf) {
      free(sb->buf);
      fprint(stderr, "failed to grow to %d bytes", end_size);
      exit(1);
    }
    sb->buf = new_buf;
    sb->capacity = new_capacity;
  }
  // include null terminator
  memcpy(sb->buf + sb->size, s, s_len + 1);
  sb->size += s_len;
}

void sb_append(str_builder *self, const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  vemitf((sink) { .self = self, .emit = _cfmt_sb_append }, fmt, ap);
}

void sb_truncate(str_builder *sb, int to) {
  if (to > sb->size) return;
  sb->buf[sb->size = to] = '\0';
}


formatter FORMATTERS = { .next = &_cfmt_sv_formatter };

#undef CFMT_IMPL

#endif
