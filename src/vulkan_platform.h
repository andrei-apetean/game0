#pragma once

#include "platform.h"
#include <vulkan/vulkan_core.h>
bool create_vk_surface(win_data_t window,VkInstance inst, VkAllocationCallbacks* alloc, VkSurfaceKHR* surface);

