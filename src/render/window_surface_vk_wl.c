#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_wayland.h>
#include "window_surface_vk.h"

void create_wl_surface(VkInstance instance, VkAllocationCallbacks* alloc, VkSurfaceKHR* surface, void *wl_surface, void *wl_display) {
VkWaylandSurfaceCreateInfoKHR ci = {
      .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
      .display = wl_display,
      .surface = wl_surface,
  };
  *surface = VK_NULL_HANDLE;
  vkCreateWaylandSurfaceKHR(instance, &ci, alloc, surface);
};
