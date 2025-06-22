#include "render_module/render.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <vulkan/vulkan_core.h>

#include "render_module/render_types.h"
#include "render_module/vkcore.h"
#include "render_module/vulkan_device.h"
#include "render_module/vulkan_pipeline.h"
#include "render_module/vulkan_swapchain.h"
#include "render_module/vulkan_types.h"

#if _DEBUG
static VkDebugUtilsMessengerEXT dbg_msgr;
#endif  //_DEBUG

static vulkan_core core = {0};

int32_t render_device_connect(uint32_t window_api, void* window) {
    int32_t err = vkcore_create_instance(&core, window_api);
    if (err) return err;

#ifdef _DEBUG
    err = vkcore_create_dbg_msgr(&core, &dbg_msgr);
#endif  // _DEBUG

    switch (window_api) {
        case 0: {
            err = vkcore_create_surface_wl(&core, window);
        } break;
        case 1: {
            err = vkcore_create_surface_xcb(&core, window);
        } break;
        case 2: {
            err = vkcore_create_surface_win32(&core, window);
        } break;
    }
    if (err) return err;

    core.device.allocator = core.allocator;  // todo;
    err = vulkan_device_create(&core.device, core.instance, core.surface);

    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = core.device.cmdpool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = MAX_FRAME_COUNT,
    };
    vkAllocateCommandBuffers(core.device.logical_device, &alloc_info,
                             core.cmdbuffers);
    return err;
}
void render_device_disconnect() {
    vkDeviceWaitIdle(core.device.logical_device);
    vkFreeCommandBuffers(core.device.logical_device, core.device.cmdpool,
                         MAX_FRAME_COUNT, core.cmdbuffers);
    vulkan_device_destroy(&core.device);
    vkcore_destroy_surface(&core);
#ifdef _DEBUG
    vkcore_destroy_dbg_msgr(&core, dbg_msgr);
#endif  //  _DEBUG
    vkcore_destroy_instance(&core);
    return;
}

int32_t render_device_create_swapchain(uint32_t width, uint32_t height) {
    int32_t err = 0;
    err = vulkan_swapchain_create(&core.device, &core.swapchain, core.surface,
                                  width, height);
    if (err) return err;

    err = vkcore_create_semaphores(&core, core.swapchain.frame_count);
    if (err) return err;

    err = vkcore_create_fences(&core, core.swapchain.frame_count);
    return err;
}

int32_t render_device_resize_swapchain(uint32_t width, uint32_t height) {
    render_device_destroy_swapchain();

    int32_t err = vkcore_create_framebuffers(
        &core, core.pipeline.renderpass, core.swapchain.frame_count,
        core.swapchain.width, core.swapchain.height);
    if (err) return err;
    return render_device_create_swapchain(width, height);
}

void render_device_destroy_swapchain() {
    vkDeviceWaitIdle(core.device.logical_device);
    vkcore_destroy_framebuffers(&core, core.swapchain.frame_count);
    vkcore_destroy_fences(&core, core.swapchain.frame_count);
    vkcore_destroy_semaphores(&core, core.swapchain.frame_count);
    vulkan_swapchain_destroy(&core.device, &core.swapchain);
}

void render_device_begin_frame() {
    vkWaitForFences(core.device.logical_device, 1,
                    &core.inflight_fences[core.current_frame], VK_TRUE, UINT64_MAX);
    vkResetFences(core.device.logical_device, 1,
                  &core.inflight_fences[core.current_frame]);
    VkResult result = vkAcquireNextImageKHR(
        core.device.logical_device, core.swapchain.handle, UINT64_MAX,
        core.image_available_semaphores[core.current_frame], VK_NULL_HANDLE,
        &core.image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        printf("Swapchain needs recreation!\n");
        // Recreate swapchain here
        return;
    } else if (result != VK_SUCCESS) {
        printf("Image acquisition error!\n");
        // Handle other errors
        return;
    }

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    VkCommandBuffer cmd = core.cmdbuffers[core.current_frame];

    vkResetCommandBuffer(cmd, 0);
    vkBeginCommandBuffer(cmd, &begin_info);
    // todo, take out
    VkClearValue clear_values[2];
    clear_values[0].color = (VkClearColorValue){{0.0941f, 0.0941f, 0.0941f, 1.0f}};
    clear_values[1].depthStencil = (VkClearDepthStencilValue){1.0f, 0};

    VkRenderPassBeginInfo rp_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = core.pipeline.renderpass,
        .framebuffer = core.framebuffers[core.image_index],
        .renderArea =
            {
                .offset = {0, 0},
                .extent = {core.swapchain.width, core.swapchain.height},
            },
        .clearValueCount = 2,
        .pClearValues = clear_values,
    };
    vkCmdBeginRenderPass(cmd, &rp_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, core.pipeline.handle);

    VkViewport viewport = {
        .x = 0,
        .y = 0,
        .width = (float)core.swapchain.width,
        .height = (float)core.swapchain.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent =
            {
                core.swapchain.width,
                core.swapchain.height,
            },
    };
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

void render_device_end_frame() {
    VkCommandBuffer  cmd = core.cmdbuffers[core.current_frame];
    VkDescriptorType type;
    // todo, takeout
    vkCmdEndRenderPass(cmd);
    vkEndCommandBuffer(cmd);

    VkSemaphore wait_semaphores[] = {
        core.image_available_semaphores[core.current_frame],
    };
    VkPipelineStageFlags wait_stages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    };

    VkSemaphore signal_semaphores[] = {
        core.render_finished_semaphores[core.current_frame],
    };

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = wait_semaphores,
        .pWaitDstStageMask = wait_stages,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signal_semaphores,
    };

    vkQueueSubmit(core.device.graphics_queue, 1, &submit_info,
                  core.inflight_fences[core.current_frame]);

    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signal_semaphores,
        .swapchainCount = 1,
        .pSwapchains = &core.swapchain.handle,
        .pImageIndices = &core.image_index,
    };

    vkQueuePresentKHR(core.device.graphics_queue, &present_info);

    core.current_frame = (core.current_frame + 1) % core.swapchain.frame_count;
}

int32_t render_device_create_render_pipeline(render_pipeline_config* config) {
    VkSurfaceFormatKHR surface_fmt;
    VkFormat           depth_fmt;
    int32_t            err = 0;
    err =
        vulkan_device_find_surface_format(&core.device, core.surface, &surface_fmt);
    if (err) return err;

    err = vulkan_device_find_depth_format(&core.device, &depth_fmt);
    if (err) return err;

    vulkan_pipeline_config pipeline_config = {
        .color_format = surface_fmt.format,
        .depth_format = depth_fmt,
        .surface = core.surface,
    };

    err = vulkan_device_create_shader_module(
        &core.device, config->vertex_shader_code, config->vertex_shader_code_size,
        &pipeline_config.vertex_shader);
    if (err) {
        vulkan_device_destroy_shader_module(&core.device,
                                            pipeline_config.vertex_shader);
        return err;
    }

    err = vulkan_device_create_shader_module(
        &core.device, config->fragment_shader_code,
        config->fragment_shader_code_size, &pipeline_config.fragment_shader);

    if (err) {
        vulkan_device_destroy_shader_module(&core.device,
                                            pipeline_config.vertex_shader);
        vulkan_device_destroy_shader_module(&core.device,
                                            pipeline_config.fragment_shader);
        return err;
    }

    err = vulkan_pipeline_create(&core.device, &core.pipeline, &pipeline_config);
    vulkan_device_destroy_shader_module(&core.device,
                                        pipeline_config.vertex_shader);
    vulkan_device_destroy_shader_module(&core.device,
                                        pipeline_config.fragment_shader);
    if (err) return err;
    // todo: hack
    err = vkcore_create_framebuffers(&core, core.pipeline.renderpass,
                                     core.swapchain.frame_count,
                                     core.swapchain.width, core.swapchain.height);
    return err;
}

void render_device_destroy_render_pipeline() {
    vkDeviceWaitIdle(core.device.logical_device);
    vulkan_pipeline_destroy(&core.device, &core.pipeline);
}

buffer_id render_device_create_buffer(buffer_config* config) {
    vulkan_buffer* buffer = 0;
    if ((config->usage_flags & vertex_buffer_bit) == vertex_buffer_bit ) {
        buffer = &core.vertex_buffer;
        printf("Creating vertex buffer %d\n", config->usage_flags & vertex_buffer_bit);
    }
    if ((config->usage_flags & index_buffer_bit) == index_buffer_bit) {
        buffer = &core.index_buffer;
        printf("Creating index buffer %d\n", config->usage_flags & index_buffer_bit);
    }
    if (!buffer){
        return RENDER_INVALID_ID;
    }

    printf("Creating buffer %d, %d\n", config->size, config->usage_flags);
    if (!buffer) return RENDER_INVALID_ID;

    int32_t result = vulkan_device_create_buffer(&core.device, buffer, config);
    if (result) return RENDER_INVALID_ID;

    return buffer->id;
}

void render_device_destroy_buffer(buffer_id buffer_id) {
    vkDeviceWaitIdle(core.device.logical_device);
    vulkan_buffer* buffer = 0;
    if (core.vertex_buffer.id == buffer_id) {
        buffer = &core.vertex_buffer;
    } else if (core.index_buffer.id == buffer_id) {
        buffer = &core.index_buffer;
    }
    if (!buffer) return;

    printf("Destroying buffer: %d, %zu\n", buffer_id, buffer->size);
    vulkan_device_destroy_buffer(&core.device, buffer);
}

void render_device_draw_mesh(static_mesh* mesh, mat4 mvp) {
    if (mesh->index_buffer == RENDER_INVALID_ID ||
        mesh->vertex_buffer == RENDER_INVALID_ID) {
        return;
    }

    VkCommandBuffer cmd = core.cmdbuffers[core.current_frame];

    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, &core.vertex_buffer.handle, offsets);
    vkCmdBindIndexBuffer(cmd, core.index_buffer.handle, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, core.pipeline.handle);

    vkCmdPushConstants(cmd, core.pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(mat4), &mvp);

    vkCmdDrawIndexed(cmd, mesh->index_count, 1, 0, 0, 0);
}
