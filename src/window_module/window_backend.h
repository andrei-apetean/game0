#pragma once

#include "window_module/window.h"

typedef struct {
    pfn_keyboard_key on_key;
    void*            user_data;
} keyboard_key_handler;

typedef struct {
    pfn_pointer_button on_button;
    void*              user_data;
} pointer_button_handler;

typedef struct {
    pfn_pointer_motion on_motion;
    void*              user_data;
} pointer_motion_handler;

typedef struct {
    pfn_pointer_axis on_axis;
    void*            user_data;
} pointer_axis_handler;

typedef struct {
    pfn_window_size on_size;
    void*           user_data;
} window_size_handler;

typedef struct {
    pfn_window_close on_close;
    void*            user_data;
} window_close_handler;

typedef struct {
    int32_t (*get_backend_state_size)();
    int32_t (*startup)(window_state*);
    int32_t (*create_window)(window_state*, window_params*);
    void (*destroy_window)(window_state*);
    void* (*get_window_handle)(window_state*);
    void (*poll_events)(window_state*);
    void (*teardown)(window_state*);
    void (*get_window_size)(window_state*, uint32_t*, uint32_t*);
    void (*set_window_title)(window_state*, const char*);
    void (*set_key_handler)(window_state*, pfn_keyboard_key, void*);
    void (*set_pointer_button_handler)(window_state*, pfn_pointer_button,
                                       void*);
    void (*set_pointer_motion_handler)(window_state*, pfn_pointer_motion,
                                       void*);
    void (*set_pointer_axis_handler)(window_state*, pfn_pointer_axis, void*);
    void (*set_window_size_handler)(window_state*, pfn_window_size, void*);
    void (*set_window_close_handler)(window_state*, pfn_window_close, void*);
} window_backend;

#ifdef WINDOW_BACKEND_LINUX

void load_window_module_backend_wl(window_backend* backend);

#endif  //  WINDOW_BACKEND_LINUX
