#pragma once

#include <stdint.h>
typedef struct window window;

struct window_config{
    uint32_t    width;
    uint32_t    height;
    const char* title;
    void(*pfn_on_event)(void*);
};

int32_t window_get_memory_size();
int32_t window_startup(window* window, struct window_config* cfg);
int32_t window_poll_events(window* window);
void    window_teardown(window* window);
void*   window_get_native_handle(window* window);
