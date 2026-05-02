#include "renderer/mesh.h"
#include "renderer/opengl_render_device.h"

namespace placeholder::renderer
{

Mesh Mesh::create(OpenGLRenderDevice& device, const MeshData& data)
{
    Mesh mesh;
    mesh.m_subMeshes = data.subMeshes;

    mesh.m_vao = std::make_unique<VertexArray>(device);
    mesh.m_vao->setVertexData(data.vertices.data(),
                              data.vertices.size() * sizeof(MeshVertex));
    mesh.m_vao->setIndexData(data.indices.data(), data.indices.size());

    uint32_t stride = sizeof(MeshVertex);
    mesh.m_vao->setAttribute(AttribLocation::POSITION, 3, VertexAttribType::Float,
                             stride, offsetof(MeshVertex, position));
    mesh.m_vao->setAttribute(AttribLocation::NORMAL, 3, VertexAttribType::Float,
                             stride, offsetof(MeshVertex, normal));
    mesh.m_vao->setAttribute(AttribLocation::TEXCOORD, 2, VertexAttribType::Float,
                             stride, offsetof(MeshVertex, texCoord));

    return mesh;
}

void Mesh::drawSubMesh(size_t subMeshIndex)
{
    if (!m_vao || subMeshIndex >= m_subMeshes.size())
    {
        return;
    }

    const auto& sub = m_subMeshes[subMeshIndex];
    m_vao->draw(PrimitiveTopology::Triangles,
                static_cast<int>(sub.indexCount),
                static_cast<int>(sub.indexOffset));
}

void Mesh::drawAll()
{
    if (!m_vao)
    {
        return;
    }

    for (size_t i = 0; i < m_subMeshes.size(); ++i)
    {
        drawSubMesh(i);
    }
}

} // namespace placeholder::renderer
