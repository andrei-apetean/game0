#include "render_module/render_surface_vk.h"
// todo processor guards
#include <stdio.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_wayland.h>

typedef struct {
    struct wl_display* display;
    struct wl_surface* surface;
} window_handle_wl;

void create_vksurface_wl(VkInstance instance, VkAllocationCallbacks* alloc,
                         VkSurfaceKHR* surface, void* window_handle) {
    window_handle_wl* handle = (window_handle_wl*)window_handle;

    VkWaylandSurfaceCreateInfoKHR ci = {
        .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
        .display = handle->display,
        .surface = handle->surface,
    };
    *surface = VK_NULL_HANDLE;
    VkResult result = vkCreateWaylandSurfaceKHR(instance, &ci, alloc, surface);

    if (result == VK_SUCCESS) {
        printf("VkWaylandSurface created!\n");
    } else {
        printf("VkWaylandSurface creation failed with code %d!\n", result);
    }
}
