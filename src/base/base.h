#ifndef NBASE_INCLUDED
#define NBASE_INCLUDED

#include <stdint.h>

#define Q(X) #X
#define QUOTE(X) Q(X)

#ifndef NBASE_WINDOW_NAME
#define NBASE_WINDOW_NAME Our Engine
#endif

#define NBASE_WINDOW_NAME_QUOTED QUOTE(NBASE_WINDOW_NAME)

// This only works on windows
#pragma comment(lib, "Advapi32")
#pragma comment(lib, "Msvcrt")
#pragma comment(lib, "shell32")
#pragma comment(lib, "opengl32")
#pragma comment(lib, "glu32")
#pragma comment(lib, "gdi32")
#pragma comment(lib, "user32")

#define MIN(A, B) (((A) < (B)) ? (A) : (B))
#define MAX(A, B) (((A) > (B)) ? (A) : (B))

// TYPES

typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;
typedef int8_t S8;
typedef int16_t S16;
typedef int32_t S32;
typedef int64_t S64;
typedef float F32;
typedef double F64;

// ASSERTION + EXCEPTION

#define ASSERT(E)                                                              \
  ((void)((E) ||                                                               \
          (NBASE_Error(__FILE__, __LINE__, "Assertion failed: %s", #E), 0)))
#define ASSERT_WITH_MESSAGE(E, M, ...)                                         \
  ((void)((E) || (NBASE_Error(__FILE__, __LINE__, "Assertion failed: %s\n" M,  \
                              #E, __VA_ARGS__),                                \
                  0)))
void NBASE_Error(const char *file, S32 line, const char *format, ...);

void NBASE_OSWindow_startup(S32 width, S32 height);
void NBASE_OSWindow_handleEvents();
void NBASE_OSWindow_swapBuffers();

#endif
