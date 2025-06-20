#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "mathf.h"
#include "render_module/render.h"
#include "time_util.h"
#include "window_module/window.h"

static uint32_t is_running;
static uint32_t needs_resize;
static int32_t  width;
static int32_t  height;
static int32_t  camera_fov = 80;

static vertex cube_vertices[] = {
    {-1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f},  // 0
    {1.0f, -1.0f, -1.0f, 0.0f, 1.0f, 0.0f},   // 1
    {1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f},    // 2
    {-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 0.0f},   // 3
    {-1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 1.0f},   // 4
    {1.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f},    // 5
    {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},     // 6
    {-1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f},    // 7
};

uint32_t cube_indices[] = {
    0, 1, 2, 2, 3, 0,  // Back face
    4, 5, 6, 6, 7, 4,  // Front face
    4, 0, 3, 3, 7, 4,  // Left face
    1, 5, 6, 6, 2, 1,  // Right face
    3, 2, 6, 6, 7, 3,  // Top face
    4, 5, 1, 1, 0, 4   // Bottom face
};

static mat4 camera;

#define NUM_CUBES 32
#define MIN_CUBE_POS -25.0f
#define MAX_CUBE_POS 25.0f
vec3  cube_positions[NUM_CUBES];
float cube_angles[NUM_CUBES];

float rand_float_range(float min, float max) {
    return min + ((float)rand() / (float)RAND_MAX) * (max - min);
}

void init_cube_positions() {
    for (int i = 0; i < NUM_CUBES; i++) {
        cube_positions[i].x = rand_float_range(MIN_CUBE_POS, MAX_CUBE_POS);
        cube_positions[i].y = rand_float_range(MIN_CUBE_POS, MAX_CUBE_POS);
        cube_positions[i].z = rand_float_range(MIN_CUBE_POS, MAX_CUBE_POS);
        cube_angles[i] = rand_float_range(0, 360);
    }
}

mat4 create_model_matrix(vec3 position, float rotation_degrees) {
    mat4 model = m4_translation(position);
    return m4_mul(model, m4_rotation_y(deg2rad(rotation_degrees)));
}

double get_time_seconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

void on_window_close(void* user) { is_running = 0; }

void on_window_resize(int32_t w, int32_t h, void* user) {
    needs_resize = 1;
    width = w;
    height = h;
    float aspect = (float)w / (float)h;

    camera = m4_perspective(deg2rad(camera_fov), aspect, 0.1f, 1000.0f);
}

static uint32_t fps_frame_count;

int main() {
    srand((unsigned int)time(NULL));
    window_initialize();

    window_params wparams = {
        .width = 1920,
        .height = 1080,
        .title = "game_0",
    };

    if (window_create(&wparams) != 0) {
        return -1;
    }

    window_set_close_handler(on_window_close, NULL);
    window_set_size_handler(on_window_resize, NULL);

    render_params rparams = {
        .swapchain_width = wparams.width,
        .swapchain_height = wparams.height,
        .window_api = window_get_backend_id(),
        .window_handle = window_get_native_handle(),
    };
    render_initialize(&rparams);
    static_mesh cube = {
        .vertices = cube_vertices,
        .vertex_count = sizeof(cube_vertices) / sizeof(cube_vertices[0]),
        .indices = cube_indices,
        .index_count = sizeof(cube_indices) / sizeof(cube_indices[0]),

    };

    printf("Cube vertex count: %d\n", cube.vertex_count);
    printf("Cube index count: %d\n", cube.index_count);

    render_create_mesh_buffers(&cube);
    is_running = 1;

    vec3 cam_pos = {0.0f, 0.0f, -20.0f};  // moved farther back
    vec3 target = {0.0f, 0.0f, 0.0f};     // looking at origin
    vec3 up = {0.0f, 1.0f, 0.0f};         // up direction

    mat4 view = m4_look_at(cam_pos, target, up);

    camera = m4_perspective(deg2rad(camera_fov),
                            (float)wparams.width / (float)wparams.height, 0.1f,
                            1000.0f);
    time_init();
    init_cube_positions();
    printf("Cubes initialized!\n");
    time_p last_time;
    time_p last_frame;
    double delta_time;
    double elapsed;
    while (is_running) {
        if (needs_resize) {
            needs_resize = 0;
        }
        window_poll_events();
        render_begin();
        double time = get_time_seconds();
        for (int i = 0; i < NUM_CUBES; i++) {
            cube_angles[i] += delta_time * 10;
            mat4  model = create_model_matrix(cube_positions[i], cube_angles[i]);
            mat4  mvp = m4_mul(camera, m4_mul(view, model));
            render_draw(&cube, mvp);
        }
        render_end();

        time_p now = time_now();
        fps_frame_count++;

        elapsed = time_diff_sec(last_time, now);
        delta_time = time_diff_sec(last_frame, now);
        last_frame = now;
        if (elapsed >= 1.0) {
            char title[128] = {0};
            sprintf(title, "game_0 - %d fps", fps_frame_count);
            window_set_title(title);
            fps_frame_count = 0;
            last_time = now;
        }
    }

    render_terminate();
    window_destroy();
    window_terminate();
    printf("Goodbye!\n");
    return 0;
}

// todo; temp
#include "mathf.c"
#include "memory.c"
#include "render_module/render.c"
#include "time_posix.c"
#include "window_module/window.c"
#include "window_module/window_backend_wl.c"
