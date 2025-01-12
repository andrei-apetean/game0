#include <vulkan/vulkan_core.h>
#include "vulkan_platform.h"
#include "vulkan/vulkan_wayland.h"

bool create_vk_surface(win_data_t window,VkInstance inst, VkAllocationCallbacks* alloc, VkSurfaceKHR* surface) {
    if (window.api != window_api_wayland) {
        printf(
            "vulkan_platform_wl.c:create_vk_surface "
            "cannot create vk surface "
            "for api: %d\n",
            window.api);
        return false;
    }
    VkWaylandSurfaceCreateInfoKHR ci_surf = {
        .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
        .display = window.connection,
        .surface = window.handle,
    };
    VkResult result = vkCreateWaylandSurfaceKHR(inst, &ci_surf, alloc, surface);
    if (result != VK_SUCCESS) {
        printf("vkCreateWaylandSurfaceKHR failed!\n");
        return false;
    }
    return true;
}
