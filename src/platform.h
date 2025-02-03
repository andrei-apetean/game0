#pragma once

#include <stddef.h>

#include "stdbool.h"

typedef struct platform_state platform_state;
typedef struct win_surface   win_surface;

typedef struct platform_interface {
    bool (*startup)(platform_state*);
    bool (*shutdown)(platform_state*);
    bool (*update)(platform_state*);
    bool (*is_running)(platform_state*);
    bool (*poll_events)(platform_state*);
    bool (*get_window_surface)(platform_state*, win_surface*);
    size_t platform_state_size;
} platform_interface;

bool get_platform_interface(platform_interface*);
