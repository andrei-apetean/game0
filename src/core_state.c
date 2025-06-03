#include <stdio.h>
#include <stdlib.h>

#include "game0.h"
#include "private/base.h"
#include "private/input.h"
#include "private/rndr_bknd.h"
#include "private/win_bknd.h"

struct core_state {
    win_bknd*    win_bknd;
    rndr_bknd*   rndr_bknd;
    struct input input;
    int32_t      is_running;
};

static struct core_state core = {0};

void process_input();

void on_event(struct input_ev e) {
    switch (e.type) {
        case INPUT_EVENT_KEY: {
            if (e.key.code > 0 && e.key.code < KB_MAX_KEYS) {
                core.input.keys[e.key.code] = e.key.state;
                // printf("Key event: code %d, state %d\n", e.key.code,
                // e.key.state);
            }
            break;
        }
        case INPUT_EVENT_POINTER: {
            core.input.pointer_x = e.pointer.x;
            core.input.pointer_y = e.pointer.y;
            break;
        }
        case INPUT_EVENT_SCROLL: {
            // unused
            break;
        }
        case INPUT_EVENT_SIZE: {
            // don't care for now
            break;
        }
        case INPUT_EVENT_QUIT: {
            stop_running();
            break;
        }
        default:
            UNREACHABLE();
            break;
    }
}

int32_t startup(struct config cfg) {
    size_t wndbknd_size = win_bknd_get_size();
    size_t rndrbknd_size = rndr_bknd_get_size();
    core.win_bknd = malloc(wndbknd_size);
    core.rndr_bknd = malloc(rndrbknd_size);
    if (!core.win_bknd) return -1;
    if (!core.rndr_bknd) return -1;

    const struct window_cfg wcfg = {
        .width = cfg.window_width,
        .height = cfg.window_height,
        .title = cfg.window_title,
    };
    if (win_bknd_startup(&core.win_bknd, wcfg) != 0) return -1;
    win_bknd_attach_handler(core.win_bknd, on_event);

    struct window_info    winfo = win_bknd_get_info(core.win_bknd);
    const struct rndr_cfg rcfg = {
        .height = cfg.window_height,
        .width = cfg.window_width,
        .win_api = winfo.win_api,
        .win_handle = winfo.win_handle,
        .win_inst = winfo.win_instance,
    };
    if (rndr_bknd_startup(&core.rndr_bknd, rcfg)) return -1;

    core.is_running = 1;
    return 0;
}

int32_t teardown() {
    if (core.rndr_bknd) rndr_bknd_teardown(core.rndr_bknd);
    if (core.win_bknd) win_bknd_teardown(core.win_bknd);
    return 0;
}

int32_t is_running() { return core.is_running; }

void stop_running() { core.is_running = 0; }

void poll_events() {
    process_input();
    win_bknd_poll_events(core.win_bknd);
}

void render_begin() {
    rndr_bknd_render_begin(core.rndr_bknd);
}

void render_end() {
    rndr_bknd_render_end(core.rndr_bknd);
}

int32_t is_key_down(int32_t key) {
    return (key > 0 && key < KB_MAX_KEYS)
               ? core.input.keys[key] == KEY_STATE_DOWN
               : 0;
}

int32_t is_key_pressed(int32_t key) {
    return (key > 0 && key < KB_MAX_KEYS)
               ? core.input.keys[key] == KEY_STATE_PRESSED
               : 0;
}

int32_t is_key_released(int32_t key) {
    return (key > 0 && key < KB_MAX_KEYS)
               ? core.input.keys[key] == KEY_STATE_RELEASED
               : 0;
}

int32_t is_button_down(int32_t button) {
    return (button > 0 && button < KB_MAX_KEYS)
               ? core.input.keys[button] == KEY_STATE_DOWN
               : 0;
}

int32_t is_button_pressed(int32_t button) {
    return (button > 0 && button < KB_MAX_KEYS)
               ? core.input.keys[button] == KEY_STATE_PRESSED
               : 0;
}

int32_t is_button_released(int32_t button) {
    return (button > 0 && button < KB_MAX_KEYS)
               ? core.input.keys[button] == KEY_STATE_RELEASED
               : 0;
}

void process_input() {
    uint32_t* keys = core.input.keys;
    for (uint32_t i = 0; i < KB_MAX_KEYS - 1; i++) {
        if (keys[i] == KEY_STATE_PRESSED) {
            keys[i] = KEY_STATE_DOWN;
        }
        if (keys[i] == KEY_STATE_RELEASED) {
            keys[i] = KEY_STATE_UP;
        }
    }
    core.input.last_pointer_x = core.input.pointer_x;
    core.input.last_pointer_y = core.input.pointer_y;
}
