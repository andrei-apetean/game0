#include "window.h"

#include <assert.h>

#include "defines.h"
#include "window_api.h"
#include "window_wl.h"

struct window{
    window_api api;
    void*      data;
};

int32_t window_get_memory_size() {
    uint32_t internal_data_size = 0;
#if GAME0_LINUX
    internal_data_size = window_get_data_size_wl();
#else
#error "Unimplemented window API"
#endif  //  GAME0_LINUX
    return sizeof(window) + internal_data_size;
}

int32_t window_startup(window* window, struct window_config* cfg) {
    assert(window);
    window->data = (void*)(window + 1);
#if GAME0_LINUX
    window->api.startup = window_startup_wl;
    window->api.poll_events = window_poll_events_wl;
    window->api.teardown = window_teardown_wl;
    window->api.get_native_handle = window_get_native_handle_wl;
#else
#error "Unimplemented window API"
#endif  //  GAME0_LINUX
    return window->api.startup(window->data, cfg);
}

int32_t window_poll_events(window* window) {
    assert(window);
    return window->api.poll_events(window->data);
}

void window_teardown(window* window) {
    assert(window);
    window->api.poll_events(window->data);
}

void* window_get_native_handle(window* window) {
    assert(window);
    return window->api.get_native_handle(window->data);
}
#if GAME0_LINUX
#include "window_wl.c"
#else
#error "Unimplemented window API"
#endif  //  GAME0_LINUX
