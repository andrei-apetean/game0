#pragma once

#include <stdint.h>

#include "private/input.h"

struct window_info {
    void*    win_handle;
    void*    win_instance;
    uint32_t win_api;
};

struct window_cfg {
    uint32_t    width;
    uint32_t    height;
    const char* title;
};

typedef struct win_bknd win_bknd;

int32_t win_bknd_get_size();
int32_t win_bknd_startup(win_bknd** backend, struct window_cfg cfg);
int32_t win_bknd_create_window(win_bknd* backend);
void    win_bknd_poll_events(win_bknd* backend);
void    win_bknd_destroy_window(win_bknd* backend);
void    win_bknd_teardown(win_bknd* backend);
void    win_bknd_attach_handler(win_bknd* backend, pfn_on_input_event on_event);

struct window_info win_bknd_get_info(win_bknd* backend);
