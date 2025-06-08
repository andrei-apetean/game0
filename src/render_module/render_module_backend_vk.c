#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

#include "render_module_backend.h"

// todo temp
#include "modules/window_module.h"

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
void        create_wl_surface(VkInstance instance, VkAllocationCallbacks* alloc,
                              VkSurfaceKHR* surface, void* win_handle);
static void check(VkResult res, const char* msg, const char* file, size_t line);
static int32_t get_family_queue(backend_vk* state, VkPhysicalDevice dev);
static void    create_swapchain(backend_vk* state, VkExtent2D size);
static void    destroy_swapchain(backend_vk* state);
static void    create_graphics_pipeline(backend_vk* state);
static VkVertexInputBindingDescription get_binding_description();
static void get_attribute_descriptions(VkVertexInputAttributeDescription* attr);

////// -------------------------- interface implementation;
int32_t startup_vk(render_module_state* state) {
    backend_vk* backend = (backend_vk*)state;
    assert(backend);
    backend->allocator = NULL;
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
        // todo, window api
        requested_extensions[1] = WL_SURFACE_EXT_NAME;

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
            vkCreateInstance(&inst_ci, backend->allocator, &backend->instance);
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
                    backend->instance, "vkCreateDebugUtilsMessengerEXT");
            if (create_debug_msgr != NULL) {
                VkResult result =
                    create_debug_msgr(backend->instance, &dbg_ci,
                                      backend->allocator, &backend->dbg_msgr);
                check(result, "vkCreateDebugUtilsMessengerEXT", __FILE__,
                      __LINE__);
            }

        } else {
            printf("Debug utils messenger extension not present!\n");
        }
    }
#endif  // _DEBUG

    //----------- Surface
    // todo: window api
    void* window_handle = window_get_native_handle();
    create_wl_surface(backend->instance, backend->allocator, &backend->surface,
                      window_handle);
    assert(backend->surface != VK_NULL_HANDLE &&
           "Failed to create window surface");

    //----------- Physical Device
    {
        VkResult result;
        uint32_t dev_count = 0;
        result =
            vkEnumeratePhysicalDevices(backend->instance, &dev_count, NULL);
        check(result, "VkEnumeratePhysicalDevices", __FILE__, __LINE__);
        VkPhysicalDevice* devs = malloc(dev_count * sizeof(VkPhysicalDevice));
        result =
            vkEnumeratePhysicalDevices(backend->instance, &dev_count, devs);
        check(result, "VkEnumeratePhysicalDevices", __FILE__, __LINE__);

        VkPhysicalDevice discrete_gpu = VK_NULL_HANDLE;
        VkPhysicalDevice integrated_gpu = VK_NULL_HANDLE;

        VkPhysicalDeviceProperties dev_props;
        for (size_t i = 0; i < dev_count; i++) {
            vkGetPhysicalDeviceProperties(devs[i], &dev_props);
            if (dev_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                if (get_family_queue(backend, devs[i])) {
                    discrete_gpu = devs[i];
                    break;
                }
            }
            if (dev_props.deviceType ==
                VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
                if (get_family_queue(backend, devs[i])) {
                    integrated_gpu = devs[i];
                    break;
                }
            }
        }

        free(devs);
        if (discrete_gpu != VK_NULL_HANDLE) {
            backend->gpu = discrete_gpu;
        } else if (integrated_gpu != VK_NULL_HANDLE) {
            backend->gpu = integrated_gpu;
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
        queue_ci[0].queueFamilyIndex = backend->queue_family_idx;
        queue_ci[0].queueCount = 1;
        queue_ci[0].pQueuePriorities = queue_priority;

        VkPhysicalDeviceFeatures2 features = {
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        };

        vkGetPhysicalDeviceFeatures2(backend->gpu, &features);

        VkDeviceCreateInfo dev_ci = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = sizeof(queue_ci) / sizeof(*queue_ci),
            .pQueueCreateInfos = queue_ci,
            .enabledExtensionCount = sizeof(dev_exts) / sizeof(*dev_exts),
            .ppEnabledExtensionNames = dev_exts,
            .pNext = &features,
        };
        VkResult result = vkCreateDevice(backend->gpu, &dev_ci,
                                         backend->allocator, &backend->device);
        check(result, "vkCreateDevice", __FILE__, __LINE__);

        vkGetDeviceQueue(backend->device, backend->queue_family_idx, 0,
                         &backend->queue);
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
        vkGetPhysicalDeviceSurfaceFormatsKHR(backend->gpu, backend->surface,
                                             &supported_count, NULL);
        VkSurfaceFormatKHR* formats =
            malloc(supported_count * sizeof(VkSurfaceFormatKHR));

        vkGetPhysicalDeviceSurfaceFormatsKHR(backend->gpu, backend->surface,
                                             &supported_count, formats);

        uint32_t       format_found = 0;
        const uint32_t desired_fmt_count =
            sizeof(s_requested_layers) / sizeof(*s_requested_layers);
        for (size_t i = 0; i < desired_fmt_count; i++) {
            for (size_t j = 0; j < supported_count; j++) {
                if (formats[j].format == desired_fmts[i] &&
                    formats[j].colorSpace == color_space) {
                    backend->surface_fmt = formats[j];
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
            backend->surface_fmt = formats[0];
        }
        free(formats);
    }

    //----------- Present Mode
    {
        uint32_t         supported_count = 0;
        VkPresentModeKHR supported[8];
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            backend->gpu, backend->surface, &supported_count, NULL);

        vkGetPhysicalDeviceSurfacePresentModesKHR(
            backend->gpu, backend->surface, &supported_count, supported);
        uint32_t         found = 0;
        VkPresentModeKHR requested = VK_PRESENT_MODE_MAILBOX_KHR;
        for (size_t i = 0; i < supported_count; i++) {
            if (requested == supported[i]) {
                found = 1;
                break;
            }
        }
        backend->present_mode = found ? requested : VK_PRESENT_MODE_FIFO_KHR;
    }

    //----------- Render pass
    {
        VkAttachmentDescription color_attachment = {
            .format = backend->surface_fmt.format,
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
            vkCreateRenderPass(backend->device, &render_pass_info,
                               backend->allocator, &backend->render_pass);
        check(result, "vkCreateRenderPass", __FILE__, __LINE__);
    }

    //----------- Command Pool
    {
        VkCommandPoolCreateInfo pool_ci = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = backend->queue_family_idx,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        };
        VkResult result = vkCreateCommandPool(
            backend->device, &pool_ci, backend->allocator, &backend->pool);
        check(result, "vkCreateCommandPool", __FILE__, __LINE__);
    }

    //----------- Swapchain
    {
        // todo
        uint32_t width = 0;
        uint32_t height = 0;
        window_get_size(&width, &height);
        VkExtent2D size = {width, height};
        create_swapchain(backend, size);
    }

    //----------- Graphics Pipeline
    {
        create_graphics_pipeline(backend);
    }
    return 0;
}

int32_t get_backend_state_size_vk() { return sizeof(backend_vk); }

void render_begin_vk(render_module_state* state) {
    backend_vk* backend = (backend_vk*)state;
    assert(backend);

    // Wait for previous frame to finish before starting this one
    vkWaitForFences(backend->device, 1,
                    &backend->frames[backend->current_frame].inflight_fence,
                    VK_TRUE, UINT64_MAX);
    vkResetFences(backend->device, 1,
                  &backend->frames[backend->current_frame].inflight_fence);

    // Acquire the next swapchain image
    uint32_t image_index;
    VkResult result = vkAcquireNextImageKHR(
        backend->device, backend->swapchain, UINT64_MAX,
        backend->frames[backend->current_frame].sem_render_start,
        VK_NULL_HANDLE, &image_index);
    if (result != VK_SUCCESS) {
        // handle swapchain recreation or errors here
        return;
    }

    backend->frames[backend->current_frame].image_index = image_index;

    VkCommandBuffer cmd = backend->frames[backend->current_frame].cmd_buf;

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
        .renderPass = backend->render_pass,
        .framebuffer = backend->frames[image_index].frame_buf,
        .renderArea =
            {
                .offset = {0, 0},
                .extent = {backend->sc_width, backend->sc_height},
            },
        .clearValueCount = 1,

        .pClearValues = &clear_color,
    };

    vkCmdBeginRenderPass(cmd, &rp_info, VK_SUBPASS_CONTENTS_INLINE);
}

void render_end_vk(render_module_state* state) {
    backend_vk* backend = (backend_vk*)state;
    assert(backend);

    uint32_t image_index = backend->frames[backend->current_frame].image_index;

    VkCommandBuffer cmd = backend->frames[backend->current_frame].cmd_buf;

    vkCmdEndRenderPass(cmd);
    vkEndCommandBuffer(cmd);

    VkSemaphore waitSemaphores[] = {
        backend->frames[backend->current_frame].sem_render_start};
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {
        backend->frames[backend->current_frame].sem_render_finished};

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

    vkQueueSubmit(backend->queue, 1, &submit_info,
                  backend->frames[backend->current_frame].inflight_fence);

    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphores,
        .swapchainCount = 1,
        .pSwapchains = &backend->swapchain,
        .pImageIndices = &image_index,
    };

    vkQueuePresentKHR(backend->queue, &present_info);

    backend->current_frame =
        (backend->current_frame + 1) % TARGET_FRAME_BUFFERS;
}

void resize_vk(render_module_state* state, uint32_t width, uint32_t height) {
    
    backend_vk* backend = (backend_vk*)state;
    assert(backend);
    vkDeviceWaitIdle(backend->device);
    destroy_swapchain(backend);
    VkExtent2D size = (VkExtent2D){width, height};
    create_swapchain(backend, size);
}

void teardown_vk(render_module_state* state) {
    backend_vk* backend = (backend_vk*)state;
    assert(backend);
    vkDeviceWaitIdle(backend->device);
    vkDestroyPipelineLayout(backend->device, backend->pipeline_layout,
                            backend->allocator);
    vkDestroyPipeline(backend->device, backend->pipeline, backend->allocator);
    vkDestroyRenderPass(backend->device, backend->render_pass,
                        backend->allocator);
    destroy_swapchain(backend);
    vkDestroyCommandPool(backend->device, backend->pool, backend->allocator);
    vkDestroyDevice(backend->device, backend->allocator);

    vkDestroySurfaceKHR(backend->instance, backend->surface,
                        backend->allocator);
#ifdef _DEBUG
    PFN_vkDestroyDebugUtilsMessengerEXT destroy_msgr =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            backend->instance, "vkDestroyDebugUtilsMessengerEXT");
    destroy_msgr(backend->instance, backend->dbg_msgr, backend->allocator);
    vkDestroyInstance(backend->instance, backend->allocator);
#endif  // _DEBUG
}

void load_render_module_backend_vk(render_module_backend* backend) {
    backend->startup = startup_vk;
    backend->teardown = teardown_vk;
    backend->get_backend_state_size = get_backend_state_size_vk;
    backend->render_begin = render_begin_vk;
    backend->render_end = render_end_vk;
    backend->resize = resize_vk;
}

////// -------------------------- internal implementation;

struct vertex {
    float pos[3];
    float col[3];
};

static VkVertexInputBindingDescription get_binding_description() {
    return (VkVertexInputBindingDescription){
        .binding = 0,
        .stride = sizeof(struct vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };
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

static void get_attribute_descriptions(
    VkVertexInputAttributeDescription* attr) {
    attr[0] = (VkVertexInputAttributeDescription){
        .binding = 0,
        .location = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(struct vertex, pos),
    };
    attr[1] = (VkVertexInputAttributeDescription){
        .binding = 0,
        .location = 1,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(struct vertex, col),
    };
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
    float               mat4[16];
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

#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_wayland.h>

typedef struct {
    struct wl_display* display;
    struct wl_surface* surface;
} window_handle_wl;
void create_wl_surface(VkInstance instance, VkAllocationCallbacks* alloc,
                       VkSurfaceKHR* surface, void* win_handle) {
    window_handle_wl* win = (window_handle_wl*)win_handle;

    VkWaylandSurfaceCreateInfoKHR ci = {
        .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
        .display = win->display,
        .surface = win->surface,
    };
    *surface = VK_NULL_HANDLE;
    vkCreateWaylandSurfaceKHR(instance, &ci, alloc, surface);
};
