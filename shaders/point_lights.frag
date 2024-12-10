#version 450

#include "utils.glsl"

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec2 out_uv;

layout(binding = 0) uniform Data {
    FrameData frame;
};

layout(binding = 1) uniform PointLightsData {
    PointLight point_lights[32]; // for now put 32 to have margin, actual pointlights would be at the start anyway
};

layout(binding = 2) uniform sampler2D in_albedo;
layout(binding = 3) uniform sampler2D in_normal;
layout(binding = 4) uniform sampler2D in_depth;

layout(location = 5) uniform uint point_light_i;


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

        vec3 point_light_dir = point_lights[point_light_i].position - actual_pos;
        // vec3 color = point_lights[point_light_i].color * max(dot(point_light_dir, normal), 0.0) * albedo;
        vec3 color = point_lights[point_light_i].color; // * max(dot(point_light_dir, normal), 0.0) * albedo;

        out_color = vec4(clamp(color, 0.0, 1.0), 1.0);

        // out_color = vec4(normal, 1.0);
    }
}