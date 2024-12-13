#version 450

// fragment shader for flatland rendering test

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec2 in_uv;

void main() {
    out_color = vec4(in_uv, 0.0, 1.0);
}