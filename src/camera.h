#pragma once
#include "math_types.h" // Your math library

typedef struct {
    vec3 position;
    vec3 target;
    vec3 up;
    
    // Projection parameters
    float fov;         // Field of view in radians
    float aspect;      // width / height
    float near_plane;
    float far_plane;
    
    // View parameters (calculated)
    vec3 front;        // Forward direction
    vec3 right;        // Right direction
    vec3 world_up;     // World up vector (usually 0,1,0)
    
    // Euler angles for FPS-style camera
    float yaw;         // Rotation around Y axis (left/right)
    float pitch;       // Rotation around X axis (up/down)
    
    // Movement speeds
    float move_speed;
    float mouse_sensitivity;
    
    // Camera type
    enum {
        CAMERA_TYPE_ORBIT,    // Orbits around a target point
        CAMERA_TYPE_FPS,      // First-person shooter style
        CAMERA_TYPE_FREE      // Free-flying camera
    } type;
} camera;

// Camera creation
camera camera_create_fps(vec3 position, float fov, float aspect, float near_plane, float far_plane);
camera camera_create_orbit(vec3 target, float distance, float fov, float aspect, float near_plane, float far_plane);
camera camera_create_free(vec3 position, vec3 target, float fov, float aspect, float near_plane, float far_plane);

// Matrix generation
mat4 camera_view_matrix(const camera* cam);
mat4 camera_projection_matrix(const camera* cam);
mat4 camera_mvp_matrix(const camera* cam, const mat4* model);

// Movement functions
void camera_move_forward(camera* cam, float delta_time);
void camera_move_backward(camera* cam, float delta_time);
void camera_move_left(camera* cam, float delta_time);
void camera_move_right(camera* cam, float delta_time);
void camera_move_up(camera* cam, float delta_time);
void camera_move_down(camera* cam, float delta_time);

// Mouse input
void camera_process_mouse(camera* cam, float xoffset, float yoffset, int constrain_pitch);

// Orbit camera specific
void camera_orbit(camera* cam, float horizontal, float vertical);
void camera_zoom(camera* cam, float zoom_amount);

// Update camera vectors (call after changing position/rotation)
void camera_update_vectors(camera* cam);

// Utility
void camera_set_aspect_ratio(camera* cam, float aspect);
void camera_look_at(camera* cam, vec3 target);

