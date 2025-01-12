#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#define OS_WINDOWS
#elif defined(__linux__) || defined(__gnu_linux__)
#define OS_LINUX
#endif

