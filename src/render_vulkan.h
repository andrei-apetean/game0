#pragma once

#include "common.h"
#include "platform.h"

typedef struct render_paket_t {
    uint32_t width;
    uint32_t height;
    bool commit_surface;
} render_paket_t;

bool render_startup(win_data_t window);
bool render_update(render_paket_t* paket);
bool render_shutdown();
