#include "render_module/vulkan_device.h"

#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>

typedef struct {
    VkPhysicalDevice           gpu;
    VkPhysicalDeviceProperties props;
    int32_t                    total_score;
    int32_t                    supports_present;
    int32_t                    gfx_family;
    int32_t                    compute_family;
    int32_t                    transfer_family;
} gpu_info;

static int32_t select_physical_device(vulkan_device* device, VkInstance instance,
                                      VkSurfaceKHR surface);
static int32_t create_logical_device(vulkan_device* device);
static void    get_device_info(VkPhysicalDevice dev, VkSurfaceKHR surface,
                               gpu_info* props);

static int32_t physical_device_supports_present(VkPhysicalDevice device,
                                                VkSurfaceKHR     surface);

int32_t vulkan_device_create(vulkan_device* device, VkInstance instance,
                             VkSurfaceKHR surface) {
    int32_t result = select_physical_device(device, instance, surface);
    if (result) return result;

    result = create_logical_device(device);
    if (result) return result;

    VkCommandPoolCreateInfo pool_ci = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = device->graphics_queue_idx,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    };
    VK_CHECK(vkCreateCommandPool(device->logical_device, &pool_ci,
                                 device->allocator, &device->cmdpool));
    printf("Command pool created!\n");
    return result;
}

void vulkan_device_destroy(vulkan_device* device) {
    vkDestroyCommandPool(device->logical_device, device->cmdpool,
                         device->allocator);
    vkDestroyDevice(device->logical_device, device->allocator);
    device->logical_device = VK_NULL_HANDLE;
    device->physical_device = VK_NULL_HANDLE;
    device->graphics_queue_idx = -1;
    device->compute_queue_idx = -1;
    device->transfer_queue_idx = -1;
    device->graphics_queue = VK_NULL_HANDLE;
    device->compute_queue = VK_NULL_HANDLE;
    device->transfer_queue = VK_NULL_HANDLE;
}

int32_t vulkan_device_find_surface_format(vulkan_device*      device,
                                          VkSurfaceKHR        surface,
                                          VkSurfaceFormatKHR* out_format) {
    VkSurfaceFormatKHR selected_fmt;
    VkPhysicalDevice   dev = device->physical_device;
    const VkFormat     desired_fmts[] = {
        VK_FORMAT_B8G8R8A8_UNORM,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_B8G8R8_UNORM,
        VK_FORMAT_R8G8B8_UNORM,
    };
    const VkColorSpaceKHR color_space = VK_COLORSPACE_SRGB_NONLINEAR_KHR;

    uint32_t count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &count, NULL);
    VkSurfaceFormatKHR* formats = malloc(count * sizeof(VkSurfaceFormatKHR));
    vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &count, formats);

    uint32_t       format_found = 0;
    const uint32_t desired_count = sizeof(desired_fmts) / sizeof(*desired_fmts);

    for (size_t i = 0; i < desired_count; i++) {
        for (size_t j = 0; j < count; j++) {
            if (formats[j].format == desired_fmts[i] &&
                formats[j].colorSpace == color_space) {
                selected_fmt = formats[j];
                format_found = 1;
                printf("Found image format: %d\n", selected_fmt.format);
                break;
            }
        }
        if (format_found) {
            break;
        }
    }
    if (!format_found) {
        printf("Desired image format not found, using available!\n");
        selected_fmt = formats[0];
    }
    *out_format = selected_fmt;
    free(formats);
    return 0;
}

int32_t vulkan_device_find_depth_format(vulkan_device* device,
                                        VkFormat*      out_format) {
    VkFormat candidates[] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
    };
    const uint32_t count = sizeof(candidates) / sizeof(candidates[0]);
    *out_format = VK_FORMAT_UNDEFINED;
    for (int i = 0; i < count; ++i) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(device->physical_device, candidates[i],
                                            &props);
        if (props.optimalTilingFeatures &
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            printf("Found depth format: %d\n", candidates[i]);
            *out_format = candidates[i];
            return 0;
        }
    }
    return -1;
}

int32_t vulkan_device_find_present_mode(vulkan_device* device, VkSurfaceKHR surface,
                                        VkPresentModeKHR* out_mode) {
    VkPresentModeKHR mode;
    uint32_t         supported_count = 0;
    VkPresentModeKHR supported[8];
    vkGetPhysicalDeviceSurfacePresentModesKHR(device->physical_device, surface,
                                              &supported_count, NULL);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device->physical_device, surface,
                                              &supported_count, supported);
    uint32_t         found = 0;
    VkPresentModeKHR requested = VK_PRESENT_MODE_MAILBOX_KHR;
    for (size_t i = 0; i < supported_count; i++) {
        if (requested == supported[i]) {
            found = 1;
            break;
        }
    }
    mode = found ? requested : VK_PRESENT_MODE_FIFO_KHR;
    *out_mode = mode;
    return found ? 0 : -1;
}

int32_t vulkan_device_find_memory_type(vulkan_device* device,
                                       uint32_t       memory_type_bits,
                                       uint32_t       memory_type) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(device->physical_device, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((memory_type_bits & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & memory_type) ==
                memory_type) {
            return i;
        }
    }
    return -1;
}

int32_t vulkan_device_create_shader_module(vulkan_device* device, uint32_t* code,
                                           size_t             code_size,
                                           VkShaderModule*    module) {
    VkShaderModuleCreateInfo ci = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code_size,
        .pCode = code,
        .flags = 0,
    };
    VK_CHECK(vkCreateShaderModule(device->logical_device, &ci, device->allocator,
                                  module));
    return 0;
}

void vulkan_device_destroy_shader_module(vulkan_device* device,
                                         VkShaderModule module) {
    vkDestroyShaderModule(device->logical_device, module, device->allocator);
}

///////////////////////////////////////////////////
//////////           Internal implementation
///////////////////////////////////////////////////
static int32_t select_physical_device(vulkan_device* device, VkInstance instance,
                                      VkSurfaceKHR surface) {
    uint32_t dev_count = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &dev_count, 0));
    if (dev_count == 0) {
        printf("No physical devices found on the system!\n");
        return 1;
    }

    VkPhysicalDevice* devices = malloc(sizeof(VkPhysicalDevice) * dev_count);
    gpu_info*         gpu_infos = malloc(sizeof(gpu_info) * dev_count);
    if (!devices || !gpu_infos) {
        printf("Failed to allocate memory for physical devices!\n");
        return 1;
    }
    printf("Found %d devices on the system\n", dev_count);
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &dev_count, devices));
    VkPhysicalDevice integrated = VK_NULL_HANDLE;
    VkPhysicalDevice dedicated = VK_NULL_HANDLE;
    for (uint32_t i = 0; i < dev_count; i++) {
        VkPhysicalDevice d = devices[i];
        gpu_info*        info = &gpu_infos[i];
        get_device_info(d, surface, info);
        printf("Device %s\n", info->props.deviceName);
        printf("Present | Graphics | Compute | Transfer \n");
        printf("   %d    |    %d     |    %d    |    %d    \n",
               info->supports_present, info->gfx_family, info->compute_family,
               info->transfer_family);

        if (!info->supports_present) continue;
        switch (info->props.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: {
                info->total_score += 10;
            } break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: {
                info->total_score += 5;
            } break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU: {
                info->total_score += 1;
            } break;
            default:
                break;
        }
        if (info->gfx_family != info->compute_family) info->total_score += 3;
        if (info->gfx_family != info->transfer_family) info->total_score += 3;
        if (info->compute_family != info->transfer_family) info->total_score += 3;
    }

    int32_t max_score = 0;
    int32_t selected = -1;
    for (uint32_t i = 0; i < dev_count; i++) {
        if (gpu_infos[i].total_score > max_score) selected = i;
    }
    if (selected == -1) {
        printf("Failed to find a suitable device!\n");
        free(gpu_infos);
        free(devices);
        return -1;
    }
    gpu_info info = gpu_infos[selected];
    device->physical_device = info.gpu;
    device->graphics_queue_idx = info.gfx_family;
    device->compute_queue_idx = info.compute_family;
    device->transfer_queue_idx = info.transfer_family;
    device->props = info.props;
    printf("Selected device: %s\n", device->props.deviceName);
    free(gpu_infos);
    free(devices);
    return 0;
}

static int32_t create_logical_device(vulkan_device* device) {
    const char* dev_exts[] = {
        "VK_KHR_swapchain",
    };
    const float    queue_priority[] = {1.0f};
    const uint32_t queue_count = 3;
    // todo, check if queue indices are shared
    VkDeviceQueueCreateInfo queue_ci[] = {
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = device->graphics_queue_idx,
            .queueCount = 1,
            .pQueuePriorities = queue_priority,
        },
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = device->transfer_queue_idx,
            .queueCount = 1,
            .pQueuePriorities = queue_priority,
        },
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = device->compute_queue_idx,
            .queueCount = 1,
            .pQueuePriorities = queue_priority,
        },
    };

    VkPhysicalDeviceFeatures2 features = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
    };

    vkGetPhysicalDeviceFeatures2(device->physical_device, &features);

    VkDeviceCreateInfo dev_ci = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = queue_count,
        .pQueueCreateInfos = queue_ci,
        .enabledExtensionCount = sizeof(dev_exts) / sizeof(*dev_exts),
        .ppEnabledExtensionNames = dev_exts,
        .pNext = &features,
    };
    VK_CHECK(vkCreateDevice(device->physical_device, &dev_ci, device->allocator,
                            &device->logical_device));
    printf("VkDevice created!\n");
    vkGetDeviceQueue(device->logical_device, device->graphics_queue_idx, 0,
                     &device->graphics_queue);
    vkGetDeviceQueue(device->logical_device, device->compute_queue_idx, 0,
                     &device->compute_queue);
    vkGetDeviceQueue(device->logical_device, device->transfer_queue_idx, 0,
                     &device->transfer_queue);
    printf("Queues obtained!\n");
    return 0;
}

static void get_device_info(VkPhysicalDevice dev, VkSurfaceKHR surface,
                            gpu_info* out_info) {
    out_info->total_score = -1;
    out_info->gpu = dev;
    out_info->gfx_family = -1;
    out_info->compute_family = -1;
    out_info->transfer_family = -1;
    out_info->supports_present = 0;
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &queue_family_count, NULL);
    VkQueueFamilyProperties* fprops =
        malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);

    vkGetPhysicalDeviceQueueFamilyProperties(dev, &queue_family_count, fprops);
    vkGetPhysicalDeviceProperties(dev, &out_info->props);
    for (uint32_t i = 0; i < queue_family_count; ++i) {
        VkBool32 supports_present = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface, &supports_present);

        // Graphics
        if ((fprops[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            if (supports_present) out_info->supports_present = VK_TRUE;
            if (out_info->gfx_family == -1) out_info->gfx_family = i;
        }

        // Dedicated Compute
        if ((fprops[i].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
            !(fprops[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
            out_info->compute_family == -1) {
            out_info->compute_family = i;
        }

        // Dedicated Transfer
        if ((fprops[i].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
            !(fprops[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
            !(fprops[i].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
            out_info->transfer_family == -1) {
            out_info->transfer_family = i;
        }
    }
    // Fallback when no dedicated queues exist
    for (uint32_t i = 0; i < queue_family_count; ++i) {
        VkQueueFamilyProperties props = fprops[i];

        if ((props.queueFlags & VK_QUEUE_COMPUTE_BIT) &&
            out_info->compute_family == -1) {
            out_info->compute_family = i;
        }

        if ((props.queueFlags & VK_QUEUE_TRANSFER_BIT) &&
            out_info->transfer_family == -1) {
            out_info->transfer_family = i;
        }
    }
    free(fprops);
}
