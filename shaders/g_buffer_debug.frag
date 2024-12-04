#version 450

#include "utils.glsl"

layout(location = 0) out vec4 out_color;

// layout(location = 0) in vec2 in_uv;

layout(location = 0) uniform uint debug_mode;

layout(binding = 1) uniform sampler2D in_albedo;
layout(binding = 2) uniform sampler2D in_normal;
layout(binding = 3) uniform sampler2D in_depth; // ?

void main() {
    const ivec2 coord = ivec2(gl_FragCoord.xy);

    out_color = vec4(1.0, 0.0, 0.0, 1.0); // in case

    if (debug_mode == 0) { // albedo
        const vec3 albedo = texelFetch(in_albedo, coord, 0).rgb;
        out_color = vec4(albedo, 1.0);
    }
    else if (debug_mode == 1) { // normals
        const vec3 normal = texelFetch(in_normal, coord, 0).rgb;
        out_color = vec4(normal, 1.0);
    }
    else if (debug_mode == 2) { // depth
        float depth = texelFetch(in_depth, coord, 0).r;
        depth = pow(depth, 0.35);
        out_color = vec4(vec3(depth), 1.0);
    }
}