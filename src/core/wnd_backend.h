#pragma once

#include "wnd.h"

typedef struct {
    void (*init)();
    void (*terminate)();
    void (*dispatch_events)();
    void (*set_title)(const char* title);
    void (*get_size)(uint32_t* w, uint32_t* h);
    void (*attach_dispatcher)(wnd_dispatcher* disp);
    void (*open)(const char* title, uint32_t w, uint32_t h);
    void* (*native_handle)();
    uint32_t (*backend_id)();
} wnd_api;

#ifdef WINDOW_BACKEND_LINUX

void load_wnd_wl(wnd_api* api);

#endif  //  WINDOW_BACKEND_LINUX
