#pragma once

#include <stdint.h>

#include "mathf.h"
#include "render_types.h"

typedef struct {
    vertex*   vertices;
    uint32_t  vertex_count;
    uint32_t* indices;
    uint32_t  index_count;
    int32_t   vertex_buffer;
    int32_t   index_buffer;
} static_mesh;

void render_initialize(render_params* params);
void render_terminate();
void render_begin();
void render_draw(static_mesh* mesh, mat4 mvp);
void render_end();
void render_resize(uint32_t width, uint32_t height);
void render_create_mesh_buffers(static_mesh* mesh);

