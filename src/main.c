#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "mathf.h"
#include "render_module/render.h"
#include "render_module/render_types.h"
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

#define NUM_CUBES 64
#define MIN_CUBE_POS -30.0f
#define MAX_CUBE_POS 30.0f
vec3  cube_positions[NUM_CUBES];
vec3 cube_rotations[NUM_CUBES];

float rand_float_range(float min, float max) {
    return min + ((float)rand() / (float)RAND_MAX) * (max - min);
}

void init_cube_positions() {
    for (int i = 0; i < NUM_CUBES; i++) {
        cube_positions[i].x = rand_float_range(MIN_CUBE_POS, MAX_CUBE_POS);
        cube_positions[i].y = rand_float_range(MIN_CUBE_POS, MAX_CUBE_POS);
        cube_positions[i].z = rand_float_range(MIN_CUBE_POS, MAX_CUBE_POS);
        cube_rotations[i].x = rand_float_range(0, 360);
        cube_rotations[i].y = rand_float_range(0, 360);
        cube_rotations[i].z = rand_float_range(0, 360);
    }
}

mat4 create_model_matrix(vec3 position, vec3 rotation_degrees) {
    mat4 model = m4_translation(position);

    mat4 rot_x = m4_rotation_x(deg2rad(rotation_degrees.x));
    mat4 rot_y = m4_rotation_y(deg2rad(rotation_degrees.y));
    mat4 rot_z = m4_rotation_z(deg2rad(rotation_degrees.z));

   mat4 rotation = m4_mul(rot_y, m4_mul(rot_x, rot_z));
    return m4_mul(model, rotation);
}

int32_t make_pipeline_config(render_pipeline_config* out_config) {
    FILE* vertex_file = fopen("./bin/assets/shaders/shader.vert.spv", "rb");
    assert(vertex_file);

    fseek(vertex_file, 0, SEEK_END);
    size_t vertex_code_size = ftell(vertex_file);
    rewind(vertex_file);

    uint32_t* vertex_code = malloc(vertex_code_size);
    assert(vertex_code);
    fread(vertex_code, 1, vertex_code_size, vertex_file);
    fclose(vertex_file);

    FILE* fragment_file = fopen("./bin/assets/shaders/shader.frag.spv", "rb");
    assert(fragment_file);

    fseek(fragment_file, 0, SEEK_END);
    size_t fragment_code_size = ftell(fragment_file);
    rewind(fragment_file);

    uint32_t* fragment_code = malloc(fragment_code_size);
    assert(fragment_code);
    fread(fragment_code, 1, fragment_code_size, fragment_file);
    
    fclose(fragment_file);
    out_config->fragment_shader_code_size = fragment_code_size;
    out_config->fragment_shader_code = fragment_code;
    out_config->vertex_shader_code_size = vertex_code_size;
    out_config->vertex_shader_code = vertex_code;
    return 0;
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

    if (window_create(&wparams) != 0) return -1;

    window_set_close_handler(on_window_close, NULL);
    window_set_size_handler(on_window_resize, NULL);

    uint32_t window_api = window_get_backend_id();
    void*    window = window_get_native_handle();
    int32_t  err = render_device_connect(window_api, window);
    if (err) return -1;

    err = render_device_create_swapchain(wparams.width, wparams.height);

    if (err) return -1;
    render_pipeline_config pipeline_config;
    err = make_pipeline_config(&pipeline_config);
    if (err) return -1;

    err = render_device_create_render_pipeline(&pipeline_config);
    if (err == 0) {
        free(pipeline_config.fragment_shader_code);
        free(pipeline_config.vertex_shader_code);
    } else {
        return -1;
    }
    static_mesh cube = {
        .vertices = cube_vertices,
        .vertex_count = sizeof(cube_vertices) / sizeof(cube_vertices[0]),
        .indices = cube_indices,
        .index_count = sizeof(cube_indices) / sizeof(cube_indices[0]),
    };

    buffer_config buffer_config = {
        .usage_flags = index_buffer_bit | transfer_dst_bit,
        .data = cube_indices,
        .size = sizeof(uint32_t) * cube.index_count,
    }; 

    buffer_id cube_index_buffer = render_device_create_buffer(&buffer_config);
    if (cube_index_buffer == RENDER_INVALID_ID) return -1;

    buffer_config.data = cube.vertices;
    buffer_config.size = sizeof(vertex) * cube.vertex_count;
    buffer_config.usage_flags = 0;
    buffer_config.usage_flags = vertex_buffer_bit | transfer_dst_bit;

    buffer_id cube_vertex_buffer = render_device_create_buffer(&buffer_config);
    if (cube_vertex_buffer == RENDER_INVALID_ID) return -1;
    cube.vertex_buffer = cube_vertex_buffer;
    cube.index_buffer = cube_index_buffer;
    is_running = 1;

    vec3 cam_pos = {0.0f, 0.0f, -20.0f};  // moved farther back
    vec3 target = {0.0f, 0.0f, 0.0f};     // looking at origin
    vec3 up = {0.0f, 1.0f, 0.0f};         // up direction

    mat4 view = m4_look_at(cam_pos, target, up);

    camera =
        m4_perspective(deg2rad(camera_fov),
                       (float)wparams.width / (float)wparams.height, 0.1f, 1000.0f);
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

        render_device_begin_frame();
        for (int i = 0; i < NUM_CUBES; i++) {
            cube_rotations[i].x += delta_time * 10;
            cube_rotations[i].y += delta_time * 10;
            cube_rotations[i].z += delta_time * 10;
            mat4 model = create_model_matrix(cube_positions[i], cube_rotations[i]);
            mat4 mvp = m4_mul(camera, m4_mul(view, model));
            render_device_draw_mesh(&cube, mvp);
        }
        render_device_end_frame();

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
    render_device_destroy_buffer(cube_index_buffer);
    render_device_destroy_buffer(cube_vertex_buffer);
    render_device_destroy_render_pipeline();
    render_device_destroy_swapchain();
    render_device_disconnect();
    window_destroy();
    window_terminate();
    printf("Goodbye!\n");
    return 0;
}

// todo; temp
#include "mathf.c"
#include "memory.c"
#include "render_module/render.c"
#include "render_module/vkcore.c"
#include "render_module/vkcore_wl.c"
#include "render_module/vulkan_device.c"
#include "render_module/vulkan_swapchain.c"
#include "render_module/vulkan_pipeline.c"
#include "time_posix.c"
#include "window_module/window.c"
#include "window_module/window_backend_wl.c"
