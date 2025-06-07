#pragma once

#include <vulkan/vulkan_core.h>

void create_wl_surface(VkInstance instance, VkAllocationCallbacks* alloc,
                       VkSurfaceKHR* surface, void* win_handle);
