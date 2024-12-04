#include "Scene.h"

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

glm::vec3 Scene::sun_direction() const {
    return _sun_direction;
}

glm::vec3 Scene::sun_color() const {
    return _sun_color;
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

    // Frustum frustum = _camera.build_frustum();

    // Render every object
    for(const SceneObject& obj : _objects) {
        // // Frustum culling (ours)
        // std::shared_ptr<StaticMesh> obj_mesh = obj.mesh();
        // glm::vec3 obj_center_to_cam = _camera.position() - obj_mesh->bounding_sphere_center();
        // float obj_bounding_sphere_radius = obj_mesh->bounding_sphere_radius();

        // if (glm::dot(obj_center_to_cam, frustum._near_normal) <= obj_bounding_sphere_radius
        //     && glm::dot(obj_center_to_cam, frustum._left_normal) <= obj_bounding_sphere_radius
        //     && glm::dot(obj_center_to_cam, frustum._right_normal) <= obj_bounding_sphere_radius
        //     && glm::dot(obj_center_to_cam, frustum._bottom_normal) <= obj_bounding_sphere_radius
        //     && glm::dot(obj_center_to_cam, frustum._top_normal) <= obj_bounding_sphere_radius)
        // {
        //     obj.render();
        // }

        obj.render();
    }
}

void Scene::setup_sun_framedata_uniform() const {
    // Fill and bind frame data buffer
}

}