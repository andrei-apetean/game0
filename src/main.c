#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <wayland-client-core.h>

#include "game0.h"
#include "private/base.h"

typedef struct wl_display* (*pfn_wl_display_connect)(const char*);

int main() {
    printf("Hello world!\n");
    struct config cfg = {
        .window_height = 900,
        .window_width = 1080,
        .window_title = "game0",
    };
    if (startup(cfg) != 0) return -1;

    while (is_running()) {
        poll_events();

        render_begin();
        render_end();
    }

    teardown();
    printf("Goodbye!\n");
    return 0;
}

#include "core_state.c"
#include "render/rndr_vulkan.c"
#ifdef OS_LINUX
#include "os/win_wayland.c"
#include "render/vulkan_surface_wayland.c"
#include "system_posix.c"
#else
#error "Unsupported backend!"
#endif  // OS_LINUX
