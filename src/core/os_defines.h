#pragma once

#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)
#define OS_WIN32
#define OS_PATH_SEP "\\"
#define OS_EXE_EXT ".exe"
#define OS_SO_EXT ".dll"
#elif defined(__linux__)
#define OS_LINUX
#define OS_PATH_SEP "/"
#define OS_EXE_EXT ""
#define OS_SO_EXT ".so"
#else
#error "unsupported platform"
#endif

#define OS_MAX_PATH 256

