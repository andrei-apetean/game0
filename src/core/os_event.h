#pragma once

#include <stdint.h>
typedef enum {
    KEY_NONE = 0,
    KEY_A = 0x04,
    KEY_B = 0x05,
    KEY_C = 0x06,
    KEY_D = 0x07,
    KEY_E = 0x08,
    KEY_F = 0x09,
    KEY_G = 0x0a,
    KEY_H = 0x0b,
    KEY_I = 0x0c,
    KEY_J = 0x0d,
    KEY_K = 0x0e,
    KEY_L = 0x0f,
    KEY_M = 0x10,
    KEY_N = 0x11,
    KEY_O = 0x12,
    KEY_P = 0x13,
    KEY_Q = 0x14,
    KEY_R = 0x15,
    KEY_S = 0x16,
    KEY_T = 0x17,
    KEY_U = 0x18,
    KEY_V = 0x19,
    KEY_W = 0x1a,
    KEY_X = 0x1b,
    KEY_Y = 0x1c,
    KEY_Z = 0x1d,
    // keyboard numbers [0-9]
    KEY_1 = 0x1e,
    KEY_2 = 0x1f,
    KEY_3 = 0x20,
    KEY_4 = 0x21,
    KEY_5 = 0x22,
    KEY_6 = 0x23,
    KEY_7 = 0x24,
    KEY_8 = 0x25,
    KEY_9 = 0x26,
    KEY_0 = 0x27,
    // edit keys
    KEY_ENTER = 0x28,
    KEY_ESCAPE = 0x29,
    KEY_BACKSPACE = 0x2a,
    KEY_TAB = 0x2b,
    KEY_SPACE = 0x2c,
    KEY_MINUS = 0x2d, // - or _
    KEY_EQUAL = 0x2e, //  or +
    KEY_LBRACE = 0x2f, // [ or {
    KEY_RBRACE = 0x30, // ] or }
    KEY_BACKSLASH = 0x31, // \ or |
    KEY_HASHTILDE = 0x32, // # or ~ on non-us kb
    KEY_SEMICOLON = 0x33, // ; or :
    KEY_APOSTROPHE = 0x34, // ' or "
    KEY_GRAVEACCENT = 0x35, // ` or ~
    KEY_COMMA = 0x36, // , or <
    KEY_PERIOD = 0x37, // . or >
    KEY_SLASH = 0x38, // / or ?
    KEY_CAPSLOCK = 0x39,
    // function keys
    KEY_F1 = 0x3a,
    KEY_F2 = 0x3b,
    KEY_F3 = 0x3c,
    KEY_F4 = 0x3d,
    KEY_F5 = 0x3e,
    KEY_F6 = 0x3f,
    KEY_F7 = 0x40,
    KEY_F8 = 0x41,
    KEY_F9 = 0x42,
    KEY_F10 = 0x43,
    KEY_F11 = 0x44,
    KEY_F12 = 0x45,
    // arrow keys
    KEY_RIGHT = 0x4f,
    KEY_LEFT = 0x50,
    KEY_DOWN = 0x51,
    KEY_UP = 0x52,
    // codemodifier keys
    KEY_LCONTROL = 0xe0,
    KEY_LSHIFT = 0xe1,
    KEY_LALT = 0xe2,
    KEY_LMETA = 0xe3,
    KEY_RCONTROL = 0xe4,
    KEY_RSHIFT = 0xe5,
    KEY_RALT = 0xe6,
    KEY_RMETA = 0xe7,
    // mouse buttons
    MOUSE_BUTTON0 = 0xf0,
    MOUSE_BUTTON1 = 0xf1,
    MOUSE_BUTTON2 = 0xf2,
    MOUSE_BUTTON_LEFT = MOUSE_BUTTON0,
    MOUSE_BUTTON_RIGHT = MOUSE_BUTTON1,
    MOUSE_BUTTON_MIDDLE = MOUSE_BUTTON2,
} key_code;

