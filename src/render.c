#include "render.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

typedef struct vulkan_ctx {
    VkInstance             instance;
    VkAllocationCallbacks *alloc,
} vulkan_ctx;

bool init_vk_instance(vulkan_ctx* ctx);

size_t render_get_size() { return sizeof(vulkan_ctx); }

bool render_startup(render_state* state) {
    vulkan_ctx* ctx = (vulkan_ctx*)state;
    if (ctx == NULL) {
        return false;
    }
    if (init_vk_instance(ctx) == false) {
        return false;
    }
    return true;
}

bool render_shutdown(render_state* state) {
    vulkan_ctx* ctx = (vulkan_ctx*)state;
    if (ctx == NULL) {
        return false;
    }
    return true;
}

bool init_vk_instance(vulkan_ctx* ctx) {
    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "game0",
        .applicationVersion = VK_MAKE_API_VERSION(1, 0, 0, 0),
        .pEngineName = "game0",
        .engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0),
        .apiVersion = VK_MAKE_API_VERSION(1, 0, 0, 0),
    };

    VkInstanceCreateInfo instance_ci = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        /* uint32_t                   */ .enabledLayerCount = 0,
        /* const char* const*         */ .ppEnabledLayerNames = 0,
        /* uint32_t                   */ .enabledExtensionCount = 0,
        /* const char* const*         */ .ppEnabledExtensionNames = 0,
    };
    VkResult r = vkCreateInstance(&instance_ci, ctx->alloc, &ctx->instance);
    return r == VK_SUCCESS;
}
