struct CameraData {
    mat4 view_proj;
    mat4 inv_view_proj;
};

struct FrameData {
    CameraData camera;

    vec3 sun_dir;
    uint point_light_count;

    vec3 sun_color;
    float padding_1; // to be able to use arrays of this struct (std140 layout, aligned on 16 bytes)
};

struct PointLight {
    vec3 position;
    float radius;
    vec3 color;
    float padding_1; // to be able to use arrays of this struct (std140 layout, aligned on 16 bytes)
};

