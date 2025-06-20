#pragma once 
#include <stdint.h>

typedef struct {
    float x;
    float y;
    float z;
    float r;
    float g;
    float b;
} vertex;

typedef struct {
    uint32_t window_api;
    uint32_t swapchain_width;
    uint32_t swapchain_height;
    void*    window_handle;
} render_params;
