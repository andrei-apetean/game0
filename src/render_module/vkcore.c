#include "render_module/vkcore.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan_core.h>
#include "render_module/vulkan_types.h"

#define VULKAN_SURFACE_EXT "VK_KHR_surface"
#define VULKAN_SWAPCHAIN_EXT "VK_KHR_swapchain"
#define VULKAN_SURFACE_EXT_WL "VK_KHR_wayland_surface"
#define VULKAN_SURFACE_EXT_XCB "VK_KHR_xcb_surface"
#define VULKAN_SURFACE_EXT_WIN32 "VK_KHR_win32_surface"
#define VULKAN_VALIDATION_LAYER "VK_LAYER_KHRONOS_validation"
#define VULKAN_ENABLEMENT_COUNT 256

int32_t vkcore_create_instance(vulkan_core* core, uint32_t win_api) {
    uint32_t    extension_count = 0;
    uint32_t    layer_count = 0;
    const char* requested_extensions[VULKAN_ENABLEMENT_COUNT] = {0};
    const char* requested_layers[VULKAN_ENABLEMENT_COUNT] = {0};

    requested_extensions[extension_count++] = VULKAN_SURFACE_EXT;
    switch (win_api) {
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
    VK_CHECK(vkCreateInstance(&inst_ci, core->allocator, &core->instance));
    printf("Instance created successfully\n");
    return 0;
}

int32_t vkcore_create_semaphores(vulkan_core* core, uint32_t count) {
    assert(count <= MAX_FRAME_COUNT);
    VkSemaphoreCreateInfo sem_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    for (uint32_t i = 0; i < count; i++) {
        VK_CHECK(vkCreateSemaphore(core->device.logical_device, &sem_info,
                                   core->allocator,
                                   &core->image_available_semaphores[i]));
        VK_CHECK(vkCreateSemaphore(core->device.logical_device, &sem_info,
                                   core->allocator,
                                   &core->render_finished_semaphores[i]));
    }
    printf("Semaphores created successfully\n");
    return 0;
}
int32_t vkcore_create_fences(vulkan_core* core, uint32_t count) {
    assert(count <= MAX_FRAME_COUNT);
    VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    for (uint32_t i = 0; i < count; i++) {
        VK_CHECK(vkCreateFence(core->device.logical_device, &fence_info,
                               core->allocator, &core->inflight_fences[i]));
    }
    printf("Fences created successfully\n");
    return 0;
}

int32_t vkcore_create_framebuffers(vulkan_core* core, VkRenderPass renderpass, uint32_t count,
                                   uint32_t width, uint32_t height) {
    assert(count <= MAX_FRAME_COUNT);
    for (uint32_t i = 0; i < count; i++) {
        VkImageView attachments[] = {
            core->swapchain.color_image_views[i],
            core->swapchain.depth_image_views[i],
        };
        VkFramebufferCreateInfo fb_ci = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = renderpass,
            .attachmentCount = sizeof(attachments) / sizeof(attachments[0]),
            .pAttachments = attachments,
            .width = width,
            .height = height,
            .layers = 1,
        };
        VK_CHECK(vkCreateFramebuffer(core->device.logical_device, &fb_ci,
                                     core->allocator, &core->framebuffers[i]));
    }
    printf("Framebuffers created successfully\n");
    return 0;
}


void vkcore_destroy_semaphores(vulkan_core* core, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        vkDestroySemaphore(core->device.logical_device,
                           core->image_available_semaphores[i], core->allocator);
        vkDestroySemaphore(core->device.logical_device,
                           core->render_finished_semaphores[i], core->allocator);
    }
}

void vkcore_destroy_fences(vulkan_core* core, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        vkDestroyFence(core->device.logical_device, core->inflight_fences[i],
                       core->allocator);
    }
}

void vkcore_destroy_framebuffers(vulkan_core* core, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        vkDestroyFramebuffer(core->device.logical_device, core->framebuffers[i],
                             core->allocator);
    }
}

// todo: temp
int32_t vkcore_create_surface_xcb(vulkan_core* core, void* win) { return -1; }
int32_t vkcore_create_surface_win32(vulkan_core* core, void* win) { return -1; }
// end todo;

void vkcore_destroy_instance(vulkan_core* core) {
    vkDestroyInstance(core->instance, core->allocator);
}

void vkcore_destroy_surface(vulkan_core* core) {
    vkDestroySurfaceKHR(core->instance, core->surface, core->allocator);
}

#ifdef _DEBUG
static VkBool32 debug_utils_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
    VkDebugUtilsMessageTypeFlagsEXT             types,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data) {
    printf(" MessageID: %s %i\nMessage: %s\n\n", callback_data->pMessageIdName,
           callback_data->messageIdNumber, callback_data->pMessage);

    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        assert(0);
    }

    return VK_FALSE;
}

int32_t vkcore_create_dbg_msgr(vulkan_core* core, VkDebugUtilsMessengerEXT* m) {
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
            core->instance, "vkCreateDebugUtilsMessengerEXT");
    if (create_debug_msgr == NULL) {
        printf("Failed to load debug messanger creation function!\n");
        return -1;
    }
    VK_CHECK(create_debug_msgr(core->instance, &dbg_ci, 0, m));
    printf("Debug messenger created!\n");
    return 0;
}

void vkcore_destroy_dbg_msgr(vulkan_core* core, VkDebugUtilsMessengerEXT m) {
    PFN_vkDestroyDebugUtilsMessengerEXT destroy_msgr =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            core->instance, "vkDestroyDebugUtilsMessengerEXT");
    if (destroy_msgr == NULL) {
        printf("Failed to load debug messanger destruction function!\n");
        return;
    }
    destroy_msgr(core->instance, m, core->allocator);
    return;
}
#endif  // _DEBUG
