#include "rdev_vulkan.h"

#include <stdint.h>
#include <string.h>
#include <vulkan/vulkan_core.h>

#include "base.h"
#include "vkutils.h"
#define VULKAN_SURFACE_EXT "VK_KHR_surface"
#define VULKAN_SWAPCHAIN_EXT "VK_KHR_swapchain"
#define VULKAN_SURFACE_EXT_WL "VK_KHR_wayland_surface"
#define VULKAN_SURFACE_EXT_XCB "VK_KHR_xcb_surface"
#define VULKAN_SURFACE_EXT_WIN32 "VK_KHR_win32_surface"
#define VULKAN_VALIDATION_LAYER "VK_LAYER_KHRONOS_validation"
#define VULKAN_ENABLEMENT_COUNT 256

VkResult rdev_vulkan_create_instance(rdev_vulkan* rdev) {
    uint32_t    extension_count = 0;
    uint32_t    layer_count = 0;
    const char* requested_extensions[VULKAN_ENABLEMENT_COUNT] = {0};
    const char* requested_layers[VULKAN_ENABLEMENT_COUNT] = {0};

    requested_extensions[extension_count++] = VULKAN_SURFACE_EXT;
    switch (rdev->window_api) {
        case 0:
            requested_extensions[extension_count++] = VULKAN_SURFACE_EXT_WL;
            break;
        case 1:
            requested_extensions[extension_count++] = VULKAN_SURFACE_EXT_XCB;
            break;
        case 2:
            requested_extensions[extension_count++] = VULKAN_SURFACE_EXT_WIN32;
            break;
    }

#ifdef _DEBUG
    requested_extensions[extension_count++] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
    requested_extensions[extension_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    requested_layers[layer_count++] = VULKAN_VALIDATION_LAYER;
    for (uint32_t i = 0; i < extension_count; i++) {
        debug_log("\tRequested extension: %s\n", requested_extensions[i]);
    }
    for (uint32_t i = 0; i < layer_count; i++) {
        debug_log("\tRequested layer: %s\n", requested_layers[i]);
    }
#endif  //_DEBUG

    const VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "game0",
        .pEngineName = "game0",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_MAKE_VERSION(1, 3, 0),
    };

    const VkInstanceCreateInfo inst_ci = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = extension_count,
        .ppEnabledExtensionNames = requested_extensions,
        .enabledLayerCount = layer_count,
        .ppEnabledLayerNames = requested_layers,
    };
    VkResult result = vkCreateInstance(&inst_ci, rdev->allocator, &rdev->instance);
    VCHECK(result);
    debug_log("Instance created successfully\n");
    return result;
}

VkResult rdev_vulkan_create_surface_xcb(rdev_vulkan* rdev, void* wnd_native) {
    unimplemented(rdev_vulkan_create_surface_xcb) unused(rdev);
    unused(wnd_native);
    return VK_ERROR_UNKNOWN;
}
VkResult rdev_vulkan_create_surface_win32(rdev_vulkan* rdev, void* wnd_native) {
    unimplemented(rdev_vulkan_create_surface_win32) unused(rdev);
    unused(wnd_native);
    return VK_ERROR_UNKNOWN;
}

VkResult rdev_vulkan_create_device(rdev_vulkan* rdev) {
    VkResult result = VK_SUCCESS;
    uint32_t pdev_count = 0;

    result = vkEnumeratePhysicalDevices(rdev->instance, &pdev_count, 0);
    if (result != VK_SUCCESS) return result;

    // physical device
    {
        VkPhysicalDevice devices[pdev_count];
        device_info      infos[pdev_count];
        result = vkEnumeratePhysicalDevices(rdev->instance, &pdev_count, devices);
        if (result != VK_SUCCESS) return result;
        const char* extensions[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };
        uint32_t extension_count = sizeof(extensions) / sizeof(*extensions);
        for (uint32_t i = 0; i < pdev_count; i++) {
            infos[i].device = devices[i];
            infos[i].families = find_queue_families(devices[i], rdev->window_api);
            vkGetPhysicalDeviceProperties(devices[i], &infos[i].properties);
            vkGetPhysicalDeviceFeatures(devices[i], &infos[i].features);
            vkGetPhysicalDeviceMemoryProperties(devices[i], &infos[i].mem_props);

            infos[i].extension_support =
                check_extension_support(devices[i], extensions, extension_count);
        }

        int32_t best_score = -1;
        int32_t best_index = -1;

        for (uint32_t i = 0; i < pdev_count; i++) {
            int32_t score = rate_device(&infos[i]);
            if (score > best_score) {
                best_score = score;
                best_index = i;
            }
        }
        if (best_index < 0 && best_score < 0) {
            return VK_ERROR_INITIALIZATION_FAILED;
        }
        rdev->dev.physical = devices[best_index];
        rdev->dev.properties = infos[best_index].properties;
        rdev->dev.mem_properties = infos[best_index].mem_props;
        rdev->dev.graphics_family = infos[best_index].families.graphics;
        rdev->dev.compute_family = infos[best_index].families.compute;
        rdev->dev.transfer_family = infos[best_index].families.transfer;
        debug_log("Selected device: %s\n", rdev->dev.properties.deviceName);
        debug_log("Graphics | Compute | Tranfer\n");
        debug_log("   %d     |   %d     |   %d\n",
                  infos[best_index].families.graphics,
                  infos[best_index].families.compute,
                  infos[best_index].families.transfer);
    }

    // logical device
    {
        const char* dev_exts[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };

        uint32_t families[] = {
            rdev->dev.graphics_family,
            rdev->dev.graphics_family,
            rdev->dev.graphics_family,
        };

        // graphics/compute/transfer
        const uint32_t max_queue_families = 3;
        uint32_t       unique_families_count = 1;
        if (rdev->dev.compute_family != rdev->dev.graphics_family) {
            families[unique_families_count++] = rdev->dev.compute_family;
        }
        if (rdev->dev.transfer_family != rdev->dev.graphics_family &&
            rdev->dev.transfer_family != rdev->dev.compute_family) {
            families[unique_families_count++] = rdev->dev.transfer_family;
        }

        const float queue_priority[] = {1.0f};
        VkDeviceQueueCreateInfo queue_infos[max_queue_families];
        for (uint32_t i = 0; i < unique_families_count; i++) {
            queue_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_infos[i].pNext = NULL;
            queue_infos[i].flags = 0;
            queue_infos[i].queueFamilyIndex = families[i];
            queue_infos[i].queueCount = 1;
            queue_infos[i].pQueuePriorities = queue_priority;
        }
        VkPhysicalDeviceFeatures2 features = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        };

        vkGetPhysicalDeviceFeatures2(rdev->dev.physical, &features);

        VkDeviceCreateInfo dev_ci = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = unique_families_count,
            .pQueueCreateInfos = queue_infos,
            .enabledExtensionCount = sizeof(dev_exts) / sizeof(*dev_exts),
            .ppEnabledExtensionNames = dev_exts,
            .pNext = &features,
        };
        result = vkCreateDevice(rdev->dev.physical, &dev_ci, rdev->allocator,
                                &rdev->dev.handle);
        if (result != VK_SUCCESS) return result;

        vkGetDeviceQueue(rdev->dev.handle, rdev->dev.graphics_family, 0,
                         &rdev->dev.graphics_queue);

        if (rdev->dev.compute_family != UINT32_MAX) {
            vkGetDeviceQueue(rdev->dev.handle, rdev->dev.compute_family, 0,
                             &rdev->dev.compute_queue);
        } else {
            rdev->dev.compute_queue = rdev->dev.graphics_queue;  // Fallback
        }

        if (rdev->dev.transfer_family != UINT32_MAX) {
            vkGetDeviceQueue(rdev->dev.handle, rdev->dev.transfer_family, 0,
                             &rdev->dev.transfer_queue);
        } else {
            rdev->dev.transfer_queue = rdev->dev.graphics_queue;  // Fallback
        }
        debug_log("Queues obtained: %d\n", unique_families_count);
    }

    // command poll
    {
        VkCommandPoolCreateInfo pool_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            // todo: pool for each queue?
            .queueFamilyIndex = rdev->dev.graphics_family,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        };
        result = vkCreateCommandPool(rdev->dev.handle, &pool_info, rdev->allocator,
                                     &rdev->dev.cmd_pool);
        if (result == VK_SUCCESS) debug_log("Command pool created!\n");
    }
    return result;
}

void rdev_vulkan_destroy_device(rdev_vulkan* rdev) {
    vkDestroyCommandPool(rdev->dev.handle, rdev->dev.cmd_pool, rdev->allocator);
    vkDestroyDevice(rdev->dev.handle, rdev->allocator);
    rdev->dev.physical = VK_NULL_HANDLE;
    rdev->dev.handle = VK_NULL_HANDLE;
    rdev->dev.graphics_queue = VK_NULL_HANDLE;
    rdev->dev.compute_queue= VK_NULL_HANDLE;
    rdev->dev.transfer_queue= VK_NULL_HANDLE;
    rdev->dev.graphics_family = UINT32_MAX;
    rdev->dev.compute_family= UINT32_MAX;
    rdev->dev.transfer_family= UINT32_MAX;
 }

VkResult rdev_vulkan_create_swapchain(rdev_vulkan* rdev, vswapchain* sc,
                                      VkExtent2D extent) {
    VkSurfaceFormatKHR fmt = find_surface_format(rdev->dev.physical, rdev->surface);
    VkFormat depth_fmt = find_depth_format(rdev->dev.physical);
    if (depth_fmt == VK_FORMAT_UNDEFINED) {
        debug_log("Failed to find depth format!\n");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    VkPresentModeKHR present = find_present_mode(rdev->dev.physical, rdev->surface);

    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(rdev->dev.physical, rdev->surface,
                                              &caps);
    uint32_t image_count = caps.minImageCount + 1;
    if (image_count > caps.maxImageCount) image_count = caps.maxImageCount;
    sc->image_count = image_count;
    VkExtent2D size = extent;
    VkExtent2D current_extent = caps.currentExtent;
    if (current_extent.width == UINT32_MAX || extent.height == UINT32_MAX) {
        current_extent.width = extent.width;
        current_extent.height = extent.height;
        current_extent.width = VCLAMP(size.width, caps.minImageExtent.width,
                                caps.maxImageExtent.width);
        current_extent.height = VCLAMP(size.height, caps.minImageExtent.height,
                                 caps.maxImageExtent.height);
    }

    VkSurfaceTransformFlagBitsKHR pretransform =
        (caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
            ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
            : caps.currentTransform;
    VkSwapchainCreateInfoKHR sc_ci = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = rdev->surface,
        .minImageCount = image_count,
        .imageFormat = fmt.format,
        .imageColorSpace = fmt.colorSpace,
        .imageExtent = current_extent,
        .imageArrayLayers = 1,
        .imageUsage =
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = pretransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = present,
        .clipped = VK_TRUE,
    };
    VkResult result = vkCreateSwapchainKHR(rdev->dev.handle, &sc_ci, rdev->allocator,
                                  &sc->handle);
    if (result != VK_SUCCESS) return result;

    debug_log("Swapchain created!\n");
    return VK_FALSE;
}

void rdev_vulkan_destroy_swapchain(rdev_vulkan* rdev, vswapchain* sc) {
    (void)rdev;
    (void)sc;
}

VkResult rdev_vulkan_create_semaphores(rdev_vulkan* rdev, uint32_t count) {
    (void)rdev;
    (void)count;
    return VK_FALSE;
}

void rdev_vulkan_destroy_semaphores(rdev_vulkan* rdev, uint32_t count) {
    (void)rdev;
    (void)count;
}

VkResult rdev_vulkan_create_pipeline(rdev_vulkan* rdev, vpipe* pipe) {
    (void)rdev;
    (void)pipe;
    return VK_FALSE;
}

void rdev_vulkan_destroy_pipeline(rdev_vulkan* rdev, vpipe* pipe) {
    (void)rdev;
    (void)pipe;
}

VkResult rdev_vulkan_create_framebuffers(rdev_vulkan* rdev, vpass* pass,
                                         uint32_t count, VkExtent2D extent) {
    (void)rdev;
    (void)pass;
    (void)count;
    (void)extent;
    return VK_FALSE;
}
void rdev_vulkan_destroy_framebuffers(rdev_vulkan* rdev, uint32_t count) {
    (void)rdev;
    (void)count;
}

VkResult rdev_vulkan_create_fences(rdev_vulkan* rdev, uint32_t count) {
    (void)rdev;
    (void)count;
    return VK_FALSE;
}
void rdev_vulkan_destroy_fences(rdev_vulkan* rdev, uint32_t count) {
    (void)rdev;
    (void)count;
}

#ifdef _DEBUG
static VkBool32 debug_utils_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
    VkDebugUtilsMessageTypeFlagsEXT             types,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data) {
    unused(types);
    unused(user_data);
    debug_log(" MessageID: %s %i\nMessage: %s\n\n", callback_data->pMessageIdName,
              callback_data->messageIdNumber, callback_data->pMessage);

    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        debug_assert(0);
    }

    return VK_FALSE;
}

VkResult rdev_vulkan_create_dbg_msgr(rdev_vulkan*              rdev,
                                     VkDebugUtilsMessengerEXT* m) {
    uint32_t ext_count = 0;
    int32_t  debug_extension_present = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &ext_count, NULL);
    VkExtensionProperties extensions[ext_count];
    vkEnumerateInstanceExtensionProperties(NULL, &ext_count, extensions);
    const char* dbg_ext_name = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    for (size_t i = 0; i < ext_count; i++) {
        if (!strcmp(extensions[i].extensionName, dbg_ext_name)) {
            debug_extension_present = 1;
            debug_log("Debug extension present!\n");
            break;
        }
    }
    if (!debug_extension_present) {
        debug_log("Debug utils messenger extension not present!\n");
        return VK_FALSE;
    }
    const VkDebugUtilsMessengerCreateInfoEXT dbg_ci = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
        .pfnUserCallback = debug_utils_callback,
    };
    PFN_vkCreateDebugUtilsMessengerEXT create_debug_msgr =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            rdev->instance, "vkCreateDebugUtilsMessengerEXT");
    if (create_debug_msgr == NULL) {
        debug_log("Failed to load debug messanger creation function!\n");
        return VK_FALSE;
    }
    VkResult result = create_debug_msgr(rdev->instance, &dbg_ci, 0, m);
    debug_assert(result == VK_SUCCESS);
    debug_log("Debug messenger created!\n");
    return result;
}

void rdev_vulkan_destroy_dbg_msgr(rdev_vulkan* rdev, VkDebugUtilsMessengerEXT m) {
    if (m == VK_NULL_HANDLE) return;

    PFN_vkDestroyDebugUtilsMessengerEXT destroy_msgr =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            rdev->instance, "vkDestroyDebugUtilsMessengerEXT");
    if (destroy_msgr == NULL) {
        debug_log("Failed to load debug messanger destruction function!\n");
        return;
    }
    destroy_msgr(rdev->instance, m, rdev->allocator);
}
#endif  // _DEBUG
