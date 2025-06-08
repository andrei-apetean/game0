#include "modules/window_module.h"

#include <assert.h>

#include "core.h"
#include "window_module_backend.h"

typedef struct {
    uint32_t             window_backend_id;
    window_module_backend       backend;
    window_module_state* state;
} window_module;

int32_t window_module_init(void* data) {
#ifdef WINDOW_BACKEND_LINUX
    window_module* module = data;
    assert(module);
    module->window_backend_id = WINDOW_BACKEND_WL_ID;
    load_window_module_backend_wl(&module->backend);
#else
#error "Unimplemented window backend!"
#endif  // WINDOW_BACKEND_LINUX
    return 0;
}

int32_t window_module_startup(void* module_data, void* module_state) {
    window_module* module = module_data;
    assert(module);
    module->state = module_state;
    // return module->backend.startup(module->state);
    module->backend.startup(module->state);

    uint32_t    window_width = 720;
    uint32_t    window_height = 1080;
    const char* window_title = "game_0";
    return module->backend.create_window(module->state, window_width, window_height, window_title);
    return 0;
}

int32_t window_module_get_size() { return sizeof(window_module); }

int32_t window_module_get_state_size(void* data) {
    window_module* module = data;
    assert(module);
    return module->backend.get_backend_state_size();
}

void window_module_teardown(void* data) {
    window_module* module = data;
    assert(module);
    module->backend.teardown(module->state);
}

module_info register_window_module() {
    module_info info = {
        .module_id = WINDOW_MODULE_ID,
        .module_name = WINDOW_MODULE_NAME,
        .get_size = window_module_get_size,
        .get_state_size = window_module_get_state_size,
        .init = window_module_init,
        .startup = window_module_startup,
        .teardown = window_module_teardown,
    };
    return info;
}

uint32_t window_get_backend_id() {
    window_module* module = core_get_module(WINDOW_MODULE_ID);
    assert(module);
    return module->window_backend_id;
}

int32_t window_create(uint32_t width, uint32_t height, const char* title) {
    window_module* module = core_get_module(WINDOW_MODULE_ID);
    assert(module);
    return module->backend.create_window(module->state, width, height, title);
}

void window_poll_events() {
    window_module* module = core_get_module(WINDOW_MODULE_ID);
    assert(module);
    module->backend.poll_events(module->state);
}

void window_destroy() {
    window_module* module = core_get_module(WINDOW_MODULE_ID);
    assert(module);
    module->backend.destroy_window(module->state);
}

void window_set_key_handler(pfn_keyboard_key on_key, void* user_data) {
    window_module* module = core_get_module(WINDOW_MODULE_ID);
    assert(module);
    module->backend.set_key_handler(module->state, on_key, user_data);
}

void window_set_button_handler(pfn_pointer_button on_button, void* user_data) {
    window_module* module = core_get_module(WINDOW_MODULE_ID);
    assert(module);
    module->backend.set_pointer_button_handler(module->state, on_button,
                                               user_data);
}

void window_set_motion_handler(pfn_pointer_motion on_motion, void* user_data) {
    window_module* module = core_get_module(WINDOW_MODULE_ID);
    assert(module);
    module->backend.set_pointer_motion_handler(module->state, on_motion,
                                               user_data);
}

void window_set_axis_handler(pfn_pointer_axis on_axis, void* user_data) {
    window_module* module = core_get_module(WINDOW_MODULE_ID);
    assert(module);
    module->backend.set_pointer_axis_handler(module->state, on_axis, user_data);
}

void window_set_size_handler(pfn_window_size on_size, void* user_data) {
    window_module* module = core_get_module(WINDOW_MODULE_ID);
    assert(module);
    module->backend.set_window_size_handler(module->state, on_size, user_data);
}

void window_set_close_handler(pfn_window_close on_close, void* user_data) {
    window_module* module = core_get_module(WINDOW_MODULE_ID);
    assert(module);
    module->backend.set_window_close_handler(module->state, on_close,
                                             user_data);
}

void window_get_size(uint32_t* width, uint32_t* height) {
    window_module* module = core_get_module(WINDOW_MODULE_ID);
    assert(module);
    module->backend.get_window_size(module->state, width, height);
}

void* window_get_native_handle() {
    window_module* module = core_get_module(WINDOW_MODULE_ID);
    assert(module);
    return module->backend.get_window_handle(module->state);
}

