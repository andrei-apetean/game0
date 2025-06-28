#pragma once

#include <stdint.h>

#if defined(_WIN32) || defined(_WIN64)
    #define GAME0_WIN32 1
#else
    #define GAME0_WIN32 0
#endif

#if defined(__linux__)
    #define GAME0_LINUX 1
#else
    #define GAME0_LINUX 0
#endif

#define mkilo(bytes) bytes * 1024
#define mmega(bytes) bytes * 1024 * 1024
#define mgiga(bytes) bytes * 1024 * 1024 * 1024

#define unused(x) (void)(x)

void debug_log(const char* fmt, ...);
void debug_abort(void);

// todo; multi compiler support
#ifdef _DEBUG
#define likely(expr) __builtin_expect(expr, 1)
#define unlikely(expr) __builtin_expect(expr, 0)
#define debug_assert(expr) {if (unlikely(!(expr))) { debug_log("Assertion failed :`" #expr"` in "__FILE__":%d\n", __LINE__); debug_abort();} }
#define unimplemented(expr) { debug_log("`"#expr"` not implemented at:"__FILE__",%d!\n", __LINE__); debug_abort();}
#else
#define debug_assert(expr) expr
#define unimplemented(expre) expr
#endif // _DEBUG
