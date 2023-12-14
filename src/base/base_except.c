#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include "base.h"

// These definitions are defined in base.h
// #define ASSERT(E) ((void)((E) || (NBASE_Error(__FILE__, __LINE__, "Assertion failed: %s", #E), 0)))
// #define ASSERT_WITH_MESSAGE(E, M, ...) ((void)((E) || (NBASE_Error(__FILE__, __LINE__, "Assertion failed: %s\n" M, #E, __VA_ARGS__), 0)))

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