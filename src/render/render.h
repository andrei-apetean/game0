#pragma once

#include <stdint.h>
#include "engine.h"

typedef struct {
    uint32_t win_api;
    uint32_t width;
    uint32_t height;
    void*    win_handle;
} render_cfg;

typedef struct render render;

uint32_t render_get_size();
int32_t  render_startup(render** backend, render_cfg cfg);
void     render_resize(render* backend, vec2 dimensions);
void     render_begin(render* backend);
void     render_end(render* backend);
void     render_teardown(render* backend);

void render_mesh(render* backend, mesh* m);
