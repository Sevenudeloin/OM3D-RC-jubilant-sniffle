#version 450

#include "utils.glsl"

layout(location = 0) out vec4 out_color;

// layout(location = 0) in vec2 in_uv;

layout(binding = 0) uniform sampler2D in_albedo;
layout(binding = 1) uniform sampler2D in_normal;

void main() {
    const ivec2 coord = ivec2(gl_FragCoord.xy);

    out_color = vec4(1.0, 0.0, 0.0, 1.0); // in case

#if 0 // albedo
    const vec3 albedo = texelFetch(in_albedo, coord, 0).rgb;
    out_color = vec4(albedo, 1.0);
#endif

#if 1 // normal
    const vec3 normal = texelFetch(in_normal, coord, 0).rgb;
    out_color = vec4(normal, 1.0);
#endif
}