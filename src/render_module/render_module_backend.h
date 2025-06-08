#pragma once
#include <stdint.h>

#include "modules/render_module.h"

typedef struct {
    int32_t (*startup)(render_module_state*);
    int32_t (*get_backend_state_size)();
    void (*render_begin)(render_module_state*);
    void (*render_end)(render_module_state*);
    void (*resize)(render_module_state*, uint32_t, uint32_t);
    void (*teardown)(render_module_state*);
} render_module_backend;

void load_render_module_backend_vk(render_module_backend* backend);
