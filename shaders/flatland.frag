#version 450

// fragment shader for flatland rendering test

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec2 in_uv;

uniform vec2 screen_res;

uniform int ray_count = 4;
uniform int max_steps = 128; // 256

layout(binding = 0) uniform sampler2D drawing_image;

#define TAU 6.2831855

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

    float one_over_ray_count = 1.0 / float(ray_count);
    float tau_over_ray_count = TAU / float(ray_count);

    float noise = tmp_rand(in_uv);

    vec4 radiance = vec4(0.0);

    for (int i = 0; i < ray_count; i++) {
        float angle = tau_over_ray_count * (float(i) + noise);
        vec2 ray_dir_uv = vec2(cos(angle), sin(angle)) / screen_res; // -sin ?

        for (int step_i = 0; step_i < max_steps; step_i++) {
            vec2 sample_uv = in_uv + ray_dir_uv * float(step_i);

            if (out_of_bounds(sample_uv)) {
                break;
            }

            vec4 sample_light = texture(drawing_image, sample_uv);
            radiance += sample_light / 50.0;
        }
    }

    return radiance * one_over_ray_count;
}

void main() {
    // out_color = vec4(raymarch().rgba);
    out_color = vec4(raymarch().rgb, 1.0);
}