#include "lc.h"

#include <stdarg.h>

static void vdebugf(const char *fmt, va_list ap) { vfprintf(stderr, fmt, ap); }

void debugf(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  vdebugf(fmt, ap);
}

[[noreturn]]
static void _vfailf(const char *file, const int line,
                    const char *fmt, va_list ap) {
  debugf("%s:%d: ", file, line);
  vdebugf(fmt, ap); debugf("\n");
  exit(1);
}

void _failf(const char *file, const int line,
            const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  _vfailf(file, line, fmt, ap);
}

void errorf(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  vdebugf(fmt, ap); debugf("\n");
  exit(1);
}
