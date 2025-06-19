#pragma once

#include <stdint.h>
#include <vulkan/vulkan.h>

void check(VkResult result, char* msg, char* file, int32_t line);

// typedef struct {
//     VkSurfaceKHR surface;
//     // Optional: store window or platform info here
// } render_surface_vk;

typedef struct {
    VkPhysicalDevice gpu;
    VkDevice         handle;
    VkQueue          gfx_queue;
    uint32_t         queue_family_idx;
} vk_device;

typedef struct {
    VkSwapchainKHR swapchain;
    VkFormat       format;
    VkExtent2D     extent;
    // images, image views, etc.
} vk_swapchain;

typedef struct {
    VkInstance             instance;
    VkAllocationCallbacks* allocator;
    VkSurfaceKHR           surface;
    vk_device       device;
    vk_swapchain    swapchain;
#if _DEBUG
    VkDebugUtilsMessengerEXT dbg_msgr;
#endif
} vk_context;
