#version 450

// fragment shader for flatland rendering test

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec2 in_uv;

uniform vec2 screen_res = vec2(1600, 900); // (width, height), for now hardcoded
uniform bool is_drawing = false;
uniform vec2 mouse_pos; // y axis is inverted to match opengl 2D coord space

void main() {
    if (is_drawing) {
        vec2 frag_pos = in_uv * screen_res;
        float frag_mouse_dist = length(frag_pos - mouse_pos);

        if (frag_mouse_dist < 10.0) { // 10 pixels
            out_color = vec4(0.0, 0.0, 1.0, 1.0);
            return;
        }
    }

    out_color = vec4(in_uv, 0.0, 1.0);
}