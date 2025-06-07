#pragma once
#include <stdint.h>
struct window_config;
typedef struct window_api {
    int32_t (*get_data_size)();
    int32_t (*startup)(void* self, struct window_config* cfg);
    void (*teardown)(void* self);
    int32_t (*poll_events)(void* self);
    void* (*get_native_handle)(void* self);
} window_api;
