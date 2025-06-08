#pragma once

#include "core.h"
#define RENDER_MODULE_ID 1
#define RENDER_MODULE_NAME "render_module"

typedef struct render_module_state render_module_state;

module_info register_render_module();

void render_begin();
void render_draw();
void render_end();
void render_resize(uint32_t width, uint32_t height);

