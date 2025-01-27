#version 450

// fragment shader for flatland rendering test

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec2 in_uv;

layout(binding = 0) uniform sampler2D scene_image;

void main() {
    out_color = vec4(texture(scene_image, in_uv).rgb, 1.0);
}