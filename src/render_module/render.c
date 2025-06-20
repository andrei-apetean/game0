#include "render_module/render.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>

#include "mathf.h"
#include "render_module/render_surface_vk.h"
#include "render_module/render_types.h"
#include "render_module/render_vk.h"

#if _DEBUG
static VkDebugUtilsMessengerEXT dbg_msgr;
#endif  //_DEBUG

static render_vk state = {0};

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

    VkFormat           depth_format = choose_depth_format(state.device.gpu);
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
