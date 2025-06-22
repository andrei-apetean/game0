#pragma once
#include <stdint.h>

#define RENDER_INVALID_ID 0xffffffff

typedef struct {
    float x, y, z, r, g, b;
} vertex;

typedef enum {
    RENDER_API_NONE = 0,
    RENDER_API_VULKAN,
} render_api;

typedef uint32_t buffer_id;
typedef uint32_t texture_id;
typedef uint32_t sampler_id;
typedef uint32_t descriptor_set_id;
typedef uint32_t descriptor_set_layout_id;
typedef uint32_t pipeline_id;
typedef uint32_t renderpass_id;

typedef struct {
    vertex*   vertices;
    uint32_t* indices;
    uint32_t  vertex_count;
    uint32_t  index_count;
    buffer_id vertex_buffer;
    buffer_id index_buffer;
} static_mesh;

typedef struct {
    uint16_t width;
    uint16_t height;
    uint16_t window_api;
    void*    window;
    // todo: add allocator
} device_config;

typedef enum {
    transfer_src_bit = 0x00000001,
    transfer_dst_bit = 0x00000002,
    uniform_buffer_bit = 0x00000010,
    storage_buffer_bit = 0x00000020,
    index_buffer_bit = 0x00000040,
    vertex_buffer_bit = 0x00000080,
} buffer_usage_flags;

typedef struct {
    uint32_t usage_flags;
    uint32_t size;
    void*    data;
} buffer_config;
