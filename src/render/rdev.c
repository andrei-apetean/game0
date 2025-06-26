#include "rdev.h"

#include <vulkan/vulkan_core.h>

#include "base.h"
#include "rdev_vulkan.h"
#include "rtypes.h"

#if _DEBUG
static VkDebugUtilsMessengerEXT dbg_msgr = VK_NULL_HANDLE;
#endif  //_DEBUG

static rdev_vulkan rvk = {0};

void rdev_init(rdev_params* params) {
    VkResult result = VK_SUCCESS;
    rvk.window_api = params->wnd_api;
    rdev_vulkan_create_instance(&rvk);

#ifdef _DEBUG
    result = rdev_vulkan_create_dbg_msgr(&rvk, &dbg_msgr);
    debug_assert(result == VK_SUCCESS);
#endif  // _DEBUG

    result = rdev_vulkan_create_device(&rvk);
    debug_assert(result == VK_SUCCESS);
    
    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = rvk.dev.cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = VSWAPCHAIN_MAX_IMG,
    };

    result = vkAllocateCommandBuffers(rvk.dev.handle, &alloc_info, rvk.cmd_buffers);
    debug_assert(result == VK_SUCCESS);
    debug_log("Command buffers allocated!\n");
    debug_log("Render context initialized\n");
}

void rdev_terminate() {
    vkDeviceWaitIdle(rvk.dev.handle);
    vkFreeCommandBuffers(rvk.dev.handle, rvk.dev.cmd_pool, VSWAPCHAIN_MAX_IMG,
                          rvk.cmd_buffers);
    rdev_vulkan_destroy_device(&rvk);
#ifdef _DEBUG
    rdev_vulkan_destroy_dbg_msgr(&rvk, dbg_msgr);
#endif  //_DEBUG
    vkDestroyInstance(rvk.instance, rvk.allocator);
}

void rdev_create_swapchain(void* wnd_native, uint32_t w, uint32_t h) {
    switch (rvk.window_api) {
        case RDEV_WND_WL: {
            rdev_vulkan_create_surface_wl(&rvk, wnd_native);
        } break;
        case RDEV_WND_XCB: {
            rdev_vulkan_create_surface_xcb(&rvk, wnd_native);
        } break;
        case RDEV_WND_WIN32: {
            rdev_vulkan_create_surface_win32(&rvk, wnd_native);
        } break;
    }

    VkExtent2D swapchain_extent = {.width = w, .height = h};
    rdev_vulkan_create_swapchain(&rvk, &rvk.swapchain, swapchain_extent);
}

void rdev_resize_swapchain(uint32_t w, uint32_t h) {
    VkExtent2D extent = {.width = w, .height = h};
    vkDeviceWaitIdle(rvk.dev.handle);
    rdev_vulkan_destroy_framebuffers(&rvk, VSWAPCHAIN_MAX_IMG);
    rdev_vulkan_destroy_swapchain(&rvk, &rvk.swapchain);
    VkResult result;
    result = rdev_vulkan_create_swapchain(&rvk, &rvk.swapchain, extent);
    debug_assert(result == VK_SUCCESS);
    result = rdev_vulkan_create_framebuffers(&rvk, &rvk.swapchain.rpass, rvk.swapchain.image_count, extent);
    debug_assert(result == VK_SUCCESS);
}

void rdev_destroy_swapchain() {
    vkDeviceWaitIdle(rvk.dev.handle);
    vkDestroySurfaceKHR(rvk.instance, rvk.surface, rvk.allocator);
    rdev_vulkan_destroy_framebuffers(&rvk, VSWAPCHAIN_MAX_IMG);
    rdev_vulkan_destroy_swapchain(&rvk, &rvk.swapchain);
}

rbuf_id rdev_create_buffer() {
    unimplemented(rdev_create_buffer);
    return 0;
}

rpass_id rdev_create_renderpass() {
    unimplemented(rdev_create_renderpass);
    return 0;
}

rpipe_id rdev_create_pipeline() {
    unimplemented(rdev_create_pipeline);
    return 0;
}

void rdev_destroy_buffer(rbuf_id id) {
    unimplemented(rdev_create_buffer);
    unused(id);
}

void rdev_destroy_renderpass(rpass_id id) {
    unimplemented(rdev_create_renderpass);
    unused(id);
}

void rdev_destroy_pipeline(rpipe_id id) {
    unimplemented(rdev_create_renderpass);
    unused(id);
}
