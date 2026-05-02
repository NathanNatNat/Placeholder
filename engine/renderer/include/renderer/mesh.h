#pragma once

#include "render_types.h"
#include "vertex_array.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace placeholder::renderer
{

class OpenGLRenderDevice;

/// Vertex layout used by the engine's standard mesh format.
struct MeshVertex
{
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f};
    glm::vec2 texCoord{0.0f};
};

/// A submesh within a loaded model (one draw call per submesh).
struct SubMesh
{
    uint32_t indexOffset = 0;
    uint32_t indexCount = 0;
    uint32_t materialIndex = 0;
};

/// CPU-side mesh data before GPU upload.
struct MeshData
{
    std::vector<MeshVertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<SubMesh> subMeshes;
};

/// GPU-ready mesh with a VAO and submesh draw ranges.
///
/// Default-constructable (creates an empty mesh). Move-only.
class Mesh
{
public:
    Mesh() = default;
    ~Mesh() = default;

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&&) noexcept = default;
    Mesh& operator=(Mesh&&) noexcept = default;

    /// Upload mesh data to the GPU.
    static Mesh create(OpenGLRenderDevice& device, const MeshData& data);

    /// Draw a specific submesh.
    void drawSubMesh(size_t subMeshIndex);

    /// Draw all submeshes.
    void drawAll();

    /// @return Number of submeshes.
    size_t subMeshCount() const { return m_subMeshes.size(); }

    /// @return The submesh descriptor at the given index.
    const SubMesh& subMesh(size_t index) const { return m_subMeshes[index]; }

    bool valid() const { return m_vao != nullptr; }

private:
    std::unique_ptr<VertexArray> m_vao;
    std::vector<SubMesh> m_subMeshes;
};

} // namespace placeholder::renderer
