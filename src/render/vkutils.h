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
VkBool32 vutl_present_supported_wl(VkPhysicalDevice d, uint32_t family);
VkBool32 vutl_present_supported_xcb(VkPhysicalDevice d, uint32_t family);
VkBool32 vutl_present_supported_win32(VkPhysicalDevice d, uint32_t family);

VkBool32 vutl_extensions_supported(VkPhysicalDevice d, const char** extensions,
                                   uint32_t count);

int32_t vutl_rate_device(device_info* info);

queue_families vutl_find_queue_families(VkPhysicalDevice d, rdev_wnd window_api);

VkSurfaceFormatKHR vutl_find_surface_format(VkPhysicalDevice d, VkSurfaceKHR surf);

VkFormat vutl_find_depth_format(VkPhysicalDevice d);

VkPresentModeKHR vutl_find_present_mode(VkPhysicalDevice d, VkSurfaceKHR surf);

int32_t vutl_find_memory_type(VkPhysicalDevice d, uint32_t bits, uint32_t flags);

VkShaderStageFlagBits vutl_to_vulkan_shader_stage(rshader_type stage);
VkShaderStageFlags    vutl_to_vulkan_shader_stage_flags(rshader_stage_flags flags);

VkFormat vutl_to_vulkan_format(rvertex_format fmt);
