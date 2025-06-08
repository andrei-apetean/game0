#include "modules/render_module.h"

#include <assert.h>

#include "render_module_backend.h"

typedef struct {
    render_module_backend backend;
    render_module_state*  state;
} render_module;

int32_t render_module_init(void* data) {
    render_module* module = data;
    assert(module);
    load_render_module_backend_vk(&module->backend);
    return 0;
}

int32_t render_module_startup(void* module_data, void* module_state) {
    render_module* module = module_data;
    assert(module);
    module->state = module_state;
    return module->backend.startup(module->state);
    return 0;
}

int32_t render_module_get_size() { return sizeof(render_module); }

int32_t render_module_get_state_size(void* data) {
    render_module* module = data;
    assert(module);
    return module->backend.get_backend_state_size();
}

void render_module_teardown(void* data) {
    render_module* module = data;
    assert(module);
    module->backend.teardown(module->state);
}

module_info register_render_module() {
    module_info info = {
        .module_id = RENDER_MODULE_ID,
        .module_name = RENDER_MODULE_NAME,
        .get_size = render_module_get_size,
        .get_state_size = render_module_get_state_size,
        .init = render_module_init,
        .startup = render_module_startup,
        .teardown = render_module_teardown,
    };
    return info;
}

void render_begin() {
    
    render_module* module = core_get_module(RENDER_MODULE_ID);
    assert(module);
    module->backend.render_begin(module->state);
}
void render_draw() {
    
    // render_module* module = core_get_module(RENDER_MODULE_ID);
    // assert(module);
    // module->backend.render_draw(module->state);
}
void render_end() {
    
    render_module* module = core_get_module(RENDER_MODULE_ID);
    assert(module);
    module->backend.render_end(module->state);
}
void render_resize(uint32_t width, uint32_t height) {
    
    render_module* module = core_get_module(RENDER_MODULE_ID);
    assert(module);
    module->backend.resize(module->state, width, height);
}

