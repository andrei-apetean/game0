#pragma once
#include "core/mem.h"

typedef uint32_t rbuf_id;
typedef uint32_t rpass_id;
typedef uint32_t rpipe_id;

typedef struct rcmd rcmd;

typedef enum {
  RDEV_WND_WL = 0,  
  RDEV_WND_XCB,  
  RDEV_WND_WIN32,  
} rdev_wnd;

typedef struct {
    rdev_wnd           wnd_api;
    scratch_allocator* scratch_allocator;
    allocator*         allocator;
} rdev_params;
