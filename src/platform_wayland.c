#include <string.h>
#include <wayland-client-core.h>
#include <wayland-client.h>  //libwayland-protocols
#include <xdg-shell.h>       //generated by wayland-scanner

#include <xdg-shell.c>  //todo: build and link

#include "platform.h"

typedef struct platform_wayland_t {
    struct wl_display   *display;
    struct wl_registry*   registry;
    struct wl_compositor* compositor;
    struct wl_surface*    surface;
    struct xdg_wm_base*   shell;
    struct xdg_surface*   xdg_surface;
    struct xdg_toplevel*  toplevel;
    uint32_t              width;
    uint32_t              height;
    bool                  quit;
} platform_wayland_t;

static platform_wayland_t platform = {0};

static void handle_shell_configure(void* data, struct xdg_surface* shell_surface,
                                   uint32_t serial) {
    xdg_surface_ack_configure(shell_surface, serial);
    //    if (resize)
    //    {
    //        readyToResize = 1;
    //    }
}

static const struct xdg_surface_listener shell_surface_listener = {
    .configure = handle_shell_configure,
};

static void handle_shell_ping(void* data, struct xdg_wm_base* xdg_wm_base,
                              uint32_t serial) {
    xdg_wm_base_pong(xdg_wm_base, serial);
}

static struct xdg_wm_base_listener shell_listener = {
    .ping = handle_shell_ping,
};

static void handle_registry(void* data, struct wl_registry* registry,
                            uint32_t name, const char* interface,
                            uint32_t version) {
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        platform.compositor = wl_registry_bind(platform.registry, name,
                                               &wl_compositor_interface, 1);
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        platform.shell =
            wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(platform.shell, &shell_listener, NULL);
    }
}

static const struct wl_registry_listener registry_listener = {
    .global = handle_registry,
};

static void handle_toplevel_configure(void* data, struct xdg_toplevel* toplevel,
                                      int32_t width, int32_t height,
                                      struct wl_array* states) {
    if (width != 0 && height != 0) {
        platform.width = width;
        platform.height = height;
    }
}

static void handle_toplevel_close(void* data, struct xdg_toplevel* toplevel) {
    platform.quit = true;
}

static const struct xdg_toplevel_listener toplevel_listener = {
    .configure = handle_toplevel_configure,
    .close = handle_toplevel_close,
};

bool platform_startup() {
    printf("platform_startup...\n");
    platform.display = wl_display_connect(NULL);
    if (platform.display == NULL) {
        printf("Failed to connect wl_display\n");
        return false;
    }

    platform.registry = wl_display_get_registry(platform.display);
    if (platform.registry == NULL) {
        printf("Failed to get wl_display registry\n");
        return false;
    }

    wl_registry_add_listener(platform.registry, &registry_listener, NULL);
    wl_display_roundtrip(platform.display);
    printf("platform_startup successful!\n");
    return true;
}

bool platform_update(bool commit_surface) {
    if (commit_surface) {
        wl_surface_commit(platform.surface);
    }

    wl_display_roundtrip(platform.display);
    return true;
}

bool platform_is_running() { return !platform.quit; }

bool platform_open_window() {
    if (platform.compositor == NULL) {
        printf("Cannot open window - compositor is not initialized!");
        return false;
    }
    platform.surface = wl_compositor_create_surface(platform.compositor);
    if (platform.surface == NULL) {
        printf("Failed to create surface\n");
        return false;
    }

    platform.xdg_surface =
        xdg_wm_base_get_xdg_surface(platform.shell, platform.surface);
    if (platform.xdg_surface == NULL) {
        printf("Failed to get xdg_surface\n");
        return false;
    }
    xdg_surface_add_listener(platform.xdg_surface, &shell_surface_listener, NULL);

    platform.toplevel = xdg_surface_get_toplevel(platform.xdg_surface);
    if (platform.toplevel == NULL) {
        printf("Failed to get xdg_toplevel\n");
        return false;
    }
    xdg_toplevel_add_listener(platform.toplevel, &toplevel_listener, NULL);

    xdg_toplevel_set_title(platform.toplevel, "game0");

    // Commit the surface to display
    wl_surface_commit(platform.surface);
    wl_display_roundtrip(platform.display);
    wl_surface_commit(platform.surface);

    printf("platform_open_window successful!\n");
    return true;
}

void platform_get_width_height(uint32_t* width, uint32_t* height) {
    *width = platform.width;
    *height = platform.height;
}

win_data_t platform_get_window_data() {
    win_data_t window = {
        .handle = platform.surface,
        .connection = platform.display,
        .api = window_api_wayland,
    };
    return window;
}

bool platform_shutdown() {
    xdg_toplevel_destroy(platform.toplevel);
    xdg_surface_destroy(platform.xdg_surface);
    wl_surface_destroy(platform.surface);
    xdg_wm_base_destroy(platform.shell);
    wl_compositor_destroy(platform.compositor);
    wl_registry_destroy(platform.registry);
    wl_display_disconnect(platform.display);

    printf("platform_shutdown successful!\n");
    return true;
}
