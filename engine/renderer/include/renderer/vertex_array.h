#pragma once

#include "render_types.h"

#include <glad/gl.h>

#include <cstddef>
#include <cstdint>

namespace placeholder::renderer
{

class OpenGLRenderDevice;

/// Well-known vertex attribute locations matching shader layout qualifiers.
namespace AttribLocation
{
    constexpr uint32_t POSITION     = 0;
    constexpr uint32_t NORMAL       = 1;
    constexpr uint32_t BONE_INDICES = 2;
    constexpr uint32_t BONE_WEIGHTS = 3;
    constexpr uint32_t TEXCOORD     = 4;
    constexpr uint32_t TEXCOORD2    = 5;
    constexpr uint32_t COLOR        = 6;
    constexpr uint32_t COLOR2       = 7;
} // namespace AttribLocation

/// RAII wrapper around an OpenGL Vertex Array Object with associated buffers.
///
/// Manages a VAO, one VBO, and one optional EBO. Move-only.
class VertexArray
{
public:
    explicit VertexArray(OpenGLRenderDevice& device);
    ~VertexArray();

    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;
    VertexArray(VertexArray&& other) noexcept;
    VertexArray& operator=(VertexArray&& other) noexcept;

    /// Bind this VAO for rendering or attribute setup.
    void bind();

    /// Upload vertex data to the VBO.
    void setVertexData(const void* data, size_t sizeBytes,
                       BufferUsage usage = BufferUsage::Static);

    /// Upload 16-bit index data to the EBO.
    void setIndexData(const uint16_t* data, size_t count,
                      BufferUsage usage = BufferUsage::Static);

    /// Upload 32-bit index data to the EBO.
    void setIndexData(const uint32_t* data, size_t count,
                      BufferUsage usage = BufferUsage::Static);

    /// Configure a floating-point vertex attribute.
    void setAttribute(uint32_t location, int32_t componentCount,
                      VertexAttribType type, uint32_t stride, uint32_t offset);

    /// Configure an integer vertex attribute (no normalization).
    void setAttributeInt(uint32_t location, int32_t componentCount,
                         VertexAttribType type, uint32_t stride, uint32_t offset);

    /// Draw using the index buffer.
    void draw(PrimitiveTopology topology, int count = -1, int offset = 0);

    /// Draw without an index buffer.
    void drawArrays(PrimitiveTopology topology, int first, int count);

    /// @return The OpenGL VAO handle.
    GLuint handle() const { return m_vao; }

    /// @return Number of indices stored.
    int indexCount() const { return m_indexCount; }

private:
    void ensureEbo();

    static GLenum toGlUsage(BufferUsage usage);
    static GLenum toGlAttribType(VertexAttribType type);

    OpenGLRenderDevice* m_device = nullptr;
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_ebo = 0;
    int m_indexCount = 0;
    GLenum m_indexType = GL_UNSIGNED_SHORT;
};

} // namespace placeholder::renderer
