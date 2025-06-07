#pragma once

#include <stdint.h>
#include "engine.h"

typedef struct {
    uint32_t win_api;
    uint32_t width;
    uint32_t height;
    void*    win_handle;
    void*    win_inst;
} render_cfg;

typedef struct render_backend render_backend;

uint32_t render_backend_get_size();
int32_t  render_backend_startup(render_backend** backend, render_cfg cfg);
void     render_backend_resize(render_backend* backend, vec2 dimensions);
void     render_backend_render_begin(render_backend* backend);
void     render_backend_render_end(render_backend* backend);
void     render_backend_teardown(render_backend* backend);

void render_backend_draw_mesh(render_backend* backend, mesh* m);
