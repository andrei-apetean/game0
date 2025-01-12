#pragma once

#include "common.h"

typedef enum window_api {
    window_api_wayland = 0,
    window_api_xcb,
    window_api_win32,
    window_api_count,
} window_api;

typedef struct win_data_t {
    void*      handle;
    void*      connection;
    window_api api;
} win_data_t;

bool  platform_startup();
bool  platform_open_window();
bool  platform_shutdown();
bool  platform_update(bool commit_surface);
bool  platform_is_running();
void  platform_get_width_height(uint32_t* width, uint32_t* height);
win_data_t platform_get_window_data();
