#version 450

// fragment shader for flatland rendering test

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec2 in_uv;

uniform vec2 screen_res;

uniform int ray_count = 16; // 8-16
uniform int max_steps = 8; // 48

layout(binding = 0) uniform sampler2D drawing_image;
layout(binding = 1) uniform sampler2D jfa_dist_image;

#define TAU 6.2831855
#define EPS3 0.001
#define EPS5 0.00001

bool out_of_bounds( in vec2 uv ) {
    return uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0;
}

float tmp_rand( in vec2 v ) {
    return fract(sin(dot(v, vec2(12.9898, 78.233))) * 43758.5453);
}

vec4 raymarch() {
    vec4 light = texture(drawing_image, in_uv);
    if (light.a > 0.1) {
        return light;
    }

    float angle_step_size = TAU / float(ray_count);

    float noise = tmp_rand(in_uv);

    vec4 radiance = vec4(0.0);

    for (int i = 0; i < ray_count; i++) {
        float angle = angle_step_size * (float(i) + noise);
        vec2 ray_dir_uv = vec2(cos(angle), sin(angle)); // -sin ?

        vec2 sample_uv = in_uv;

        for (int step_i = 1; step_i < max_steps; step_i++) {
            float dist = texture(jfa_dist_image, sample_uv).r;
            sample_uv += ray_dir_uv * dist;

            if (out_of_bounds(sample_uv)) {
                break;
            }

            if (dist < EPS3) {
                radiance += texture(drawing_image, sample_uv);
                break;
            }
        }
    }

    return radiance / float(ray_count);
}

void main() {
    out_color = vec4(raymarch().rgb, 1.0);
}