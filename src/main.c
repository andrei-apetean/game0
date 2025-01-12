
#include "platform.h"
#include "render.h"

int main() {
    if (platform_startup() != true) {
        printf("Error executing platform_startup!\n");
        return -1;
    }
    if (platform_open_window() != true) {
        printf("Error executing platform_open_window!\n");
        return -1;
    }
    win_data_t win = platform_get_window_data();

    if (render_startup(win) != true) {
        printf("Error executing platform_render_startup!\n");
        return -1;
    }
    uint32_t iterations = 0;
    while (platform_is_running()) {
        render_paket_t paket = {0};
        platform_get_width_height(&paket.width, &paket.height);
        bool result = render_update(&paket);
        if (!result) {
            break;
        }
        platform_update(paket.commit_surface);
        iterations++;
    }
    render_shutdown();
    platform_shutdown();

    return 0;
}

#include "platform_wayland.c"
#include "render_vulkan.c"
#include "vulkan_platform_wl.c"
