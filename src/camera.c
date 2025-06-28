// camera.c
#include "camera.h"
#include <math.h>
#include "mathf.h"

// Default camera values
#define DEFAULT_YAW         -90.0f
#define DEFAULT_PITCH       0.0f
#define DEFAULT_SPEED       5.0f
#define DEFAULT_SENSITIVITY 0.1f
#define DEFAULT_FOV         45.0f

camera camera_create_fps(vec3 position, float fov, float aspect, float near_plane, float far_plane) {
    camera cam = {0};
    cam.position = position;
    cam.world_up = (vec3){0.0f, 1.0f, 0.0f};
    cam.up = cam.world_up;
    cam.fov = fov;
    cam.aspect = aspect;
    cam.near_plane = near_plane;
    cam.far_plane = far_plane;
    cam.yaw = DEFAULT_YAW;
    cam.pitch = DEFAULT_PITCH;
    cam.move_speed = DEFAULT_SPEED;
    cam.mouse_sensitivity = DEFAULT_SENSITIVITY;
    cam.type = CAMERA_TYPE_FPS;
    
    camera_update_vectors(&cam);
    return cam;
}

camera camera_create_orbit(vec3 target, float distance, float fov, float aspect, float near_plane, float far_plane) {
    camera cam = {0};
    cam.target = target;
    cam.position = v3_add(target, (vec3){0.0f, 0.0f, distance});
    cam.world_up = (vec3){0.0f, 1.0f, 0.0f };
    cam.up = cam.world_up;
    cam.fov = fov;
    cam.aspect = aspect;
    cam.near_plane = near_plane;
    cam.far_plane = far_plane;
    cam.move_speed = DEFAULT_SPEED;
    cam.mouse_sensitivity = DEFAULT_SENSITIVITY;
    cam.type = CAMERA_TYPE_ORBIT;
    
    camera_update_vectors(&cam);
    return cam;
}

camera camera_create_free(vec3 position, vec3 target, float fov, float aspect, float near_plane, float far_plane) {
    camera cam = {0};
    cam.position = position;
    cam.target = target;
    cam.world_up = (vec3){0.0f, 1.0f, 0.0f};
    cam.up = cam.world_up;
    cam.fov = fov;
    cam.aspect = aspect;
    cam.near_plane = near_plane;
    cam.far_plane = far_plane;
    cam.move_speed = DEFAULT_SPEED;
    cam.mouse_sensitivity = DEFAULT_SENSITIVITY;
    cam.type = CAMERA_TYPE_FREE;
    
    camera_update_vectors(&cam);
    return cam;
}

mat4 camera_view_matrix(const camera* cam) {
    switch(cam->type) {
        case CAMERA_TYPE_ORBIT:
            return mat4_look_at(cam->position, cam->target, cam->up);
        case CAMERA_TYPE_FPS:
        case CAMERA_TYPE_FREE:
            return mat4_look_at(cam->position, v3_add(cam->position, cam->front), cam->up);
        default:
            return mat4_identity();
    }
}

mat4 camera_projection_matrix(const camera* cam) {
    return mat4_perspective(cam->fov, cam->aspect, cam->near_plane, cam->far_plane);
}

mat4 camera_mvp_matrix(const camera* cam, const mat4* model) {
    mat4 view = camera_view_matrix(cam);
    mat4 proj = camera_projection_matrix(cam);
    mat4 vp = mat4_mul(proj, view);
    return mat4_mul(vp, *model);
}

void camera_update_vectors(camera* cam) {
    if(cam->type == CAMERA_TYPE_FPS) {
        // Calculate front vector from yaw and pitch
        vec3 front;
        front.x = cos(cam->yaw * M_PI / 180.0f) * cos(cam->pitch * M_PI / 180.0f);
        front.y = sin(cam->pitch * M_PI / 180.0f);
        front.z = sin(cam->yaw * M_PI / 180.0f) * cos(cam->pitch * M_PI / 180.0f);
        cam->front = v3_normalize(front);
        
        // Calculate right and up vectors
        cam->right = v3_normalize(v3_cross(cam->front, cam->world_up));
        cam->up = v3_normalize(v3_cross(cam->right, cam->front));
    } else if(cam->type == CAMERA_TYPE_ORBIT) {
        // For orbit camera, calculate vectors based on position and target
        cam->front = v3_normalize(v3_sub(cam->target, cam->position));
        cam->right = v3_normalize(v3_cross(cam->front, cam->world_up));
        cam->up = v3_normalize(v3_cross(cam->right, cam->front));
    } else { // FREE camera
        vec3 direction = v3_sub(cam->target, cam->position);
        cam->front = v3_normalize(direction);
        cam->right = v3_normalize(v3_cross(cam->front, cam->world_up));
        cam->up = v3_normalize(v3_cross(cam->right, cam->front));
    }
}

void camera_move_forward(camera* cam, float delta_time) {
    float velocity = cam->move_speed * delta_time;
    if(cam->type == CAMERA_TYPE_ORBIT) {
        // For orbit camera, move closer to target
        vec3 direction = v3_sub(cam->target, cam->position);
        float distance = v3_length(direction);
        if(distance > 1.0f) {
            direction = v3_normalize(direction);
            cam->position = v3_add(cam->position, v3_scale(direction, velocity));
        }
    } else {
        cam->position = v3_add(cam->position, v3_scale(cam->front, velocity));
    }
    if(cam->type != CAMERA_TYPE_FPS) camera_update_vectors(cam);
}

void camera_move_backward(camera* cam, float delta_time) {
    float velocity = cam->move_speed * delta_time;
    if(cam->type == CAMERA_TYPE_ORBIT) {
        // For orbit camera, move away from target
        vec3 direction = v3_sub(cam->position, cam->target);
        direction = v3_normalize(direction);
        cam->position = v3_add(cam->position, v3_scale(direction, velocity));
    } else {
        cam->position = v3_sub(cam->position, v3_scale(cam->front, velocity));
    }
    if(cam->type != CAMERA_TYPE_FPS) camera_update_vectors(cam);
}

void camera_move_left(camera* cam, float delta_time) {
    float velocity = cam->move_speed * delta_time;
    cam->position = v3_sub(cam->position, v3_scale(cam->right, velocity));
    if(cam->type != CAMERA_TYPE_FPS) camera_update_vectors(cam);
}

void camera_move_right(camera* cam, float delta_time) {
    float velocity = cam->move_speed * delta_time;
    cam->position = v3_add(cam->position, v3_scale(cam->right, velocity));
    if(cam->type != CAMERA_TYPE_FPS) camera_update_vectors(cam);
}

void camera_move_up(camera* cam, float delta_time) {
    float velocity = cam->move_speed * delta_time;
    cam->position = v3_add(cam->position, v3_scale(cam->up, velocity));
    if(cam->type != CAMERA_TYPE_FPS) camera_update_vectors(cam);
}

void camera_move_down(camera* cam, float delta_time) {
    float velocity = cam->move_speed * delta_time;
    cam->position = v3_sub(cam->position, v3_scale(cam->up, velocity));
    if(cam->type != CAMERA_TYPE_FPS) camera_update_vectors(cam);
}

void camera_process_mouse(camera* cam, float xoffset, float yoffset, int constrain_pitch) {
    xoffset *= cam->mouse_sensitivity;
    yoffset *= cam->mouse_sensitivity;

    if(cam->type == CAMERA_TYPE_FPS) {
        cam->yaw += xoffset;
        cam->pitch += yoffset;

        // Constrain pitch to prevent screen flipping
        if(constrain_pitch) {
            if(cam->pitch > 89.0f) cam->pitch = 89.0f;
            if(cam->pitch < -89.0f) cam->pitch = -89.0f;
        }

        camera_update_vectors(cam);
    } else if(cam->type == CAMERA_TYPE_ORBIT) {
        camera_orbit(cam, xoffset, yoffset);
    }
}

void camera_orbit(camera* cam, float horizontal, float vertical) {
    // Get current position relative to target
    vec3 offset = v3_sub(cam->position, cam->target);
    float radius = v3_length(offset);
    
    // Convert to spherical coordinates
    float theta = atan2(offset.z, offset.x);
    float phi = acos(offset.y / radius);
    
    // Apply rotation
    theta += horizontal * cam->mouse_sensitivity * 0.01f;
    phi += vertical * cam->mouse_sensitivity * 0.01f;
    
    // Constrain phi to prevent flipping
    const float epsilon = 0.01f;
    if(phi < epsilon) phi = epsilon;
    if(phi > M_PI - epsilon) phi = M_PI - epsilon;
    
    // Convert back to cartesian
    offset.x = radius * sin(phi) * cos(theta);
    offset.y = radius * cos(phi);
    offset.z = radius * sin(phi) * sin(theta);
    
    cam->position = v3_add(cam->target, offset);
    camera_update_vectors(cam);
}

void camera_zoom(camera* cam, float zoom_amount) {
    if(cam->type == CAMERA_TYPE_ORBIT) {
        vec3 direction = v3_sub(cam->position, cam->target);
        float distance = v3_length(direction);
        distance -= zoom_amount;
        if(distance < 1.0f) distance = 1.0f;
        if(distance > 100.0f) distance = 100.0f;
        
        direction = v3_normalize(direction);
        cam->position = v3_add(cam->target, v3_scale(direction, distance));
        camera_update_vectors(cam);
    } else {
        // For other camera types, adjust FOV
        cam->fov -= zoom_amount * 0.1f;
        if(cam->fov < 1.0f) cam->fov = 1.0f;
        if(cam->fov > 120.0f) cam->fov = 120.0f;
    }
}

void camera_set_aspect_ratio(camera* cam, float aspect) {
    cam->aspect = aspect;
}

void camera_look_at(camera* cam, vec3 target) {
    cam->target = target;
    if(cam->type == CAMERA_TYPE_FPS) {
        // For FPS camera, calculate yaw and pitch from direction
        vec3 direction = v3_normalize(v3_sub(target, cam->position));
        cam->pitch = asin(direction.y) * 180.0f / M_PI;
        cam->yaw = atan2(direction.z, direction.x) * 180.0f / M_PI;
    }
    camera_update_vectors(cam);
}

// Example usage:
/*
// Create an FPS camera
camera fps_cam = camera_create_fps(
    v3_create(0.0f, 0.0f, 3.0f),  // position
    45.0f * M_PI / 180.0f,          // fov in radians
    (float)window_width / window_height,  // aspect ratio
    0.1f,                           // near plane
    100.0f                          // far plane
);

// In your main loop:
mat4 model = mat4_identity();
mat4 mvp = camera_mvp_matrix(&fps_cam, &model);
rcmd_push_constants(cmd, pipeline, RSHADER_STAGE_VERTEX, 0, sizeof(mat4), &mvp);

// Handle input:
if(key_pressed('W')) camera_move_forward(&fps_cam, delta_time);
if(key_pressed('S')) camera_move_backward(&fps_cam, delta_time);
if(key_pressed('A')) camera_move_left(&fps_cam, delta_time);
if(key_pressed('D')) camera_move_right(&fps_cam, delta_time);

// Handle mouse:
camera_process_mouse(&fps_cam, mouse_xoffset, mouse_yoffset, 1);
*/
