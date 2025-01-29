#version 450

layout(location = 0) in vec3 fragColor; // Interpolated color from the vertex shader
layout(location = 0) out vec4 outColor; // Final fragment color

void main() {
    // Output the interpolated color
    outColor = vec4(fragColor, 1.0); // Add alpha = 1.0 (fully opaque)
}

