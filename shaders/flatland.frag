#version 450

// fragment shader for flatland rendering test

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec2 in_uv;

uniform vec2 screen_res;
uniform bool is_drawing = false;
uniform vec2 prev_mouse_pos; // y axis is inverted to match opengl 2D coord space
uniform vec2 mouse_pos; // y axis is inverted to match opengl 2D coord space
uniform vec3 line_color = vec3(1.0);
uniform float line_width = 10.0;

layout(binding = 0) uniform sampler2D prev_frame;

void main() {
    vec4 prev_color = texture(prev_frame, in_uv);

    // Drawing logic

    if (is_drawing) {
        vec2 frag_pos = in_uv * screen_res;
        float frag_mouse_dist = length(frag_pos - mouse_pos);

        if (frag_mouse_dist < line_width) {
            prev_color = vec4(line_color, 1.0);
        }
    }

    out_color = prev_color;
}