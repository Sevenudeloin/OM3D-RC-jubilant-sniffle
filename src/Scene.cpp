#include "Scene.h"
#include "Camera.h"
#include "SceneObject.h"

#include <TypedBuffer.h>

#include <memory>
#include <shader_structs.h>

namespace OM3D {

Scene::Scene() {
}

void Scene::add_object(SceneObject obj) {
    _objects.emplace_back(std::move(obj));
}

void Scene::add_light(PointLight obj) {
    _point_lights.emplace_back(std::move(obj));
}

Span<const SceneObject> Scene::objects() const {
    return _objects;
}

Span<const PointLight> Scene::point_lights() const {
    return _point_lights;
}

Camera& Scene::camera() {
    return _camera;
}

const Camera& Scene::camera() const {
    return _camera;
}

void Scene::set_sun(glm::vec3 direction, glm::vec3 color) {
    _sun_direction = direction;
    _sun_color = color;
}

static bool is_on_frustum(const Frustum& frustum, const SceneObject& obj, const Camera& camera) {
    std::shared_ptr<StaticMesh> obj_mesh = obj.mesh();
    glm::vec3 obj_bounding_sphere_center = obj_mesh->bounding_sphere_center();
    float obj_bounding_sphere_radius = obj_mesh->bounding_sphere_radius();

    const glm::vec3 global_scale = glm::vec3(
        glm::length(glm::vec3(obj.transform()[0])),
        glm::length(glm::vec3(obj.transform()[1])),
        glm::length(glm::vec3(obj.transform()[2]))
    );

    const glm::vec3 global_center = glm::vec3(obj.transform() * glm::vec4(obj_bounding_sphere_center, 1.0f));
    // const glm::vec3 global_center = obj_bounding_sphere_center;

    const float max_scale = std::max(std::max(global_scale.x, global_scale.y), global_scale.z);

    const float global_radius = obj_bounding_sphere_radius * (0.5f*max_scale);
    // const float global_radius = obj_bounding_sphere_radius;

    return (glm::dot(frustum._near_normal, global_center - camera.position()) > -global_radius
            && glm::dot(frustum._top_normal, global_center - camera.position()) > -global_radius
            && glm::dot(frustum._bottom_normal, global_center - camera.position()) > -global_radius
            && glm::dot(frustum._right_normal, global_center - camera.position()) > -global_radius
            && glm::dot(frustum._left_normal, global_center - camera.position()) > -global_radius);
}

void Scene::render() const {
    // Fill and bind frame data buffer
    TypedBuffer<shader::FrameData> buffer(nullptr, 1);
    {
        auto mapping = buffer.map(AccessType::WriteOnly);
        mapping[0].camera.view_proj = _camera.view_proj_matrix();
        mapping[0].point_light_count = u32(_point_lights.size());
        mapping[0].sun_color = _sun_color;
        mapping[0].sun_dir = glm::normalize(_sun_direction);
    }
    buffer.bind(BufferUsage::Uniform, 0);

    // Fill and bind lights buffer
    TypedBuffer<shader::PointLight> light_buffer(nullptr, std::max(_point_lights.size(), size_t(1)));
    {
        auto mapping = light_buffer.map(AccessType::WriteOnly);
        for(size_t i = 0; i != _point_lights.size(); ++i) {
            const auto& light = _point_lights[i];
            mapping[i] = {
                light.position(),
                light.radius(),
                light.color(),
                0.0f
            };
        }
    }
    light_buffer.bind(BufferUsage::Storage, 1);

    Frustum frustum = _camera.build_frustum();

    // Render every object
    for(const SceneObject& obj : _objects) {
        // Frustum culling (ours)
        if (is_on_frustum(frustum, obj, _camera))
        {
            obj.render();
        }
    }
}

}
