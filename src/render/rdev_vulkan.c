#include "rdev_vulkan.h"

#include <stdint.h>
#include <string.h>
#include <vulkan/vulkan_core.h>

#include "base.h"
#include "render/rtypes.h"
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

VkResult vcreate_instance(vstate* v) {
    uint32_t    extension_count = 0;
    uint32_t    layer_count = 0;
    const char* requested_extensions[VULKAN_ENABLEMENT_COUNT] = {0};
    const char* requested_layers[VULKAN_ENABLEMENT_COUNT] = {0};

    requested_extensions[extension_count++] = VULKAN_SURFACE_EXT;
    switch (v->window_api) {
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
        debug_log("\trequested extension: %s\n", requested_extensions[i]);
    }
    for (uint32_t i = 0; i < layer_count; i++) {
        debug_log("\trequested layer: %s\n", requested_layers[i]);
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
    VkResult result = vkCreateInstance(&inst_ci, v->allocator, &v->instance);
    VCHECK(result);
    return result;
}

VkResult vcreate_device(vstate* v) {
    VkResult result = VK_SUCCESS;
    uint32_t pdev_count = 0;

    result = vkEnumeratePhysicalDevices(v->instance, &pdev_count, 0);
    if (result != VK_SUCCESS) return result;

    // physical device
    {
        VkPhysicalDevice devices[pdev_count];
        device_info      infos[pdev_count];
        result = vkEnumeratePhysicalDevices(v->instance, &pdev_count, devices);
        if (result != VK_SUCCESS) return result;
        const char* extensions[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };
        uint32_t extension_count = sizeof(extensions) / sizeof(*extensions);
        for (uint32_t i = 0; i < pdev_count; i++) {
            infos[i].device = devices[i];
            infos[i].families = vutl_find_queue_families(devices[i], v->window_api);
            vkGetPhysicalDeviceProperties(devices[i], &infos[i].properties);
            vkGetPhysicalDeviceFeatures(devices[i], &infos[i].features);
            vkGetPhysicalDeviceMemoryProperties(devices[i], &infos[i].mem_props);

            infos[i].extension_support =
                vutl_extensions_supported(devices[i], extensions, extension_count);
        }

        int32_t best_score = -1;
        int32_t best_index = -1;

        for (uint32_t i = 0; i < pdev_count; i++) {
            int32_t score = vutl_rate_device(&infos[i]);
            if (score > best_score) {
                best_score = score;
                best_index = i;
            }
        }
        if (best_index < 0 && best_score < 0) {
            return VK_ERROR_INITIALIZATION_FAILED;
        }
        v->dev.physical = devices[best_index];
        v->dev.properties = infos[best_index].properties;
        v->dev.mem_properties = infos[best_index].mem_props;
        v->dev.graphics_family = infos[best_index].families.graphics;
        v->dev.compute_family = infos[best_index].families.compute;
        v->dev.transfer_family = infos[best_index].families.transfer;
        debug_log("selected device: %s\n", v->dev.properties.deviceName);
        debug_log("graphics | compute | tranfer\n");
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
            v->dev.graphics_family,
            v->dev.graphics_family,
            v->dev.graphics_family,
        };

        // graphics/compute/transfer
        const uint32_t max_queue_families = 3;
        uint32_t       unique_families_count = 1;
        if (v->dev.compute_family != v->dev.graphics_family) {
            families[unique_families_count++] = v->dev.compute_family;
        }
        if (v->dev.transfer_family != v->dev.graphics_family &&
            v->dev.transfer_family != v->dev.compute_family) {
            families[unique_families_count++] = v->dev.transfer_family;
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

        vkGetPhysicalDeviceFeatures2(v->dev.physical, &features);

        VkDeviceCreateInfo dev_ci = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = unique_families_count,
            .pQueueCreateInfos = queue_infos,
            .enabledExtensionCount = sizeof(dev_exts) / sizeof(*dev_exts),
            .ppEnabledExtensionNames = dev_exts,
            .pNext = &features,
        };
        result =
            vkCreateDevice(v->dev.physical, &dev_ci, v->allocator, &v->dev.handle);
        if (result != VK_SUCCESS) return result;

        vkGetDeviceQueue(v->dev.handle, v->dev.graphics_family, 0,
                         &v->dev.graphics_queue);

        if (v->dev.compute_family != UINT32_MAX) {
            vkGetDeviceQueue(v->dev.handle, v->dev.compute_family, 0,
                             &v->dev.compute_queue);
        } else {
            v->dev.compute_queue = v->dev.graphics_queue;  // Fallback
        }

        if (v->dev.transfer_family != UINT32_MAX) {
            vkGetDeviceQueue(v->dev.handle, v->dev.transfer_family, 0,
                             &v->dev.transfer_queue);
        } else {
            v->dev.transfer_queue = v->dev.graphics_queue;  // Fallback
        }
    }

    // command pool
    {
        VkCommandPoolCreateInfo pool_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            // todo: pool for each queue?
            .queueFamilyIndex = v->dev.graphics_family,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        };
        result = vkCreateCommandPool(v->dev.handle, &pool_info, v->allocator,
                                     &v->dev.cmd_pool);
        VCHECK(result);
    }
    return result;
}

void vdestroy_device(vstate* v) {
    vkDestroyCommandPool(v->dev.handle, v->dev.cmd_pool, v->allocator);
    vkDestroyDevice(v->dev.handle, v->allocator);
    v->dev.physical = VK_NULL_HANDLE;
    v->dev.handle = VK_NULL_HANDLE;
    v->dev.graphics_queue = VK_NULL_HANDLE;
    v->dev.compute_queue = VK_NULL_HANDLE;
    v->dev.transfer_queue = VK_NULL_HANDLE;
    v->dev.graphics_family = UINT32_MAX;
    v->dev.compute_family = UINT32_MAX;
    v->dev.transfer_family = UINT32_MAX;
}

//=========================================================
//
// presentation resources
//
//=========================================================

VkResult vcreate_surface_xcb(vstate* v, void* wnd_native) {
    unimplemented(vcreate_surface_xcb) unused(v);
    unused(wnd_native);
    return VK_ERROR_UNKNOWN;
}
VkResult vcreate_surface_win32(vstate* v, void* wnd_native) {
    unimplemented(vcreate_surface_win32) unused(v);
    unused(wnd_native);
    return VK_ERROR_UNKNOWN;
}

VkResult vcreate_swapchain(vstate* v, vswapchain* sc, VkExtent2D extent) {
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(v->dev.physical, v->surface, &caps);
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
        .surface = v->surface,
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
    VkResult result =
        vkCreateSwapchainKHR(v->dev.handle, &sc_ci, v->allocator, &sc->handle);
    if (result != VK_SUCCESS) return result;

    uint32_t img_count = 0;
    vkGetSwapchainImagesKHR(v->dev.handle, sc->handle, &img_count, NULL);
    debug_assert(img_count <= VSWAPCHAIN_MAX_IMG);
    vkGetSwapchainImagesKHR(v->dev.handle, sc->handle, &img_count, sc->color_imgs);

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
        result = vkCreateImageView(v->dev.handle, &color_view_info, v->allocator,
                                   &sc->color_views[i]);

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

        result = vkCreateImage(v->dev.handle, &depth_img_info, v->allocator,
                               &sc->depth_imgs[i]);
        VCHECK(result);
        if (result != VK_SUCCESS) return result;

        VkMemoryRequirements mem_reqs;
        vkGetImageMemoryRequirements(v->dev.handle, sc->depth_imgs[i], &mem_reqs);

        // Allocate
        int32_t memory_type =
            vutl_find_memory_type(v->dev.physical, mem_reqs.memoryTypeBits,
                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        debug_assert(memory_type != -1);
        VkMemoryAllocateInfo alloc_info = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = mem_reqs.size,
            .memoryTypeIndex = memory_type,
        };

        result = vkAllocateMemory(v->dev.handle, &alloc_info, v->allocator,
                                  &sc->depth_mem[i]);
        if (result != VK_SUCCESS) return result;

        result = vkBindImageMemory(v->dev.handle, sc->depth_imgs[i],
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

        result = vkCreateImageView(v->dev.handle, &depth_view_info, v->allocator,
                                   &sc->depth_views[i]);
        if (result != VK_SUCCESS) return result;
    }
    // todo: handle result failure

    return result;
}

void vdestroy_swapchain(vstate* v, vswapchain* sc) {
    vkDeviceWaitIdle(v->dev.handle);
    for (uint32_t i = 0; i < sc->image_count; i++) {
        vkDestroyImage(v->dev.handle, sc->depth_imgs[i], v->allocator);
        vkDestroyImageView(v->dev.handle, sc->color_views[i], v->allocator);
        vkDestroyImageView(v->dev.handle, sc->depth_views[i], v->allocator);
        vkFreeMemory(v->dev.handle, sc->depth_mem[i], v->allocator);
    }
    vkDestroySwapchainKHR(v->dev.handle, sc->handle, v->allocator);
    debug_log("swapchain destroyed\n");
}

VkResult vcreate_framebuffers(vstate* v, vswapchain* sc) {
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
        result = vkCreateFramebuffer(v->dev.handle, &frame_info, v->allocator,
                                     &sc->framebuffers[i]);
        if (result != VK_SUCCESS) return result;
    }
    return result;
}

void vdestroy_framebuffers(vstate* v, vswapchain* sc) {
    for (uint32_t i = 0; i < sc->image_count; i++) {
        vkDestroyFramebuffer(v->dev.handle, sc->framebuffers[i], v->allocator);
    }
}

VkResult vcreate_semaphores(vstate* v, uint32_t count) {
    VkSemaphoreCreateInfo sem_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    VkResult result;
    for (uint32_t i = 0; i < count; i++) {
        result = vkCreateSemaphore(v->dev.handle, &sem_info, v->allocator,
                                   &v->image_available_semaphores[i]);
        if (result != VK_SUCCESS) return result;
        result = vkCreateSemaphore(v->dev.handle, &sem_info, v->allocator,
                                   &v->render_finished_semaphores[i]);
        if (result != VK_SUCCESS) return result;
    }
    return result;
}

void vdestroy_semaphores(vstate* v, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        vkDestroySemaphore(v->dev.handle, v->image_available_semaphores[i],
                           v->allocator);
        vkDestroySemaphore(v->dev.handle, v->render_finished_semaphores[i],
                           v->allocator);
    }
}

VkResult vcreate_fences(vstate* v, uint32_t count) {
    VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    VkResult result;
    for (uint32_t i = 0; i < count; i++) {
        result = vkCreateFence(v->dev.handle, &fence_info, v->allocator,
                               &v->inflight_fences[i]);
        if (result != VK_SUCCESS) return result;
    }
    return result;
}

void vdestroy_fences(vstate* v, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        vkDestroyFence(v->dev.handle, v->inflight_fences[i], v->allocator);
    }
}

//=========================================================
//
// graphics resources
//
//=========================================================

VkResult vcreate_swapchainpass(vstate* v, vswapchain* sc) {
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

    VkResult result = vkCreateRenderPass(v->dev.handle, &renderpass_info,
                                         v->allocator, &sc->rpass.handle);
    VCHECK(result);
    return result;
}

void vdestroy_swapchainpass(vstate* v, vswapchain* sc) {
    vkDestroyRenderPass(v->dev.handle, sc->rpass.handle, v->allocator);
}

VkResult vcreate_pipeline(vstate* v, vpipe* pipe, rpipe_params* params,
                          vshader* modules) {
    VkResult result;

    //  =============== pipline layout
    VkPushConstantRange push_constant_ranges[params->push_constant_count];
    for (uint32_t i = 0; i < params->push_constant_count; i++) {
        push_constant_ranges[i].stageFlags = vutl_to_vulkan_shader_stage_flags(
            params->push_constants[i].stage_flags);
        push_constant_ranges[i].offset = params->push_constants[i].offset;
        push_constant_ranges[i].size = params->push_constants[i].size;
    }

    VkPipelineLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pushConstantRangeCount = params->push_constant_count,
        .pPushConstantRanges = push_constant_ranges,
        .setLayoutCount = 0,
        .pSetLayouts = NULL,
    };
    result = vkCreatePipelineLayout(v->dev.handle, &layout_info, v->allocator,
                                    &pipe->layout);
    VCHECK(result);
    if (result != VK_SUCCESS) return result;

    //  =============== graphics pipline

    VkPipelineShaderStageCreateInfo shader_stages[params->shader_stage_count];
    for (uint32_t i = 0; i < params->shader_stage_count; i++) {
        shader_stages[i].sType =
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader_stages[i].pName = "main";
        shader_stages[i].stage = vutl_to_vulkan_shader_stage(modules[i].type);
        shader_stages[i].module = modules[i].handle;
        shader_stages[i].flags = 0;
        shader_stages[i].pNext = 0;
        shader_stages[i].pSpecializationInfo = 0;
    }

    VkVertexInputBindingDescription vertex_bindings[params->vertex_binding_count];
    for (uint32_t i = 0; i < params->vertex_binding_count; i++) {
        vertex_bindings[i].binding = params->vertex_bindings[i].binding;
        vertex_bindings[i].stride = params->vertex_bindings[i].stride;
        vertex_bindings[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    }

    VkVertexInputAttributeDescription
        vertex_attributes[params->vertex_attribute_count];
    for (uint32_t i = 0; i < params->vertex_attribute_count; i++) {
        vertex_attributes[i].binding = params->vertex_attributes[i].binding;
        vertex_attributes[i].location = params->vertex_attributes[i].location;
        vertex_attributes[i].offset = params->vertex_attributes[i].offset;
        vertex_attributes[i].format =
            vutl_to_vulkan_format(params->vertex_attributes[i].format);
    }

    // todo: add params for all the other stages - maybe - if needed in the future
    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = NULL,  // ignored
        .scissorCount = 1,
        .pScissors = NULL};

    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_LINE_WIDTH,
    };

    VkPipelineDynamicStateCreateInfo dynamic_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = sizeof(dynamic_states) / sizeof(dynamic_states[0]),
        .pDynamicStates = dynamic_states,
    };

    // Vertex input
    VkPipelineVertexInputStateCreateInfo vertex_input_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = params->vertex_binding_count,
        .pVertexBindingDescriptions = vertex_bindings,
        .vertexAttributeDescriptionCount = params->vertex_attribute_count,
        .pVertexAttributeDescriptions = vertex_attributes,
    };

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo input_assembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1.0f,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
    };

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
    };

    // Color blending
    VkPipelineColorBlendAttachmentState color_blend_attachment = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_FALSE,
    };

    VkPipelineColorBlendStateCreateInfo color_blending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment,
    };

    VkPipelineDepthStencilStateCreateInfo depth_stencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
    };
    VkRenderPass renderpass = vrenderpass_from_id(v, params->renderpass);
    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = params->shader_stage_count,
        .pStages = shader_stages,
        .pVertexInputState = &vertex_input_info,
        .pInputAssemblyState = &input_assembly,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depth_stencil,
        .pColorBlendState = &color_blending,
        .pDynamicState = &dynamic_state,
        .layout = pipe->layout,
        .renderPass = renderpass,
        .subpass = 0,
    };
    result = vkCreateGraphicsPipelines(v->dev.handle, VK_NULL_HANDLE, 1,
                                       &pipeline_info, v->allocator, &pipe->handle);
    VCHECK(result);
    return result;
}

void vdestroy_pipeline(vstate* v, vpipe* pipe) {
    vkDestroyPipelineLayout(v->dev.handle, pipe->layout, v->allocator);
    vkDestroyPipeline(v->dev.handle, pipe->handle, v->allocator);
}

VkResult vcreate_shader_modules(vstate* v, vshader* shaders, uint32_t count) {
    VkShaderModuleCreateInfo infos[count];
    VkResult                 result;
    for (uint32_t i = 0; i < count; i++) {
        infos[i].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        infos[i].codeSize = shaders[i].code_size;
        infos[i].pCode = shaders[i].code;
        infos[i].flags = 0;
        infos[i].pNext = 0;
        result = vkCreateShaderModule(v->dev.handle, &infos[i], v->allocator,
                                      &shaders[i].handle);
        VCHECK(result);
        if (result != VK_SUCCESS) return result;
    }
    return result;
}

void vdestroy_shader_modules(vstate* v, vshader* shaders, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        vkDestroyShaderModule(v->dev.handle, shaders[i].handle, v->allocator);
    }
}

VkRenderPass vrenderpass_from_id(vstate* v, rpass_id id) {
    unused(id);
    // todo: only using the swapchain renderpass for now;
    return v->swapchain.rpass.handle;
}

VkCommandBuffer vbegin_transfer_cmd(vstate* v) {
    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandBufferCount = 1,
        .commandPool = v->dev.cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .pNext = 0,
    };
    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(v->dev.handle, &alloc_info, &cmd);

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(cmd, &begin_info);
    return cmd;
}

void vend_transfer_cmd(vstate* v, VkCommandBuffer cmd) {
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
    };

    vkQueueSubmit(v->dev.graphics_queue, 1, &submit_info, NULL);
    // todo
    vkQueueWaitIdle(v->dev.graphics_queue);
}

VkResult vcreate_buffer(vstate* v, rbuf_params* params, vbuf* buf) {
    buf->size = params->size;
    buf->usage = params->usage_flags;
    buf->mem_props = params->memory_flags;
    buf->mapped_ptr = NULL;
    buf->is_mapped = 0;

    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = params->size,
        .usage = params->usage_flags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VkResult result =
        vkCreateBuffer(v->dev.handle, &buffer_info, v->allocator, &buf->handle);
    VCHECK(result);
    if (result != VK_SUCCESS) return result;

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(v->dev.handle, buf->handle, &mem_requirements);

    int32_t mem_idx = vutl_find_memory_type(
        v->dev.physical, mem_requirements.memoryTypeBits, params->memory_flags);
    if (mem_idx < 0) return VK_ERROR_OUT_OF_DEVICE_MEMORY;

    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .memoryTypeIndex = mem_idx,
        .allocationSize = mem_requirements.size,
    };
    result =
        vkAllocateMemory(v->dev.handle, &alloc_info, v->allocator, &buf->memory);
    if (result != VK_SUCCESS) {
        debug_log("allocating memory for buffer %d failed!\n", buf->id);
        vkDestroyBuffer(v->dev.handle, buf->handle, v->allocator);
        return result;
    }
    vkBindBufferMemory(v->dev.handle, buf->handle, buf->memory, 0);
    if (params->initial_data) {
        result = vupload_buffer(v, buf, params->initial_data, params->size, 0);
    }
    return result;
}

void vdestroy_buffer(vstate* v, vbuf* buf) {
    vkDestroyBuffer(v->dev.handle, buf->handle, v->allocator);
    vkFreeMemory(v->dev.handle, buf->memory, v->allocator);
}

VkResult vupload_buffer(vstate* v, vbuf* buf, void* data, uint32_t size,
                        uint32_t offset) {
    VkResult result;
    if (buf->mem_props & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        // Direct upload to host-visible memory
        void* mapped_data;
        result =
            vkMapMemory(v->dev.handle, buf->memory, offset, size, 0, &mapped_data);
        if (result != VK_SUCCESS) return result;

        memcpy(mapped_data, data, size);

        // Flush if not coherent
        if (!(buf->mem_props & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            VkMappedMemoryRange range = {
                .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
                .memory = buf->memory,
                .size = size,
                .offset = offset};
            result = vkFlushMappedMemoryRanges(v->dev.handle, 1, &range);
        }

        vkUnmapMemory(v->dev.handle, buf->memory);
        return result;
    } else {
        // Need staging buffer for device-local memory
        rbuf_params staging_params = {
            .size = size,
            .usage_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            .initial_data = NULL,
        };
        vbuf staging;
        result = vcreate_buffer(v, &staging_params, &staging);
        if (result != VK_SUCCESS) return result;

        // Upload data to staging buffer
        void* mapped_data;
        result =
            vkMapMemory(v->dev.handle, staging.memory, 0, size, 0, &mapped_data);
        if (result != VK_SUCCESS) {
            vdestroy_buffer(v, &staging);
            return result;
        }

        memcpy(mapped_data, data, size);  // MISSING: Copy data to staging
        vkUnmapMemory(v->dev.handle, staging.memory);  // MISSING: Unmap staging

        // Copy from staging to target buffer
        VkCommandBuffer cmd = vbegin_transfer_cmd(v);
        VkBufferCopy    copy_region = {
               .srcOffset = 0,
               .dstOffset = offset,
               .size = size,
        };
        vkCmdCopyBuffer(cmd, staging.handle, buf->handle, 1, &copy_region);
        vend_transfer_cmd(v, cmd);

        // Clean up staging buffer
        vdestroy_buffer(v, &staging);
        return result;
    }
}

VkResult vdownload_buffer(vstate* v, vbuf* buf, void* data, uint32_t size,
                          uint32_t offset) {
    VkResult result;
    if (buf->mem_props & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        // Direct download from host-visible memory
        void* mapped_data;
        result =
            vkMapMemory(v->dev.handle, buf->memory, offset, size, 0, &mapped_data);
        if (result != VK_SUCCESS) return result;

        // Invalidate if NOT coherent
        if (!(buf->mem_props & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            VkMappedMemoryRange range = {
                .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
                .memory = buf->memory,
                .offset = offset,
                .size = size,
            };
            result = vkInvalidateMappedMemoryRanges(v->dev.handle, 1, &range);
            if (result != VK_SUCCESS) {
                vkUnmapMemory(v->dev.handle, buf->memory);
                return result;
            }
        }

        memcpy(data, mapped_data, size);
        vkUnmapMemory(v->dev.handle, buf->memory);  // FIXED: Always unmap
        return VK_SUCCESS;
    } else {
        // Need staging buffer for device-local memory
        rbuf_params staging_params = {
            .size = size,
            .usage_flags = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            .initial_data = NULL};
        vbuf staging;
        result = vcreate_buffer(v, &staging_params, &staging);
        if (result != VK_SUCCESS) return result;

        // Copy from target to staging buffer
        VkCommandBuffer cmd = vbegin_transfer_cmd(v);
        VkBufferCopy    copy_region = {
               .srcOffset = offset,
               .dstOffset = 0,
               .size = size,
        };
        vkCmdCopyBuffer(cmd, buf->handle, staging.handle, 1, &copy_region);
        vend_transfer_cmd(v, cmd);

        // Download from staging buffer
        void* mapped_data;
        result =
            vkMapMemory(v->dev.handle, staging.memory, 0, size, 0, &mapped_data);
        if (result != VK_SUCCESS) {
            vdestroy_buffer(v, &staging);
            return result;
        }

        memcpy(data, mapped_data, size);
        vkUnmapMemory(v->dev.handle, staging.memory);

        // Clean up staging buffer
        vdestroy_buffer(v, &staging);
        return VK_SUCCESS;
    }
}

void* vbuffer_map(vstate* v, vbuf* buf) {
    if (buf->is_mapped) {
        return buf->mapped_ptr;
    }
    if (vkMapMemory(v->dev.handle, buf->memory, 0, buf->size, 0,
                    &buf->mapped_ptr) != VK_SUCCESS) {
        return NULL;
    }
    buf->is_mapped = 1;
    return buf->mapped_ptr;
}

void vbuffer_unmap(vstate* v, vbuf* buf) {
    if (!(buf->mem_props & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
        VkMappedMemoryRange range = {
            .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            .memory = buf->memory,
            .offset = 0,
            .size = buf->size,
        };
        vkFlushMappedMemoryRanges(v->dev.handle, 1, &range);
    }

    vkUnmapMemory(v->dev.handle, buf->memory);
    buf->mapped_ptr = NULL;
    buf->is_mapped = 0;
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

VkResult vcreate_dbg_msgr(vstate* v, VkDebugUtilsMessengerEXT* m) {
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
            v->instance, "vkCreateDebugUtilsMessengerEXT");
    if (create_debug_msgr == NULL) {
        debug_log("Failed to load debug messanger creation function!\n");
        return VK_FALSE;
    }
    VkResult result = create_debug_msgr(v->instance, &dbg_ci, 0, m);
    VCHECK(result);
    return result;
}

void vdestroy_dbg_msgr(vstate* v, VkDebugUtilsMessengerEXT m) {
    if (m == VK_NULL_HANDLE) return;

    PFN_vkDestroyDebugUtilsMessengerEXT destroy_msgr =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            v->instance, "vkDestroyDebugUtilsMessengerEXT");
    if (destroy_msgr == NULL) {
        debug_log("Failed to load debug messanger destruction function!\n");
        return;
    }
    destroy_msgr(v->instance, m, v->allocator);
}
#endif  // _DEBUG
