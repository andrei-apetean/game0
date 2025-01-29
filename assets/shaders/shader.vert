#version 450

layout(location = 0) in vec3 inPosition; // Vertex position
layout(location = 1) in vec3 inColor;    // Vertex color

layout(location = 0) out vec3 fragColor; // Color passed to fragment shader
layout(location = 1) out vec3 fragPosition; // Position passed to fragment shader


layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 modelViewProj; // Model-view-projection matrix
} ubo;

void main() {
    fragColor = inColor; // Pass color to fragment shader
    fragPosition = inPosition; // Pass position to fragment shader
    gl_Position = ubo.modelViewProj* vec4(inPosition, 1.0); // Transform vertex position
}

