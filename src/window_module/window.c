#include "window_module/window.h"

#include <assert.h>

#include "memory.h"
#include "window_module/window_backend.h"

typedef struct {
    uint32_t       window_backend_id;
    uint32_t       is_intialized;
    window_backend backend;
    window_state*  state;
} window_module;

static window_module module = {0};

void window_initialize() {
#ifdef WINDOW_BACKEND_LINUX
    module.window_backend_id = WINDOW_BACKEND_WL_ID;
    load_window_module_backend_wl(&module.backend);

    uint32_t sz = module.backend.get_backend_state_size();
    module.state = mem_alloc(sz);
    assert(module.state);
    if (module.backend.startup(module.state) == 0) {
        module.is_intialized = 1;
    }
    assert(module.is_intialized && "Failed to initialize window backend!");
#else
#error "Unimplemented window backend!"
#endif  // WINDOW_BACKEND_LINUX
}

void window_terminate() {
    assert(module.is_intialized);
    module.backend.teardown(module.state);
    module.state = NULL;
    module.is_intialized = 0;
}

uint32_t window_get_backend_id() {
    assert(module.is_intialized);
    return module.window_backend_id;
}

int32_t window_create(window_params* params) {
    assert(module.is_intialized);
    return module.backend.create_window(module.state, params);
}

void window_poll_events() {
    assert(module.is_intialized);
    module.backend.poll_events(module.state);
}

void window_destroy() {
    assert(module.is_intialized);
    module.backend.destroy_window(module.state);
}

void window_set_title(const char* title) {
    assert(module.is_intialized);
    module.backend.set_window_title(module.state, title);
}

void window_set_key_handler(pfn_keyboard_key on_key, void* user_data) {
    assert(module.is_intialized);
    module.backend.set_key_handler(module.state, on_key, user_data);
}

void window_set_button_handler(pfn_pointer_button on_button, void* user_data) {
    assert(module.is_intialized);
    module.backend.set_pointer_button_handler(module.state, on_button,
                                              user_data);
}

void window_set_motion_handler(pfn_pointer_motion on_motion, void* user_data) {
    assert(module.is_intialized);
    module.backend.set_pointer_motion_handler(module.state, on_motion,
                                              user_data);
}

void window_set_axis_handler(pfn_pointer_axis on_axis, void* user_data) {
    assert(module.is_intialized);
    module.backend.set_pointer_axis_handler(module.state, on_axis, user_data);
}

void window_set_size_handler(pfn_window_size on_size, void* user_data) {
    assert(module.is_intialized);
    module.backend.set_window_size_handler(module.state, on_size, user_data);
}

void window_set_close_handler(pfn_window_close on_close, void* user_data) {
    assert(module.is_intialized);
    module.backend.set_window_close_handler(module.state, on_close, user_data);
}

void window_get_size(uint32_t* width, uint32_t* height) {
    assert(module.is_intialized);
    module.backend.get_window_size(module.state, width, height);
}

void* window_get_native_handle() {
    assert(module.is_intialized);
    return module.backend.get_window_handle(module.state);
}
