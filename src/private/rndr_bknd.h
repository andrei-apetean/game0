#pragma once

#include <stdint.h>

struct rndr_cfg {
    uint32_t win_api;
    uint32_t width;
    uint32_t height;
    void*    win_handle;
    void*    win_inst;
};

typedef struct rndr_bknd rndr_bknd;

uint32_t rndr_bknd_get_size();
int32_t  rndr_bknd_startup(rndr_bknd** bknd, struct rndr_cfg cfg);
void     rndr_bknd_render_begin(rndr_bknd* bknd);
void     rndr_bknd_render_end(rndr_bknd* bknd);
void     rndr_bknd_teardown(rndr_bknd* bknd);
