#pragma once
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "common.h"
#include "platform.h"
#include "render.h"
#include "stdlib.h"
#include "vulkan_platform.h"

#define CHECK_VK_RESULT(_expr)                              \
    result = _expr;                                         \
    if (result != VK_SUCCESS) {                             \
        printf("Error executing %s: %i\n", #_expr, result); \
        return false;                                       \
    }

#define INST_EXT_COUNT 2
#define DEV_EXT_COUNT 1
#define LAYER_COUNT 1
#define TARGET_SC_FRAMES 3

static VkBool32 on_error(VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
                         VkDebugUtilsMessageTypeFlagsEXT             type,
                         const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
                         void*                                       userData) {
    printf("Vulkan ");
    switch (type) {
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
            printf("general ");
            break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
            printf("validation ");
            break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
            printf("performance ");
            break;
    }

    switch (severity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            printf("(verbose): ");
            break;
        default:
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            printf("(info): ");
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            printf("(warning): ");
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            printf("(error): ");
            break;
    }

    printf("%s\n", callbackData->pMessage);
    return 0;
}

typedef struct frame_t {
    VkImage         img;
    VkImageView     imgv;
    VkCommandBuffer cmd_buf;
    VkFramebuffer   frame_buf;
    VkSemaphore     semaphore_render_start;
    VkSemaphore     semaphore_render_finish;
    VkFence         inflight_fence;
} frame_t;

typedef struct vulkan_t {
    VkInstance               inst;
    VkSurfaceKHR             surf;
    VkPhysicalDevice         pdev;
    VkDevice                 dev;
    VkQueue                  gfx_queue;
    VkCommandPool            cmd_pool;
    VkPipeline               pipeline;
    VkShaderModule           vert_shader;
    VkShaderModule           frag_shader;
    VkSwapchainKHR           swapchain;
    VkSurfaceFormatKHR       surf_fmt;
    VkPresentModeKHR         present_mode;
    VkRenderPass             render_pass;
    VkAllocationCallbacks*   alloc;
    VkDebugUtilsMessengerEXT debug_msgr;
    uint32_t                 present_family_idx;
    uint32_t                 sc_frame_count;
    uint32_t                 width;
    uint32_t                 height;
    uint32_t                 frame_idx;
    frame_t                  sc_frames[TARGET_SC_FRAMES];
} vulkan_t;

static vulkan_t vulkan = {0};

bool create_swapchain();
bool recreate_swapchain();
void destroy_swapchain();

bool create_shader_module(VkShaderModule* module, const char* file_name);
bool create_pipeline();

bool render_startup(win_data_t window) {
    printf("render_startup...\n");
    VkResult result = VK_FALSE;

    const char* const target_layer_names[LAYER_COUNT] = {
        "VK_LAYER_KHRONOS_validation",
    };

    {  // instance creation
        const char* surface_ext[window_api_count] = {
            [window_api_wayland] = "VK_KHR_wayland_surface",
            [window_api_xcb] = "VK_KHR_xcb_surface",
            [window_api_win32] = "VK_KHR_win32_surface",
        };

        const char* app_extension_names[INST_EXT_COUNT] = {
            "VK_EXT_debug_utils",
            "VK_KHR_surface",
        };
        const char*       app_extensions[INST_EXT_COUNT + 1];
        VkApplicationInfo app_info = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "game0",
            .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
            .pEngineName = "game0",
            .engineVersion = VK_MAKE_VERSION(0, 0, 1),
            .apiVersion = VK_MAKE_VERSION(1, 0, 0),
        };

        for (int i = 0; i < INST_EXT_COUNT; i++) {
            app_extensions[i] = app_extension_names[i];
        }
        switch (window.api) {
            case window_api_wayland:
                app_extensions[INST_EXT_COUNT] = surface_ext[window_api_wayland];
                break;
            case window_api_xcb:
                app_extensions[INST_EXT_COUNT] = surface_ext[window_api_xcb];
                break;
            case window_api_win32:
                app_extensions[INST_EXT_COUNT] = surface_ext[window_api_win32];
                break;
            default:
                break;
        }
        uint32_t available_instance_layer_count;
        CHECK_VK_RESULT(vkEnumerateInstanceLayerProperties(
            &available_instance_layer_count, NULL));

        VkLayerProperties* layer_properties =
            malloc(sizeof(VkLayerProperties) * available_instance_layer_count);
        if (layer_properties == NULL) {
            printf("VkLayerProperties allocation failed!\n");
            return false;
        }
        CHECK_VK_RESULT(vkEnumerateInstanceLayerProperties(
            &available_instance_layer_count, layer_properties));

        uint32_t    enabled_instance_layer_count = 0;
        const char* enabled_instance_layer_names[LAYER_COUNT];
        for (size_t i = 0; i < LAYER_COUNT; i++) {
            for (size_t j = 0; j < available_instance_layer_count; j++) {
                if (strcmp(target_layer_names[i],
                           layer_properties[j].layerName) == 0) {
                    enabled_instance_layer_names[enabled_instance_layer_count] =
                        target_layer_names[i];
                    enabled_instance_layer_count++;
                    printf("Found enabled layer: %s\n", target_layer_names[i]);
                    break;
                }
            }
        }
        free(layer_properties);
        VkInstanceCreateInfo ci_inst = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &app_info,
            .enabledLayerCount = enabled_instance_layer_count,
            .ppEnabledLayerNames = enabled_instance_layer_names,
            .enabledExtensionCount = INST_EXT_COUNT + 1,
            .ppEnabledExtensionNames = app_extensions,
        };

        CHECK_VK_RESULT(vkCreateInstance(&ci_inst, vulkan.alloc, &vulkan.inst));
    }
    {  // validation layer
        VkDebugUtilsMessengerCreateInfoEXT ci_debug_ext = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = on_error,
            .pUserData = 0,
        };

        PFN_vkCreateDebugUtilsMessengerEXT create_messanger =
            (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
                vulkan.inst, "vkCreateDebugUtilsMessengerEXT");

        if (create_messanger == NULL) {
            printf(
                "vkGetInstanceProcAddr(vkCreateDebugUtilsMessengerEXT) "
                "failed!\n");
        }

        CHECK_VK_RESULT(create_messanger(vulkan.inst, &ci_debug_ext, vulkan.alloc,
                                         &vulkan.debug_msgr));
    }
    // window surface
    if (!create_vk_surface(window, vulkan.inst, vulkan.alloc, &vulkan.surf)) {
        return false;
    }
    {  // physical device
        uint32_t device_count;
        CHECK_VK_RESULT(
            vkEnumeratePhysicalDevices(vulkan.inst, &device_count, NULL));

        VkPhysicalDevice devices[device_count];
        CHECK_VK_RESULT(
            vkEnumeratePhysicalDevices(vulkan.inst, &device_count, devices));
        uint32_t                   best_score = 0;
        VkPhysicalDeviceProperties selected_props;
        for (size_t i = 0; i < device_count; i++) {
            VkPhysicalDevice           d = devices[i];
            uint32_t                   score = 0;
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(d, &props);
            printf("Analyzing physical device: %s\n", props.deviceName);
            switch (props.deviceType) {
                case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                    score = 1;
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    score = 4;
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    score = 5;
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    score = 3;
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    score = 1;
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM:
                    score = 0;
                    break;
            }
            if (score > best_score) {
                best_score = score;
                vulkan.pdev = d;
            }
        }
        printf("Selected device: %s\n", selected_props.deviceName);
    }
    {  // queue selection && logical device
        uint32_t queue_family_count;
        vkGetPhysicalDeviceQueueFamilyProperties(vulkan.pdev, &queue_family_count,
                                                 NULL);

        VkQueueFamilyProperties queue_familiy_props[queue_family_count];
        vkGetPhysicalDeviceQueueFamilyProperties(vulkan.pdev, &queue_family_count,
                                                 queue_familiy_props);

        for (size_t i = 0; i < queue_family_count; i++) {
            VkBool32 present = VK_FALSE;
            CHECK_VK_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR(
                vulkan.pdev, i, vulkan.surf, &present));
            if (present == VK_TRUE &&
                (queue_familiy_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                vulkan.present_family_idx = i;
                break;
            }
        }
        float                   priority = 1.0f;
        VkDeviceQueueCreateInfo ci_queue = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = vulkan.present_family_idx,
            .queueCount = 1,  // todo
            .pQueuePriorities = &priority,
        };

        const char* device_ext_names[DEV_EXT_COUNT] = {"VK_KHR_swapchain"};
        uint32_t    device_layer_count;
        CHECK_VK_RESULT(vkEnumerateDeviceLayerProperties(
            vulkan.pdev, &device_layer_count, NULL));
        uint32_t           enabled_layer_count = 0;
        const char*        device_layer_names[LAYER_COUNT];
        VkLayerProperties* properties =
            malloc(sizeof(VkLayerProperties) * device_layer_count);

        if (properties == NULL) {
            printf("Allocating memory for device layer properties failed!\n");
            return false;
        }

        CHECK_VK_RESULT(vkEnumerateDeviceLayerProperties(
            vulkan.pdev, &device_layer_count, properties));

        uint32_t    enabled_device_layer_count = 0;
        const char* enabled_device_layers[LAYER_COUNT];
        for (size_t i = 0; i < LAYER_COUNT; i++) {
            for (size_t j = 0; j < device_layer_count; j++) {
                if (strcmp(target_layer_names[i], properties[j].layerName) == 0) {
                    enabled_device_layers[enabled_device_layer_count] =
                        target_layer_names[i];
                    enabled_device_layer_count++;
                }
            }
        }
        free(properties);
        VkDeviceCreateInfo ci_dev = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .flags = 0,
            .queueCreateInfoCount = 1,  // todo
            .pQueueCreateInfos = &ci_queue,
            .enabledExtensionCount = DEV_EXT_COUNT,
            .ppEnabledExtensionNames = device_ext_names,
            .enabledLayerCount = enabled_device_layer_count,
            .ppEnabledLayerNames = enabled_device_layers,
        };
        CHECK_VK_RESULT(
            vkCreateDevice(vulkan.pdev, &ci_dev, vulkan.alloc, &vulkan.dev));
        vkGetDeviceQueue(vulkan.dev, vulkan.present_family_idx, 0,
                         &vulkan.gfx_queue);
    }

    VkCommandPoolCreateInfo ci_pool = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = vulkan.present_family_idx,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT};
    result =
        vkCreateCommandPool(vulkan.dev, &ci_pool, vulkan.alloc, &vulkan.cmd_pool);
    if (!create_swapchain()) {
        return false;
    }
   // if (!create_shader_module(&vulkan.vert_shader, "./shaders/vert.spirv")) {
   //     return false;
   // }
   // if (!create_shader_module(&vulkan.vert_shader, "./shaders/frag.spirv")) {
   //     return false;
   // }

    printf("render_startup successful!\n");
    return true;
}

bool render_update(render_paket_t* paket) {
    frame_t frame = vulkan.sc_frames[vulkan.frame_idx];
    vkWaitForFences(vulkan.dev, 1, &frame.inflight_fence, VK_TRUE, UINT64_MAX);
    uint32_t image_idx;
    VkResult result = vkAcquireNextImageKHR(
        vulkan.dev, vulkan.swapchain, UINT64_MAX, frame.semaphore_render_start,
        VK_NULL_HANDLE, &image_idx);
    // printf("Current: %d Acquired: %d\n", vulkan.frame_idx, image_idx);
    bool should_recreate_sc =
        paket->width != vulkan.width || paket->height != vulkan.height;
    if (result == VK_ERROR_OUT_OF_DATE_KHR || should_recreate_sc) {
        // printf("Attempting to recreate swapchain!\n");
        vulkan.height = paket->height;
        vulkan.width = paket->width;
        recreate_swapchain();
        paket->commit_surface = true;
        return true;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        printf("Error: Failed to acquire next swapchain image!");
    }
    vkResetFences(vulkan.dev, 1, &frame.inflight_fence);

    vkResetCommandBuffer(frame.cmd_buf, 0);
    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };

    if (vkBeginCommandBuffer(frame.cmd_buf, &begin_info) != VK_SUCCESS) {
        printf("Error: Failed to begin recording into vulkan command buffer!\n");
    }

    VkClearValue clear_color = {{0.094f, 0.094f, 0.094f, 1.0f}};

    VkRenderPassBeginInfo ci_renderpass = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = vulkan.render_pass,
        .framebuffer = frame.frame_buf,
        .renderArea.offset = (VkOffset2D){0, 0},
        .renderArea.extent = (VkExtent2D){vulkan.width, vulkan.height},
        .clearValueCount = 1,
        .pClearValues = &clear_color,
    };

    vkCmdBeginRenderPass(frame.cmd_buf, &ci_renderpass,
                         VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {
        viewport.x = 0.0f,
        viewport.y = 0.0f,
        viewport.width = (float)vulkan.width,
        viewport.height = (float)vulkan.height,
        viewport.minDepth = 0.0f,
        viewport.maxDepth = 1.0f,
    };
    vkCmdSetViewport(frame.cmd_buf, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = (VkOffset2D){0, 0},
        .extent = (VkExtent2D){vulkan.width, vulkan.height},
    };
    vkCmdSetScissor(frame.cmd_buf, 0, 1, &scissor);

    // vkCmdDraw(buffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(frame.cmd_buf);

    if (vkEndCommandBuffer(frame.cmd_buf) != VK_SUCCESS) {
        printf("Error: Failed to begin recording into vulkan command buffer!\n");
        return false;
    }

    VkSemaphore          wait_semaphores[] = {frame.semaphore_render_start};
    VkSemaphore          signal_semaphores[] = {frame.semaphore_render_finish};
    VkPipelineStageFlags wait_stages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = wait_semaphores,
        .pWaitDstStageMask = wait_stages,
        .commandBufferCount = 1,
        .pCommandBuffers = &frame.cmd_buf,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signal_semaphores,
    };

    if (vkQueueSubmit(vulkan.gfx_queue, 1, &submit_info, frame.inflight_fence) !=
        VK_SUCCESS) {
        printf("Error: Vulkan queue submission failed!\n");
    }

    VkSwapchainKHR   swapchains[] = {vulkan.swapchain};
    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signal_semaphores,
        .swapchainCount = 1,
        .pSwapchains = swapchains,
        .pImageIndices = &image_idx,
    };

    result = vkQueuePresentKHR(vulkan.gfx_queue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreate_swapchain();
        paket->commit_surface = true;
        return true;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        printf("Error: Failed to present to VkQueue!");
    }
    vulkan.frame_idx = (vulkan.frame_idx + 1) % vulkan.sc_frame_count;

    return true;
}

bool render_shutdown() {
    printf("render_shutdown...\n");
    destroy_swapchain();
    vkDestroyCommandPool(vulkan.dev, vulkan.cmd_pool, vulkan.alloc);
    vkDestroyDevice(vulkan.dev, vulkan.alloc);
    vkDestroySurfaceKHR(vulkan.inst, vulkan.surf, vulkan.alloc);

    PFN_vkDestroyDebugUtilsMessengerEXT destroy_messenger =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            vulkan.inst, "vkDestroyDebugUtilsMessengerEXT");

    if (destroy_messenger == NULL) {
        printf(
            "vkGetInstanceProcAddr(vkDestroyDebugUtilsMessengerEXT) "
            "failed!\n");
    }

    destroy_messenger(vulkan.inst, vulkan.debug_msgr, vulkan.alloc);

    vkDestroyInstance(vulkan.inst, vulkan.alloc);
    printf("render_shutdown successful!\n");
    return true;
}

bool create_swapchain() {
    VkResult                 result;
    VkSurfaceCapabilitiesKHR surface_caps;
    CHECK_VK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        vulkan.pdev, vulkan.surf, &surface_caps));
    vulkan.sc_frame_count =
        surface_caps.minImageCount + 1 < surface_caps.maxImageCount
            ? surface_caps.minImageCount + 1
            : surface_caps.maxImageCount;
    vulkan.width = surface_caps.currentExtent.width == ((uint32_t)-1)
                       ? 1080
                       : surface_caps.currentExtent.width;
    vulkan.height = surface_caps.currentExtent.height == ((uint32_t)-1)
                        ? 720
                        : surface_caps.currentExtent.height;

    uint32_t surface_fmt_count;
    CHECK_VK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(
        vulkan.pdev, vulkan.surf, &surface_fmt_count, NULL));

    VkSurfaceFormatKHR surface_fmts[surface_fmt_count];
    CHECK_VK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(
        vulkan.pdev, vulkan.surf, &surface_fmt_count, surface_fmts));
    vulkan.surf_fmt = surface_fmts[0];
    for (size_t i = 0; i < surface_fmt_count; i++) {
        if (surface_fmts[i].format == VK_FORMAT_B8G8R8_UNORM) {
            vulkan.surf_fmt = surface_fmts[i];
            break;
        }
    }
    uint32_t present_mode_count;
    CHECK_VK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(
        vulkan.pdev, vulkan.surf, &present_mode_count, NULL));
    VkPresentModeKHR present_modes[present_mode_count];
    CHECK_VK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(
        vulkan.pdev, vulkan.surf, &present_mode_count, present_modes));

    bool present_mode_found = false;
    for (size_t i = 0; i < present_mode_count; i++) {
        if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            vulkan.present_mode = present_modes[i];
            break;
        }
    }
    if (present_mode_found == false) {
        vulkan.present_mode = VK_PRESENT_MODE_FIFO_KHR;
    }
    return recreate_swapchain();
}

bool recreate_swapchain() {
    destroy_swapchain();

    printf("Recreating swapchain: %d x %d\n", vulkan.width, vulkan.height);
    VkResult                 result;
    VkSwapchainCreateInfoKHR ci_sc = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = vulkan.surf,
        .minImageCount = vulkan.sc_frame_count,
        .imageFormat = vulkan.surf_fmt.format,
        .imageColorSpace = vulkan.surf_fmt.colorSpace,
        .imageExtent = (VkExtent2D){vulkan.width, vulkan.height},
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = vulkan.present_mode,
        .clipped = 1,
        .oldSwapchain = VK_NULL_HANDLE,
    };
    result =
        vkCreateSwapchainKHR(vulkan.dev, &ci_sc, vulkan.alloc, &vulkan.swapchain);

    VkAttachmentDescription attachment = {
        .format = vulkan.surf_fmt.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference attachment_ref = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachment_ref,
    };

    VkRenderPassCreateInfo ci_reder_pass = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
    };

    result =
        vkCreateRenderPass(vulkan.dev, &ci_reder_pass, NULL, &vulkan.render_pass);
    uint32_t img_count;
    result =
        vkGetSwapchainImagesKHR(vulkan.dev, vulkan.swapchain, &img_count, NULL);

    // printf("Image count: %d\n", img_count);
    // printf("Swapchain frame count: %d\n", vulkan.sc_frame_count);
    if (img_count > TARGET_SC_FRAMES || img_count != vulkan.sc_frame_count) {
        printf(
            "Somehow we ended up with more swapchain frames than we can "
            "handle!\n");
        exit(-1);
    }
    VkImage images[img_count];
    result =
        vkGetSwapchainImagesKHR(vulkan.dev, vulkan.swapchain, &img_count, images);

    for (size_t i = 0; i < vulkan.sc_frame_count; i++) {
        VkCommandBufferAllocateInfo ai_cmd_buf = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = vulkan.cmd_pool,
            .commandBufferCount = 1,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        };

        vkAllocateCommandBuffers(vulkan.dev, &ai_cmd_buf,
                                 &vulkan.sc_frames[i].cmd_buf);
        vulkan.sc_frames[i].img = images[i];

        VkImageViewCreateInfo ci_imgv = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.levelCount = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount = 1,
            .image = vulkan.sc_frames[i].img,
            .format = vulkan.surf_fmt.format,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        };

        CHECK_VK_RESULT(vkCreateImageView(vulkan.dev, &ci_imgv, NULL,
                                          &vulkan.sc_frames[i].imgv));

        VkFramebufferCreateInfo ci_frame_buf = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = vulkan.render_pass,
            .attachmentCount = 1,
            .pAttachments = &vulkan.sc_frames[i].imgv,
            .width = vulkan.width,
            .height = vulkan.height,
            .layers = 1,
        };
        CHECK_VK_RESULT(vkCreateFramebuffer(vulkan.dev, &ci_frame_buf, NULL,
                                            &vulkan.sc_frames[i].frame_buf));

        VkSemaphoreCreateInfo ci_semaphore = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };

        CHECK_VK_RESULT(
            vkCreateSemaphore(vulkan.dev, &ci_semaphore, NULL,
                              &vulkan.sc_frames[i].semaphore_render_start));

        CHECK_VK_RESULT(
            vkCreateSemaphore(vulkan.dev, &ci_semaphore, NULL,
                              &vulkan.sc_frames[i].semaphore_render_finish));

        VkFenceCreateInfo ci_fence = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };

        CHECK_VK_RESULT(vkCreateFence(vulkan.dev, &ci_fence, NULL,
                                      &vulkan.sc_frames[i].inflight_fence));
    }

    vulkan.frame_idx = 0;
    return true;
}

void destroy_swapchain() {
    vkDeviceWaitIdle(vulkan.dev);
    for (uint32_t i = 0; i < vulkan.sc_frame_count; i++) {
        vkDestroyFence(vulkan.dev, vulkan.sc_frames[i].inflight_fence, NULL);
        vkDestroySemaphore(vulkan.dev, vulkan.sc_frames[i].semaphore_render_start,
                           NULL);
        vkDestroySemaphore(vulkan.dev,
                           vulkan.sc_frames[i].semaphore_render_finish, NULL);
        vkDestroyFramebuffer(vulkan.dev, vulkan.sc_frames[i].frame_buf, NULL);
        vkDestroyImageView(vulkan.dev, vulkan.sc_frames[i].imgv, NULL);
        vkFreeCommandBuffers(vulkan.dev, vulkan.cmd_pool, 1,
                             &vulkan.sc_frames[i].cmd_buf);
    }

    vkDestroyRenderPass(vulkan.dev, vulkan.render_pass, vulkan.alloc);
    vkDestroySwapchainKHR(vulkan.dev, vulkan.swapchain, vulkan.alloc);
}

bool create_shader_module(VkShaderModule* module, const char* file_name) {
    FILE* file = fopen(file_name, "rb");
    if (!file) {
        printf("Error: Failed to open shader file: %s\n", file_name);
        return false;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    uint32_t* code = malloc(size);
    fread(code, 1, size, file);
    fclose(file);

    VkShaderModuleCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = size,
        .pCode = code,
    };

    if (vkCreateShaderModule(vulkan.dev, &create_info, NULL, module) !=
        VK_SUCCESS) {
        printf("Error: Failed to create shader module!\n");
        free(code);
        return false;
    }

    free(code);
    return true;
}

bool create_pipeline() {
    VkPipelineShaderStageCreateInfo vert_shader_stage = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vulkan.vert_shader,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo frag_shader_stage = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = vulkan.frag_shader,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage,
                                                       frag_shader_stage};

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .vertexAttributeDescriptionCount = 0,
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
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
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineColorBlendStateCreateInfo color_blending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment,
    };

    VkPipelineLayoutCreateInfo pipeline_layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0,
        .pushConstantRangeCount = 0,
    };

    VkPipelineLayout pipeline_layout;
    vkCreatePipelineLayout(vulkan.dev, &pipeline_layout_info, NULL,
                           &pipeline_layout);

    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shader_stages,
        .pVertexInputState = &vertex_input_info,
        .pInputAssemblyState = &input_assembly,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pColorBlendState = &color_blending,
        .renderPass = vulkan.render_pass,
        .layout = pipeline_layout,
    };
    VkResult result;
    CHECK_VK_RESULT(vkCreateGraphicsPipelines(
        vulkan.dev, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &vulkan.pipeline));
    return true;
}
