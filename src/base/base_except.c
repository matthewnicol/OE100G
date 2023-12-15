#include "base.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void NBASE_Error(const char *file, S32 line, const char *format, ...) {
  fflush(stdout);
  fprintf(stderr, "Program Terminated at %s:%d\n", file, line);
  if (format) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
  }
  fflush(stderr);
  abort();
}
