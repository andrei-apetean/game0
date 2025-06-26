#pragma once

#include "base.h"
#include "math_types.h"
#include "render_module/render_types.h"

typedef u32 rbuf_id;
typedef u32 rpass_id;
typedef u32 rpipe_id;

typedef struct rcmd rcmd;

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

void rcmd_draw(rcmd* cmd, u32 first_vertex, u32 vertex_count, u32 first_instance,
               u32 instance_count);
void rcmd_draw_indexed(rcmd* cmd, u32 instance_count, u32 first_instance,
                       u32 first_index, u32 vertex_offset);

void rdev_init();
void rdev_term();

void rdev_create_swapchain(void* window, u32 w, u32 h);
void rdev_resize_swapchain(u32 w, u32 height);
void rdev_destroy_swapchain();

rbuf_id  rdev_create_buffer();
rpass_id rdev_create_renderpass();
rpipe_id rdev_create_pipeline();

void rdev_destroy_buffer(rbuf_id id);
void rdev_destroy_renderpass(rpass_id id);
void rdev_destroy_pipeline(rpipe_id id);
