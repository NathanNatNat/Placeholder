#include "renderer/vertex_array.h"
#include "renderer/opengl_render_device.h"

namespace placeholder::renderer
{

VertexArray::VertexArray(OpenGLRenderDevice& device)
    : m_device(&device)
{
    glCreateVertexArrays(1, &m_vao);
    glCreateBuffers(1, &m_vbo);
}

VertexArray::~VertexArray()
{
    if (m_vao != 0)
    {
        if (m_device)
        {
            m_device->unbindVao(m_vao);
        }
        glDeleteVertexArrays(1, &m_vao);
    }
    if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);
    if (m_ebo != 0) glDeleteBuffers(1, &m_ebo);
}

VertexArray::VertexArray(VertexArray&& other) noexcept
    : m_device(other.m_device)
    , m_vao(other.m_vao)
    , m_vbo(other.m_vbo)
    , m_ebo(other.m_ebo)
    , m_indexCount(other.m_indexCount)
    , m_indexType(other.m_indexType)
{
    other.m_device = nullptr;
    other.m_vao = 0;
    other.m_vbo = 0;
    other.m_ebo = 0;
    other.m_indexCount = 0;
}

VertexArray& VertexArray::operator=(VertexArray&& other) noexcept
{
    if (this != &other)
    {
        if (m_vao != 0)
        {
            if (m_device) m_device->unbindVao(m_vao);
            glDeleteVertexArrays(1, &m_vao);
        }
        if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);
        if (m_ebo != 0) glDeleteBuffers(1, &m_ebo);

        m_device = other.m_device;
        m_vao = other.m_vao;
        m_vbo = other.m_vbo;
        m_ebo = other.m_ebo;
        m_indexCount = other.m_indexCount;
        m_indexType = other.m_indexType;

        other.m_device = nullptr;
        other.m_vao = 0;
        other.m_vbo = 0;
        other.m_ebo = 0;
        other.m_indexCount = 0;
    }
    return *this;
}

void VertexArray::bind()
{
    if (m_device)
    {
        m_device->bindVao(m_vao);
    }
}

void VertexArray::setVertexData(const void* data, size_t sizeBytes, BufferUsage usage)
{
    bind();
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeBytes), data, toGlUsage(usage));
}

void VertexArray::setIndexData(const uint16_t* data, size_t count, BufferUsage usage)
{
    bind();
    ensureEbo();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(count * sizeof(uint16_t)),
                 data, toGlUsage(usage));
    m_indexCount = static_cast<int>(count);
    m_indexType = GL_UNSIGNED_SHORT;
}

void VertexArray::setIndexData(const uint32_t* data, size_t count, BufferUsage usage)
{
    bind();
    ensureEbo();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(count * sizeof(uint32_t)),
                 data, toGlUsage(usage));
    m_indexCount = static_cast<int>(count);
    m_indexType = GL_UNSIGNED_INT;
}

void VertexArray::setAttribute(uint32_t location, int32_t componentCount,
                                VertexAttribType type, uint32_t stride, uint32_t offset)
{
    bind();
    glEnableVertexAttribArray(location);
    glVertexAttribPointer(location, componentCount, toGlAttribType(type),
                          (type == VertexAttribType::UByteNormalized) ? GL_TRUE : GL_FALSE,
                          static_cast<GLsizei>(stride),
                          reinterpret_cast<const void*>(static_cast<uintptr_t>(offset)));
}

void VertexArray::setAttributeInt(uint32_t location, int32_t componentCount,
                                   VertexAttribType type, uint32_t stride, uint32_t offset)
{
    bind();
    glEnableVertexAttribArray(location);
    glVertexAttribIPointer(location, componentCount, toGlAttribType(type),
                           static_cast<GLsizei>(stride),
                           reinterpret_cast<const void*>(static_cast<uintptr_t>(offset)));
}

void VertexArray::draw(PrimitiveTopology topology, int count, int offset)
{
    bind();
    int drawCount = (count < 0) ? m_indexCount : count;
    GLenum mode = GL_TRIANGLES;
    switch (topology)
    {
        case PrimitiveTopology::Triangles: mode = GL_TRIANGLES; break;
        case PrimitiveTopology::Lines:     mode = GL_LINES; break;
        case PrimitiveTopology::Points:    mode = GL_POINTS; break;
    }

    size_t byteOffset = 0;
    if (m_indexType == GL_UNSIGNED_SHORT)
        byteOffset = static_cast<size_t>(offset) * sizeof(uint16_t);
    else
        byteOffset = static_cast<size_t>(offset) * sizeof(uint32_t);

    glDrawElements(mode, drawCount, m_indexType,
                   reinterpret_cast<const void*>(byteOffset));
}

void VertexArray::drawArrays(PrimitiveTopology topology, int first, int count)
{
    bind();
    GLenum mode = GL_TRIANGLES;
    switch (topology)
    {
        case PrimitiveTopology::Triangles: mode = GL_TRIANGLES; break;
        case PrimitiveTopology::Lines:     mode = GL_LINES; break;
        case PrimitiveTopology::Points:    mode = GL_POINTS; break;
    }
    glDrawArrays(mode, first, count);
}

void VertexArray::ensureEbo()
{
    if (m_ebo == 0)
    {
        glCreateBuffers(1, &m_ebo);
    }
}

GLenum VertexArray::toGlUsage(BufferUsage usage)
{
    switch (usage)
    {
        case BufferUsage::Static:  return GL_STATIC_DRAW;
        case BufferUsage::Dynamic: return GL_DYNAMIC_DRAW;
        case BufferUsage::Stream:  return GL_STREAM_DRAW;
    }
    return GL_STATIC_DRAW;
}

GLenum VertexArray::toGlAttribType(VertexAttribType type)
{
    switch (type)
    {
        case VertexAttribType::Float:           return GL_FLOAT;
        case VertexAttribType::UByte:           return GL_UNSIGNED_BYTE;
        case VertexAttribType::UByteNormalized: return GL_UNSIGNED_BYTE;
        case VertexAttribType::UInt:            return GL_UNSIGNED_INT;
    }
    return GL_FLOAT;
}

} // namespace placeholder::renderer
