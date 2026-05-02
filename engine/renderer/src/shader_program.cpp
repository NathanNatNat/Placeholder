#include "renderer/shader_program.h"
#include "renderer/opengl_render_device.h"

#include "core/logging.h"

#include <stdexcept>
#include <vector>

namespace placeholder::renderer
{

ShaderProgram::ShaderProgram(OpenGLRenderDevice& device,
                             const std::string& vertexSource,
                             const std::string& fragmentSource)
    : m_device(&device)
{
    compile(vertexSource, fragmentSource);
}

ShaderProgram::~ShaderProgram()
{
    if (m_program != 0)
    {
        glDeleteProgram(m_program);
    }
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept
    : m_device(other.m_device)
    , m_program(other.m_program)
    , m_uniformLocations(std::move(other.m_uniformLocations))
{
    other.m_program = 0;
    other.m_device = nullptr;
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept
{
    if (this != &other)
    {
        if (m_program != 0)
        {
            glDeleteProgram(m_program);
        }
        m_device = other.m_device;
        m_program = other.m_program;
        m_uniformLocations = std::move(other.m_uniformLocations);
        other.m_program = 0;
        other.m_device = nullptr;
    }
    return *this;
}

bool ShaderProgram::isValid() const
{
    return m_program != 0;
}

void ShaderProgram::bind()
{
    if (m_device && m_program != 0)
    {
        m_device->useProgram(m_program);
    }
}

GLint ShaderProgram::getUniformLocation(const std::string& name)
{
    auto it = m_uniformLocations.find(name);
    if (it != m_uniformLocations.end())
    {
        return it->second;
    }

    GLint location = glGetUniformLocation(m_program, name.c_str());
    m_uniformLocations[name] = location;
    return location;
}

void ShaderProgram::setUniform(const std::string& name, int value)
{
    GLint loc = getUniformLocation(name);
    if (loc >= 0) glUniform1i(loc, value);
}

void ShaderProgram::setUniform(const std::string& name, float value)
{
    GLint loc = getUniformLocation(name);
    if (loc >= 0) glUniform1f(loc, value);
}

void ShaderProgram::setUniform(const std::string& name, float x, float y)
{
    GLint loc = getUniformLocation(name);
    if (loc >= 0) glUniform2f(loc, x, y);
}

void ShaderProgram::setUniform(const std::string& name, float x, float y, float z)
{
    GLint loc = getUniformLocation(name);
    if (loc >= 0) glUniform3f(loc, x, y, z);
}

void ShaderProgram::setUniform(const std::string& name, float x, float y, float z, float w)
{
    GLint loc = getUniformLocation(name);
    if (loc >= 0) glUniform4f(loc, x, y, z, w);
}

void ShaderProgram::setUniformVec3(const std::string& name, const float* value, int count)
{
    GLint loc = getUniformLocation(name);
    if (loc >= 0) glUniform3fv(loc, count, value);
}

void ShaderProgram::setUniformVec4(const std::string& name, const float* value, int count)
{
    GLint loc = getUniformLocation(name);
    if (loc >= 0) glUniform4fv(loc, count, value);
}

void ShaderProgram::setUniformMat3(const std::string& name, const float* value, bool transpose)
{
    GLint loc = getUniformLocation(name);
    if (loc >= 0) glUniformMatrix3fv(loc, 1, transpose ? GL_TRUE : GL_FALSE, value);
}

void ShaderProgram::setUniformMat4(const std::string& name, const float* value, bool transpose)
{
    GLint loc = getUniformLocation(name);
    if (loc >= 0) glUniformMatrix4fv(loc, 1, transpose ? GL_TRUE : GL_FALSE, value);
}

void ShaderProgram::setUniformMat4Array(const std::string& name, const float* value,
                                         int count, bool transpose)
{
    GLint loc = getUniformLocation(name);
    if (loc >= 0) glUniformMatrix4fv(loc, count, transpose ? GL_TRUE : GL_FALSE, value);
}

void ShaderProgram::bindUniformBlock(const std::string& name, uint32_t bindingPoint)
{
    GLuint index = glGetUniformBlockIndex(m_program, name.c_str());
    if (index != GL_INVALID_INDEX)
    {
        glUniformBlockBinding(m_program, index, bindingPoint);
    }
}

bool ShaderProgram::recompile(const std::string& vertexSource, const std::string& fragmentSource)
{
    auto logger = core::getLogger("renderer");

    GLuint vertShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    if (vertShader == 0)
    {
        return false;
    }

    GLuint fragShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (fragShader == 0)
    {
        glDeleteShader(vertShader);
        return false;
    }

    GLuint newProgram = glCreateProgram();
    glAttachShader(newProgram, vertShader);
    glAttachShader(newProgram, fragShader);
    glLinkProgram(newProgram);

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    GLint linkStatus = 0;
    glGetProgramiv(newProgram, GL_LINK_STATUS, &linkStatus);
    if (linkStatus == GL_FALSE)
    {
        GLint logLength = 0;
        glGetProgramiv(newProgram, GL_INFO_LOG_LENGTH, &logLength);
        std::string infoLog(logLength, '\0');
        glGetProgramInfoLog(newProgram, logLength, nullptr, infoLog.data());
        logger->error("Shader recompile link failed: {}", infoLog);
        glDeleteProgram(newProgram);
        return false;
    }

    glDeleteProgram(m_program);
    m_program = newProgram;
    m_uniformLocations.clear();

    logger->info("Shader recompiled successfully");
    return true;
}

void ShaderProgram::compile(const std::string& vertexSource, const std::string& fragmentSource)
{
    auto logger = core::getLogger("renderer");

    GLuint vertShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    if (vertShader == 0)
    {
        throw std::runtime_error("Vertex shader compilation failed");
    }

    GLuint fragShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (fragShader == 0)
    {
        glDeleteShader(vertShader);
        throw std::runtime_error("Fragment shader compilation failed");
    }

    m_program = glCreateProgram();
    glAttachShader(m_program, vertShader);
    glAttachShader(m_program, fragShader);
    glLinkProgram(m_program);

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    GLint linkStatus = 0;
    glGetProgramiv(m_program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus == GL_FALSE)
    {
        GLint logLength = 0;
        glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &logLength);
        std::string infoLog(logLength, '\0');
        glGetProgramInfoLog(m_program, logLength, nullptr, infoLog.data());
        logger->error("Shader link failed: {}", infoLog);
        glDeleteProgram(m_program);
        m_program = 0;
        throw std::runtime_error("Shader program link failed: " + infoLog);
    }
}

GLuint ShaderProgram::compileShader(GLenum type, const std::string& source)
{
    auto logger = core::getLogger("renderer");

    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint compileStatus = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
    if (compileStatus == GL_FALSE)
    {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::string infoLog(logLength, '\0');
        glGetShaderInfoLog(shader, logLength, nullptr, infoLog.data());

        const char* typeName = (type == GL_VERTEX_SHADER) ? "vertex" : "fragment";
        logger->error("{} shader compilation failed: {}", typeName, infoLog);

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

} // namespace placeholder::renderer
