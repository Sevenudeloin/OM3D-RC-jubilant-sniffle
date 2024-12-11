#version 450

#include "utils.glsl"

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec2 out_uv;

layout(binding = 0) uniform Data {
    FrameData frame;
};

layout(binding = 1) uniform sampler2D in_albedo;
layout(binding = 2) uniform sampler2D in_normal;
layout(binding = 3) uniform sampler2D in_depth;


vec3 unproject(vec2 uv, float depth, mat4 inv_viewproj) {
    const vec3 ndc = vec3(uv * 2.0 - vec2(1.0), depth);
    const vec4 p = inv_viewproj * vec4(ndc, 1.0);
    return p.xyz / p.w;
}

void main() {
    const ivec2 coord = ivec2(gl_FragCoord.xy);

    float depth = texelFetch(in_depth, coord, 0).r;

    const vec3 albedo = texelFetch(in_albedo, coord, 0).rgb;

    if (depth == 0.0f) {
        out_color = vec4(albedo, 1.0);
    } else {
        vec3 actual_pos = unproject(out_uv, depth, frame.camera.inv_view_proj);

        const vec3 normal = normalize(texelFetch(in_normal, coord, 0).rgb * 2.0 - 1.0);
        // vec3 color = frame.sun_color * max(dot(frame.sun_dir, normal), 0.0) * albedo;
        vec3 color = vec3(0.9, 0.3, 0.1) * max(dot(vec3(0.2, 1.0, 0.1), normal), 0.0) * albedo; // hardcoded for tests only

        out_color = vec4(color, 1.0);

        // out_color = vec4(normal, 1.0);
    }
}