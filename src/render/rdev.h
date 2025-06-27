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

rbuf_id rdev_create_buffer();
void    rdev_destroy_buffer(rbuf_id id);

rpass_id rdev_create_renderpass();
rpass_id rdev_swapchain_renderpass();
void     rdev_destroy_renderpass(rpass_id id);

rpipe_id rdev_create_pipeline();
void     rdev_destroy_pipeline(rpipe_id id);

//=========================================================
//
// draw commands
// 
//=========================================================
rcmd* rdev_begin();
void  rdev_end(rcmd* cmd);

void rcmd_begin_pass(rcmd* cmd, rpass_id id);
void rcmd_begin_pipe(rcmd* cmd, rpipe_id id);
void rcmd_begin_vertex_buf(rcmd* cmd, rbuf_id id);
void rcmd_begin_index_buf(rcmd* cmd, rbuf_id id);
void rcmd_begin_descriptor_set(rcmd* cmd, rbuf_id id);

void rcmd_end_pass(rcmd* cmd, rpass_id id);
void rcmd_end_pipe(rcmd* cmd, rpipe_id id);
void rcmd_end_vertex_buf(rcmd* cmd, rbuf_id id);
void rcmd_end_index_buf(rcmd* cmd, rbuf_id id);
void rcmd_end_descriptor_set(rcmd* cmd, rbuf_id id);

void rcmd_clear_color(float r, float g, float b);
void rcmd_draw(rcmd* cmd, uint32_t first_vertex, uint32_t vertex_count,
               uint32_t first_instance, uint32_t instance_count);
void rcmd_draw_indexed(rcmd* cmd, uint32_t instance_count, uint32_t first_instance,
                       uint32_t first_index, uint32_t vertex_offset);
