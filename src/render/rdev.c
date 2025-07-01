#include "rdev.h"

#include "../base.h"
#include "rdev_vulkan.h"
#include "vkutils.h"
#include "rtypes.h"

#if _DEBUG
static VkDebugUtilsMessengerEXT dbg_msgr = VK_NULL_HANDLE;
#endif  //_DEBUG

static vstate vk = {0};

void rdev_init(rdev_params* params) {
    debug_log("initializing rdev...\n");
    debug_log("window api: %d\n", params->wnd_api);
    
    VkResult result = VK_SUCCESS;
    vk.window_api = params->wnd_api;
    vcreate_instance(&vk);

#ifdef _DEBUG
    result = vcreate_dbg_msgr(&vk, &dbg_msgr);
    debug_assert(result == VK_SUCCESS);
#endif  // _DEBUG

    result = vcreate_device(&vk);
    debug_assert(result == VK_SUCCESS);

    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = vk.dev.cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = VSWAPCHAIN_MAX_IMG,
    };
    VkCommandBuffer cmd_buffers[VSWAPCHAIN_MAX_IMG];

    result = vkAllocateCommandBuffers(vk.dev.handle, &alloc_info, cmd_buffers);
    for (uint32_t i = 0; i < VSWAPCHAIN_MAX_IMG; i++) {
        vk.rcmds[i].handle = cmd_buffers[i];
    }

    debug_assert(result == VK_SUCCESS);

    result = vcreate_semaphores(&vk, VSWAPCHAIN_MAX_IMG);
    debug_assert(result == VK_SUCCESS);
    result = vcreate_fences(&vk, VSWAPCHAIN_MAX_IMG);
    debug_assert(result == VK_SUCCESS);
    debug_log("rdev context initialized\n");
}

void rdev_terminate() {
    vkDeviceWaitIdle(vk.dev.handle);
    VkCommandBuffer cmd_buffers[VSWAPCHAIN_MAX_IMG];
    for (uint32_t i = 0; i < VSWAPCHAIN_MAX_IMG; i++) {
        cmd_buffers[i] = vk.rcmds[i].handle;
    }
    vdestroy_fences(&vk, VSWAPCHAIN_MAX_IMG);
    vdestroy_semaphores(&vk, VSWAPCHAIN_MAX_IMG);
    vkFreeCommandBuffers(vk.dev.handle, vk.dev.cmd_pool, VSWAPCHAIN_MAX_IMG,
                         cmd_buffers);
    vdestroy_device(&vk);
#ifdef _DEBUG
    vdestroy_dbg_msgr(&vk, dbg_msgr);
#endif  //_DEBUG
    vkDestroyInstance(vk.instance, vk.allocator);
}

void rdev_create_swapchain(void* wnd_native, uint32_t w, uint32_t h) {
    VkResult result;
    switch (vk.window_api) {
        case RDEV_WND_WL: {
            result = vcreate_surface_wl(&vk, wnd_native);
        } break;
        case RDEV_WND_XCB: {
            result = vcreate_surface_xcb(&vk, wnd_native);
        } break;
        case RDEV_WND_WIN32: {
            result = vcreate_surface_win32(&vk, wnd_native);
        } break;
    }
    debug_assert(result == VK_SUCCESS);
    VkSurfaceFormatKHR fmt = vutl_find_surface_format(vk.dev.physical, vk.surface);
    VkFormat           depth_fmt = vutl_find_depth_format(vk.dev.physical);
    if (depth_fmt == VK_FORMAT_UNDEFINED) {
        debug_log("failed to find depth format!\n");
        debug_assert(0);
        return;
    }

    VkPresentModeKHR present = vutl_find_present_mode(vk.dev.physical, vk.surface);
    vk.swapchain.surface_fmt = fmt;
    vk.swapchain.depth_fmt = depth_fmt;
    vk.swapchain.present_mode = present;
    VkExtent2D swapchain_extent = {.width = w, .height = h};
    result = vcreate_swapchain(&vk, &vk.swapchain, swapchain_extent);
    debug_assert(result == VK_SUCCESS);
    result = vcreate_swapchainpass(&vk, &vk.swapchain);
    debug_assert(result == VK_SUCCESS);
    result = vcreate_framebuffers(&vk, &vk.swapchain);
    debug_assert(result == VK_SUCCESS);
    debug_log("swapchain created: %d, %d\n", vk.swapchain.extent.width,
              vk.swapchain.extent.height);
}

void rdev_resize_swapchain(uint32_t w, uint32_t h) {
    VkExtent2D extent = {.width = w, .height = h};
    vkDeviceWaitIdle(vk.dev.handle);
    vdestroy_framebuffers(&vk, &vk.swapchain);
    vdestroy_swapchainpass(&vk, &vk.swapchain);
    vdestroy_swapchain(&vk, &vk.swapchain);
    VkResult result;
    result = vcreate_swapchain(&vk, &vk.swapchain, extent);
    debug_assert(result == VK_SUCCESS);
    result = vcreate_swapchainpass(&vk, &vk.swapchain);
    debug_assert(result == VK_SUCCESS);
    result = vcreate_framebuffers(&vk, &vk.swapchain);
    debug_assert(result == VK_SUCCESS);
    debug_log("swapchain resized: %d, %d\n", w, h);
    vk.swapchain_needs_resize = 0;
}

void rdev_destroy_swapchain() {
    vkDeviceWaitIdle(vk.dev.handle);
    vdestroy_framebuffers(&vk, &vk.swapchain);
    vdestroy_swapchainpass(&vk, &vk.swapchain);
    vdestroy_swapchain(&vk, &vk.swapchain);
    vkDestroySurfaceKHR(vk.instance, vk.surface, vk.allocator);
}

rpass_id rdev_swapchain_renderpass() { return vk.swapchain.rpass.id; }

rpass_id rdev_create_renderpass() {
    unimplemented(rdev_create_renderpass);
    return 0;
}

rpipe_id rdev_create_pipeline(rpipe_params* params) {
    // hack:
    vpipe* pipe = &vk.pipes[0];  // only using a single pipeline for now

    uint32_t shader_count = params->shader_stage_count;
    vshader  shaders[shader_count];
    for (uint32_t i = 0; i < params->shader_stage_count; i++) {
        shaders[i].type = params->shader_stages[i].type;
        shaders[i].code = params->shader_stages[i].code;
        shaders[i].code_size = params->shader_stages[i].code_size;
    }
    VkResult result;
    result = vcreate_shader_modules(&vk, shaders, shader_count);
    if (result != VK_SUCCESS) return RDEV_INVALID_ID;

    result = vcreate_pipeline(&vk, pipe, params, shaders);
    if (result != VK_SUCCESS) return RDEV_INVALID_ID;

    vdestroy_shader_modules(&vk, shaders, params->shader_stage_count);
    debug_log("graphics pipeline created!\n");
    // todo:
    return 0;
}

void rdev_destroy_pipeline(rpipe_id id) {
    vkDeviceWaitIdle(vk.dev.handle);
    vpipe pipe = vk.pipes[id];  //
    vkDestroyPipelineLayout(vk.dev.handle, pipe.layout, vk.allocator);
    vkDestroyPipeline(vk.dev.handle, pipe.handle, vk.allocator);
}

void rdev_destroy_renderpass(rpass_id id) {
    unimplemented(rdev_create_renderpass);
    unused(id);
}

//=========================================================
//
// buffers
//
//=========================================================

rbuffer_id rdev_create_buffer(rbuf_params* params) {
    static uint32_t buf_count = 0;
    vbuf*           buf = &vk.buffers[buf_count];
    buf->id = buf_count;
    VkResult result = vcreate_buffer(&vk, params, buf);
    if (result != VK_SUCCESS) return RDEV_INVALID_ID;
    buf_count++;
    return buf->id;
}

void rdev_destroy_buffer(rbuffer_id id) {
    vbuf buf = vk.buffers[id];
    vdestroy_buffer(&vk, &buf);
}

void rdev_buffer_upload(rbuffer_id id, void* data, uint32_t size, uint32_t offset) {
    vbuf* buf = &vk.buffers[id];
    vupload_buffer(&vk, buf, data, size, offset);
}

void rdev_buffer_download(rbuffer_id id, void* data, uint32_t size, uint32_t offset) {
    vbuf buf = vk.buffers[id];
    vdownload_buffer(&vk, &buf, data, size, offset);
}
void* rdev_buffer_map(rbuffer_id id) {
    vbuf* buf = &vk.buffers[id];
    return vbuffer_map(&vk, buf);
}
void rdev_buffer_unmap(rbuffer_id id) {
    vbuf* buf = &vk.buffers[id];
    vbuffer_unmap(&vk, buf);
}

rbuffer_id rdev_create_vertex_buffer(uint32_t size, void* data) {
    rbuf_params params = {
        .size = size,
        .usage_flags =
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .initial_data = data,
    };
    return rdev_create_buffer(&params);
}

rbuffer_id rdev_create_index_buffer(uint32_t size, void* data) {
    rbuf_params params = {
        .size = size,
        .usage_flags =
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .initial_data = data,
    };
    return rdev_create_buffer(&params);
}

rbuffer_id rdev_create_uniform_buffer(uint32_t size) {
    rbuf_params params = {
        .size = size,
        .usage_flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        .initial_data = NULL,
    };
    return rdev_create_buffer(&params);
}

rbuffer_id rdev_create_staging_buffer(uint32_t size) {
    rbuf_params params = {
        .size = size,
        .usage_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        .initial_data = NULL,
    };
    return rdev_create_buffer(&params);
}

//=========================================================
//
// drawing
//
//=========================================================

rcmd* rdev_begin() {
    if (vk.swapchain_needs_resize) {
        return 0;
    }
    vkWaitForFences(vk.dev.handle, 1, &vk.inflight_fences[vk.current_frame],
                    VK_TRUE, UINT64_MAX);
    vkResetFences(vk.dev.handle, 1, &vk.inflight_fences[vk.current_frame]);
    VkResult result =
        vkAcquireNextImageKHR(vk.dev.handle, vk.swapchain.handle, UINT64_MAX,
                              vk.image_available_semaphores[vk.current_frame],
                              VK_NULL_HANDLE, &vk.image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        debug_log("swapchain needs recreation!\n");
        vk.swapchain_needs_resize = 1;
        return 0;
    } else if (result != VK_SUCCESS) {
        debug_log("image acquisition error!\n");
        // Handle other errors
        return 0;
    }

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    rcmd* cmd = &vk.rcmds[vk.current_frame];

    vkResetCommandBuffer(cmd->handle, 0);
    vkBeginCommandBuffer(cmd->handle, &begin_info);
    return cmd;
}

void rdev_end(rcmd* cmd) {
    vkEndCommandBuffer(cmd->handle);

    VkSemaphore wait_semaphores[] = {
        vk.image_available_semaphores[vk.current_frame],
    };
    VkPipelineStageFlags wait_stages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    };

    VkSemaphore signal_semaphores[] = {
        vk.render_finished_semaphores[vk.current_frame],
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

    vkQueueSubmit(vk.dev.graphics_queue, 1, &submit_info,
                  vk.inflight_fences[vk.current_frame]);

    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signal_semaphores,
        .swapchainCount = 1,
        .pSwapchains = &vk.swapchain.handle,
        .pImageIndices = &vk.image_index,
    };

    vkQueuePresentKHR(vk.dev.graphics_queue, &present_info);

    vk.current_frame = (vk.current_frame + 1) % vk.swapchain.image_count;
};

void rcmd_begin_pass(rcmd* cmd, rpass_id id) {
    // only supporting the swapchain pass for now;
    debug_assert(id == vk.swapchain.rpass.id);

    VkClearValue clear_values[2];
    clear_values[0].color = (VkClearColorValue){{0.0941f, 0.0941f, 0.0941f, 1.0f}};
    clear_values[1].depthStencil = (VkClearDepthStencilValue){1.0f, 0};

    VkRenderPassBeginInfo rp_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = vk.swapchain.rpass.handle,
        .framebuffer = vk.swapchain.framebuffers[vk.image_index],
        .renderArea =
            {
                .offset = {0, 0},
                .extent = vk.swapchain.extent,
            },
        .clearValueCount = 2,
        .pClearValues = clear_values,
    };
    vkCmdBeginRenderPass(cmd->handle, &rp_info, VK_SUBPASS_CONTENTS_INLINE);
}
void rcmd_bind_pipe(rcmd* cmd, rpipe_id id) {
    // todo: rvk.pipes[id] when using multiple pipelines
    VkViewport viewport = {
        .x = 0,
        .y = 0,
        .width = vk.swapchain.extent.width,
        .height = vk.swapchain.extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    vkCmdSetViewport(cmd->handle, 0, 1, &viewport);
    VkRect2D scissor = {
        .offset.x = 0,
        .offset.y = 0,
        .extent = vk.swapchain.extent,
    };
    vkCmdSetScissor(cmd->handle, 0, 1, &scissor);
    vkCmdBindPipeline(cmd->handle, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      vk.pipes[id].handle);
}
void rcmd_bind_vertex_buffer(rcmd* cmd, rbuffer_id id) {
    vbuf buf = vk.buffers[id];
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd->handle, 0, 1, &buf.handle, offsets);
}

void rcmd_bind_index_buffer(rcmd* cmd, rbuffer_id id) {
    vbuf buf = vk.buffers[id];
    vkCmdBindIndexBuffer(cmd->handle, buf.handle, 0, VK_INDEX_TYPE_UINT32);
}

void rcmd_bind_descriptor_set(rcmd* cmd, rbuffer_id id);

void rcmd_end_pass(rcmd* cmd, rpass_id id) {
    unused(id);  // todo
    vkCmdEndRenderPass(cmd->handle);
}

void rcmd_push_constants(rcmd* cmd, rpipe_id id, rshader_stage_flags flags,
                         uint32_t offset, uint32_t size, void* data) {
    vpipe pipe = vk.pipes[id];
    VkShaderStageFlags stage_flags = vutl_to_vulkan_shader_stage_flags(flags);
    vkCmdPushConstants(cmd->handle, pipe.layout, stage_flags, offset, size, data);
}

void rcmd_draw(rcmd* cmd, uint32_t first_vertex, uint32_t vertex_count,
               uint32_t first_instance, uint32_t instance_count) {
    vkCmdDraw(cmd->handle, vertex_count, instance_count, first_vertex, first_instance);
}

void rcmd_draw_indexed(rcmd* cmd, uint32_t index_count, uint32_t instance_count,
                       uint32_t first_index, uint32_t vertex_offset,
                       uint32_t first_instance) {
    vkCmdDrawIndexed(cmd->handle, index_count, instance_count, first_index,
                     vertex_offset, first_instance);
}
