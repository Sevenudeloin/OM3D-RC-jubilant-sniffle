#include "StaticMesh.h"

#include <glad/gl.h>

#include <utility>

#include <glm/geometric.hpp>

namespace OM3D {

extern bool audit_bindings_before_draw;

static std::pair<glm::vec3, float> compute_mesh_bounding_sphere(const MeshData& mesh_data) {
    // LEGACY
    // for (size_t i = 0; i < mesh_data.indices.size(); i += 3) {
    //     // use const glm::vec3& instead ?
    //     glm::vec3 v0 = mesh_data.vertices[mesh_data.indices[i + 0]].position;
    //     glm::vec3 v1 = mesh_data.vertices[mesh_data.indices[i + 1]].position;
    //     glm::vec3 v2 = mesh_data.vertices[mesh_data.indices[i + 2]].position;
    // }

    // first compute bounding box

    glm::vec3 min_corner = mesh_data.vertices[0].position;
    glm::vec3 max_corner = mesh_data.vertices[0].position;

    for (const Vertex& v : mesh_data.vertices) {
        glm::vec3 v_pos = v.position;

        min_corner.x = std::min(min_corner.x, v_pos.x);
        min_corner.y = std::min(min_corner.y, v_pos.y);
        min_corner.z = std::min(min_corner.z, v_pos.z);

        max_corner.x = std::max(max_corner.x, v_pos.x);
        max_corner.y = std::max(max_corner.y, v_pos.y);
        max_corner.z = std::max(max_corner.z, v_pos.z);
    }

    // then compute bounding sphere

    glm::vec3 bounding_sphere_center = glm::vec3(
        (min_corner.x + max_corner.x) * 0.5f,
        (min_corner.y + max_corner.y) * 0.5f,
        (min_corner.z + max_corner.z) * 0.5f 
    );

    // compute radius best way :
    glm::vec3 corners[8] = {
        glm::vec3(min_corner.x, min_corner.y, min_corner.z),
        glm::vec3(max_corner.x, min_corner.y, min_corner.z),
        glm::vec3(min_corner.x, max_corner.y, min_corner.z),
        glm::vec3(max_corner.x, max_corner.y, min_corner.z),
        glm::vec3(min_corner.x, min_corner.y, max_corner.z),
        glm::vec3(max_corner.x, min_corner.y, max_corner.z),
        glm::vec3(min_corner.x, max_corner.y, max_corner.z),
        glm::vec3(max_corner.x, max_corner.y, max_corner.z)
    };

    float bounding_sphere_radius = 0.0f;
    for (const auto& corner : corners) {
        bounding_sphere_radius = std::max(bounding_sphere_radius, glm::length(corner - bounding_sphere_center));
    }

    return std::make_pair(bounding_sphere_center, bounding_sphere_radius);
}

StaticMesh::StaticMesh(const MeshData& data) :
    _vertex_buffer(data.vertices),
    _index_buffer(data.indices) {
        std::pair<glm::vec3, float> bounding_sphere_info = compute_mesh_bounding_sphere(data);
        _bounding_sphere_center = bounding_sphere_info.first;
        _bounding_sphere_radius = bounding_sphere_info.second;
}

void StaticMesh::draw() const {
    _vertex_buffer.bind(BufferUsage::Attribute);
    _index_buffer.bind(BufferUsage::Index);

    // Vertex position
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), nullptr);
    // Vertex normal
    glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(3 * sizeof(float)));
    // Vertex uv
    glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(6 * sizeof(float)));
    // Tangent / bitangent sign
    glVertexAttribPointer(3, 4, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(8 * sizeof(float)));
    // Vertex color
    glVertexAttribPointer(4, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(12 * sizeof(float)));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);

    if(audit_bindings_before_draw) {
        audit_bindings();
    }

    glDrawElements(GL_TRIANGLES, int(_index_buffer.element_count()), GL_UNSIGNED_INT, nullptr);
}

glm::vec3 StaticMesh::bounding_sphere_center() const {
    return _bounding_sphere_center;
}

float StaticMesh::bounding_sphere_radius() const {
    return _bounding_sphere_radius;
}

}
