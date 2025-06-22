#include "render_module/vkcore.h"
#include "render_module/vulkan_types.h"
// todo processor guards
#include <stdio.h>
#include <vulkan/vulkan_wayland.h>

typedef struct {
    struct wl_display* display;
    struct wl_surface* surface;
} window_handle_wl;

int32_t vkcore_create_surface_wl(vulkan_core* core, void* win) {
    window_handle_wl* handle = (window_handle_wl*)win;

    VkWaylandSurfaceCreateInfoKHR ci = {
        .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
        .display = handle->display,
        .surface = handle->surface,
    };
    VK_CHECK(
        vkCreateWaylandSurfaceKHR(core->instance, &ci, core->allocator, &core->surface));
    printf("VkWaylandSurface created successfully\n");
    return 0;
}
