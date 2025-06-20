#pragma once

#include <stdint.h>

#if defined(_WIN32) || defined(_WIN64)
#define WINDOW_BACKEND_WIN32
#elif defined(__linux__)
#define WINDOW_BACKEND_LINUX
#endif

#define WINDOW_BACKEND_WL_ID 0
#define WINDOW_BACKEND_XCB_ID 1
#define WINDOW_BACKEND_WIN32_ID 2

#define KEY_NONE 0
#define KEY_A 0x04
#define KEY_B 0x05
#define KEY_C 0x06
#define KEY_D 0x07
#define KEY_E 0x08
#define KEY_F 0x09
#define KEY_G 0x0a
#define KEY_H 0x0b
#define KEY_I 0x0c
#define KEY_J 0x0d
#define KEY_K 0x0e
#define KEY_L 0x0f
#define KEY_M 0x10
#define KEY_N 0x11
#define KEY_O 0x12
#define KEY_P 0x13
#define KEY_Q 0x14
#define KEY_R 0x15
#define KEY_S 0x16
#define KEY_T 0x17
#define KEY_U 0x18
#define KEY_V 0x19
#define KEY_W 0x1a
#define KEY_X 0x1b
#define KEY_Y 0x1c
#define KEY_Z 0x1d
// keyboard numbers [0-9]
#define KEY_1 0x1e
#define KEY_2 0x1f
#define KEY_3 0x20
#define KEY_4 0x21
#define KEY_5 0x22
#define KEY_6 0x23
#define KEY_7 0x24
#define KEY_8 0x25
#define KEY_9 0x26
#define KEY_0 0x27
// edit keys
#define KEY_ENTER 0x28
#define KEY_ESCAPE 0x29
#define KEY_BACKSPACE 0x2a
#define KEY_TAB 0x2b
#define KEY_SPACE 0x2c
// - or _
#define KEY_MINUS 0x2d
//  or +
#define KEY_EQUAL 0x2e
// [ or {
#define KEY_LBRACE 0x2f
// ] or }
#define KEY_RBRACE 0x30
// \ or |
#define KEY_BACKSLASH 0x31
// # or ~ on non-us kb
#define KEY_HASHTILDE 0x32
// ; or :
#define KEY_SEMICOLON 0x33
// ' or "
#define KEY_APOSTROPHE 0x34
// ` or ~
#define KEY_GRAVEACCENT 0x35
// , or <
#define KEY_COMMA 0x36
// . or >
#define KEY_PERIOD 0x37
// / or ?
#define KEY_SLASH 0x38
#define KEY_CAPSLOCK 0x39
// function keys
#define KEY_F1 0x3a
#define KEY_F2 0x3b
#define KEY_F3 0x3c
#define KEY_F4 0x3d
#define KEY_F5 0x3e
#define KEY_F6 0x3f
#define KEY_F7 0x40
#define KEY_F8 0x41
#define KEY_F9 0x42
#define KEY_F10 0x43
#define KEY_F11 0x44
#define KEY_F12 0x45
// arrow keys
#define KEY_RIGHT 0x4f
#define KEY_LEFT 0x50
#define KEY_DOWN 0x51
#define KEY_UP 0x52
// codemodifier keys
#define KEY_LCONTROL 0xe0
#define KEY_LSHIFT 0xe1
#define KEY_LALT 0xe2
#define KEY_LMETA 0xe3
#define KEY_RCONTROL 0xe4
#define KEY_RSHIFT 0xe5
#define KEY_RALT 0xe6
#define KEY_RMETA 0xe7
// mouse buttons
#define MOUSE_BUTTON0 0xf0
#define MOUSE_BUTTON1 0xf1
#define MOUSE_BUTTON2 0xf2
#define MOUSE_BUTTON_LEFT MOUSE_BUTTON0
#define MOUSE_BUTTON_RIGHT MOUSE_BUTTON1
#define MOUSE_BUTTON_MIDDLE MOUSE_BUTTON2

typedef struct window_state window_state;

typedef struct {
    uint32_t    width;
    uint32_t    height;
    const char* title;
} window_params;

typedef void (*pfn_keyboard_key)(int32_t key, int32_t state, void* user);
typedef void (*pfn_pointer_button)(int32_t key, int32_t state, void* user);
typedef void (*pfn_pointer_motion)(float x, float y, void* user);
typedef void (*pfn_pointer_axis)(float value, void* user);
typedef void (*pfn_window_size)(int32_t width, int32_t height, void* user);
typedef void (*pfn_window_close)(void* user);

void     window_initialize();
void     window_terminate();
void     window_poll_events();
void     window_destroy();
uint32_t window_get_backend_id();
int32_t  window_create(window_params* params);

void window_set_title(const char* title);
void  window_set_key_handler(pfn_keyboard_key on_key, void* user_data);
void  window_set_button_handler(pfn_pointer_button on_button, void* user_data);
void  window_set_motion_handler(pfn_pointer_motion on_motion, void* user_data);
void  window_set_axis_handler(pfn_pointer_axis on_axis, void* user_data);
void  window_set_size_handler(pfn_window_size on_size, void* user_data);
void  window_set_close_handler(pfn_window_close on_close, void* user_data);
void  window_get_size(uint32_t* width, uint32_t* height);
void* window_get_native_handle();
