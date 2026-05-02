#pragma once

#include "render_types.h"

#include <glad/gl.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace placeholder::renderer
{

class OpenGLRenderDevice;

/// RAII wrapper around an OpenGL Uniform Buffer Object with a CPU shadow copy.
///
/// Write data to the CPU shadow via typed setters, then call upload() to
/// transfer dirty data to the GPU. Move-only.
class UniformBuffer
{
public:
    /// std140 alignment constants.
    static constexpr size_t ALIGN_VEC4 = 16;
    static constexpr size_t ALIGN_MAT4 = 64;

    UniformBuffer(OpenGLRenderDevice& device, size_t size,
                  BufferUsage usage = BufferUsage::Dynamic);
    ~UniformBuffer();

    UniformBuffer(const UniformBuffer&) = delete;
    UniformBuffer& operator=(const UniformBuffer&) = delete;
    UniformBuffer(UniformBuffer&& other) noexcept;
    UniformBuffer& operator=(UniformBuffer&& other) noexcept;

    /// Bind this UBO to a binding point.
    void bind(uint32_t bindingPoint);

    /// @name Typed Setters (write to CPU shadow, mark dirty)
    /// @{
    void setFloat(size_t offset, float value);
    void setInt(size_t offset, int32_t value);
    void setVec3(size_t offset, float x, float y, float z);
    void setVec4(size_t offset, float x, float y, float z, float w);
    void setMat4(size_t offset, const float* matrix);
    void setData(size_t offset, const void* data, size_t size);
    /// @}

    /// Upload the entire shadow buffer to the GPU.
    void upload();

    /// @return The OpenGL buffer handle.
    GLuint handle() const { return m_buffer; }

    /// @return Total size of the buffer in bytes.
    size_t size() const { return m_data.size(); }

private:
    OpenGLRenderDevice* m_device = nullptr;
    GLuint m_buffer = 0;
    std::vector<uint8_t> m_data;
    bool m_dirty = false;
};

} // namespace placeholder::renderer
