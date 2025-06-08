#include "mathf.h"
#include "math.h"

float deg2rad(float degrees) { return degrees * (PI / 180.0f); }
float rad2deg(float radians) { return radians * (180.0f / PI); }

vec2  v2_add(vec2 a, vec2 b) { return (vec2){a.x + b.x, a.y + b.y}; }
vec2  v2_sub(vec2 a, vec2 b) { return (vec2){a.x - b.x, a.y - b.y}; }
vec2  v2_mul(vec2 a, vec2 b) { return (vec2){a.x * b.x, a.y * b.y}; }
vec2  v2_scale(vec2 v, float s) { return (vec2){v.x * s, v.y * s}; }
vec2  v2_div(vec2 v, float s) { return (vec2){v.x / s, v.y / s}; }
float v2_dot(vec2 a, vec2 b) { return a.x * b.x + a.y * b.y; }
float v2_length(vec2 v) { return sqrtf(v2_dot(v, v)); }
vec2  v2_normalize(vec2 v) {
    float len = v2_length(v);
    return (len != 0.0f) ? v2_div(v, len) : (vec2){0.0f, 0.0f};
}
vec2 v2_lerp(vec2 a, vec2 b, float t) {
    return (vec2){a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}

vec3  v3_zero() { return (vec3){0.0f, 0.0f, 0.0f}; }
vec3  v3_one() { return (vec3){1.0f, 1.0f, 1.0f}; }
vec3  v3_left() { return (vec3){-1.0f, 0.0f, 0.0f}; }
vec3  v3_right() { return (vec3){1.0f, 0.0f, 0.0f}; }
vec3  v3_up() { return (vec3){0.0f, 1.0f, 0.0f}; }
vec3  v3_add(vec3 a, vec3 b) { return (vec3){a.x + b.x, a.y + b.y, a.z + b.z}; }
vec3  v3_sub(vec3 a, vec3 b) { return (vec3){a.x - b.x, a.y - b.y, a.z - b.z}; }
vec3  v3_mul(vec3 a, vec3 b) { return (vec3){a.x * b.x, a.y * b.y, a.z * b.z}; }
vec3  v3_scale(vec3 v, float s) { return (vec3){v.x * s, v.y * s, v.z * s}; }
vec3  v3_div(vec3 v, float s) { return (vec3){v.x / s, v.y / s, v.z / s}; }
float v3_dot(vec3 a, vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
vec3  v3_cross(vec3 a, vec3 b) {
    return (vec3){a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
                   a.x * b.y - a.y * b.x};
}
float v3_length(vec3 v) { return sqrtf(v3_dot(v, v)); }
vec3  v3_normalize(vec3 v) {
    float len = v3_length(v);
    return (len != 0.0f) ? v3_div(v, len) : (vec3){0.0f, 0.0f, 0.0f};
}
vec3 v3_lerp(vec3 a, vec3 b, float t) {
    return (vec3){a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t,
                  a.z + (b.z - a.z) * t};
}

// Vec4 implementations
vec4 v4_add(vec4 a, vec4 b) {
    return (vec4){a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
}
vec4 v4_sub(vec4 a, vec4 b) {
    return (vec4){a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
}
vec4 v4_mul(vec4 a, vec4 b) {
    return (vec4){a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w};
}
vec4 v4_scale(vec4 v, float s) {
    return (vec4){v.x * s, v.y * s, v.z * s, v.w * s};
}
vec4 v4_div(vec4 v, float s) {
    return (vec4){v.x / s, v.y / s, v.z / s, v.w / s};
}
float v4_dot(vec4 a, vec4 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}
float v4_length(vec4 v) { return sqrtf(v4_dot(v, v)); }
vec4  v4_normalize(vec4 v) {
    float len = v4_length(v);
    return (len != 0.0f) ? v4_div(v, len) : (vec4){0.0f, 0.0f, 0.0f, 0.0f};
}
vec4 v4_lerp(vec4 a, vec4 b, float t) {
    return (vec4){a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t,
                  a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t};
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
    mat4  result = m4_identity();
    result.m[5] = c;
    result.m[6] = s;
    result.m[9] = -s;
    result.m[10] = c;
    return result;
}

mat4 m4_rotation_y(float radians) {
    float c = cosf(radians), s = sinf(radians);
    mat4  result = m4_identity();
    result.m[0] = c;
    result.m[2] = -s;
    result.m[8] = s;
    result.m[10] = c;
    return result;
}

mat4 m4_rotation_z(float radians) {
    float c = cosf(radians), s = sinf(radians);
    mat4  result = m4_identity();
    result.m[0] = c;
    result.m[1] = s;
    result.m[4] = -s;
    result.m[5] = c;
    return result;
}

mat4 m4_perspective(float fov_radians, float aspect, float near_z,
                    float far_z) {
    float f = 1.0f / tanf(fov_radians / 2.0f);
    mat4  result = {0};
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
