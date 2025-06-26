#pragma once

#include "rtypes.h"

#define RDEV_INVALID_ID 0xFFFFFFFF
typedef enum {
  RDEV_WND_API_WL = 0,  
  RDEV_WND_API_XCB,  
  RDEV_WND_API_WIN32,  
} rdev_wnd_api;

void rdev_init(rdev_params* params);
void rdev_terminate();

rcmd* rdev_begin();
void  rdev_end(rcmd* cmd);

void rdev_create_swapchain(void* wnd_native, uint32_t w, uint32_t h);
void rdev_resize_swapchain(uint32_t w, uint32_t h);
void rdev_destroy_swapchain();

rbuf_id rdev_create_buffer();
void    rdev_destroy_buffer(rbuf_id id);

rpass_id rdev_create_renderpass();
rpass_id rdev_swapchain_renderpass();
void     rdev_destroy_renderpass(rpass_id id);

rpipe_id rdev_create_pipeline();
void     rdev_destroy_pipeline(rpipe_id id);
