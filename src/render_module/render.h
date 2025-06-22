#pragma once

#include <stdint.h>

#include "render_types.h"

typedef struct {
    vertex*   vertices;
    uint32_t  vertex_count;
    uint32_t* indices;
    uint32_t  index_count;
    int32_t   vertex_buffer;
    int32_t   index_buffer;
} static_mesh;

typedef struct {
    uint32_t* vertex_shader_code;
    uint32_t vertex_shader_code_size;
    uint32_t* fragment_shader_code;
    uint32_t fragment_shader_code_size;
} render_pipeline_config;


int32_t render_device_connect(uint32_t window_api, void* window);
void    render_device_disconnect();
int32_t render_device_create_swapchain(uint32_t width, uint32_t height);
int32_t render_device_resize_swapchain(uint32_t width, uint32_t height);
void    render_device_destroy_swapchain();
int32_t render_swapchain_acquire_next_image();
void render_swapchain_present();

int32_t render_device_create_render_pipeline(render_pipeline_config* config);
void render_device_destroy_render_pipeline();

// void render_initialize(render_params* params);
// void render_terminate();
// void render_begin();
// void render_draw(static_mesh* mesh, mat4 mvp);
// void render_end();
// void render_resize(uint32_t width, uint32_t height);
// void render_create_mesh_buffers(static_mesh* mesh);
