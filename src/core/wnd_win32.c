#include "wnd.h"

#if defined(WINDOW_BACKEND_WIN32)
#include "base.h"
#include "core/os_event.h"
#include "core/wnd_backend.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#define WINDOW_CLASS_NAME "game0"

typedef struct {
    HWND      handle;
    HINSTANCE instance;
} handle_win32;

typedef struct {
    uint32_t       width;
    uint32_t       height;
    wnd_dispatcher dispatcher;
    handle_win32   window;
} wnd_win32;

static wnd_win32 backend = {0};

static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM param);
static uint32_t         win32_key_to_keycode(uint32_t code);

void wnd_init_win32() {
    backend.window.instance = GetModuleHandle(NULL);

    WNDCLASSA wc = {0};
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = backend.window.instance;
    wc.lpszClassName = WINDOW_CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;

    if (!RegisterClassA(&wc)) {
        debug_assert(0);
        return;
    }
}

void wnd_terminate_win32() {
    if (backend.window.handle) {
        DestroyWindow(backend.window.handle);
    }
    UnregisterClassA(WINDOW_CLASS_NAME, backend.window.instance);
}

void wnd_dispatch_events_win32() {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            break;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void wnd_set_title_win32(const char* title) {
    if (backend.window.handle) {
        SetWindowText(backend.window.handle, title);
    }
}

void wnd_get_size_win32(uint32_t* w, uint32_t* h) {
    *w = backend.width;
    *h = backend.height;
}

void wnd_attach_dispatcher_win32(wnd_dispatcher* disp) {
    backend.dispatcher.on_key = disp->on_key;
    backend.dispatcher.on_pointer_axis = disp->on_pointer_axis;
    backend.dispatcher.on_pointer_button = disp->on_pointer_button;
    backend.dispatcher.on_pointer_motion = disp->on_pointer_motion;
    backend.dispatcher.on_window_size = disp->on_window_size;
    backend.dispatcher.on_window_close = disp->on_window_close;
    backend.dispatcher.user_data = disp->user_data;
}

void wnd_open_win32(const char* title, uint32_t w, uint32_t h) {
    backend.width = w;
    backend.height = h;

    // Calculate window size including borders/titlebar
    RECT  rect = {0, 0, (LONG)w, (LONG)h};
    DWORD style = WS_OVERLAPPEDWINDOW;
    AdjustWindowRect(&rect, style, FALSE);

    int window_width = rect.right - rect.left;
    int window_height = rect.bottom - rect.top;

    backend.window.handle = CreateWindowA(
        WINDOW_CLASS_NAME, title, style, CW_USEDEFAULT, CW_USEDEFAULT, window_width,
        window_height, NULL, NULL, backend.window.instance, NULL);

    if (!backend.window.handle) {
        debug_assert(0);
        return;
    }

    ShowWindow(backend.window.handle, SW_SHOW);
    UpdateWindow(backend.window.handle);
}

void* wnd_native_handle_win32() { return &backend.window; }

uint32_t wnd_backend_id_win32() { return WINDOW_BACKEND_WIN32_ID; }

void load_wnd_win32(wnd_api* api) {
    api->init = wnd_init_win32;
    api->terminate = wnd_terminate_win32;
    api->open = wnd_open_win32;
    api->dispatch_events = wnd_dispatch_events_win32;
    api->backend_id = wnd_backend_id_win32;
    api->native_handle = wnd_native_handle_win32;
    api->set_title = wnd_set_title_win32;
    api->get_size = wnd_get_size_win32;
    api->attach_dispatcher = wnd_attach_dispatcher_win32;
    // api->get_window_size = get_window_size_wl;
}
static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wparam,
                                 LPARAM lparam) {
    uint32_t code = win32_key_to_keycode(wparam);
    void* usr = backend.dispatcher.user_data;
    switch (msg) {
        case WM_CLOSE: {
            if (backend.dispatcher.on_window_close != NULL) {
                backend.dispatcher.on_window_close(usr);
            }
            return 0;
        }
        case WM_KEYDOWN: {
            if (backend.dispatcher.on_key != NULL) {
                backend.dispatcher.on_key(code, 1, usr);
            }
            return 0;
        }
        case WM_KEYUP: {
            if (backend.dispatcher.on_key != NULL) {
                backend.dispatcher.on_key(code, 0, usr);
            }
            return 0;
        }
        case WM_MOUSEMOVE: {
            if (backend.dispatcher.on_pointer_motion != NULL) {
                float x = (int)LOWORD(lparam);
                float y = (int)HIWORD(lparam);
                backend.dispatcher.on_pointer_motion(x, y, usr);
            }
            return 0;
        }
        case WM_MBUTTONUP: {
            if (backend.dispatcher.on_pointer_button != NULL) {
                backend.dispatcher.on_pointer_button(code, 0, usr);
            }
            return 0;
        }
        case WM_MBUTTONDOWN: {
            if (backend.dispatcher.on_pointer_button != NULL) {
                backend.dispatcher.on_pointer_button(code, 1, usr);
            }
            return 0;
        }
        case WM_SIZE : {
            if (backend.dispatcher.on_window_size!= NULL) {
                int width = LOWORD(lparam);
                int height = HIWORD(lparam);
                backend.dispatcher.on_window_size(width, height, usr);
            }
            return 0;
                
            }
    }
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static uint32_t win32_key_to_keycode(uint32_t code) {
    switch (code) {
        case 65:
            return KEY_A;
        case 66:
            return KEY_B;
        case 67:
            return KEY_C;
        case 68:
            return KEY_D;
        case 69:
            return KEY_E;
        case 70:
            return KEY_F;
        case 71:
            return KEY_G;
        case 72:
            return KEY_H;
        case 73:
            return KEY_I;
        case 74:
            return KEY_J;
        case 75:
            return KEY_K;
        case 76:
            return KEY_L;
        case 77:
            return KEY_M;
        case 78:
            return KEY_N;
        case 79:
            return KEY_O;
        case 80:
            return KEY_P;
        case 81:
            return KEY_Q;
        case 82:
            return KEY_R;
        case 83:
            return KEY_S;
        case 84:
            return KEY_T;
        case 85:
            return KEY_U;
        case 86:
            return KEY_V;
        case 87:
            return KEY_W;
        case 88:
            return KEY_X;
        case 89:
            return KEY_Y;
        case 90:
            return KEY_Z;
        case 49:
            return KEY_1;
        case 50:
            return KEY_2;
        case 51:
            return KEY_3;
        case 52:
            return KEY_4;
        case 53:
            return KEY_5;
        case 54:
            return KEY_6;
        case 55:
            return KEY_7;
        case 56:
            return KEY_8;
        case 57:
            return KEY_9;
        case 48:
            return KEY_0;
        case 13:
            return KEY_ENTER;
        case 27:
            return KEY_ESCAPE;
        case 8:
            return KEY_BACKSPACE;
        case 9:
            return KEY_TAB;
        case 32:
            return KEY_SPACE;
        case 189:
            return KEY_MINUS;
        case 187:
            return KEY_EQUAL;
        case 219:
            return KEY_LBRACE;
        case 221:
            return KEY_RBRACE;
        case 220:
            return KEY_BACKSLASH;
        case 186:
            return KEY_SEMICOLON;
        case 222:
            return KEY_APOSTROPHE;
        case 192:
            return KEY_GRAVEACCENT;
        case 188:
            return KEY_COMMA;
        case 190:
            return KEY_PERIOD;
        case 191:
            return KEY_SLASH;
        case 112:
            return KEY_F1;
        case 113:
            return KEY_F2;
        case 114:
            return KEY_F3;
        case 115:
            return KEY_F4;
        case 116:
            return KEY_F5;
        case 117:
            return KEY_F6;
        case 118:
            return KEY_F7;
        case 119:
            return KEY_F8;
        case 120:
            return KEY_F9;
        case 121:
            return KEY_F10;
        case 122:
            return KEY_F11;
        case 123:
            return KEY_F12;
        case 39:
            return KEY_RIGHT;
        case 37:
            return KEY_LEFT;
        case 40:
            return KEY_DOWN;
        case 38:
            return KEY_UP;
        case 20:
            return KEY_CAPSLOCK;
        case 17:
            return KEY_LCONTROL;
        case 18:
            return KEY_LALT;
        case 42:
            return KEY_LSHIFT;
        case 91:
            return KEY_LMETA;
        case 16:
            return KEY_RSHIFT;
        default:
            return KEY_NONE;
    }
}

#endif  // WINDOW_BACKEND_WIN32
