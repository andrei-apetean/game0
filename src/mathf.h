#pragma once 


#define PI 3.14159265358979323846

typedef struct {
    float x;
    float y;
} vec2;

typedef struct {
    float x;
    float y;
    float z;
} vec3;

typedef struct {
    float x;
    float y;
    float z;
    float w;
} vec4;

typedef struct {
    float m[16];  // column major
} mat4;

float deg2rad(float degrees);
float rad2deg(float radians);

vec2  v2_add(vec2 a, vec2 b);
vec2  v2_sub(vec2 a, vec2 b);
vec2  v2_mul(vec2 a, vec2 b);
vec2  v2_scale(vec2 v, float s);
vec2  v2_div(vec2 v, float s);
vec2  v2_normalize(vec2 v);
vec2  v2_lerp(vec2 a, vec2 b, float t);
float v2_dot(vec2 a, vec2 b);
float v2_length(vec2 v);

vec3  v3_zero();
vec3  v3_one();
vec3  v3_left();
vec3  v3_right();
vec3  v3_up();
vec3  v3_add(vec3 a, vec3 b);
vec3  v3_sub(vec3 a, vec3 b);
vec3  v3_mul(vec3 a, vec3 b);
vec3  v3_scale(vec3 v, float s);
vec3  v3_div(vec3 v, float s);
vec3  v3_cross(vec3 a, vec3 b);
vec3  v3_normalize(vec3 v);
vec3  v3_lerp(vec3 a, vec3 b, float t);
float v3_length(vec3 v);
float v3_dot(vec3 a, vec3 b);

vec4  v4_add(vec4 a, vec4 b);
vec4  v4_sub(vec4 a, vec4 b);
vec4  v4_mul(vec4 a, vec4 b);
vec4  v4_scale(vec4 v, float s);
vec4  v4_div(vec4 v, float s);
vec4  v4_normalize(vec4 v);
vec4  v4_lerp(vec4 a, vec4 b, float t);
float v4_dot(vec4 a, vec4 b);
float v4_length(vec4 v);

mat4 m4_identity(void);
mat4 m4_translation(vec3 translation);
mat4 m4_rotation_x(float radians);
mat4 m4_rotation_y(float radians);
mat4 m4_rotation_z(float radians);
mat4 m4_perspective(float fov_radians, float aspect, float near_z, float far_z);
mat4 m4_mul(mat4 a, mat4 b);
