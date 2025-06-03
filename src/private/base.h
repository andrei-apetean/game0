#pragma once
#include <stdint.h>
#include <stddef.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#define OS_WINDOWS
#elif defined(__linux__) || defined(__gnu_linux__)
#define OS_LINUX
#endif

#define UNUSED(x) ((void)(x))

#if _DEBUG
#include <assert.h>
#define ASSERT(expr) assert(expr)
#else
#define ASSERT(expr) UNUSED(0)
#endif

#if _DEBUG
#define UNREACHABLE() __builtin_unreachable()
#else
#define UNREACHABLE() UNUSED(0)
#endif

#define ARR_SIZE(T) sizeof(T)/sizeof(T[0])

#define STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)

#define ARRAY_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

