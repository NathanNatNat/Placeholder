#pragma once

#include <glad/gl.h>

#include <string>
#include <unordered_map>

namespace placeholder::renderer
{

class OpenGLRenderDevice;

/// RAII wrapper around an OpenGL shader program.
///
/// Handles compilation, linking, uniform location caching, and hot-reload.
/// Move-only — cannot be copied.
class ShaderProgram
{
public:
    /// Compile and link a shader program from vertex and fragment source.
    /// @throws std::runtime_error on compilation or link failure.
    ShaderProgram(OpenGLRenderDevice& device,
                  const std::string& vertexSource,
                  const std::string& fragmentSource);

    ~ShaderProgram();

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;
    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;

    /// @return true if the program linked successfully.
    bool isValid() const;

    /// Bind this program for rendering.
    void bind();

    /// @return Cached uniform location, or -1 if not found.
    GLint getUniformLocation(const std::string& name);

    /// @name Uniform Setters
    /// @{
    void setUniform(const std::string& name, int value);
    void setUniform(const std::string& name, float value);
    void setUniform(const std::string& name, float x, float y);
    void setUniform(const std::string& name, float x, float y, float z);
    void setUniform(const std::string& name, float x, float y, float z, float w);
    void setUniformVec3(const std::string& name, const float* value, int count = 1);
    void setUniformVec4(const std::string& name, const float* value, int count = 1);
    void setUniformMat3(const std::string& name, const float* value, bool transpose = false);
    void setUniformMat4(const std::string& name, const float* value, bool transpose = false);
    void setUniformMat4Array(const std::string& name, const float* value,
                             int count, bool transpose = false);
    /// @}

    /// Bind a named uniform block to a binding point.
    void bindUniformBlock(const std::string& name, uint32_t bindingPoint);

    /// Recompile the shader from new source. Returns true on success.
    /// On failure, the existing program remains active.
    bool recompile(const std::string& vertexSource, const std::string& fragmentSource);

    /// @return The OpenGL program handle.
    GLuint handle() const { return m_program; }

private:
    void compile(const std::string& vertexSource, const std::string& fragmentSource);
    GLuint compileShader(GLenum type, const std::string& source);

    OpenGLRenderDevice* m_device = nullptr;
    GLuint m_program = 0;
    std::unordered_map<std::string, GLint> m_uniformLocations;
};

} // namespace placeholder::renderer
