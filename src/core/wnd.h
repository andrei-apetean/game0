#pragma once

#include <stdint.h>


#if defined(_WIN32) || defined(_WIN64)
#define WINDOW_BACKEND_WIN32
#elif defined(__linux__)
#define WINDOW_BACKEND_LINUX
#endif

#define WINDOW_BACKEND_WL_ID 0
#define WINDOW_BACKEND_XCB_ID 1
#define WINDOW_BACKEND_WIN32_ID 2

typedef void (*pfn_keyboard_key)(int32_t key, int32_t state, void* user);
typedef void (*pfn_pointer_button)(int32_t key, int32_t state, void* user);
typedef void (*pfn_pointer_motion)(float x, float y, void* user);
typedef void (*pfn_pointer_axis)(float value, void* user);
typedef void (*pfn_window_size)(int32_t width, int32_t height, void* user);
typedef void (*pfn_window_close)(void* user);

typedef struct {
    pfn_keyboard_key   on_key;
    pfn_pointer_button on_pointer_button;
    pfn_pointer_motion on_pointer_motion;
    pfn_pointer_axis   on_pointer_axis;
    pfn_window_size    on_window_size;
    pfn_window_close   on_window_close;
    void* user_data;
} wnd_dispatcher;

void  wnd_init();
void  wnd_terminate();
void  wnd_dispatch_events();
void  wnd_set_title(const char* title);
void  wnd_get_size(uint32_t* w, uint32_t* h);
void  wnd_attach_dispatcher(wnd_dispatcher* disp);
void  wnd_open(const char* title, uint32_t w, uint32_t h);
void* wnd_native_handle();

uint32_t wnd_backend_id();
