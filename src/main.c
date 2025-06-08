#include <stdio.h>
#include <stdlib.h>
#include "core.h"
#include "modules/window_module.h"
#include "modules/render_module.h"

static uint32_t is_running;
static uint32_t needs_resize;
static int32_t width;
static int32_t height;

void on_window_close(void* user) {
    is_running = 0;
}

void on_window_resize(int32_t w, int32_t h, void* user) {
    needs_resize = 1;    
    width = w;
    height = h;
}

int main() {
    module_info modules[] = {
        register_window_module(),
        register_render_module(),
    };

    uint32_t module_count = sizeof(modules)/sizeof(modules[0]);

    if (core_load_modules(modules, module_count) != 0) {
        return -1;
    }

    // uint32_t    window_width = 720;
    // uint32_t    window_height = 1080;
    // const char* window_title = "game_0";

    // if (window_create(window_width, window_height, window_title) != 0) {
    //     return -1;
    // }
    window_set_close_handler(on_window_close, NULL);
    window_set_size_handler(on_window_resize, NULL);
    is_running = 1;
    while (is_running) {
        if (needs_resize) {
            render_resize(width, height);
            needs_resize = 0;
        }
        render_begin();
        window_poll_events();
        render_end();
    }

    window_destroy();
    core_teardown();
    printf("Goodbye!\n");
    return 0;
}

// todo; temp
#include "core.c"
#include "window_module/window_module_backend_wl.c"
#include "window_module/window_module.c"

#include "render_module/render_module_backend_vk.c"
#include "render_module/render_module.c"
