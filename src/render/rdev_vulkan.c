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

//=========================================================
//
// context
//
//=========================================================

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
    return result;
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
            infos[i].families =
                vkutil_find_queue_families(devices[i], rdev->window_api);
            vkGetPhysicalDeviceProperties(devices[i], &infos[i].properties);
            vkGetPhysicalDeviceFeatures(devices[i], &infos[i].features);
            vkGetPhysicalDeviceMemoryProperties(devices[i], &infos[i].mem_props);

            infos[i].extension_support = vkutil_check_extension_support(
                devices[i], extensions, extension_count);
        }

        int32_t best_score = -1;
        int32_t best_index = -1;

        for (uint32_t i = 0; i < pdev_count; i++) {
            int32_t score = vkutil_rate_device(&infos[i]);
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

        const float             queue_priority[] = {1.0f};
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
    }

    // command pool
    {
        VkCommandPoolCreateInfo pool_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            // todo: pool for each queue?
            .queueFamilyIndex = rdev->dev.graphics_family,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        };
        result = vkCreateCommandPool(rdev->dev.handle, &pool_info, rdev->allocator,
                                     &rdev->dev.cmd_pool);
        VCHECK(result);
    }
    return result;
}

void rdev_vulkan_destroy_device(rdev_vulkan* rdev) {
    vkDestroyCommandPool(rdev->dev.handle, rdev->dev.cmd_pool, rdev->allocator);
    vkDestroyDevice(rdev->dev.handle, rdev->allocator);
    rdev->dev.physical = VK_NULL_HANDLE;
    rdev->dev.handle = VK_NULL_HANDLE;
    rdev->dev.graphics_queue = VK_NULL_HANDLE;
    rdev->dev.compute_queue = VK_NULL_HANDLE;
    rdev->dev.transfer_queue = VK_NULL_HANDLE;
    rdev->dev.graphics_family = UINT32_MAX;
    rdev->dev.compute_family = UINT32_MAX;
    rdev->dev.transfer_family = UINT32_MAX;
}

//=========================================================
//
// presentation resources
//
//=========================================================

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

VkResult rdev_vulkan_create_swapchain(rdev_vulkan* rdev, vswapchain* sc,
                                      VkExtent2D extent) {
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(rdev->dev.physical, rdev->surface,
                                              &caps);
    uint32_t image_count = caps.minImageCount + 1;
    if (image_count > caps.maxImageCount) image_count = caps.maxImageCount;
    sc->image_count = image_count;
    VkExtent2D current_extent = caps.currentExtent;
    if (current_extent.width != UINT32_MAX) {
        extent = current_extent;
    } else {
        current_extent.width = VCLAMP(extent.width, caps.minImageExtent.width,
                                      caps.maxImageExtent.width);
        current_extent.height = VCLAMP(extent.height, caps.minImageExtent.height,
                                       caps.maxImageExtent.height);
    }

    VkSurfaceTransformFlagBitsKHR pretransform =
        (caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
            ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
            : caps.currentTransform;
    sc->extent = current_extent;
    VkSwapchainCreateInfoKHR sc_ci = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = rdev->surface,
        .minImageCount = image_count,
        .imageFormat = sc->surface_fmt.format,
        .imageColorSpace = sc->surface_fmt.colorSpace,
        .imageExtent = current_extent,
        .imageArrayLayers = 1,
        .imageUsage =
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = pretransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = sc->present_mode,
        .clipped = VK_TRUE,
    };
    VkResult result = vkCreateSwapchainKHR(rdev->dev.handle, &sc_ci,
                                           rdev->allocator, &sc->handle);
    if (result != VK_SUCCESS) return result;

    uint32_t img_count = 0;
    vkGetSwapchainImagesKHR(rdev->dev.handle, sc->handle, &img_count, NULL);
    debug_assert(img_count <= VSWAPCHAIN_MAX_IMG);
    vkGetSwapchainImagesKHR(rdev->dev.handle, sc->handle, &img_count,
                            sc->color_imgs);

    for (uint32_t i = 0; i < image_count; i++) {
        VkImageViewCreateInfo color_view_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = sc->color_imgs[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = sc->surface_fmt.format,
            .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.levelCount = 1,
            .subresourceRange.layerCount = 1,

        };
        result = vkCreateImageView(rdev->dev.handle, &color_view_info,
                                   rdev->allocator, &sc->color_views[i]);

        VCHECK(result);
        if (result != VK_SUCCESS) return result;

        // depth attachments

        VkImageCreateInfo depth_img_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = sc->depth_fmt,
            .extent = {current_extent.width, current_extent.height, 1},
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };

        result = vkCreateImage(rdev->dev.handle, &depth_img_info, rdev->allocator,
                               &sc->depth_imgs[i]);
        VCHECK(result);
        if (result != VK_SUCCESS) return result;

        VkMemoryRequirements mem_reqs;
        vkGetImageMemoryRequirements(rdev->dev.handle, sc->depth_imgs[i],
                                     &mem_reqs);

        // Allocate
        int32_t memory_type =
            vkutil_find_memory_type(rdev->dev.physical, mem_reqs.memoryTypeBits,
                                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        debug_assert(memory_type != -1);
        VkMemoryAllocateInfo alloc_info = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = mem_reqs.size,
            .memoryTypeIndex = memory_type,
        };

        result = vkAllocateMemory(rdev->dev.handle, &alloc_info, rdev->allocator,
                                  &sc->depth_mem[i]);
        if (result != VK_SUCCESS) return result;

        result = vkBindImageMemory(rdev->dev.handle, sc->depth_imgs[i],
                                   sc->depth_mem[i], 0);
        if (result != VK_SUCCESS) return result;

        VkImageViewCreateInfo depth_view_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = sc->depth_imgs[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = sc->depth_fmt,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.levelCount = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount = 1,
        };

        result = vkCreateImageView(rdev->dev.handle, &depth_view_info,
                                   rdev->allocator, &sc->depth_views[i]);
        if (result != VK_SUCCESS) return result;
    }
    // todo: handle result failure

    return result;
}

void rdev_vulkan_destroy_swapchain(rdev_vulkan* rdev, vswapchain* sc) {
    vkDeviceWaitIdle(rdev->dev.handle);
    for (uint32_t i = 0; i < sc->image_count; i++) {
        vkDestroyImage(rdev->dev.handle, sc->depth_imgs[i], rdev->allocator);
        vkDestroyImageView(rdev->dev.handle, sc->color_views[i], rdev->allocator);
        vkDestroyImageView(rdev->dev.handle, sc->depth_views[i], rdev->allocator);
        vkFreeMemory(rdev->dev.handle, sc->depth_mem[i], rdev->allocator);
    }
    vkDestroySwapchainKHR(rdev->dev.handle, sc->handle, rdev->allocator);
    debug_log("Renderer: swapchain destroyed\n");
}

VkResult rdev_vulkan_create_framebuffers(rdev_vulkan* rdev, vswapchain* sc) {
    VkResult result;
    for (uint32_t i = 0; i < sc->image_count; i++) {
        VkImageView attachments[] = {
            sc->color_views[i],
            sc->depth_views[i],
        };
        VkFramebufferCreateInfo frame_info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = sc->rpass.handle,
            .attachmentCount = sizeof(attachments) / sizeof(attachments[0]),
            .pAttachments = attachments,
            .width = sc->extent.width,
            .height = sc->extent.height,
            .layers = 1,
        };
        result = vkCreateFramebuffer(rdev->dev.handle, &frame_info, rdev->allocator,
                                     &sc->framebuffers[i]);
        if (result != VK_SUCCESS) return result;
    }
    return result;
}

void rdev_vulkan_destroy_framebuffers(rdev_vulkan* rdev, vswapchain* sc) {
    for (uint32_t i = 0; i < sc->image_count; i++) {
        vkDestroyFramebuffer(rdev->dev.handle, sc->framebuffers[i],
                             rdev->allocator);
    }
}

VkResult rdev_vulkan_create_semaphores(rdev_vulkan* rdev, uint32_t count) {
    VkSemaphoreCreateInfo sem_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    VkResult result;
    for (uint32_t i = 0; i < count; i++) {
        result = vkCreateSemaphore(rdev->dev.handle, &sem_info, rdev->allocator,
                                   &rdev->image_available_semaphores[i]);
        if (result != VK_SUCCESS) return result;
        result = vkCreateSemaphore(rdev->dev.handle, &sem_info, rdev->allocator,
                          &rdev->render_finished_semaphores[i]);
        if (result != VK_SUCCESS) return result;
    }
    return result;
}

void rdev_vulkan_destroy_semaphores(rdev_vulkan* rdev, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        vkDestroySemaphore(rdev->dev.handle, rdev->image_available_semaphores[i],
                           rdev->allocator);
        vkDestroySemaphore(rdev->dev.handle, rdev->render_finished_semaphores[i],
                           rdev->allocator);
    }
}

VkResult rdev_vulkan_create_fences(rdev_vulkan* rdev, uint32_t count) {
    VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    VkResult result;
    for (uint32_t i = 0; i < count; i++) {
        result = vkCreateFence(rdev->dev.handle, &fence_info, rdev->allocator,
                               &rdev->inflight_fences[i]);
        if (result != VK_SUCCESS) return result;
    }
    return result;
}

void rdev_vulkan_destroy_fences(rdev_vulkan* rdev, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        vkDestroyFence(rdev->dev.handle, rdev->inflight_fences[i], rdev->allocator);
    }
}

//=========================================================
//
// rendering resources
//
//=========================================================

VkResult rdev_vulkan_create_swapchainpass(rdev_vulkan* rdev, vswapchain* sc) {
    VkAttachmentDescription depth_attachment = {
        .format = sc->depth_fmt,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentDescription color_attachment = {
        .format = sc->surface_fmt.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference color_attachment_ref = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentReference depth_attachment_ref = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_ref,
        .pDepthStencilAttachment = &depth_attachment_ref,
    };

    VkAttachmentDescription attachments[2] = {
        color_attachment,
        depth_attachment,
    };

    VkRenderPassCreateInfo renderpass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = sizeof(attachments) / sizeof(attachments[0]),
        .pAttachments = attachments,
        .subpassCount = 1,
        .pSubpasses = &subpass,
    };

    VkResult result = vkCreateRenderPass(rdev->dev.handle, &renderpass_info,
                                         rdev->allocator, &sc->rpass.handle);
    VCHECK(result);
    return result;
}

void rdev_vulkan_destroy_swapchainpass(rdev_vulkan* rdev, vswapchain* sc) {
    vkDestroyRenderPass(rdev->dev.handle, sc->rpass.handle, rdev->allocator);
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


//=========================================================
//
// graphics resources
//
//=========================================================

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
            break;
        }
    }
    if (!debug_extension_present) {
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
    VCHECK(result);
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
