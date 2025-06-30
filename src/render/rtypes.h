#pragma once
#include "../core/mem.h"

typedef uint32_t rbuffer_id;
typedef uint32_t rpass_id;
typedef uint32_t rpipe_id;

typedef struct rcmd rcmd;

typedef enum {
    RDEV_WND_WL = 0,
    RDEV_WND_XCB,
    RDEV_WND_WIN32,
} rdev_wnd;

typedef struct {
    rdev_wnd   wnd_api;
    allocator* scratch_allocator;
    allocator* allocator;
} rdev_params;

// ==============================================================
//
// pipeline
//
// ==============================================================

typedef enum {
    RSHADER_TYPE_VERTEX,
    RSHADER_TYPE_FRAGMENT,
} rshader_type;

typedef enum {
    RVERTEX_FORMAT_FLOAT = 0,
    RVERTEX_FORMAT_FLOAT2,
    RVERTEX_FORMAT_FLOAT3,
    RVERTEX_FORMAT_FLOAT4,
    RVERTEX_FORMAT_INT,
    RVERTEX_FORMAT_INT2,
    RVERTEX_FORMAT_INT3,
    RVERTEX_FORMAT_INT4,
    RVERTEX_FORMAT_UINT,
    RVERTEX_FORMAT_UINT2,
    RVERTEX_FORMAT_UINT3,
    RVERTEX_FORMAT_UINT4,
    RVERTEX_FORMAT_UNORM8_4,  // 4 bytes, normalized to 0-1
} rvertex_format;

typedef struct {
    uint32_t       binding;
    uint32_t       location;
    uint32_t       offset;
    rvertex_format format;
} rvertex_attrib;

typedef struct {
    uint32_t binding;
    uint32_t stride;
} rvertex_binding;

typedef struct {
    rshader_type type;
    uint32_t     code_size;
    uint32_t*    code;
} rshader_stage;

typedef enum {
    RSHADER_STAGE_VERTEX = 0x01,
    RSHADER_STAGE_FRAGMENT = 0x02,
    RSHADER_STAGE_COMPUTE = 0x04,
    RSHADER_STAGE_GEOMETRY = 0x08,
    RSHADER_STAGE_ALL_GRAPHICS = 0x0F,
} rshader_stage_flags;

typedef enum {
    RDESCRIPTOR_TYPE_UNIFORM_BUFFER = 0,
    RDESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    RDESCRIPTOR_TYPE_STORAGE_BUFFER,
    RDESCRIPTOR_TYPE_STORAGE_IMAGE,
} rdescriptor_type;

typedef struct {
    uint32_t         binding;
    rdescriptor_type type;
} rdescriptor_binding;

typedef struct {
    rshader_stage_flags stage_flags;
    uint32_t            size;
    uint32_t            offset;
} rpush_constant;

typedef struct {
    rpass_id         renderpass;
    rshader_stage*   shader_stages;
    uint32_t         shader_stage_count;
    rvertex_attrib*  vertex_attributes;
    uint32_t         vertex_attribute_count;
    rvertex_binding* vertex_bindings;
    uint32_t         vertex_binding_count;
    rpush_constant*  push_constants;
    uint32_t         push_constant_count;
} rpipe_params;

// ==============================================================
//
// buffers
//
// ==============================================================

typedef enum {
    RBUF_USAGE_VERTEX = 0x00000020,
    RBUF_USAGE_INDEX = 0x00000040,
    RBUF_USAGE_UNIFORM = 0x00000010,
    RBUF_USAGE_STORAGE = 0x00000200,
    RBUF_USAGE_TRANSFER_SRC = 0x00000001,
    RBUF_USAGE_TRANSFER_DST = 0x00000002,
} rbuf_usage;

typedef enum {
    RBUF_MEM_DEVICE_LOCAL = 0x00000001,
    RBUF_MEM_HOST_VISIBLE = 0x00000002,
    RBUF_MEM_HOST_COHERENT = 0x00000004,
    RBUF_MEM_HOST_CACHED = 0x00000008,
} rbuf_memory;

typedef struct {
    uint32_t size;
    uint32_t usage_flags;
    uint32_t memory_flags;
    void*    initial_data;
} rbuf_params;
