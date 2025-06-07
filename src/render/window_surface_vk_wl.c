#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_wayland.h>

#include "window_surface_vk.h"

struct win_handle {
    struct wl_display* display;
    struct wl_surface* surface;
};

void create_wl_surface(VkInstance instance, VkAllocationCallbacks* alloc,
                       VkSurfaceKHR* surface, void* win_handle) {
    struct win_handle* win = (struct win_handle*)win_handle;

    VkWaylandSurfaceCreateInfoKHR ci = {
        .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
        .display = win->display,
        .surface = win->surface,
    };
    *surface = VK_NULL_HANDLE;
    vkCreateWaylandSurfaceKHR(instance, &ci, alloc, surface);
};
