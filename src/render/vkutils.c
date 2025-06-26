#pragma once
#include "vkutils.h"

#include <stdint.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "base.h"
#include "render/rtypes.h"

// todo: move in platform specific
VkBool32 check_presentation_support_xcb(VkPhysicalDevice d, uint32_t family) {
    unused(d);
    unused(family);
    return VK_FALSE;
}

VkBool32 check_presentation_support_win32(VkPhysicalDevice d, uint32_t family) {
    unused(d);
    unused(family);
    return VK_FALSE;
}

VkBool32 check_extension_support(VkPhysicalDevice d, const char** extensions,
                                 uint32_t count) {
    uint32_t ext_count = 0;
    vkEnumerateDeviceExtensionProperties(d, NULL, &ext_count, NULL);
    VkExtensionProperties props[ext_count];

    vkEnumerateDeviceExtensionProperties(d, NULL, &ext_count, props);
    for (uint32_t i = 0; i < count; i++) {
        uint32_t found = 0;
        for (uint32_t j = 0; j < ext_count; j++) {
            if (!strcmp(extensions[i], props[j].extensionName)) {
                found = 1;
                break;
            }
        }
        if (!found) {
            return VK_FALSE;
        }
    }

    return VK_TRUE;
}

queue_families find_queue_families(VkPhysicalDevice d, rdev_wnd window_api) {
    queue_families qf = {
        .graphics = UINT32_MAX,
        .compute = UINT32_MAX,
        .transfer = UINT32_MAX,
    };

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(d, &queue_family_count, NULL);
    VkQueueFamilyProperties props[queue_family_count];
    vkGetPhysicalDeviceQueueFamilyProperties(d, &queue_family_count, props);

    for (uint32_t i = 0; i < queue_family_count; i++) {
        VkQueueFlags flags = props[i].queueFlags;

        if ((flags & VK_QUEUE_GRAPHICS_BIT) && qf.graphics == UINT32_MAX) {
            VkBool32 supports_present = VK_FALSE;
            switch (window_api) {
                case RDEV_WND_WL: {
                    supports_present = check_presentation_support_wl(d, i);
                    break;
                }
                case RDEV_WND_XCB: {
                    supports_present = check_presentation_support_xcb(d, i);
                    break;
                }
                case RDEV_WND_WIN32: {
                    supports_present = check_presentation_support_win32(d, i);
                    break;
                }
            }
            if (supports_present) {
                qf.graphics = i;
            }
        }
        // dedicated compute?
        if ((flags & VK_QUEUE_COMPUTE_BIT) && qf.compute == UINT32_MAX &&
            !(flags & VK_QUEUE_GRAPHICS_BIT)) {
            qf.compute = i;
        }
        // dedicated transfer?
        if ((flags & VK_QUEUE_TRANSFER_BIT) && qf.transfer == UINT32_MAX &&
            !(flags & VK_QUEUE_GRAPHICS_BIT) && !(flags & VK_QUEUE_COMPUTE_BIT)) {
            qf.transfer = i;
        }

        if (qf.graphics != UINT32_MAX && qf.compute != UINT32_MAX &&
            qf.transfer != UINT32_MAX) {
            break;
        }
    }

    // fallback
    if (qf.compute == UINT32_MAX || qf.transfer == UINT32_MAX) {
        for (uint32_t i = 0; i < queue_family_count; i++) {
            VkQueueFlags flags = props[i].queueFlags;
            if ((flags & VK_QUEUE_COMPUTE_BIT) && qf.compute == UINT32_MAX) {
                qf.compute = i;
            }
            if ((flags & VK_QUEUE_TRANSFER_BIT) && qf.transfer == UINT32_MAX) {
                qf.transfer = i;
            }
            if (qf.transfer != UINT32_MAX && qf.compute != UINT32_MAX) {
                break;
            }
        }
    }
    return qf;
}

int32_t rate_device(device_info* info) {
    if (info->families.graphics == UINT32_MAX || !info->extension_support) {
        return -1;
    }
    uint32_t score = 0;
    switch (info->properties.deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: {
            score += 100;
            break;
        }
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: {
            score += 50;
            break;
        }
        case VK_PHYSICAL_DEVICE_TYPE_CPU: {
            score += 10;
            break;
        }
        // don't care
        default:
            break;
    }
    if (info->families.graphics != info->families.compute &&
        info->families.compute != UINT32_MAX) {
        score += 20;
    }
    if (info->families.graphics != info->families.transfer &&
        info->families.transfer != UINT32_MAX) {
        score += 10;
    }
    if (info->families.compute != info->families.transfer &&
        info->families.compute != UINT32_MAX &&
        info->families.transfer != UINT32_MAX) {
        score += 5;
    }
    for (uint32_t i = 0; i < info->mem_props.memoryHeapCount; i++) {
        if (info->mem_props.memoryHeaps[i].flags &
            VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
            size_t vram_mb = info->mem_props.memoryHeaps[i].size / (1024 * 1024);
            score += vram_mb;
            break;
        }
    }
    return score;
}

VkSurfaceFormatKHR find_surface_format(VkPhysicalDevice d, VkSurfaceKHR surf) {
    const VkFormat desired_fmts[] = {
        VK_FORMAT_B8G8R8A8_UNORM,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_B8G8R8_UNORM,
        VK_FORMAT_R8G8B8_UNORM,
    };
    const VkColorSpaceKHR color_space = VK_COLORSPACE_SRGB_NONLINEAR_KHR;

    uint32_t count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(d, surf, &count, NULL);
    VkSurfaceFormatKHR formats[count];
    vkGetPhysicalDeviceSurfaceFormatsKHR(d, surf, &count, formats);

    const uint32_t desired_count = sizeof(desired_fmts) / sizeof(*desired_fmts);

    for (size_t i = 0; i < desired_count; i++) {
        for (size_t j = 0; j < count; j++) {
            if (formats[j].format == desired_fmts[i] &&
                formats[j].colorSpace == color_space) {
                debug_log("Found image format: %d\n", formats[j].format);
                return formats[j];
            }
        }
    }
    debug_log("Desired image format not found, using available!\n");
    return formats[0];
}

VkFormat find_depth_format(VkPhysicalDevice d) {
    VkFormat candidates[] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
    };
    const uint32_t count = sizeof(candidates) / sizeof(candidates[0]);
    for (uint32_t i = 0; i < count; ++i) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(d, candidates[i], &props);
        if (props.optimalTilingFeatures &
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            debug_log("Found depth format: %d\n", candidates[i]);
            return candidates[i];
        }
    }
    return VK_FORMAT_UNDEFINED;
}

VkPresentModeKHR find_present_mode(VkPhysicalDevice d, VkSurfaceKHR surf) {
    uint32_t supported_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(d, surf, &supported_count, NULL);
    VkPresentModeKHR supported[supported_count];
    vkGetPhysicalDeviceSurfacePresentModesKHR(d, surf, &supported_count, supported);
    VkPresentModeKHR requested = VK_PRESENT_MODE_MAILBOX_KHR;
    for (size_t i = 0; i < supported_count; i++) {
        if (requested == supported[i]) {
            return supported[i];
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}
