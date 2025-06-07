#pragma once

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
