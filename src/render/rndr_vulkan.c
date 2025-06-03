
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "private/base.h"
#include "private/rndr_bknd.h"
#include "render/vulkan_surface.h"

#define WL_SURFACE_EXT_NAME "VK_KHR_wayland_surface"
#define TARGET_FRAME_BUFFERS 3

static VkBool32 debug_utils_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
    VkDebugUtilsMessageTypeFlagsEXT             types,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void*                                       user_data) {
    printf(" MessageID: %s %i\nMessage: %s\n\n", callback_data->pMessageIdName,
           callback_data->messageIdNumber, callback_data->pMessage);

    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        // __debugbreak();
    }

    return VK_FALSE;
}

struct frame {
    VkImage         img;
    VkImageView     img_view;
    VkCommandBuffer cmd_buf;
    VkFramebuffer   frame_buf;
    VkSemaphore     sem_render_start;
    VkSemaphore     sem_render_finished;
    VkFence         inflight_fence;
    uint32_t        image_index;
};

struct vulkan_bknd {
    VkAllocationCallbacks* allocator;
    VkInstance             instance;
    VkSurfaceKHR           surface;
    VkPhysicalDevice       gpu;
    VkDevice               device;
    VkQueue                queue;
    VkPresentModeKHR       present_mode;
    VkSurfaceFormatKHR     surface_fmt;
    VkSwapchainKHR         swapchain;
    VkCommandPool          pool;
    VkRenderPass           render_pass;
    uint32_t               current_frame;
    uint32_t               queue_family_idx;
    uint16_t               sc_width;
    uint16_t               sc_height;

    struct frame frames[TARGET_FRAME_BUFFERS];
#ifdef _DEBUG
    VkDebugUtilsMessengerEXT dbg_msgr;
#endif
};

static const char* s_requested_layers[] = {
#ifdef _DEBUG
    "VK_LAYER_KHRONOS_validation",
#else
    ""
#endif  // _DEBUG
};

////// -------------------------- forward declarations;
//
static void    check(VkResult result, const char* msg, const char* file,
                     size_t line);
static int32_t get_family_queue(struct vulkan_bknd* state,
                                VkPhysicalDevice    dev);
static void    create_swapchain(struct vulkan_bknd* state, VkExtent2D size);
static void    destroy_swapchain(struct vulkan_bknd* state);

////// -------------------------- interface implementation;
//
uint32_t rndr_bknd_get_size() { return sizeof(struct vulkan_bknd); }

int32_t rndr_bknd_startup(rndr_bknd** bknd, struct rndr_cfg cfg) {
    struct vulkan_bknd* state = (struct vulkan_bknd*)*bknd;
    ASSERT(state);
    state->allocator = NULL;
    //----------- Instance
    {
        const VkApplicationInfo app_info = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "game0",
            .pEngineName = "game0",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_MAKE_VERSION(1, 3, 0),
        };

#ifdef _DEBUG
        const uint32_t ext_count = 4;
#else
        const uint32_t ext_count = 2;
#endif  //_DEBUG
        const char* requested_extensions[ext_count];
        requested_extensions[0] = VK_KHR_SURFACE_EXTENSION_NAME;
        switch (cfg.win_api) {
            case 0:
                requested_extensions[1] = WL_SURFACE_EXT_NAME;
                break;
            default:
                ASSERT(0 && "Unknown window API!\n");
        }

#ifdef _DEBUG
        requested_extensions[ext_count - 2] =
            VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
        requested_extensions[ext_count - 1] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

        for (size_t i = 0; i < ext_count; i++) {
            printf("Requested ext: %s\n", requested_extensions[i]);
        }
#endif  // _DEBUG
        const VkInstanceCreateInfo inst_ci = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &app_info,
            .enabledExtensionCount = ext_count,
            .ppEnabledExtensionNames = requested_extensions,
            .enabledLayerCount = ARR_SIZE(s_requested_layers),
            .ppEnabledLayerNames = s_requested_layers,
        };
        VkResult result =
            vkCreateInstance(&inst_ci, state->allocator, &state->instance);
        check(result, "vkCreateInstance", __FILE__, __LINE__);
    }

//----------- Debug messenger
#ifdef _DEBUG
    {
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
        if (debug_extension_present) {
            const VkDebugUtilsMessengerCreateInfoEXT dbg_ci = {
                .sType =
                    VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                .messageSeverity =
                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
                .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
                .pfnUserCallback = debug_utils_callback,
            };
            PFN_vkCreateDebugUtilsMessengerEXT create_debug_msgr =
                (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
                    state->instance, "vkCreateDebugUtilsMessengerEXT");
            if (create_debug_msgr != NULL) {
                VkResult result =
                    create_debug_msgr(state->instance, &dbg_ci,
                                      state->allocator, &state->dbg_msgr);
                check(result, "vkCreateDebugUtilsMessengerEXT", __FILE__,
                      __LINE__);
            }

        } else {
            printf("Debug utils messenger extension not present!\n");
        }
    }
#endif  // _DEBUG

    //----------- Surface
    switch (cfg.win_api) {
        case 0:
            create_wl_surface(state->instance, state->allocator,
                              &state->surface, cfg.win_handle, cfg.win_inst);
            break;
        default:
            ASSERT(0 && "Unknown window API\n");
    }
    ASSERT(state->surface != VK_NULL_HANDLE &&
           "Failed to create window surface");

    //----------- Physical Device
    {
        VkResult result;
        uint32_t dev_count = 0;
        result = vkEnumeratePhysicalDevices(state->instance, &dev_count, NULL);
        check(result, "VkEnumeratePhysicalDevices", __FILE__, __LINE__);
        VkPhysicalDevice* devs = malloc(dev_count * sizeof(VkPhysicalDevice));
        result = vkEnumeratePhysicalDevices(state->instance, &dev_count, devs);
        check(result, "VkEnumeratePhysicalDevices", __FILE__, __LINE__);

        VkPhysicalDevice discrete_gpu = VK_NULL_HANDLE;
        VkPhysicalDevice integrated_gpu = VK_NULL_HANDLE;

        VkPhysicalDeviceProperties dev_props;
        for (size_t i = 0; i < dev_count; i++) {
            vkGetPhysicalDeviceProperties(devs[i], &dev_props);
            if (dev_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                if (get_family_queue(state, devs[i])) {
                    discrete_gpu = devs[i];
                    break;
                }
            }
            if (dev_props.deviceType ==
                VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
                if (get_family_queue(state, devs[i])) {
                    integrated_gpu = devs[i];
                    break;
                }
            }
        }

        free(devs);
        if (discrete_gpu != VK_NULL_HANDLE) {
            state->gpu = discrete_gpu;
        } else if (integrated_gpu != VK_NULL_HANDLE) {
            state->gpu = integrated_gpu;
        } else {
            return -1;
        }
        printf("Selected GPU: %s\n", dev_props.deviceName);
    }

    //----------- Logical Device
    {
        const char* dev_exts[] = {
            "VK_KHR_swapchain",
        };
        const float             queue_priority[] = {1.0f};
        VkDeviceQueueCreateInfo queue_ci[1] = {0};
        queue_ci[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_ci[0].queueFamilyIndex = state->queue_family_idx;
        queue_ci[0].queueCount = 1;
        queue_ci[0].pQueuePriorities = queue_priority;

        VkPhysicalDeviceFeatures2 features = {
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        };

        vkGetPhysicalDeviceFeatures2(state->gpu, &features);

        VkDeviceCreateInfo dev_ci = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = ARR_SIZE(queue_ci),
            .pQueueCreateInfos = queue_ci,
            .enabledExtensionCount = ARR_SIZE(dev_exts),
            .ppEnabledExtensionNames = dev_exts,
            .pNext = &features,
        };
        VkResult result = vkCreateDevice(state->gpu, &dev_ci, state->allocator,
                                         &state->device);
        check(result, "vkCreateDevice", __FILE__, __LINE__);

        vkGetDeviceQueue(state->device, state->queue_family_idx, 0,
                         &state->queue);
    }

    //----------- Surface Format
    {
        const VkFormat desired_fmts[] = {
            VK_FORMAT_B8G8R8A8_UNORM,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_FORMAT_B8G8R8_UNORM,
            VK_FORMAT_R8G8B8_UNORM,
        };
        const VkColorSpaceKHR color_space = VK_COLORSPACE_SRGB_NONLINEAR_KHR;

        uint32_t supported_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(state->gpu, state->surface,
                                             &supported_count, NULL);
        VkSurfaceFormatKHR* formats =
            malloc(supported_count * sizeof(VkSurfaceFormatKHR));

        vkGetPhysicalDeviceSurfaceFormatsKHR(state->gpu, state->surface,
                                             &supported_count, formats);

        uint32_t       format_found = 0;
        const uint32_t desired_fmt_count = ARR_SIZE(desired_fmts);
        for (size_t i = 0; i < desired_fmt_count; i++) {
            for (size_t j = 0; j < supported_count; j++) {
                if (formats[j].format == desired_fmts[i] &&
                    formats[j].colorSpace == color_space) {
                    state->surface_fmt = formats[j];
                    format_found = 1;
                    printf("Found image format!\n");
                    break;
                }
            }
            if (format_found) {
                break;
            }
        }
        if (!format_found) {
            printf("Image format not found!");
            state->surface_fmt = formats[0];
        }
        free(formats);
    }

    //----------- Present Mode
    {
        uint32_t         supported_count = 0;
        VkPresentModeKHR supported[8];
        vkGetPhysicalDeviceSurfacePresentModesKHR(state->gpu, state->surface,
                                                  &supported_count, NULL);

        vkGetPhysicalDeviceSurfacePresentModesKHR(state->gpu, state->surface,
                                                  &supported_count, supported);
        uint32_t         found = 0;
        VkPresentModeKHR requested = VK_PRESENT_MODE_MAILBOX_KHR;
        for (size_t i = 0; i < supported_count; i++) {
            if (requested == supported[i]) {
                found = 1;
                break;
            }
        }
        state->present_mode = found ? requested : VK_PRESENT_MODE_FIFO_KHR;
    }

    //----------- Render pass
    {
        VkAttachmentDescription color_attachment = {
            .format = state->surface_fmt.format,
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

        VkSubpassDescription subpass = {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &color_attachment_ref,
        };

        VkRenderPassCreateInfo render_pass_info = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments = &color_attachment,
            .subpassCount = 1,
            .pSubpasses = &subpass,
        };

        VkResult result =
            vkCreateRenderPass(state->device, &render_pass_info,
                               state->allocator, &state->render_pass);
        check(result, "vkCreateRenderPass", __FILE__, __LINE__);
    }

    //----------- Command Pool
    {
        VkCommandPoolCreateInfo pool_ci = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = state->queue_family_idx,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        };
        VkResult result = vkCreateCommandPool(state->device, &pool_ci,
                                              state->allocator, &state->pool);
        check(result, "vkCreateCommandPool", __FILE__, __LINE__);
    }

    //----------- Swapchain
    {
        VkExtent2D size = {cfg.width, cfg.height};
        create_swapchain(state, size);
    }

    return 0;
}

void rndr_bknd_render_begin(rndr_bknd* bknd) {
    struct vulkan_bknd* state = (struct vulkan_bknd*)bknd;
    ASSERT(state);

    // Wait for previous frame to finish before starting this one
    vkWaitForFences(state->device, 1,
                    &state->frames[state->current_frame].inflight_fence,
                    VK_TRUE, UINT64_MAX);
    vkResetFences(state->device, 1,
                  &state->frames[state->current_frame].inflight_fence);

    // Acquire the next swapchain image
    uint32_t image_index;
    VkResult result = vkAcquireNextImageKHR(state->device, state->swapchain, UINT64_MAX,
                                            state->frames[state->current_frame].sem_render_start,
                                            VK_NULL_HANDLE, &image_index);
    if (result != VK_SUCCESS) {
        // handle swapchain recreation or errors here
        return;
    }

    state->frames[state->current_frame].image_index = image_index;

    VkCommandBuffer cmd = state->frames[state->current_frame].cmd_buf;

    // Reset command buffer before recording
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(cmd, &begin_info);

    VkClearValue clear_color = {.color = {{1.0f, 0.0f, 0.0f, 1.0f}}};

    VkRenderPassBeginInfo rp_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = state->render_pass,
        .framebuffer = state->frames[image_index].frame_buf,
        .renderArea = {
            .offset = {0, 0},
            .extent = {state->sc_width, state->sc_height},
        },
        .clearValueCount = 1,
        .pClearValues = &clear_color,
    };

    vkCmdBeginRenderPass(cmd, &rp_info, VK_SUBPASS_CONTENTS_INLINE);

    // Ready to record draw calls here...
}

void rndr_bknd_render_end(rndr_bknd* bknd) {
    struct vulkan_bknd* state = (struct vulkan_bknd*)bknd;
    ASSERT(state);

    uint32_t image_index = state->frames[state->current_frame].image_index;

    VkCommandBuffer cmd = state->frames[state->current_frame].cmd_buf;

    vkCmdEndRenderPass(cmd);
    vkEndCommandBuffer(cmd);

    VkSemaphore waitSemaphores[] = {state->frames[state->current_frame].sem_render_start};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {state->frames[state->current_frame].sem_render_finished};

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signalSemaphores,
    };

    vkQueueSubmit(state->queue, 1, &submit_info,
                  state->frames[state->current_frame].inflight_fence);

    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphores,
        .swapchainCount = 1,
        .pSwapchains = &state->swapchain,
        .pImageIndices = &image_index,
    };

    vkQueuePresentKHR(state->queue, &present_info);

    state->current_frame = (state->current_frame + 1) % TARGET_FRAME_BUFFERS;
}

void rndr_bknd_teardown(rndr_bknd* bknd) {
    struct vulkan_bknd* state = (struct vulkan_bknd*)bknd;
    ASSERT(state);
    vkDeviceWaitIdle(state->device);

    vkDestroyRenderPass(state->device, state->render_pass, state->allocator);
    destroy_swapchain(state);
    vkDestroyCommandPool(state->device, state->pool, state->allocator);
    vkDestroyDevice(state->device, state->allocator);

    vkDestroySurfaceKHR(state->instance, state->surface, state->allocator);
#ifdef _DEBUG
    PFN_vkDestroyDebugUtilsMessengerEXT destroy_msgr =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            state->instance, "vkDestroyDebugUtilsMessengerEXT");
    destroy_msgr(state->instance, state->dbg_msgr, state->allocator);
    vkDestroyInstance(state->instance, state->allocator);
#endif  // _DEBUG
}

static void check(VkResult result, const char* msg, const char* file,
                  size_t line) {
    if (result != VK_SUCCESS) {
        printf("Vulkan assertion failed `%s` with result %d in %s:%zu\n", msg,
               result, file, line);
        ASSERT(0);
    } else {
        printf("Success `%s`: %d in %s:%zu\n", msg, result, file, line);
    }
}

static int32_t get_family_queue(struct vulkan_bknd* state,
                                VkPhysicalDevice    dev) {
    uint32_t family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &family_count, NULL);

    VkQueueFamilyProperties* properties =
        malloc(sizeof(VkQueueFamilyProperties) * family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &family_count, properties);
    VkBool32 supported;
    for (size_t idx = 0; idx < family_count; idx++) {
        VkQueueFamilyProperties queue_family = properties[idx];
        if (queue_family.queueCount > 0 &&
            queue_family.queueFlags &
                (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) {
            vkGetPhysicalDeviceSurfaceSupportKHR(dev, idx, state->surface,
                                                 &supported);

            if (supported) {
                state->queue_family_idx = idx;
                break;
            }
        }
    }
    free(properties);

    return supported;
}

static void create_swapchain(struct vulkan_bknd* state, VkExtent2D size) {
    VkSurfaceCapabilitiesKHR surf_caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(state->gpu, state->surface,
                                              &surf_caps);

    VkExtent2D sc_extent = surf_caps.currentExtent;

    printf("Current extent: { %d, %d }\n", sc_extent.width, sc_extent.height);

    if (surf_caps.currentExtent.width != UINT32_MAX) {
        // The surface size is defined and must be used.
        sc_extent = surf_caps.currentExtent;
    } else {
        // Clamp the window size to the allowed min/max extent.
        sc_extent.width = size.width;
        sc_extent.height = size.height;

        if (sc_extent.width < surf_caps.minImageExtent.width)
            sc_extent.width = surf_caps.minImageExtent.width;
        else if (sc_extent.width > surf_caps.maxImageExtent.width)
            sc_extent.width = surf_caps.maxImageExtent.width;

        if (sc_extent.height < surf_caps.minImageExtent.height)
            sc_extent.height = surf_caps.minImageExtent.height;
        else if (sc_extent.height > surf_caps.maxImageExtent.height)
            sc_extent.height = surf_caps.maxImageExtent.height;
    }

    printf("Modified extent: { %d, %d }\n", sc_extent.width, sc_extent.height);
    state->sc_height = sc_extent.height;
    state->sc_width = sc_extent.width;
    VkSurfaceTransformFlagBitsKHR pre_transform;

    if (surf_caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
        pre_transform = surf_caps.currentTransform;
    }

    VkSwapchainCreateInfoKHR sc_ci = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = state->surface,
        .minImageCount = TARGET_FRAME_BUFFERS,  // todo
        .imageExtent = sc_extent,
        .clipped = VK_TRUE,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                      VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = state->present_mode,
        .imageFormat = state->surface_fmt.format,
        .imageColorSpace = state->surface_fmt.colorSpace,
        .preTransform = pre_transform,
    };

    VkResult result = vkCreateSwapchainKHR(state->device, &sc_ci,
                                           state->allocator, &state->swapchain);
    check(result, "vkCreateSwapchainKHR", __FILE__, __LINE__);

    uint32_t img_count = TARGET_FRAME_BUFFERS;
    VkImage  images[TARGET_FRAME_BUFFERS];
    vkGetSwapchainImagesKHR(state->device, state->swapchain, &img_count,
                            images);

    vkGetSwapchainImagesKHR(state->device, state->swapchain, &img_count,
                            images);
    for (uint32_t i = 0; i < TARGET_FRAME_BUFFERS; ++i) {
        state->frames[i].img = images[i];

        VkImageViewCreateInfo view_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = state->frames[i].img,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = state->surface_fmt.format,
            .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.levelCount = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount = 1,
        };
        vkCreateImageView(state->device, &view_info, state->allocator,
                          &state->frames[i].img_view);

        // Framebuffer
        VkFramebufferCreateInfo fb_ci = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = state->render_pass,
            .attachmentCount = 1,
            .pAttachments = &state->frames[i].img_view,
            .width = state->sc_width,
            .height = state->sc_height,
            .layers = 1,
        };
        vkCreateFramebuffer(state->device, &fb_ci, state->allocator,
                            &state->frames[i].frame_buf);

        // Command Buffer
        VkCommandBufferAllocateInfo alloc_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = state->pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        vkAllocateCommandBuffers(state->device, &alloc_info,
                                 &state->frames[i].cmd_buf);

        VkSemaphoreCreateInfo sem_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        VkFenceCreateInfo fence_info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,  // Start signaled so first
                                                    // wait doesn't stall
        };

        vkCreateSemaphore(state->device, &sem_info, state->allocator,
                          &state->frames[i].sem_render_start);
        vkCreateSemaphore(state->device, &sem_info, state->allocator,
                          &state->frames[i].sem_render_finished);
        vkCreateFence(state->device, &fence_info, state->allocator,
                      &state->frames[i].inflight_fence);
    }
}

static void destroy_swapchain(struct vulkan_bknd* state) {
    for (size_t i = 0; i < TARGET_FRAME_BUFFERS; i++) {
        vkDestroySemaphore(state->device, state->frames[i].sem_render_finished,
                           state->allocator);
        vkDestroySemaphore(state->device, state->frames[i].sem_render_start,
                           state->allocator);
        vkDestroyFence(state->device, state->frames[i].inflight_fence,
                       state->allocator);
        vkFreeCommandBuffers(state->device, state->pool, 1,
                             &state->frames[i].cmd_buf);
        vkDestroyImageView(state->device, state->frames[i].img_view,
                           state->allocator);
        vkDestroyFramebuffer(state->device, state->frames[i].frame_buf,
                             state->allocator);
    }
    vkDestroySwapchainKHR(state->device, state->swapchain, state->allocator);
}
