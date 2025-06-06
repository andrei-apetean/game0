#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "game0.h"
#include "private/base.h"
#include "private/input.h"
#include "private/render_backend.h"
#include "private/window_backend.h"

typedef struct {
    window_backend* window_backend;
    render_backend* render_backend;
    input_state     input;
    int32_t         is_running;
} core_state;

static core_state core = {0};

void process_input();

void on_event(input_ev e) {
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
            if (core.is_running && core.render_backend) {
                vec2 size = get_window_size();
                render_backend_resize(core.render_backend, size);
            }
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

int32_t startup(win_config cfg) {
    size_t window_backend_size = window_backend_get_size();
    size_t rndrbknd_size = render_backend_get_size();
    core.window_backend = malloc(window_backend_size);
    core.render_backend = malloc(rndrbknd_size);
    if (!core.window_backend) return -1;
    if (!core.render_backend) return -1;

    const window_cfg wcfg = {
        .width = cfg.window_width,
        .height = cfg.window_height,
        .title = cfg.window_title,
    };
    if (window_backend_startup(&core.window_backend, wcfg) != 0) return -1;
    window_backend_attach_handler(core.window_backend, on_event);

    const window_info    winfo = window_backend_get_info(core.window_backend);
    const render_cfg rcfg = {
        .height = cfg.window_height,
        .width = cfg.window_width,
        .win_api = winfo.win_api,
        .win_handle = winfo.win_handle,
        .win_inst = winfo.win_instance,
    };
    if (render_backend_startup(&core.render_backend, rcfg)) return -1;

    core.is_running = 1;
    return 0;
}

int32_t teardown() {
    if (core.render_backend) render_backend_teardown(core.render_backend);
    if (core.window_backend) window_backend_teardown(core.window_backend);
    return 0;
}

int32_t is_running() { return core.is_running; }

void stop_running() { core.is_running = 0; }

void poll_events() {
    process_input();
    window_backend_poll_events(core.window_backend);
}

vec2 get_window_size() { return window_backend_window_size(core.window_backend);}

void render_begin() { render_backend_render_begin(core.render_backend); }
void draw_mesh(mesh* m) { render_backend_draw_mesh(core.render_backend, m); }
void render_end() { render_backend_render_end(core.render_backend); }

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
vec2 get_mouse_delta() {
    return (vec2) {
        core.input.pointer_x - core.input.last_pointer_x,
            core.input.pointer_y - core.input.last_pointer_y,
    };
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


vec2 v2_add(vec2 a, vec2 b) { return (vec2){a.x + b.x, a.y + b.y}; }
vec2 v2_sub(vec2 a, vec2 b) { return (vec2){a.x - b.x, a.y - b.y}; }
vec2 v2_mul(vec2 a, vec2 b) { return (vec2){a.x * b.x, a.y * b.y}; }
vec2 v2_scale(vec2 v, float s) { return (vec2){v.x * s, v.y * s}; }
vec2 v2_div(vec2 v, float s) { return (vec2){v.x / s, v.y / s}; }
float v2_dot(vec2 a, vec2 b) { return a.x * b.x + a.y * b.y; }
float v2_length(vec2 v) { return sqrtf(v2_dot(v, v)); }
vec2 v2_normalize(vec2 v) {
    float len = v2_length(v);
    return (len != 0.0f) ? v2_div(v, len) : (vec2){0.0f, 0.0f};
}
vec2 v2_lerp(vec2 a, vec2 b, float t) { return (vec2){a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t}; }

vec3 v3_zero() { return (vec3){0.0f, 0.0f, 0.0f}; }
vec3 v3_one() { return (vec3){1.0f, 1.0f, 1.0f}; }
vec3 v3_left() { return (vec3){-1.0f, 0.0f, 0.0f}; }
vec3 v3_right() { return (vec3){1.0f, 0.0f, 0.0f}; }
vec3  v3_up() { return (vec3){0.0f, 1.0f, 0.0f}; }
vec3  v3_add(vec3 a, vec3 b) { return (vec3){a.x + b.x, a.y + b.y, a.z + b.z}; }
vec3 v3_sub(vec3 a, vec3 b) { return (vec3){a.x - b.x, a.y - b.y, a.z - b.z}; }
vec3 v3_mul(vec3 a, vec3 b) { return (vec3){a.x * b.x, a.y * b.y, a.z * b.z}; }
vec3 v3_scale(vec3 v, float s) { return (vec3){v.x * s, v.y * s, v.z * s}; }
vec3 v3_div(vec3 v, float s) { return (vec3){v.x / s, v.y / s, v.z / s}; }
float v3_dot(vec3 a, vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
vec3 v3_cross(vec3 a, vec3 b) {
    return (vec3){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}
float v3_length(vec3 v) { return sqrtf(v3_dot(v, v)); }
vec3 v3_normalize(vec3 v) {
    float len = v3_length(v);
    return (len != 0.0f) ? v3_div(v, len) : (vec3){0.0f, 0.0f, 0.0f};
}
vec3 v3_lerp(vec3 a, vec3 b, float t) {
    return (vec3){a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t};
}

// Vec4 implementations
vec4 v4_add(vec4 a, vec4 b) { return (vec4){a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w}; }
vec4 v4_sub(vec4 a, vec4 b) { return (vec4){a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w}; }
vec4 v4_mul(vec4 a, vec4 b) { return (vec4){a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w}; }
vec4 v4_scale(vec4 v, float s) { return (vec4){v.x * s, v.y * s, v.z * s, v.w * s}; }
vec4 v4_div(vec4 v, float s) { return (vec4){v.x / s, v.y / s, v.z / s, v.w / s}; }
float v4_dot(vec4 a, vec4 b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
float v4_length(vec4 v) { return sqrtf(v4_dot(v, v)); }
vec4 v4_normalize(vec4 v) {
    float len = v4_length(v);
    return (len != 0.0f) ? v4_div(v, len) : (vec4){0.0f, 0.0f, 0.0f, 0.0f};
}
vec4 v4_lerp(vec4 a, vec4 b, float t) {
    return (vec4){
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t,
        a.w + (b.w - a.w) * t
    };
}


mat4 m4_identity(void) {
    mat4 result = {0};
    result.m[0] = result.m[5] = result.m[10] = result.m[15] = 1.0f;
    return result;
}

mat4 m4_translation(vec3 translation) {
    mat4 result = m4_identity();
    result.m[12] = translation.x;
    result.m[13] = translation.y;
    result.m[14] = translation.z;
    return result;
}

mat4 m4_rotation_x(float radians) {
    float c = cosf(radians), s = sinf(radians);
    mat4 result = m4_identity();
    result.m[5] = c;
    result.m[6] = s;
    result.m[9] = -s;
    result.m[10] = c;
    return result;
}

mat4 m4_rotation_y(float radians) {
    float c = cosf(radians), s = sinf(radians);
    mat4 result = m4_identity();
    result.m[0] = c;
    result.m[2] = -s;
    result.m[8] = s;
    result.m[10] = c;
    return result;
}

mat4 m4_rotation_z(float radians) {
    float c = cosf(radians), s = sinf(radians);
    mat4 result = m4_identity();
    result.m[0] = c;
    result.m[1] = s;
    result.m[4] = -s;
    result.m[5] = c;
    return result;
}

mat4 m4_perspective(float fov_radians, float aspect, float near_z, float far_z) {
    float f = 1.0f / tanf(fov_radians / 2.0f);
    mat4 result = {0};
    result.m[0] = f / aspect;
    result.m[5] = f;
    result.m[10] = (far_z + near_z) / (near_z - far_z);
    result.m[11] = -1.0f;
    result.m[14] = (2.0f * far_z * near_z) / (near_z - far_z);
    return result;
}

mat4 m4_mul(mat4 a, mat4 b) {
    mat4 result = {0};
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            for (int i = 0; i < 4; ++i) {
                result.m[col * 4 + row] += a.m[i * 4 + row] * b.m[col * 4 + i];
            }
        }
    }
    return result;
}
