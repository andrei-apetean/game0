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

/*
void render_initialize(render_params* params) {
    state.width = params->swapchain_width;
    state.height = params->swapchain_height;
    state.current_frame = 0;
    init_instance_vk(&state.instance, state.allocator, params->window_api);

#if _DEBUG
    init_debug_msgr(state.instance, state.allocator, &dbg_msgr);
#endif  // _DEBUG

    switch (params->window_api) {
        case 0:
            create_vksurface_wl(state.instance, state.allocator, &state.surface,
                                params->window_handle);
            break;
        default:
            assert(0 && "Unimplemented surface");
    }

    device_selection_params selection_params = {
        .instance = state.instance,
        .device = state.device.gpu,
        .surface = state.surface,
    };
    init_physical_device_vk(&selection_params);
    state.device.gfx_queue_family = selection_params.queue_family_idx;
    state.device.gpu = selection_params.device;

    device_creation_params creation_params = {
        .instance = state.instance,
        .allocator = state.allocator,
        .phys_device = state.device.gpu,
        .device = &state.device.handle,
        .queue = &state.device.gfx_queue,
        .queue_family_idx = state.device.gfx_queue_family,

    };
    init_logical_device_vk(&creation_params);

    VkFormat depth_format = choose_depth_format(state.device.gpu);
    assert(state.swapchain.depth_format != VK_FORMAT_MAX_ENUM);
    VkSurfaceFormatKHR surface_format =
        choose_surface_fmt(state.device.gpu, state.surface);
    VkPresentModeKHR present_mode =
        choose_present_mode(state.device.gpu, state.surface);
    init_renderpass_vk(state.device.handle, state.allocator, &state.renderpass,
                       surface_format.format, depth_format);

    VkCommandPoolCreateInfo pool_ci = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = state.device.gfx_queue_family,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    };
    VkResult result = vkCreateCommandPool(state.device.handle, &pool_ci,
                                          state.allocator, &state.cmdpool);
    state.swapchain.format = surface_format;
    state.swapchain.present_mode = present_mode;
    state.swapchain.depth_format = depth_format;

    swapchain_creation_params swapchain_params = {
        .swapchain = &state.swapchain,
        .device = &state.device,
        .allocator = state.allocator,
        .surface = state.surface,
        .renderpass = state.renderpass,
        .width = params->swapchain_width,
        .height = params->swapchain_height};
    init_swapchain_vk(&swapchain_params);

    state.cmdbuffers =
        malloc(sizeof(VkCommandBuffer) * state.swapchain.frame_count);
    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = state.cmdpool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = state.swapchain.frame_count,
    };
    vkAllocateCommandBuffers(state.device.handle, &alloc_info,
                             state.cmdbuffers);

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
    result = vkCreatePipelineLayout(state.device.handle, &pipeline_layout_info,
                                    state.allocator, &state.pipeline_layout);
    if (result == VK_SUCCESS) {
        printf("VkPipelineLayout created!\n");
    } else {
        printf("VkPipelineLayout creation failed with code %d!\n", result);
        assert(0);
    }

    VkShaderModule vert = load_shader_module(
        state.device.handle, "./bin/assets/shaders/shader.vert.spv",
        state.allocator);
    VkShaderModule frag = load_shader_module(
        state.device.handle, "./bin/assets/shaders/shader.frag.spv",
        state.allocator);
    create_graphics_pipeline(state.device.handle, state.renderpass,
                             state.allocator, state.pipeline_layout, vert, frag,
                             &state.pipeline, params->swapchain_width,
                             params->swapchain_height);
    vkDestroyShaderModule(state.device.handle, vert, state.allocator);
    vkDestroyShaderModule(state.device.handle, frag, state.allocator);

    for (uint32_t i = 0; i < 32; i++) {
        buffer_vk b = state.buffers[i];
        state.buffers[i] = (buffer_vk){
            .id = -1,
            .handle = VK_NULL_HANDLE,
            .memory = 0,
            .allocation_size = 0,
        };
    }
}

void render_terminate() {
    VkDevice               dev = state.device.handle;
    VkAllocationCallbacks* alloc = state.allocator;
    vkDeviceWaitIdle(dev);
    for (uint32_t i = 0; i < 32; i++) {
        buffer_vk b = state.buffers[i];
        if (b.id != -1) {
            printf("Destroying buffer with id %d\n", b.id);
            vkFreeMemory(dev, b.memory, alloc);
            vkDestroyBuffer(dev, b.handle, alloc);
        }
    }
    vkDestroyPipelineLayout(dev, state.pipeline_layout, alloc);
    vkDestroyPipeline(dev, state.pipeline, alloc);
    vkDestroyRenderPass(dev, state.renderpass, alloc);
    destroy_swapchain_vk(dev, alloc, state.swapchain, state.cmdpool);
    vkFreeCommandBuffers(dev, state.cmdpool, state.swapchain.frame_count,
                         state.cmdbuffers);
    free(state.cmdbuffers);
    vkDestroyCommandPool(dev, state.cmdpool, alloc);
    vkDestroyDevice(dev, alloc);

    vkDestroySurfaceKHR(state.instance, state.surface, alloc);
#if _DEBUG
    destroy_debug_msgr(state.instance, alloc, dbg_msgr);
#endif  // _DEBUG

    vkDestroyInstance(state.instance, alloc);
}

void render_begin() {
    vkWaitForFences(state.device.handle, 1,
                    &state.swapchain.frames[state.current_frame].inflight,
                    VK_TRUE, UINT64_MAX);
    vkResetFences(state.device.handle, 1,
                  &state.swapchain.frames[state.current_frame].inflight);

    uint32_t image_index;
    VkResult result = vkAcquireNextImageKHR(
        state.device.handle, state.swapchain.handle, UINT64_MAX,
        state.swapchain.frames[state.current_frame].image_available,
        VK_NULL_HANDLE, &image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        printf("Swapchain needs recreation!\n");
        // Recreate swapchain here
        return;
    } else if (result != VK_SUCCESS) {
        printf("Image acquisition error!\n");
        // Handle other errors
        return;
    }

    state.image_index = image_index;

    VkCommandBuffer cmd = state.cmdbuffers[state.current_frame];

    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(cmd, &begin_info);

    VkClearValue clear_values[2];
    clear_values[0].color =
        (VkClearColorValue){{0.0941f, 0.0941f, 0.0941f, 1.0f}};
    clear_values[1].depthStencil = (VkClearDepthStencilValue){1.0f, 0};

    VkRenderPassBeginInfo rp_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = state.renderpass,
        .framebuffer = state.swapchain.frames[image_index].framebuffer,
        .renderArea =
            {
                .offset = {0, 0},
                .extent = {state.width, state.height},
            },
        .clearValueCount = 2,
        .pClearValues = clear_values,
    };
    vkCmdBeginRenderPass(cmd, &rp_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, state.pipeline);
}

void render_end() {
    uint32_t        image_index = state.image_index;
    VkCommandBuffer cmd = state.cmdbuffers[state.current_frame];

    vkCmdEndRenderPass(cmd);
    vkEndCommandBuffer(cmd);

    VkSemaphore wait_semaphores[] = {
        state.swapchain.frames[state.current_frame].image_available,
    };
    VkPipelineStageFlags wait_stages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    };

    VkSemaphore signal_semaphores[] = {
        state.swapchain.frames[state.current_frame].render_finished,
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

    vkQueueSubmit(state.device.gfx_queue, 1, &submit_info,
                  state.swapchain.frames[state.current_frame].inflight);

    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signal_semaphores,
        .swapchainCount = 1,
        .pSwapchains = &state.swapchain.handle,
        .pImageIndices = &image_index,
    };

    vkQueuePresentKHR(state.device.gfx_queue, &present_info);

    state.current_frame =
        (state.current_frame + 1) % state.swapchain.frame_count;
}

void render_draw(static_mesh* mesh, mat4 mvp) {
    if (mesh->index_buffer == -1 || mesh->vertex_buffer == -1) {
        return;
    }

    VkCommandBuffer cmd = state.cmdbuffers[state.current_frame];
    buffer_vk       vbuf = state.buffers[mesh->vertex_buffer];
    buffer_vk       ibuf = state.buffers[mesh->index_buffer];

    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, &vbuf.handle, offsets);
    vkCmdBindIndexBuffer(cmd, ibuf.handle, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, state.pipeline);

    vkCmdPushConstants(cmd, state.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT,
                       0, sizeof(mat4), &mvp);

    vkCmdDrawIndexed(cmd, mesh->index_count, 1, 0, 0, 0);
}

void render_create_mesh_buffers(static_mesh* mesh) {
    uint32_t vertex_buf_size = sizeof(vertex) * mesh->vertex_count;
    uint32_t index_buf_size = sizeof(uint32_t) * mesh->index_count;

    create_buffer_vk(
        state, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &state.buffers[0],
        vertex_buf_size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

    if (state.buffers[0].id == -1) {
        return;
    }
    create_buffer_vk(
        state, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &state.buffers[1],
        index_buf_size,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

    if (state.buffers[1].id == -1) {
        return;
    }
    upload_to_gpu_vk(state, &state.buffers[0], mesh->vertices, vertex_buf_size);
    upload_to_gpu_vk(state, &state.buffers[1], mesh->indices, index_buf_size);
    mesh->vertex_buffer = state.buffers[0].id;
    mesh->index_buffer = state.buffers[1].id;
}

#include "render_module/render_surface_vk_wl.c"
#include "render_module/render_vk.c"
*/
