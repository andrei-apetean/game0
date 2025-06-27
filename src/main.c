#include <assert.h>
#include <stdalign.h>
#include <stdio.h>

#include "base.h"
#include "core/wnd.h"
#include "render/rdev.h"
#include "render/rtypes.h"
#include "time_util.h"

static uint32_t is_running = 0;
static uint32_t needs_resize = 0;
static uint32_t fps_frame_count;
static uint32_t window_width = 1920;
static uint32_t window_height = 720;

void on_window_close(void* user) {
    unused(user);
    is_running = 0;
}

void on_window_resize(int32_t w, int32_t h, void* user) {
    unused(user);
    needs_resize = 1;
    window_width = w;
    window_height = h;
}

int main(void) {
    debug_log("Initializing...\n");

    wnd_init();
    time_init();
    uint32_t window_api = wnd_backend_id();
    rdev_params rparams = {
        .wnd_api = window_api,
        .allocator = 0,
        .scratch_allocator = 0,
    };
    rdev_init(&rparams);

    wnd_open("game0", window_width, window_height);
    wnd_dispatcher wnd_disp = {
        .on_window_close = on_window_close,
        .on_window_size = on_window_resize,
    };
    wnd_attach_dispatcher(&wnd_disp);
    void*    wnd_native = wnd_native_handle();
    wnd_get_size(&window_width, &window_height);
    rdev_create_swapchain(wnd_native, window_width, window_height);
    time_p last_time;

    double elapsed;
    is_running = 1;
    needs_resize = 0;
    rpass_id swapchain_pass = rdev_swapchain_renderpass();
    while(is_running) {
        wnd_dispatch_events();

        if(needs_resize) {
            rdev_resize_swapchain(window_width, window_height);
            needs_resize = 0;
        }

        rcmd* cmd = rdev_begin();
        rcmd_begin_pass(cmd, swapchain_pass);

        rcmd_end_pass(cmd, swapchain_pass);
        rdev_end(cmd);
        
        time_p now = time_now();
        fps_frame_count++;

        elapsed = time_diff_sec(last_time, now);
        if (elapsed >= 1.0) {
            char title[128] = {0};
            sprintf(title, "game_0 - %d fps", fps_frame_count);
            wnd_set_title(title);
            fps_frame_count = 0;
            last_time = now;
        }
    }
    rdev_destroy_swapchain();
    rdev_terminate();
    wnd_terminate();
    debug_log("Terminated successfully!\n");
 return 0;   
}
#include "base.c"
#include "mathf.c"
#include "render/rdev.c"
#include "render/rdev_vulkan.c"
#include "render/rdev_vulkan_wl.c"
#include "render/vkutils.c"
#include "core/mem.c"
#include "core/wnd.c"
#include "core/wnd_wl.c"
#include "time_posix.c"

