#include "rdev.h"

#include <vulkan/vulkan_core.h>

#include "base.h"
#include "rdev_vulkan.h"
#include "render/vkutils.h"
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
    VkCommandBuffer cmd_buffers[VSWAPCHAIN_MAX_IMG];

    result = vkAllocateCommandBuffers(rvk.dev.handle, &alloc_info, cmd_buffers);
    for (uint32_t i = 0; i < VSWAPCHAIN_MAX_IMG; i++) {
        rvk.rcmds[i].handle = cmd_buffers[i];
    }

    debug_assert(result == VK_SUCCESS);

    result = rdev_vulkan_create_semaphores(&rvk, VSWAPCHAIN_MAX_IMG);
    debug_assert(result == VK_SUCCESS);
    result = rdev_vulkan_create_fences(&rvk, VSWAPCHAIN_MAX_IMG);
    debug_assert(result == VK_SUCCESS);
    debug_log("Renderer: Render context initialized\n");
}

void rdev_terminate() {
    vkDeviceWaitIdle(rvk.dev.handle);
    VkCommandBuffer cmd_buffers[VSWAPCHAIN_MAX_IMG];
    for (uint32_t i = 0; i < VSWAPCHAIN_MAX_IMG; i++) {
        cmd_buffers[i] = rvk.rcmds[i].handle;
    }
    rdev_vulkan_destroy_fences(&rvk, VSWAPCHAIN_MAX_IMG);
    rdev_vulkan_destroy_semaphores(&rvk, VSWAPCHAIN_MAX_IMG);
    vkFreeCommandBuffers(rvk.dev.handle, rvk.dev.cmd_pool, VSWAPCHAIN_MAX_IMG,
                         cmd_buffers);
    rdev_vulkan_destroy_device(&rvk);
#ifdef _DEBUG
    rdev_vulkan_destroy_dbg_msgr(&rvk, dbg_msgr);
#endif  //_DEBUG
    vkDestroyInstance(rvk.instance, rvk.allocator);
}

void rdev_create_swapchain(void* wnd_native, uint32_t w, uint32_t h) {
    VkResult result;
    switch (rvk.window_api) {
        case RDEV_WND_WL: {
            result = rdev_vulkan_create_surface_wl(&rvk, wnd_native);
        } break;
        case RDEV_WND_XCB: {
            result = rdev_vulkan_create_surface_xcb(&rvk, wnd_native);
        } break;
        case RDEV_WND_WIN32: {
            result = rdev_vulkan_create_surface_win32(&rvk, wnd_native);
        } break;
    }
    debug_assert(result == VK_SUCCESS);
    VkSurfaceFormatKHR fmt =
        vkutil_find_surface_format(rvk.dev.physical, rvk.surface);
    debug_log("\nFound format during creation: %d, color space %d\n", fmt.format, fmt.colorSpace);
    VkFormat depth_fmt = vkutil_find_depth_format(rvk.dev.physical);
    if (depth_fmt == VK_FORMAT_UNDEFINED) {
        debug_log("Failed to find depth format!\n");
        debug_assert(0);
        return;
    }

    VkPresentModeKHR present =
        vkutil_find_present_mode(rvk.dev.physical, rvk.surface);
    rvk.swapchain.surface_fmt = fmt;
    rvk.swapchain.depth_fmt = depth_fmt;
    rvk.swapchain.present_mode = present;
    VkExtent2D swapchain_extent = {.width = w, .height = h};
    result = rdev_vulkan_create_swapchain(&rvk, &rvk.swapchain, swapchain_extent);
    debug_assert(result == VK_SUCCESS);
    result = rdev_vulkan_create_swapchainpass(&rvk, &rvk.swapchain);
    debug_assert(result == VK_SUCCESS);
    result = rdev_vulkan_create_framebuffers(&rvk, &rvk.swapchain);
    debug_assert(result == VK_SUCCESS);
    debug_log("Renderer: Swapchain created: %d, %d\n", rvk.swapchain.extent.width,
              rvk.swapchain.extent.height);
}

void rdev_resize_swapchain(uint32_t w, uint32_t h) {
    debug_log("Renderer: Resizing swapchain: %d, %d\n", w, h);
    VkExtent2D extent = {.width = w, .height = h};
    vkDeviceWaitIdle(rvk.dev.handle);
    rdev_vulkan_destroy_framebuffers(&rvk, &rvk.swapchain);
    rdev_vulkan_destroy_swapchainpass(&rvk, &rvk.swapchain);
    rdev_vulkan_destroy_swapchain(&rvk, &rvk.swapchain);
    VkResult result;
    VkSurfaceFormatKHR fmt = vkutil_find_surface_format(rvk.dev.physical, rvk.surface);
    debug_log("\nFound format during resize: %d, color space %d\n", fmt.format, fmt.colorSpace);
    result = rdev_vulkan_create_swapchain(&rvk, &rvk.swapchain, extent);
    debug_assert(result == VK_SUCCESS);
    result = rdev_vulkan_create_swapchainpass(&rvk, &rvk.swapchain);
    debug_assert(result == VK_SUCCESS);
    result = rdev_vulkan_create_framebuffers(&rvk, &rvk.swapchain);
    debug_assert(result == VK_SUCCESS);
    debug_log("Renderer: Swapchain resized: %d, %d\n", w, h);
}

void rdev_destroy_swapchain() {
    vkDeviceWaitIdle(rvk.dev.handle);
    rdev_vulkan_destroy_framebuffers(&rvk, &rvk.swapchain);
    rdev_vulkan_destroy_swapchainpass(&rvk, &rvk.swapchain);
    rdev_vulkan_destroy_swapchain(&rvk, &rvk.swapchain);
    vkDestroySurfaceKHR(rvk.instance, rvk.surface, rvk.allocator);
}

rpass_id rdev_swapchain_renderpass() { return rvk.swapchain.rpass.id; }

rbuf_id  rdev_create_buffer() {
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

//=========================================================
//
// drawing
//
//=========================================================

rcmd* rdev_begin() {
    vkWaitForFences(rvk.dev.handle, 1, &rvk.inflight_fences[rvk.current_frame],
                    VK_TRUE, UINT64_MAX);
    vkResetFences(rvk.dev.handle, 1, &rvk.inflight_fences[rvk.current_frame]);
    VkResult result =
        vkAcquireNextImageKHR(rvk.dev.handle, rvk.swapchain.handle, UINT64_MAX,
                              rvk.image_available_semaphores[rvk.current_frame],
                              VK_NULL_HANDLE, &rvk.image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        debug_log("Renderer: Swapchain needs recreation!\n");
        // Recreate swapchain here
        return 0;
    } else if (result != VK_SUCCESS) {
        debug_log("Renderer: Image acquisition error!\n");
        // Handle other errors
        return 0;
    }

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    rcmd* cmd = &rvk.rcmds[rvk.current_frame];

    vkResetCommandBuffer(cmd->handle, 0);
    vkBeginCommandBuffer(cmd->handle, &begin_info);
    return cmd;
}

void rdev_end(rcmd* cmd) {
    vkEndCommandBuffer(cmd->handle);

    VkSemaphore wait_semaphores[] = {
        rvk.image_available_semaphores[rvk.current_frame],
    };
    VkPipelineStageFlags wait_stages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    };

    VkSemaphore signal_semaphores[] = {
        rvk.render_finished_semaphores[rvk.current_frame],
    };

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = wait_semaphores,
        .pWaitDstStageMask = wait_stages,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd->handle,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signal_semaphores,
    };

    vkQueueSubmit(rvk.dev.graphics_queue, 1, &submit_info,
                  rvk.inflight_fences[rvk.current_frame]);

    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signal_semaphores,
        .swapchainCount = 1,
        .pSwapchains = &rvk.swapchain.handle,
        .pImageIndices = &rvk.image_index,
    };

    vkQueuePresentKHR(rvk.dev.graphics_queue, &present_info);

    rvk.current_frame = (rvk.current_frame + 1) % rvk.swapchain.image_count;
};

void rcmd_begin_pass(rcmd* cmd, rpass_id id) {
    // only supporting the swapchain pass for now;
    debug_assert(id == rvk.swapchain.rpass.id);

    VkClearValue clear_values[2];
    clear_values[0].color = (VkClearColorValue){{0.0941f, 0.0941f, 0.0941f, 1.0f}};
    clear_values[1].depthStencil = (VkClearDepthStencilValue){1.0f, 0};

    VkRenderPassBeginInfo rp_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = rvk.swapchain.rpass.handle,
        .framebuffer = rvk.swapchain.framebuffers[rvk.image_index],
        .renderArea =
            {
                .offset = {0, 0},
                .extent = rvk.swapchain.extent,
            },
        .clearValueCount = 2,
        .pClearValues = clear_values,
    };
    vkCmdBeginRenderPass(cmd->handle, &rp_info, VK_SUBPASS_CONTENTS_INLINE);
}
void rcmd_begin_pipe(rcmd* cmd, rpipe_id id);
void rcmd_begin_vertex_buf(rcmd* cmd, rbuf_id id);
void rcmd_begin_index_buf(rcmd* cmd, rbuf_id id);
void rcmd_begin_descriptor_set(rcmd* cmd, rbuf_id id);

void rcmd_end_pass(rcmd* cmd, rpass_id id) {
    unused(id); // todo
    vkCmdEndRenderPass(cmd->handle);
}
void rcmd_end_pipe(rcmd* cmd, rpipe_id id);
void rcmd_end_vertex_buf(rcmd* cmd, rbuf_id id);
void rcmd_end_index_buf(rcmd* cmd, rbuf_id id);
void rcmd_end_descriptor_set(rcmd* cmd, rbuf_id id);

void rcmd_draw(rcmd* cmd, uint32_t first_vertex, uint32_t vertex_count,
               uint32_t first_instance, uint32_t instance_count);
void rcmd_draw_indexed(rcmd* cmd, uint32_t instance_count, uint32_t first_instance,
                       uint32_t first_index, uint32_t vertex_offset);
