#include "render_module/render_vk.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "render_module/render_types.h"

#define VULKAN_SURFACE_EXT "VK_KHR_surface"
#define VULKAN_SWAPCHAIN_EXT "VK_KHR_swapchain"
#define VULKAN_SURFACE_EXT_WL "VK_KHR_wayland_surface"
#define VULKAN_SURFACE_EXT_XCB "VK_KHR_xcb_surface"
#define VULKAN_SURFACE_EXT_WIN32 "VK_KHR_win32_surface"
#define VULKAN_VALIDATION_LAYER "VK_LAYER_KHRONOS_validation"
#define VULKAN_ENABLEMENT_COUNT 256

static int32_t get_family_queue(VkPhysicalDevice dev, VkSurfaceKHR surface);
static void    create_swapchain(swapchain_creation_params* params);

void init_instance_vk(VkInstance* inst, VkAllocationCallbacks* allocator,
                      uint32_t window_api) {
    uint32_t    extension_count = 0;
    uint32_t    layer_count = 0;
    const char* requested_extensions[VULKAN_ENABLEMENT_COUNT] = {0};
    const char* requested_layers[VULKAN_ENABLEMENT_COUNT] = {0};

    requested_extensions[extension_count++] = VULKAN_SURFACE_EXT;
    switch (window_api) {
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
    requested_extensions[extension_count++] =
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
    requested_extensions[extension_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    requested_layers[layer_count++] = VULKAN_VALIDATION_LAYER;
    for (uint32_t i = 0; i < extension_count; i++) {
        printf("\tRequested extension: %s\n", requested_extensions[i]);
    }
    for (uint32_t i = 0; i < layer_count; i++) {
        printf("\tRequested layer: %s\n", requested_layers[i]);
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
    VkResult result = vkCreateInstance(&inst_ci, allocator, inst);

    if (result == VK_SUCCESS) {
        printf("VkInstance created!\n");
    } else {
        printf("VkInstance creation failed with code %d!\n", result);
    }
}

void init_physical_device_vk(device_selection_params* params) {
    VkResult result;
    uint32_t dev_count = 0;
    result = vkEnumeratePhysicalDevices(params->instance, &dev_count, NULL);
    assert(result == VK_SUCCESS);
    VkPhysicalDevice* devs = malloc(dev_count * sizeof(VkPhysicalDevice));
    result = vkEnumeratePhysicalDevices(params->instance, &dev_count, devs);
    assert(result == VK_SUCCESS);

    VkPhysicalDevice discrete_gpu = VK_NULL_HANDLE;
    VkPhysicalDevice integrated_gpu = VK_NULL_HANDLE;

    VkPhysicalDeviceProperties props;
    VkPhysicalDeviceProperties selected_props;

    for (size_t i = 0; i < dev_count; i++) {
        vkGetPhysicalDeviceProperties(devs[i], &props);
        int32_t family_idx = get_family_queue(devs[i], params->surface);
        if (family_idx == -1) {
            continue;
        }
        params->queue_family_idx = family_idx;
        selected_props = props;
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            discrete_gpu = devs[i];

        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
            integrated_gpu = devs[i];

        break;
    }

    free(devs);
    if (discrete_gpu != VK_NULL_HANDLE) {
        params->device = discrete_gpu;
    } else if (integrated_gpu != VK_NULL_HANDLE) {
        params->device = integrated_gpu;
    } else {
        params->device = VK_NULL_HANDLE;
    }
    if (params->device != VK_NULL_HANDLE) {
        printf("Selected GPU: %s\n", selected_props.deviceName);
    }
}

void init_logical_device_vk(device_creation_params* params) {
    const char* dev_exts[] = {
        "VK_KHR_swapchain",
    };
    const float             queue_priority[] = {1.0f};
    VkDeviceQueueCreateInfo queue_ci[1] = {0};
    queue_ci[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_ci[0].queueFamilyIndex = params->queue_family_idx;
    queue_ci[0].queueCount = 1;
    queue_ci[0].pQueuePriorities = queue_priority;

    VkPhysicalDeviceFeatures2 features = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
    };

    vkGetPhysicalDeviceFeatures2(params->phys_device, &features);

    VkDeviceCreateInfo dev_ci = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = sizeof(queue_ci) / sizeof(*queue_ci),
        .pQueueCreateInfos = queue_ci,
        .enabledExtensionCount = sizeof(dev_exts) / sizeof(*dev_exts),
        .ppEnabledExtensionNames = dev_exts,
        .pNext = &features,
    };
    VkResult result = vkCreateDevice(params->phys_device, &dev_ci,
                                     params->allocator, params->device);
    if (result == VK_SUCCESS) {
        printf("VkDevice created!\n");
    } else {
        printf("VkDevice creation failed with code %d!\n", result);
    }
    // todo: proper queues
    VkDevice d = *(params->device);
    vkGetDeviceQueue(d, params->queue_family_idx, 0, params->queue);
}

void init_renderpass_vk(VkDevice device, VkAllocationCallbacks* allocator,
                        VkRenderPass* renderpass, VkFormat format) {
    VkAttachmentDescription depth_attachment = {
        .format = format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentDescription color_attachment = {
        .format = format,
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

    VkRenderPassCreateInfo render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = sizeof(attachments) / sizeof(attachments[0]),
        .pAttachments = attachments,
        .subpassCount = 1,
        .pSubpasses = &subpass,
    };

    VkResult result =
        vkCreateRenderPass(device, &render_pass_info, allocator, renderpass);
    if (result == VK_SUCCESS) {
        printf("VkRenderPass created!\n");
    } else {
        printf("VkRenderPass creation failed with code %d!\n", result);
    }
}

void init_swapchain_vk(swapchain_creation_params* params) {
    VkPhysicalDevice dev = params->device->gpu;
    VkSurfaceKHR     surface = params->surface;

    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, surface, &caps);
    params->capabilities = caps;
    uint32_t frame_count = caps.minImageCount + 1;
    if (frame_count > caps.maxImageCount) frame_count = caps.maxImageCount;
    params->swapchain->frame_count = frame_count;
    VkExtent2D size = (VkExtent2D){params->width, params->height};
    VkExtent2D extent = caps.currentExtent;
    // x < min ? min: x > max ? max : x  // todo: clampf
    if (extent.width == UINT32_MAX || extent.height == UINT32_MAX) {
        extent.width = params->width;
        extent.height = params->height;

        extent.width =
            size.width < caps.minImageExtent.width   ? caps.maxImageExtent.width
            : size.width > caps.maxImageExtent.width ? caps.maxImageExtent.width
                                                     : size.width;
        extent.height = size.height < caps.minImageExtent.height
                            ? caps.maxImageExtent.height
                        : size.height > caps.maxImageExtent.height
                            ? caps.maxImageExtent.height
                            : size.height;
    }
    params->extent = extent;
    params->pretransform =
        (caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
            ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
            : caps.currentTransform;
    params->swapchain->frames = malloc(sizeof(swapchain_frame) * frame_count);
    create_swapchain(params);
}

void destroy_swapchain_vk(VkDevice dev, VkAllocationCallbacks* alloc,
                          swapchain_vk swapchain, VkCommandPool pool) {
    vkDeviceWaitIdle(dev);
    for (uint32_t i = 0; i < swapchain.frame_count; i++) {
        swapchain_frame frame = swapchain.frames[i];
        // vkDestroyImage(dev, swapchain.images[i], alloc);
        vkDestroyImageView(dev, frame.color_img_view, alloc);
        vkDestroyFramebuffer(dev, frame.framebuffer, alloc);
        vkDestroySemaphore(dev, frame.image_available, alloc);
        vkDestroySemaphore(dev, frame.render_finished, alloc);
        vkDestroyFence(dev, frame.inflight, alloc);
    }

    vkDestroySwapchainKHR(dev, swapchain.handle, alloc);
    free(swapchain.frames);
}

VkShaderModule load_shader_module(VkDevice device, const char* filename,
                                  VkAllocationCallbacks* allocator) {
    FILE* f = fopen(filename, "rb");
    assert(f);

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    rewind(f);

    uint32_t* code = malloc(size);
    fread(code, 1, size, f);
    fclose(f);

    VkShaderModuleCreateInfo ci = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = size,
        .pCode = code,
    };

    VkShaderModule module;
    VkResult       res = vkCreateShaderModule(device, &ci, allocator, &module);
    free(code);
    return module;
}

void create_graphics_pipeline(VkDevice device, VkRenderPass renderpass,
                              VkAllocationCallbacks* allocator,
                              VkPipelineLayout       pipeline_layout,
                              VkShaderModule         vert_shader_module,
                              VkShaderModule         frag_shader_module,
                              VkPipeline* pipeline, uint32_t width,
                              uint32_t height) {
    // Shader stages
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vert_shader_module,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = frag_shader_module,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        vertShaderStageInfo,
        fragShaderStageInfo,
    };

    VkVertexInputBindingDescription vertex_binding_desc = {
        .binding = 0,
        .stride = sizeof(vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    VkVertexInputAttributeDescription vertex_attr_desc[] = {
        {
            .binding = 0,
            .location = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = 0,
        },
        {

            .binding = 0,
            .location = 1,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(vertex, r),
        },
    };

    // Vertex input
    VkPipelineVertexInputStateCreateInfo vertex_input_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertex_binding_desc,
        .vertexAttributeDescriptionCount = 2,
        .pVertexAttributeDescriptions = vertex_attr_desc,
    };

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo input_assembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    // Viewport and scissor (dynamic in real use cases)
    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = width,  // example size
        .height = height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    VkRect2D scissor = {.offset = {0, 0}, .extent = {width, height}};

    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
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
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_FALSE,
    };

    VkPipelineColorBlendStateCreateInfo color_blending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
    };

    VkPipelineDepthStencilStateCreateInfo depth_stencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
    };
    // Pipeline creation
    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertex_input_info,
        .pInputAssemblyState = &input_assembly,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pColorBlendState = &color_blending,
        .pDepthStencilState = &depth_stencil,
        .layout = pipeline_layout,
        .renderPass = renderpass,
        .subpass = 0,
    };

    VkResult result = vkCreateGraphicsPipelines(
        device, VK_NULL_HANDLE, 1, &pipelineInfo, allocator, pipeline);
    if (result == VK_SUCCESS) {
        printf("VkPipeline created!\n");
    } else {
        printf("VkPipeline creation failed with code %d!\n", result);
    }
}

VkSurfaceFormatKHR choose_surface_fmt(VkPhysicalDevice dev,
                                      VkSurfaceKHR     surface) {
    VkSurfaceFormatKHR selected_fmt;
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
        printf("Image format not found!");
        selected_fmt = formats[0];
    }
    free(formats);
    return selected_fmt;
}

VkFormat choose_depth_format(VkPhysicalDevice physical_device) {
    VkFormat candidates[] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM,
    };
    for (int i = 0; i < sizeof(candidates) / sizeof(candidates[0]); ++i) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physical_device, candidates[i],
                                            &props);
        if (props.optimalTilingFeatures &
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            printf("Found depth format: %d\n", candidates[i]);
            return candidates[i];
        }
    }
    return VK_FORMAT_MAX_ENUM;
}
VkPresentModeKHR choose_present_mode(VkPhysicalDevice dev,
                                     VkSurfaceKHR     surface) {
    VkPresentModeKHR mode;
    uint32_t         supported_count = 0;
    VkPresentModeKHR supported[8];
    vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &supported_count,
                                              NULL);
    vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &supported_count,
                                              supported);
    uint32_t         found = 0;
    VkPresentModeKHR requested = VK_PRESENT_MODE_MAILBOX_KHR;
    for (size_t i = 0; i < supported_count; i++) {
        if (requested == supported[i]) {
            found = 1;
            break;
        }
    }
    mode = found ? requested : VK_PRESENT_MODE_FIFO_KHR;
    return mode;
}

void create_buffer_vk(render_vk state, VkMemoryPropertyFlags properties,
                      buffer_vk* buffer, uint32_t size,
                      VkBufferUsageFlags usage_flags) {
    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage_flags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    buffer->id = -1;
    VkResult result = vkCreateBuffer(state.device.handle, &buffer_info,
                                     state.allocator, &buffer->handle);

    if (result == VK_SUCCESS) {
        printf("vkCreateBuffer created!\n");
    } else {
        printf("vkCreateBuffer creation failed with code %d!\n", result);
    }

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(state.device.handle, buffer->handle,
                                  &mem_reqs);

    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(state.device.gpu, &mem_properties);
    uint32_t memory_type_idx;
    uint32_t found = 0;

    printf("mem_reqs.memoryTypeBits = 0x%x\n", mem_reqs.memoryTypeBits);
    printf("Requested properties = 0x%x\n", properties);
    printf("Available memory types:\n");
    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; ++i) {
        printf("  Type %u: propertyFlags = 0x%x\n", i,
               mem_properties.memoryTypes[i].propertyFlags);
        if ((mem_reqs.memoryTypeBits & (1 << i)) &&
            (mem_properties.memoryTypes[i].propertyFlags & properties) ==
                properties) {
            memory_type_idx = i;
            found = 1;
            break;
        }
    }
    if (!found) {
        printf("Failed to find requested memory type!\n");
        return;
    }

    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_reqs.size,
        .memoryTypeIndex = memory_type_idx,
    };

    result = vkAllocateMemory(state.device.handle, &alloc_info, state.allocator,
                              &buffer->memory);
    if (result == VK_SUCCESS) {
        printf("vkCreateBuffer created!\n");
    } else {
        printf("vkCreateBuffer creation failed with code %d!\n", result);
    }
    vkBindBufferMemory(state.device.handle, buffer->handle, buffer->memory, 0);
    static uint32_t id = 0;
    buffer->id = id++;
    buffer->allocation_size = mem_reqs.size;
}

void upload_to_gpu_vk(render_vk state, buffer_vk* buffer, void* data,
                      uint32_t size) {
    buffer_vk stg;
    // Create staging buffer
    create_buffer_vk(state,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     &stg, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

    // Map and copy data
    void* mapped;
    vkMapMemory(state.device.handle, stg.memory, 0, size, 0, &mapped);
    memcpy(mapped, data, (size_t)size);
    vkUnmapMemory(state.device.handle, stg.memory);

    // Begin a one-time command buffer
    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = state.cmdpool,
        .commandBufferCount = 1,
    };
    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(state.device.handle, &alloc_info, &cmd);

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(cmd, &begin_info);

    // Copy buffer command
    VkBufferCopy copy_region = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size,
    };
    vkCmdCopyBuffer(cmd, stg.handle, buffer->handle, 1, &copy_region);
    vkEndCommandBuffer(cmd);

    // Submit and wait for completion
    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
    };
    vkQueueSubmit(state.device.gfx_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(state.device.gfx_queue);  // You can improve perf later with
                                              // a dedicated transfer queue

    // Cleanup staging buffer
    vkFreeCommandBuffers(state.device.handle, state.cmdpool, 1, &cmd);
    vkDestroyBuffer(state.device.handle, stg.handle, state.allocator);
    vkFreeMemory(state.device.handle, stg.memory, state.allocator);
}

//////////////////////////////////////////////////////////
////////// -------------      Internals     -------------
//////////////////////////////////////////////////////////
static void create_swapchain(swapchain_creation_params* params) {
    VkDevice                 dev = params->device->handle;
    uint32_t                 frame_count = params->swapchain->frame_count;
    VkSwapchainCreateInfoKHR sc_ci = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = params->surface,
        .minImageCount = frame_count,
        .imageFormat = params->swapchain->format.format,
        .imageColorSpace = params->swapchain->format.colorSpace,
        .imageExtent = params->extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                      VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = params->pretransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = params->swapchain->present_mode,
        .clipped = VK_TRUE,
    };

    VkResult result = vkCreateSwapchainKHR(dev, &sc_ci, params->allocator,
                                           &params->swapchain->handle);

    if (result == VK_SUCCESS) {
        printf("VkSwapchainKHR created!\n");
    } else {
        printf("VkSwapchainKHR creation failed with code %d!\n", result);
    }
    VkSwapchainKHR sc = params->swapchain->handle;
    uint32_t       img_count = 0;
    vkGetSwapchainImagesKHR(dev, sc, &img_count, NULL);
    printf("Desired number of frames: %d\n", frame_count);
    printf("Retrieved swapchain img count: %d!\n", img_count);
    assert(img_count <= frame_count);
    VkImage* imgs = malloc(sizeof(VkImage) * img_count);
    vkGetSwapchainImagesKHR(dev, sc, &img_count, imgs);
    swapchain_frame* frames = params->swapchain->frames;
    for (uint32_t i = 0; i < img_count; ++i) {
        frames[i].color_img = imgs[i];
        VkImageViewCreateInfo view_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = imgs[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = params->swapchain->format.format,
            .components = {VK_COMPONENT_SWIZZLE_IDENTITY,
                           VK_COMPONENT_SWIZZLE_IDENTITY,
                           VK_COMPONENT_SWIZZLE_IDENTITY,
                           VK_COMPONENT_SWIZZLE_IDENTITY},
            .subresourceRange =
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
        };
        result = vkCreateImageView(dev, &view_info, params->allocator,
                                   &frames[i].color_img_view);

        if (result == VK_SUCCESS) {
            printf("VkImageView %d created!\n", i);
        } else {
            printf("VkImageView %d creation failed with code %d!\n", i, result);
        }
        // depth attachments

        VkImageCreateInfo depth_img_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = params->swapchain->depth_format,
            .extent = {params->extent.width, params->extent.height, 1},
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };

        result = vkCreateImage(dev, &depth_img_info, params->allocator,
                               &frames[i].depth_img);

        if (result == VK_SUCCESS) {
            printf("VkImage depth %d created!\n", i);
        } else {
            printf("VkImage depth %d creation failed with code %d!\n", i,
                   result);
        }

        VkImageViewCreateInfo depth_view_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = frames[i].depth_img,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = params->swapchain->depth_format,
            .subresourceRange =
                {
                    .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
        };

        result = vkCreateImageView(dev, &depth_view_info, params->allocator,
                                   &frames[i].depth_img_view);

        if (result == VK_SUCCESS) {
            printf("VkImageView depth %d created!\n", i);
        } else {
            printf("VkImageView depth %d creation failed with code %d!\n", i,
                   result);
        }

        // Create framebuffer
        VkImageView attachments[] = {
            frames[i].color_img_view,
            frames[i].depth_img_view,
        };
        VkFramebufferCreateInfo fb_ci = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = params->renderpass,
            .attachmentCount = sizeof(attachments) / sizeof(attachments[0]),
            .pAttachments = attachments,
            .width = params->extent.width,
            .height = params->extent.height,
            .layers = 1,
        };
        result = vkCreateFramebuffer(dev, &fb_ci, params->allocator,
                                     &frames[i].framebuffer);
        if (result == VK_SUCCESS) {
            printf("VkFramebuffer %d created!\n", i);
        } else {
            printf("VkFramebuffer %d creation failed with code %d!\n", i,
                   result);
        }

        if (result == VK_SUCCESS) {
            printf("VkCommandBuffer %d created!\n", i);
        } else {
            printf("VkCommandBuffer %d creation failed with code %d!\n", i,
                   result);
        }

        //  // Semaphores and fence
        VkSemaphoreCreateInfo sem_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };

        VkFenceCreateInfo fence_info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };
        result = vkCreateSemaphore(dev, &sem_info, params->allocator,
                                   &frames[i].image_available);

        if (result == VK_SUCCESS) {
            printf("VkSemaphore image_available %d created!\n", i);
        } else {
            printf(
                "VkSemaphore image_available %d creation failed with code "
                "%d!\n",
                i, result);
        }
        result = vkCreateSemaphore(dev, &sem_info, params->allocator,
                                   &frames[i].render_finished);
        if (result == VK_SUCCESS) {
            printf("VkSemaphore render_finished %d created!\n", i);
        } else {
            printf(
                "VkSemaphore render_finished %d creation failed with code "
                "%d!\n",
                i, result);
        }
        result = vkCreateFence(dev, &fence_info, params->allocator,
                               &frames[i].inflight);
        if (result == VK_SUCCESS) {
            printf("VkFence %d created!\n", i);
        } else {
            printf("VkFence %d creation failed with code %d!\n", i, result);
        }
    }
    free(imgs);
}

static int32_t get_family_queue(VkPhysicalDevice dev, VkSurfaceKHR surface) {
    uint32_t family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &family_count, NULL);

    VkQueueFamilyProperties* properties =
        malloc(sizeof(VkQueueFamilyProperties) * family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &family_count, properties);
    VkBool32 supported;
    int32_t  family_idx = -1;
    for (size_t idx = 0; idx < family_count; idx++) {
        VkQueueFamilyProperties queue_family = properties[idx];
        if (queue_family.queueCount > 0 &&
            queue_family.queueFlags &
                (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) {
            vkGetPhysicalDeviceSurfaceSupportKHR(dev, idx, surface, &supported);
            if (supported) {
                family_idx = idx;
                break;
            }
        }
    }
    free(properties);

    return family_idx;
}

//////////////////////////////////////////////////////////
////////// -------------   Debug messenger   -------------
//////////////////////////////////////////////////////////
#if _DEBUG
static VkBool32 debug_utils_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
    VkDebugUtilsMessageTypeFlagsEXT             types,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void*                                       user_data) {
    printf(" MessageID: %s %i\nMessage: %s\n\n", callback_data->pMessageIdName,
           callback_data->messageIdNumber, callback_data->pMessage);

    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        // ASSERT(0); // todo
    }

    return VK_FALSE;
}

void init_debug_msgr(VkInstance instance, VkAllocationCallbacks* allocator,
                     VkDebugUtilsMessengerEXT* msgr) {
    uint32_t ext_count = 0;
    int32_t  debug_extension_present = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &ext_count, NULL);
    VkExtensionProperties* extensions =
        malloc(ext_count * sizeof(VkExtensionProperties));
    vkEnumerateInstanceExtensionProperties(NULL, &ext_count, extensions);
    const char* dbg_ext_name = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    for (size_t i = 0; i < ext_count; i++) {
        if (!strcmp(extensions[i].extensionName, dbg_ext_name)) {
            debug_extension_present = 1;
            printf("Debug extension present!\n");
            break;
        }
    }
    free(extensions);
    if (!debug_extension_present) {
        printf("Debug utils messenger extension not present!\n");
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
            instance, "vkCreateDebugUtilsMessengerEXT");
    if (create_debug_msgr == NULL) {
        printf("Failed to load debug messanger creation function!\n");
        return;
    }
    VkResult result = create_debug_msgr(instance, &dbg_ci, 0, msgr);
    if (result == VK_SUCCESS) {
        printf("Debug messenger created!\n");
    } else {
        printf("Debug messenger creation failed with code %d!\n", result);
    }
}

void destroy_debug_msgr(VkInstance instance, VkAllocationCallbacks* allocator,
                        VkDebugUtilsMessengerEXT msgr) {
    PFN_vkDestroyDebugUtilsMessengerEXT destroy_msgr =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkDestroyDebugUtilsMessengerEXT");
    if (destroy_msgr == NULL) {
        printf("Failed to load debug messanger destruction function!\n");
        return;
    }
    destroy_msgr(instance, msgr, allocator);
}
#endif  // _DEBUG
