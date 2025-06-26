#include <wayland-client-core.h>
#include "base.h"
#include "rdev_vulkan.h"

#ifdef GAME0_LINUX

#include <vulkan/vulkan_wayland.h>

typedef struct {
    struct wl_display* display;
    struct wl_surface* surface;
} window_handle_wl;

VkResult rdev_vulkan_create_surface_wl(rdev_vulkan* rdev_v, void* wnd_native) {
    window_handle_wl* handle = (window_handle_wl*)wnd_native;
    VkWaylandSurfaceCreateInfoKHR ci = {
        .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
        .display = handle->display,
        .surface = handle->surface,
    };
    VkResult result = vkCreateWaylandSurfaceKHR(
        rdev_v->instance, &ci, rdev_v->allocator, &rdev_v->surface);
    VCHECK(result);
    return result;
}

VkBool32 check_presentation_support_wl(VkPhysicalDevice d, uint32_t family) {
    struct wl_display* display = wl_display_connect(NULL);
    if (!display) return VK_FALSE;

    VkBool32 result = vkGetPhysicalDeviceWaylandPresentationSupportKHR(d, family, display);
    wl_display_disconnect(display);
    return result;
}
#else

VkBool32 check_presentation_support_wl(VkPhysicalDevice d, uint32_t family) {
    return VK_FALSE;
}
void rdev_vulkan_create_surface_wl(rdev_vulkan* rdev_v, void* wnd_native) {}
#endif  // GAME0_LINUX
