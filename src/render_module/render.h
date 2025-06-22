#pragma once

#include <stdint.h>

#include "render_module/render_types.h"
#include "math_types.h"

typedef struct {
    uint32_t* vertex_shader_code;
    uint32_t  vertex_shader_code_size;
    uint32_t* fragment_shader_code;
    uint32_t  fragment_shader_code_size;
} render_pipeline_config;

int32_t render_device_connect(uint32_t window_api, void* window);
void    render_device_disconnect();
int32_t render_device_create_swapchain(uint32_t width, uint32_t height);
int32_t render_device_resize_swapchain(uint32_t width, uint32_t height);
void    render_device_destroy_swapchain();

void render_device_begin_frame();
void render_device_end_frame();

buffer_id render_device_create_buffer(buffer_config* config);
void      render_device_destroy_buffer(buffer_id buffer);
// todo: temp
void render_device_draw_mesh(static_mesh* mesh, mat4 mvp);

int32_t render_device_create_render_pipeline(render_pipeline_config* config);
void    render_device_destroy_render_pipeline();
