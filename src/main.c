#include <assert.h>
#include <stdalign.h>
#include <stdio.h>
#include <stdlib.h>

#include "base.h"
#include "camera.h"
#include "core/os_event.h"
#include "core/wnd.h"
#include "math_types.h"
#include "mathf.h"
#include "render/rdev.h"
#include "render/rtypes.h"
#include "time_util.h"

typedef struct {
    float x, y, z;
    float r, g, b;
} vertex;

static int w_pressed;
static int a_pressed;
static int s_pressed;
static int d_pressed;
static int q_pressed;      // Move up
static int e_pressed;      // Move down
static int shift_pressed;  // Speed boost
static int ctrl_pressed;   // Slow down

// Mouse state
static int   mouse_captured;
static int   first_mouse;
static float last_mouse_x;
static float last_mouse_y;

// Settings
static float    normal_speed = 5.0f;
static float    fast_speed = 15.0f;
static float    slow_speed = 1.0f;
static uint32_t is_running = 0;
static uint32_t needs_resize = 0;
static uint32_t fps_frame_count;
static uint32_t window_width = 1920;
static uint32_t window_height = 720;
static camera   cam;

static vertex vertices[] = {
    {-1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f},  // 0
    {1.0f, -1.0f, -1.0f, 0.0f, 1.0f, 0.0f},   // 1
    {1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f},    // 2
    {-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 0.0f},   // 3
    {-1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 1.0f},   // 4
    {1.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f},    // 5
    {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},     // 6
    {-1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f},    // 7
};

static uint32_t indices[] = {
    0, 1, 2, 2, 3, 0,  // Back face
    4, 5, 6, 6, 7, 4,  // Front face
    4, 0, 3, 3, 7, 4,  // Left face
    1, 5, 6, 6, 2, 1,  // Right face
    3, 2, 6, 6, 7, 3,  // Top face
    4, 5, 1, 1, 0, 4   // Bottom face
};

static uint32_t index_count = sizeof(indices) / sizeof(indices[0]);

void on_window_close(void* user) {
    unused(user);
    is_running = 0;
}

void on_window_resize(int32_t w, int32_t h, void* user) {
    unused(user);

    camera_set_aspect_ratio(&cam, (float)window_width / (float)window_height);
    needs_resize = 1;
    window_width = w;
    window_height = h;
}

void on_key(int32_t key, int32_t state, void* user) {
    unused(user);
    switch (key) {
        case KEY_W:
            w_pressed = state;
            break;
        case KEY_A:
            a_pressed = state;
            break;
        case KEY_S:
            s_pressed = state;
            break;
        case KEY_D:
            d_pressed = state;
            break;
        case KEY_Q:
            q_pressed = state;
            break;
        case KEY_E:
            e_pressed = state;
            break;
        case KEY_LSHIFT:
            shift_pressed = state;
            break;
        case KEY_LCONTROL:
            ctrl_pressed = state;
            break;
    }
}

void on_pointer_button(int32_t button, int32_t state, void* user) {
    unused(user);
    if (button == MOUSE_BUTTON_LEFT && state) {
        // Capture mouse on left click
        mouse_captured = !mouse_captured;
        first_mouse = 1;

        if (mouse_captured) {
            printf("Mouse captured - camera control enabled\n");
            // You might want to hide the cursor here
            // wnd_set_cursor_mode(CURSOR_DISABLED);
        } else {
            printf("Mouse released - camera control disabled\n");
            // Show cursor again
            // wnd_set_cursor_mode(CURSOR_NORMAL);
        }
    }
}

void on_pointer_motion(float x, float y, void* user) {
    unused(user);
    if (!mouse_captured) return;

    if (first_mouse) {
        last_mouse_x = x;
        last_mouse_y = y;
        first_mouse = 0;
    }

    float xoffset = x - last_mouse_x;
    float yoffset =
        last_mouse_y - y;  // Reversed since y-coordinates go from bottom to top

    last_mouse_x = x;
    last_mouse_y = y;

    // Process mouse movement
    camera_process_mouse(&cam, xoffset, yoffset, 1);
}
void on_pointer_axis(float value, void* user) {
    unused(user);
    // Use scroll wheel for zoom (orbit camera) or FOV adjustment (FPS camera)
    camera_zoom(&cam, value * 0.1f);
}
rshader_stage read_shader(const char* path, rshader_type type);

int main(void) {
    debug_log("Initializing...\n");

    wnd_init();
    time_init();
    uint32_t    window_api = wnd_backend_id();
    rdev_params rparams = {
        .wnd_api = window_api,
        .allocator = 0,
        .scratch_allocator = 0,
    };
    rdev_init(&rparams);

    wnd_open("game0", window_width, window_height);
    wnd_dispatcher wnd_disp = {
        .on_window_close = on_window_close,
        .on_window_size = on_window_resize,
        .on_key = on_key,
        .on_pointer_axis = on_pointer_axis,
        .on_pointer_motion = on_pointer_motion,
        .on_pointer_button = on_pointer_button,
    };
    wnd_attach_dispatcher(&wnd_disp);
    void* wnd_native = wnd_native_handle();
    wnd_get_size(&window_width, &window_height);
    rdev_create_swapchain(wnd_native, window_width, window_height);
    time_p last_time;

    double elapsed;
    is_running = 1;
    needs_resize = 0;
    rpass_id swapchain_pass = rdev_swapchain_renderpass();

    rshader_stage stages[2] = {
        read_shader("./bin/assets/shaders/shader.vert.spv", RSHADER_TYPE_VERTEX),
        read_shader("./bin/assets/shaders/shader.frag.spv", RSHADER_TYPE_FRAGMENT),
    };

    rvertex_attrib attribs[2] = {
        {
            .location = 0,
            .offset = 0,
            .format = RVERTEX_FORMAT_FLOAT3,
        },
        {
            .location = 1,
            .offset = offsetof(vertex, r),
            .format = RVERTEX_FORMAT_FLOAT3,
        },

    };
    rvertex_binding bindings[] = {
        {
            .binding = 0,
            .stride = sizeof(vertex),
        },
    };
    rpush_constant push_constant = {
        .stage_flags = RSHADER_STAGE_VERTEX, .size = sizeof(mat4), .offset = 0

    };

    rpipe_params pipe_params = {0};
    pipe_params.push_constant_count = 0;
    pipe_params.shader_stages = stages;
    pipe_params.shader_stage_count = sizeof(stages) / sizeof(stages[0]);
    pipe_params.vertex_attributes = attribs;
    pipe_params.vertex_attribute_count = sizeof(attribs) / sizeof(attribs[0]);
    pipe_params.vertex_bindings = bindings;
    pipe_params.vertex_binding_count = 1;
    pipe_params.push_constant_count = 1;
    pipe_params.push_constants = &push_constant;

    rpipe_id pipeline = rdev_create_pipeline(&pipe_params);
    for (uint32_t i = 0; i < pipe_params.shader_stage_count; i++) {
        free(pipe_params.shader_stages[i].code);
    }

    if (pipeline == RDEV_INVALID_ID) {
        debug_log("failed to create graphics pipeline\n");
        return -1;
    }
    rbuffer_id vertex_buffer =
        rdev_create_vertex_buffer(sizeof(vertices), &vertices);
    rbuffer_id index_buffer = rdev_create_index_buffer(sizeof(indices), &indices);
    if (vertex_buffer == RDEV_INVALID_ID || index_buffer == RDEV_INVALID_ID) {
        debug_log("failed to allocated buffers\n");
        return -1;
    }

    cam = camera_create_fps((vec3){0, 0, 5}, 45.0f * PI / 180.0f,
                            (float)window_width / window_height, 0.1f, 100.0f);

    time_p last_frame;
    double delta_time = 0;

    while (is_running) {
        wnd_dispatch_events();

        time_p now = time_now();
        fps_frame_count++;

        elapsed = time_diff_sec(last_time, now);
        delta_time = time_diff_sec(last_frame, now);
        last_frame = now;
        if (elapsed >= 1.0) {
            char title[128] = {0};
            sprintf(title, "game_0 - %d fps", fps_frame_count);
            wnd_set_title(title);
            fps_frame_count = 0;
            last_time = now;
        }

        // Determine movement speed
        float speed = normal_speed;
        if (shift_pressed) speed = fast_speed;
        if (ctrl_pressed) speed = slow_speed;

        // Update camera movement speed
        cam.move_speed = speed;

        // Process movement
        if (w_pressed) camera_move_forward(&cam, delta_time);
        if (s_pressed) camera_move_backward(&cam, delta_time);
        if (a_pressed) camera_move_left(&cam, delta_time);
        if (d_pressed) camera_move_right(&cam, delta_time);
        if (q_pressed) camera_move_up(&cam, delta_time);
        if (e_pressed) camera_move_down(&cam, delta_time);
        if (needs_resize) {
            rdev_resize_swapchain(window_width, window_height);
            needs_resize = 0;
        }

        mat4 model = mat4_identity();
        mat4 mvp = camera_mvp_matrix(&cam, &model);

        rcmd* cmd = rdev_begin();
        rcmd_begin_pass(cmd, swapchain_pass);
        rcmd_bind_pipe(cmd, pipeline);
        rcmd_push_constants(cmd, pipeline, RSHADER_STAGE_VERTEX, 0, sizeof(mat4),
                            &mvp);
        rcmd_bind_vertex_buffer(cmd, vertex_buffer);
        rcmd_bind_index_buffer(cmd, index_buffer);
        rcmd_draw_indexed(cmd, index_count, 1, 0, 0, 0);
        rcmd_end_pass(cmd, swapchain_pass);
        rdev_end(cmd);
    }
    // todo: need to wait device idle
    rdev_destroy_pipeline(pipeline);
    rdev_destroy_buffer(index_buffer);
    rdev_destroy_buffer(vertex_buffer);
    rdev_destroy_swapchain();
    rdev_terminate();
    wnd_terminate();
    debug_log("Terminated successfully!\n");
    return 0;
}

rshader_stage read_shader(const char* path, rshader_type type) {
    rshader_stage result = {0};

    FILE* f = fopen(path, "rb");
    if (!f) {
        debug_log("Failed to open shader file: %s\n", path);
        return result;  // result.code will be NULL
    }

    // Get file size
    fseek(f, 0, SEEK_END);
    size_t file_size = ftell(f);
    if (file_size <= 0) {
        debug_log("Invalid shader file size: %s\n", path);
        fclose(f);
        return result;
    }
    rewind(f);

    // Allocate and read
    uint32_t* code = malloc(file_size);
    if (!code) {
        debug_log("Failed to allocate memory for shader: %s\n", path);
        fclose(f);
        return result;
    }

    size_t bytes_read = fread(code, 1, file_size, f);
    fclose(f);  // Close file immediately after reading

    if (bytes_read != file_size) {
        debug_log("Failed to read complete shader file: %s\n", path);
        free(code);
        return result;
    }

    // SPIR-V files should be 4-byte aligned
    if (file_size % 4 != 0) {
        debug_log("Warning: SPIR-V file size not 4-byte aligned: %s\n", path);
    }

    result.code = code;
    result.code_size = file_size;
    result.type = type;
    return result;
}

