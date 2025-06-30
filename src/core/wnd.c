#include "wnd.h"

#include "../base.h"
#include "wnd_backend.h"

static wnd_api wnd = {0};
static uint8_t is_initialized = 0;

void wnd_init() {
#ifdef WINDOW_BACKEND_LINUX
    load_wnd_wl(&wnd);
    wnd.init();
    is_initialized = 1;
    debug_log("Window initialized\n");

#else
#error "Unimplemented window backend!"
#endif  // WINDOW_BACKEND_LINUX
}

void wnd_terminate() {
    debug_assert(is_initialized);
    wnd.terminate();
    is_initialized = 0;
}

void wnd_open(const char* title, uint32_t w, uint32_t h) {
    debug_assert(is_initialized);
    wnd.open(title, w, h);
}
void wnd_dispatch_events() {
    debug_assert(is_initialized);
    wnd.dispatch_events();
}

void wnd_set_title(const char* title) {
    debug_assert(is_initialized);
    wnd.set_title(title);
}

void wnd_get_size(uint32_t* w, uint32_t* h) {
    debug_assert(is_initialized);
    wnd.get_size(w, h);
}

void wnd_attach_dispatcher(wnd_dispatcher* disp) {
    debug_assert(is_initialized);
    wnd.attach_dispatcher(disp);
}

uint32_t wnd_backend_id() {
    debug_assert(is_initialized);
    return wnd.backend_id();
}

void* wnd_native_handle() {
    debug_assert(is_initialized);
    return wnd.native_handle();
}
