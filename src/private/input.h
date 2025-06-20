#pragma once

#include <stdint.h>

#define KB_MAX_KEYS 256

#define KEY_STATE_UP 0
#define KEY_STATE_PRESSED 1
#define KEY_STATE_DOWN 2
#define KEY_STATE_RELEASED 3

#define INPUT_EVENT_KEY 0
#define INPUT_EVENT_POINTER 1
#define INPUT_EVENT_SCROLL 2
#define INPUT_EVENT_SIZE 3
#define INPUT_EVENT_QUIT 4

typedef struct {
    int32_t type;
    union {
        struct {
            uint32_t code;
            uint32_t state;
        } key;
        struct {
            float x;
            float y;
        } pointer;
        struct {
            int32_t value;
        } scroll;
        struct {
            int32_t width;
            int32_t height;
        } window;
    };
} input_ev;


typedef void (*pfn_on_input_event)(input_ev);

typedef struct {
    uint32_t keys[KB_MAX_KEYS];
    float pointer_x;
    float pointer_y;
    float last_pointer_x;
    float last_pointer_y;
} input_state;
