#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "game0.h"
#include "private/base.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static vertex cube_vertices[] = {
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},  // 0
    {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},   // 1
    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},    // 2
    {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}},   // 3
    {{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 1.0f}},   // 4
    {{0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 1.0f}},    // 5
    {{0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}},     // 6
    {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 0.0f}},    // 7
};

static uint32_t vertices_count = ARR_SIZE(cube_vertices);

static uint16_t cube_indices[] = {
    // Front face
    0, 1, 2, 2, 3, 0,
    // Right face
    1, 5, 6, 6, 2, 1,
    // Back face
    5, 4, 7, 7, 6, 5,
    // Left face
    4, 0, 3, 3, 7, 4,
    // Top face
    3, 2, 6, 6, 7, 3,
    // Bottom face
    4, 5, 1, 1, 0, 4};
static uint32_t index_count = ARR_SIZE(cube_indices);

int main() {
    printf("Hello world!\n");
    win_config cfg = {
        .window_height = 1080,
        .window_width = 1920,
        .window_title = "game0",
    };
    if (startup(cfg) != 0) return -1;

    mesh m = {
        .index_count = index_count,
        .vertex_count = vertices_count,
        .indices = cube_indices,
        .vertices = cube_vertices,
    };

    float angle_x = 0;
    float angle_y = 0;
    vec3  translation = (vec3){.x = 0.0f, .y = 0.0f, .z = -3.0f};
    vec3  camera = (vec3){.x = 0.0f, .y = 0.0f, .z = 0.0f};
    while (is_running()) {
        poll_events();

        vec2 delta = get_mouse_delta();
        if (button_down(MOUSE_BUTTON_LEFT)) {
            camera.x += delta.x * 0.1f;
            camera.y += delta.y * 0.1f;
        }
        if (button_down(MOUSE_BUTTON_RIGHT)) {
            angle_x += delta.x * 0.1f;
            angle_y += delta.y * 0.1f;
        }
        camera.z += get_mouse_scroll() * 0.1f;
        vec2  win_size = get_window_size();
        float aspect_ratio = win_size.x / win_size.y;
        float radians_x = deg2rad(angle_x);
        float radians_y = deg2rad(angle_y);

        mat4 proj = m4_perspective(deg2rad(60), aspect_ratio, 0.1f, 100.0f);
        mat4 view = m4_translation(camera);
        mat4 model = m4_mul(m4_rotation_x(radians_x), m4_rotation_y(radians_y));

        mat4 mvp = m4_mul(proj, m4_mul(view, model));
        m.mvp = mvp;
        render_begin();
        draw_mesh(&m);
        render_end();
    }

    teardown();
    printf("Goodbye!\n");
    return 0;
}

#include "core_state.c"
#include "render/render_backend_vk.c"
#ifdef OS_LINUX
#include "os/window_backend_wl.c"
#include "render/window_surface_vk_wl.c"
#include "system_posix.c"
#else
#error "Unsupported backend!"
#endif  // OS_LINUX
