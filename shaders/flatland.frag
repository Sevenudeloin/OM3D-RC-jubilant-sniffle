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

        vec2 line_vec = mouse_pos - prev_mouse_pos;
        vec2 start_frag_vec = frag_pos - prev_mouse_pos;
        float t = clamp(dot(line_vec, start_frag_vec) / dot(line_vec, line_vec), 0.0, 1.0);
        vec2 closest_point = prev_mouse_pos + t * line_vec;

        float frag_dist = length(frag_pos - closest_point);

        if (frag_dist < line_width) {
            prev_color = vec4(line_color, 1.0);
        }
    }

    out_color = prev_color;
}