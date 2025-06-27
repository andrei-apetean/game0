#pragma once
#include <vulkan/vulkan.h>

#include "render/rtypes.h"

typedef struct {
    uint32_t graphics;
    uint32_t compute;
    uint32_t transfer;
} queue_families;

typedef struct {
    VkPhysicalDevice                 device;
    VkPhysicalDeviceProperties       properties;
    VkPhysicalDeviceMemoryProperties mem_props;
    VkPhysicalDeviceFeatures         features;
    queue_families                   families;
    VkBool32                         extension_support;
    uint32_t                         score;
} device_info;

// todo: maybe chuck this all under one unified api
VkBool32 vkutil_check_presentation_support_wl(VkPhysicalDevice d, uint32_t family);
VkBool32 vkutil_check_presentation_support_xcb(VkPhysicalDevice d, uint32_t family);
VkBool32 vkutil_check_presentation_support_win32(VkPhysicalDevice d,
                                                 uint32_t         family);

VkBool32 vkutil_check_extension_support(VkPhysicalDevice d, const char** extensions,
                                        uint32_t count);

int32_t vkutil_rate_device(device_info* info);

queue_families vkutil_find_queue_families(VkPhysicalDevice d, rdev_wnd window_api);

VkSurfaceFormatKHR vkutil_find_surface_format(VkPhysicalDevice d,
                                              VkSurfaceKHR     surf);

VkFormat vkutil_find_depth_format(VkPhysicalDevice d);

VkPresentModeKHR vkutil_find_present_mode(VkPhysicalDevice d, VkSurfaceKHR surf);

int32_t vkutil_find_memory_type(VkPhysicalDevice d, uint32_t bits, uint32_t flags);
