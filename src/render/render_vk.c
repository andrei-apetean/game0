#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "render/render.h"
#include "render/window_surface_vk.h"

// todo
#define CLAMP(x, min, max) (((x) < (min)) ? (min) : ((x) > (max)) ? (max) : (x))

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
        // ASSERT(0); // todo
    }

    return VK_FALSE;
}

typedef struct {
    VkBuffer       vertex_buffer;
    VkDeviceMemory vertex_memory;
    VkBuffer       index_buffer;
    VkDeviceMemory index_memory;
    uint32_t       index_count;
} gpu_mesh;

typedef struct {
    VkImage         img;
    VkImageView     img_view;
    VkCommandBuffer cmd_buf;
    VkFramebuffer   frame_buf;
    VkSemaphore     sem_render_start;
    VkSemaphore     sem_render_finished;
    VkFence         inflight_fence;
    uint32_t        image_index;
} frame;

typedef struct {
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
    VkPipelineLayout       pipeline_layout;
    VkPipeline             pipeline;
    uint32_t               current_frame;
    uint32_t               queue_family_idx;
    uint16_t               sc_width;
    uint16_t               sc_height;

    frame frames[TARGET_FRAME_BUFFERS];
#ifdef _DEBUG
    VkDebugUtilsMessengerEXT dbg_msgr;
#endif
} backend_vk;

static const char* s_requested_layers[] = {
#ifdef _DEBUG
    "VK_LAYER_KHRONOS_validation",
#else
    ""
#endif  // _DEBUG
};

////// -------------------------- forward declarations;
//
static void check(VkResult res, const char* msg, const char* file, size_t line);
static int32_t get_family_queue(backend_vk* state, VkPhysicalDevice dev);
static void    create_swapchain(backend_vk* state, VkExtent2D size);
static void    destroy_swapchain(backend_vk* state);
static void    create_graphics_pipeline(backend_vk* state);
static VkVertexInputBindingDescription get_binding_description();
static void get_attribute_descriptions(VkVertexInputAttributeDescription* attr);

static VkShaderModule load_shader_module(VkDevice device, const char* filename,
                                         VkAllocationCallbacks* allocator);

static int32_t create_buffer(backend_vk* state, VkDeviceSize size,
                             VkBufferUsageFlags    usage,
                             VkMemoryPropertyFlags properties, VkBuffer* buffer,
                             VkDeviceMemory* buffer_memory);

static uint32_t find_memory_type(backend_vk* state, uint32_t type_filter,
                                 VkMemoryPropertyFlags properties);
static void get_attribute_descriptions(VkVertexInputAttributeDescription* attr);
static void upload_to_gpu(backend_vk* state, void* data, VkDeviceSize size,
                          VkBuffer dst_buffer);
static void gpu_mesh_create(backend_vk* state, mesh* src, gpu_mesh* out);

////// -------------------------- interface implementation;
//
uint32_t render_get_size() { return sizeof(backend_vk); }

int32_t render_startup(render** bknd, render_cfg cfg) {
    backend_vk* state = (backend_vk*)*bknd;
    assert(state);
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
                assert(0 && "Unknown window API!\n");
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
            .enabledLayerCount =
                sizeof(s_requested_layers) / sizeof(*s_requested_layers),
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
                              &state->surface, cfg.win_handle);
            break;
        default:
            assert(0 && "Unknown window API\n");
    }
    assert(state->surface != VK_NULL_HANDLE &&
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
            .queueCreateInfoCount = sizeof(queue_ci) / sizeof(*queue_ci),
            .pQueueCreateInfos = queue_ci,
            .enabledExtensionCount = sizeof(dev_exts) / sizeof(*dev_exts),
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
        const uint32_t desired_fmt_count =
            sizeof(s_requested_layers) / sizeof(*s_requested_layers);
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

    //----------- Graphics Pipeline
    {
        create_graphics_pipeline(state);
    }
    return 0;
}

void render_begin(render* bknd) {
    backend_vk* state = (backend_vk*)bknd;
    assert(state);

    // Wait for previous frame to finish before starting this one
    vkWaitForFences(state->device, 1,
                    &state->frames[state->current_frame].inflight_fence,
                    VK_TRUE, UINT64_MAX);
    vkResetFences(state->device, 1,
                  &state->frames[state->current_frame].inflight_fence);

    // Acquire the next swapchain image
    uint32_t image_index;
    VkResult result = vkAcquireNextImageKHR(
        state->device, state->swapchain, UINT64_MAX,
        state->frames[state->current_frame].sem_render_start, VK_NULL_HANDLE,
        &image_index);
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

    VkClearValue clear_color = {.color = {{0.0941f, 0.0941f, 0.0941f, 1.0f}}};

    VkRenderPassBeginInfo rp_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = state->render_pass,
        .framebuffer = state->frames[image_index].frame_buf,
        .renderArea =
            {
                .offset = {0, 0},
                .extent = {state->sc_width, state->sc_height},
            },
        .clearValueCount = 1,

        .pClearValues = &clear_color,
    };

    vkCmdBeginRenderPass(cmd, &rp_info, VK_SUBPASS_CONTENTS_INLINE);

    // Ready to record draw calls here...
}

void render_end(render* bknd) {
    backend_vk* state = (backend_vk*)bknd;
    assert(state);

    uint32_t image_index = state->frames[state->current_frame].image_index;

    VkCommandBuffer cmd = state->frames[state->current_frame].cmd_buf;

    vkCmdEndRenderPass(cmd);
    vkEndCommandBuffer(cmd);

    VkSemaphore waitSemaphores[] = {
        state->frames[state->current_frame].sem_render_start};
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {
        state->frames[state->current_frame].sem_render_finished};

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

void render_mesh(render* bknd, mesh* m) {
    backend_vk* state = (backend_vk*)bknd;
    assert(state);
    VkCommandBuffer cmd = state->frames[state->current_frame].cmd_buf;

    if (!m->uploaded) {
        m->uploaded = malloc(sizeof(gpu_mesh));
        gpu_mesh_create(state, m, m->uploaded);
    }
    gpu_mesh* mesh = m->uploaded;

    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, &mesh->vertex_buffer, offsets);
    vkCmdBindIndexBuffer(cmd, mesh->index_buffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, state->pipeline);

    vkCmdPushConstants(cmd, state->pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT,
                       0, sizeof(mat4), &m->mvp);
    vkCmdDrawIndexed(cmd, mesh->index_count, 1, 0, 0, 0);
}

void render_resize(render* bknd, vec2 dimensions) {
    backend_vk* state = (backend_vk*)bknd;
    assert(state);
    vkDeviceWaitIdle(state->device);
    destroy_swapchain(state);
    VkExtent2D size = (VkExtent2D){dimensions.x, dimensions.y};
    create_swapchain(state, size);
}

void render_teardown(render* bknd) {
    backend_vk* state = (backend_vk*)bknd;
    assert(state);
    vkDeviceWaitIdle(state->device);
    vkDestroyPipelineLayout(state->device, state->pipeline_layout,
                            state->allocator);
    vkDestroyPipeline(state->device, state->pipeline, state->allocator);
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
        assert(0);
    } else {
        printf("Success `%s`: %d in %s:%zu\n", msg, result, file, line);
    }
}

static int32_t get_family_queue(backend_vk* state, VkPhysicalDevice dev) {
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

static void create_swapchain(backend_vk* state, VkExtent2D size) {
    VkSurfaceCapabilitiesKHR surf_caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(state->gpu, state->surface,
                                              &surf_caps);

    VkExtent2D sc_extent = surf_caps.currentExtent;
    if (sc_extent.width == UINT32_MAX) {
        sc_extent.width = size.width;
        sc_extent.height = size.height;

        sc_extent.width = CLAMP(sc_extent.width, surf_caps.minImageExtent.width,
                                surf_caps.maxImageExtent.width);
        sc_extent.height =
            CLAMP(sc_extent.height, surf_caps.minImageExtent.height,
                  surf_caps.maxImageExtent.height);
    }

    state->sc_width = sc_extent.width;
    state->sc_height = sc_extent.height;

    VkSurfaceTransformFlagBitsKHR pre_transform =
        (surf_caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
            ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
            : surf_caps.currentTransform;

    uint32_t desired_img_count = TARGET_FRAME_BUFFERS;
    if (desired_img_count < surf_caps.minImageCount)
        desired_img_count = surf_caps.minImageCount;
    else if (surf_caps.maxImageCount > 0 &&
             desired_img_count > surf_caps.maxImageCount)
        desired_img_count = surf_caps.maxImageCount;

    VkSwapchainCreateInfoKHR sc_ci = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = state->surface,
        .minImageCount = desired_img_count,
        .imageFormat = state->surface_fmt.format,
        .imageColorSpace = state->surface_fmt.colorSpace,
        .imageExtent = sc_extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                      VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = pre_transform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = state->present_mode,
        .clipped = VK_TRUE,
    };

    check(vkCreateSwapchainKHR(state->device, &sc_ci, state->allocator,
                               &state->swapchain),
          "vkCreateSwapchainKHR", __FILE__, __LINE__);

    uint32_t img_count = 0;
    vkGetSwapchainImagesKHR(state->device, state->swapchain, &img_count, NULL);

    VkImage* images = malloc(img_count * sizeof(VkImage));
    check(images != NULL ? VK_SUCCESS : VK_ERROR_OUT_OF_HOST_MEMORY,
          "malloc swapchain images", __FILE__, __LINE__);

    vkGetSwapchainImagesKHR(state->device, state->swapchain, &img_count,
                            images);
    assert(img_count <= TARGET_FRAME_BUFFERS);

    for (uint32_t i = 0; i < img_count; ++i) {
        state->frames[i].img = images[i];

        // Create image view
        VkImageViewCreateInfo view_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = state->frames[i].img,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = state->surface_fmt.format,
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
        check(vkCreateImageView(state->device, &view_info, state->allocator,
                                &state->frames[i].img_view),
              "vkCreateImageView", __FILE__, __LINE__);

        // Create framebuffer
        VkFramebufferCreateInfo fb_ci = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = state->render_pass,
            .attachmentCount = 1,
            .pAttachments = &state->frames[i].img_view,
            .width = state->sc_width,
            .height = state->sc_height,
            .layers = 1,
        };
        check(vkCreateFramebuffer(state->device, &fb_ci, state->allocator,
                                  &state->frames[i].frame_buf),
              "vkCreateFramebuffer", __FILE__, __LINE__);

        // Allocate command buffer
        VkCommandBufferAllocateInfo alloc_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = state->pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        check(vkAllocateCommandBuffers(state->device, &alloc_info,
                                       &state->frames[i].cmd_buf),
              "vkAllocateCommandBuffers", __FILE__, __LINE__);

        // Semaphores and fence
        VkSemaphoreCreateInfo sem_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VkFenceCreateInfo fence_info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };
        check(vkCreateSemaphore(state->device, &sem_info, state->allocator,
                                &state->frames[i].sem_render_start),
              "vkCreateSemaphore (start)", __FILE__, __LINE__);
        check(vkCreateSemaphore(state->device, &sem_info, state->allocator,
                                &state->frames[i].sem_render_finished),
              "vkCreateSemaphore (finished)", __FILE__, __LINE__);
        check(vkCreateFence(state->device, &fence_info, state->allocator,
                            &state->frames[i].inflight_fence),
              "vkCreateFence", __FILE__, __LINE__);
    }

    free(images);
}

static void destroy_swapchain(backend_vk* state) {
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
    check(res, "vkCreateShaderModule", __FILE__, __LINE__);
    free(code);
    return module;
}

void create_graphics_pipeline(backend_vk* state) {
    VkDevice device = state->device;

    // Load shader modules
    VkShaderModule vert = load_shader_module(
        device, "./bin/assets/shaders/shader.vert.spv", state->allocator);
    VkShaderModule frag = load_shader_module(
        device, "./bin/assets/shaders/shader.frag.spv", state->allocator);

    VkPipelineShaderStageCreateInfo stages[2] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vert,
            .pName = "main",
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = frag,
            .pName = "main",
        }};

    // Vertex input
    VkVertexInputBindingDescription   binding = get_binding_description();
    VkVertexInputAttributeDescription attr[2];
    get_attribute_descriptions(attr);

    VkPipelineVertexInputStateCreateInfo vertex_input = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &binding,
        .vertexAttributeDescriptionCount = 2,
        .pVertexAttributeDescriptions = attr,
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)state->sc_width,
        .height = (float)state->sc_height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = {state->sc_width, state->sc_height},
    };

    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
    };

    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .lineWidth = 1.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

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

    VkPushConstantRange push_constant_range = {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(mat4),
    };

    VkPipelineLayoutCreateInfo pipeline_layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_constant_range,
        .setLayoutCount = 0,
        .pSetLayouts = NULL,
    };

    VkResult res =
        vkCreatePipelineLayout(device, &pipeline_layout_info, state->allocator,
                               &state->pipeline_layout);
    check(res, "vkCreatePipelineLayout", __FILE__, __LINE__);

    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = stages,
        .pVertexInputState = &vertex_input,
        .pInputAssemblyState = &input_assembly,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pColorBlendState = &color_blending,
        .layout = state->pipeline_layout,
        .renderPass = state->render_pass,
        .subpass = 0,
    };

    res = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info,
                                    state->allocator, &state->pipeline);
    check(res, "vkCreateGraphicsPipelines", __FILE__, __LINE__);

    vkDestroyShaderModule(device, vert, state->allocator);
    vkDestroyShaderModule(device, frag, state->allocator);
}

static void gpu_mesh_create(backend_vk* state, mesh* src, gpu_mesh* out) {
    VkDeviceSize vbuf_size = sizeof(vertex) * src->vertex_count;
    VkDeviceSize ibuf_size = sizeof(uint16_t) * src->index_count;

    // Create vertex buffer
    create_buffer(
        state, vbuf_size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &out->vertex_buffer,
        &out->vertex_memory);

    // Create index buffer
    create_buffer(
        state, ibuf_size,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &out->index_buffer,
        &out->index_memory);

    // Upload data to staging buffer and copy to GPU
    upload_to_gpu(state, src->vertices, vbuf_size, out->vertex_buffer);
    upload_to_gpu(state, src->indices, ibuf_size, out->index_buffer);

    out->index_count = src->index_count;
}
static VkVertexInputBindingDescription get_binding_description() {
    return (VkVertexInputBindingDescription){
        .binding = 0,
        .stride = sizeof(vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };
}

static void get_attribute_descriptions(
    VkVertexInputAttributeDescription* attr) {
    attr[0] = (VkVertexInputAttributeDescription){
        .binding = 0,
        .location = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(vertex, pos),
    };
    attr[1] = (VkVertexInputAttributeDescription){
        .binding = 0,
        .location = 1,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(vertex, col),
    };
}

uint32_t find_memory_type(backend_vk* state, uint32_t type_filter,
                          VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(state->gpu, &mem_properties);

    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) &&
            (mem_properties.memoryTypes[i].propertyFlags & properties) ==
                properties) {
            return i;
        }
    }

    assert(0 && "Failed to find suitable memory type");
    return 0;
}

static int32_t create_buffer(backend_vk* state, VkDeviceSize size,
                             VkBufferUsageFlags    usage,
                             VkMemoryPropertyFlags properties, VkBuffer* buffer,
                             VkDeviceMemory* buffer_memory) {
    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VkResult result =
        vkCreateBuffer(state->device, &buffer_info, state->allocator, buffer);
    if (result != VK_SUCCESS) return -1;

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(state->device, *buffer, &mem_reqs);

    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_reqs.size,
        .memoryTypeIndex =
            find_memory_type(state, mem_reqs.memoryTypeBits, properties),
    };

    result = vkAllocateMemory(state->device, &alloc_info, state->allocator,
                              buffer_memory);
    if (result != VK_SUCCESS) return -1;

    vkBindBufferMemory(state->device, *buffer, *buffer_memory, 0);

    return 0;
}

static void upload_to_gpu(backend_vk* state, void* data, VkDeviceSize size,
                          VkBuffer dst_buffer) {
    VkBuffer       staging_buffer;
    VkDeviceMemory staging_memory;

    // Create staging buffer
    create_buffer(state, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  &staging_buffer, &staging_memory);

    // Map and copy data
    void* mapped;
    vkMapMemory(state->device, staging_memory, 0, size, 0, &mapped);
    memcpy(mapped, data, (size_t)size);
    vkUnmapMemory(state->device, staging_memory);

    // Begin a one-time command buffer
    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = state->pool,
        .commandBufferCount = 1,
    };
    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(state->device, &alloc_info, &cmd);

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
    vkCmdCopyBuffer(cmd, staging_buffer, dst_buffer, 1, &copy_region);
    vkEndCommandBuffer(cmd);

    // Submit and wait for completion
    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
    };
    vkQueueSubmit(state->queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(state->queue);  // You can improve perf later with a
                                    // dedicated transfer queue

    // Cleanup staging buffer
    vkFreeCommandBuffers(state->device, state->pool, 1, &cmd);
    vkDestroyBuffer(state->device, staging_buffer, state->allocator);
    vkFreeMemory(state->device, staging_memory, state->allocator);
}
