#pragma once

#include <stddef.h>

#include "stdbool.h"

typedef struct platform_state platform_state;
typedef struct win_surface    win_surface;

size_t platform_get_size();
bool   platform_startup(platform_state*);
bool   platform_shutdown(platform_state*);
bool   platform_update(platform_state*);
bool   platform_is_running(platform_state*);
bool   platform_poll_events(platform_state*);
bool   platform_get_window_surface(platform_state*, win_surface*);

