#pragma once
#include "render_module/vulkan_types.h"

int32_t vulkan_swapchain_create(vulkan_device* device, vulkan_swapchain* swapchain,
                                VkSurfaceKHR surface, uint32_t width,
                                uint32_t height);

void vulkan_swapchain_destroy(vulkan_device* device, vulkan_swapchain* swapchain);
