#pragma once

#include "rtypes.h"

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

void rcmd_draw(rcmd* cmd, uint32_t first_vertex, uint32_t vertex_count, uint32_t first_instance,
               uint32_t instance_count);
void rcmd_draw_indexed(rcmd* cmd, uint32_t instance_count, uint32_t first_instance,
                       uint32_t first_index, uint32_t vertex_offset);
