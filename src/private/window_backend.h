#pragma once

#include <stdint.h>

#include "engine.h"
#include "private/input.h"

typedef struct {
    void*    win_handle;
    void*    win_instance;
    uint32_t win_api;
}  window_info;

typedef struct {
    uint32_t    width;
    uint32_t    height;
    const char* title;
}  window_cfg;

typedef struct window_backend window_backend;

int32_t window_backend_get_size();
int32_t window_backend_startup(window_backend** backend, window_cfg cfg);
int32_t window_backend_create_window(window_backend* backend);
void    window_backend_poll_events(window_backend* backend);
void    window_backend_destroy_window(window_backend* backend);
void    window_backend_teardown(window_backend* backend);
void    window_backend_attach_handler(window_backend* backend, pfn_on_input_event on_event);
vec2 window_backend_window_size(window_backend* backend);

window_info window_backend_get_info(window_backend* backend);
