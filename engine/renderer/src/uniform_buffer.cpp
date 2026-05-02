#include "renderer/uniform_buffer.h"
#include "renderer/opengl_render_device.h"

#include <cstring>

namespace placeholder::renderer
{

static GLenum toGlUsage(BufferUsage usage)
{
    switch (usage)
    {
        case BufferUsage::Static:  return GL_STATIC_DRAW;
        case BufferUsage::Dynamic: return GL_DYNAMIC_DRAW;
        case BufferUsage::Stream:  return GL_STREAM_DRAW;
    }
    return GL_DYNAMIC_DRAW;
}

UniformBuffer::UniformBuffer(OpenGLRenderDevice& device, size_t size, BufferUsage usage)
    : m_device(&device)
    , m_data(size, 0)
{
    glCreateBuffers(1, &m_buffer);
    glNamedBufferData(m_buffer, static_cast<GLsizeiptr>(size), nullptr, toGlUsage(usage));
}

UniformBuffer::~UniformBuffer()
{
    if (m_buffer != 0)
    {
        glDeleteBuffers(1, &m_buffer);
    }
}

UniformBuffer::UniformBuffer(UniformBuffer&& other) noexcept
    : m_device(other.m_device)
    , m_buffer(other.m_buffer)
    , m_data(std::move(other.m_data))
    , m_dirty(other.m_dirty)
{
    other.m_device = nullptr;
    other.m_buffer = 0;
    other.m_dirty = false;
}

UniformBuffer& UniformBuffer::operator=(UniformBuffer&& other) noexcept
{
    if (this != &other)
    {
        if (m_buffer != 0)
        {
            glDeleteBuffers(1, &m_buffer);
        }

        m_device = other.m_device;
        m_buffer = other.m_buffer;
        m_data = std::move(other.m_data);
        m_dirty = other.m_dirty;

        other.m_device = nullptr;
        other.m_buffer = 0;
        other.m_dirty = false;
    }
    return *this;
}

void UniformBuffer::bind(uint32_t bindingPoint)
{
    glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, m_buffer);
}

void UniformBuffer::setFloat(size_t offset, float value)
{
    std::memcpy(m_data.data() + offset, &value, sizeof(float));
    m_dirty = true;
}

void UniformBuffer::setInt(size_t offset, int32_t value)
{
    std::memcpy(m_data.data() + offset, &value, sizeof(int32_t));
    m_dirty = true;
}

void UniformBuffer::setVec3(size_t offset, float x, float y, float z)
{
    float v[3] = {x, y, z};
    std::memcpy(m_data.data() + offset, v, sizeof(v));
    m_dirty = true;
}

void UniformBuffer::setVec4(size_t offset, float x, float y, float z, float w)
{
    float v[4] = {x, y, z, w};
    std::memcpy(m_data.data() + offset, v, sizeof(v));
    m_dirty = true;
}

void UniformBuffer::setMat4(size_t offset, const float* matrix)
{
    std::memcpy(m_data.data() + offset, matrix, 16 * sizeof(float));
    m_dirty = true;
}

void UniformBuffer::setData(size_t offset, const void* data, size_t size)
{
    std::memcpy(m_data.data() + offset, data, size);
    m_dirty = true;
}

void UniformBuffer::upload()
{
    if (m_dirty && m_buffer != 0)
    {
        glNamedBufferSubData(m_buffer, 0,
                             static_cast<GLsizeiptr>(m_data.size()),
                             m_data.data());
        m_dirty = false;
    }
}

} // namespace placeholder::renderer
