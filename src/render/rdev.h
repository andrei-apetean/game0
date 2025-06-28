#pragma once

#include "rtypes.h"

#define RDEV_INVALID_ID 0xFFFFFFFF
typedef enum {
    RDEV_WND_API_WL = 0,
    RDEV_WND_API_XCB,
    RDEV_WND_API_WIN32,
} rdev_wnd_api;

void rdev_init(rdev_params* params);
void rdev_terminate();

void rdev_create_swapchain(void* wnd_native, uint32_t w, uint32_t h);
void rdev_resize_swapchain(uint32_t w, uint32_t h);
void rdev_destroy_swapchain();

rpass_id rdev_create_renderpass();
rpass_id rdev_swapchain_renderpass();
void     rdev_destroy_renderpass(rpass_id id);

rpipe_id rdev_create_pipeline(rpipe_params* params);
void     rdev_destroy_pipeline(rpipe_id id);

//=========================================================
//
// buffers
//
//=========================================================

rbuffer_id rdev_create_buffer(rbuf_params* params);
void       rdev_destroy_buffer(rbuffer_id id);

void  rdev_buffer_upload(rbuffer_id id, void* data, uint32_t size, uint32_t offset);
void  rdev_buffer_download(rbuffer_id id, void* data, uint32_t size,
                           uint32_t offset);
void* rdev_buffer_map(rbuffer_id id);
void  rdev_buffer_unmap(rbuffer_id id);

rbuffer_id rdev_create_vertex_buffer(uint32_t size, void* data);
rbuffer_id rdev_create_index_buffer(uint32_t size, void* data);
rbuffer_id rdev_create_uniform_buffer(uint32_t size);
rbuffer_id rdev_create_staging_buffer(uint32_t size);

//=========================================================
//
// draw commands
//
//=========================================================

rcmd* rdev_begin();
void  rdev_end(rcmd* cmd);

void rcmd_begin_pass(rcmd* cmd, rpass_id id);
void rcmd_end_pass(rcmd* cmd, rpass_id id);

void rcmd_bind_pipe(rcmd* cmd, rpipe_id id);
void rcmd_bind_vertex_buffer(rcmd* cmd, rbuffer_id id);
void rcmd_bind_index_buffer(rcmd* cmd, rbuffer_id id);
void rcmd_bind_descriptor_set(rcmd* cmd, rbuffer_id id);

void rcmd_push_constants(rcmd* cmd, rpipe_id id, rshader_stage_flags flags,
                         uint32_t offset, uint32_t size, void* data);
void rcmd_draw(rcmd* cmd, uint32_t first_vertex, uint32_t vertex_count,
               uint32_t first_instance, uint32_t instance_count);

void rcmd_draw_indexed(rcmd* cmd, uint32_t index_count, uint32_t instance_count,
                       uint32_t first_index, uint32_t vertex_offset,
                       uint32_t first_instance);
