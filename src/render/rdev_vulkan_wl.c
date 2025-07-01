#include "base.h"
#include "rdev_vulkan.h"

#if defined(OS_LINUX)
#include <wayland-client-core.h>
#include <vulkan/vulkan_wayland.h>

typedef struct {
    struct wl_display* display;
    struct wl_surface* surface;
} native_handle_wl;

VkResult vcreate_surface_wl(vstate* v, void* wnd_native) {
    native_handle_wl* handle = (native_handle_wl*)wnd_native;
    VkWaylandSurfaceCreateInfoKHR ci = {
        .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
        .display = handle->display,
        .surface = handle->surface,
    };
    VkResult result = vkCreateWaylandSurfaceKHR(
        v->instance, &ci, v->allocator, &v->surface);
    VCHECK(result);
    return result;
}

VkBool32 vutl_present_supported_wl(VkPhysicalDevice d, uint32_t family) {
    struct wl_display* display = wl_display_connect(NULL);
    if (!display) return VK_FALSE;

    VkBool32 result = vkGetPhysicalDeviceWaylandPresentationSupportKHR(d, family, display);
    wl_display_disconnect(display);
    return result;
}
#else

VkBool32 vutl_present_supported_wl(VkPhysicalDevice d, uint32_t family) {
    unused(d);
    unused(family);
    return VK_FALSE;
}
VkResult vcreate_surface_wl(vstate* v, void* wnd_native) {
    unused(v);
    unused(wnd_native);
    return VK_ERROR_UNKNOWN;
}
#endif  // OF_LINUX
