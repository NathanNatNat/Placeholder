#include "renderer/opengl_render_device.h"

#include "core/logging.h"

#include <stdexcept>

namespace placeholder::renderer
{

OpenGLRenderDevice::OpenGLRenderDevice()
{
    auto logger = core::getLogger("renderer");
    logger->info("Initializing OpenGL render device");

    initExtensions();
    initDefaultState();

    logger->info("OpenGL render device ready");
}

void OpenGLRenderDevice::initExtensions()
{
    auto logger = core::getLogger("renderer");

    GLint numExtensions = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

    for (GLint i = 0; i < numExtensions; ++i)
    {
        const char* ext = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
        if (!ext)
        {
            continue;
        }

        std::string name(ext);

        if (name == "GL_EXT_texture_compression_s3tc" ||
            name == "GL_S3_s3tc")
        {
            m_extS3tc = true;
        }
        else if (name == "GL_EXT_texture_filter_anisotropic" ||
                 name == "GL_ARB_texture_filter_anisotropic")
        {
            m_extAniso = true;
        }
    }

    if (m_extAniso)
    {
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &m_maxAnisotropy);
    }

    logger->info("Extensions: S3TC={}, Aniso={} (max={:.1f})",
                 m_extS3tc, m_extAniso, m_maxAnisotropy);
}

void OpenGLRenderDevice::initDefaultState()
{
    glEnable(GL_DEPTH_TEST);
    m_depthTest = true;

    glDepthMask(GL_TRUE);
    m_depthWrite = true;

    glDepthFunc(GL_LEQUAL);
    m_depthFunc = GL_LEQUAL;

    glDisable(GL_CULL_FACE);
    m_cullEnabled = false;
    m_cullMode = GL_BACK;

    glDisable(GL_BLEND);
    m_blendEnabled = false;
    m_blendSrcRgb = GL_ONE;
    m_blendDstRgb = GL_ZERO;
    m_blendSrcAlpha = GL_ONE;
    m_blendDstAlpha = GL_ZERO;

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    m_polygonMode = GL_FILL;
}

void OpenGLRenderDevice::beginFrame(const FrameContext& /*ctx*/)
{
}

void OpenGLRenderDevice::endFrame()
{
}

void OpenGLRenderDevice::setViewport(int x, int y, int width, int height)
{
    if (x != m_viewportX || y != m_viewportY ||
        width != m_viewportWidth || height != m_viewportHeight)
    {
        glViewport(x, y, width, height);
        m_viewportX = x;
        m_viewportY = y;
        m_viewportWidth = width;
        m_viewportHeight = height;
    }
}

void OpenGLRenderDevice::setClearColor(float r, float g, float b, float a)
{
    glClearColor(r, g, b, a);
}

void OpenGLRenderDevice::clear(bool color, bool depth, bool stencil)
{
    GLbitfield mask = 0;
    if (color) mask |= GL_COLOR_BUFFER_BIT;
    if (depth) mask |= GL_DEPTH_BUFFER_BIT;
    if (stencil) mask |= GL_STENCIL_BUFFER_BIT;

    if (depth && !m_depthWrite)
    {
        glDepthMask(GL_TRUE);
        glClear(mask);
        glDepthMask(GL_FALSE);
    }
    else
    {
        glClear(mask);
    }
}

void OpenGLRenderDevice::setDepthTest(bool enabled)
{
    if (enabled != m_depthTest)
    {
        if (enabled)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);
        m_depthTest = enabled;
    }
}

void OpenGLRenderDevice::setDepthWrite(bool enabled)
{
    if (enabled != m_depthWrite)
    {
        glDepthMask(enabled ? GL_TRUE : GL_FALSE);
        m_depthWrite = enabled;
    }
}

void OpenGLRenderDevice::setDepthFunc(DepthFunc func)
{
    GLenum glFunc = toGlDepthFunc(func);
    if (glFunc != m_depthFunc)
    {
        glDepthFunc(glFunc);
        m_depthFunc = glFunc;
    }
}

void OpenGLRenderDevice::setCullMode(CullMode mode)
{
    if (mode == CullMode::None)
    {
        if (m_cullEnabled)
        {
            glDisable(GL_CULL_FACE);
            m_cullEnabled = false;
        }
    }
    else
    {
        if (!m_cullEnabled)
        {
            glEnable(GL_CULL_FACE);
            m_cullEnabled = true;
        }

        GLenum glMode = (mode == CullMode::Back) ? GL_BACK : GL_FRONT;
        if (glMode != m_cullMode)
        {
            glCullFace(glMode);
            m_cullMode = glMode;
        }
    }
}

void OpenGLRenderDevice::setBlendEnabled(bool enabled)
{
    if (enabled != m_blendEnabled)
    {
        if (enabled)
            glEnable(GL_BLEND);
        else
            glDisable(GL_BLEND);
        m_blendEnabled = enabled;
    }
}

void OpenGLRenderDevice::setBlendFunc(BlendFactor src, BlendFactor dst)
{
    GLenum glSrc = toGlBlendFactor(src);
    GLenum glDst = toGlBlendFactor(dst);

    if (glSrc != m_blendSrcRgb || glDst != m_blendDstRgb ||
        glSrc != m_blendSrcAlpha || glDst != m_blendDstAlpha)
    {
        glBlendFunc(glSrc, glDst);
        m_blendSrcRgb = glSrc;
        m_blendDstRgb = glDst;
        m_blendSrcAlpha = glSrc;
        m_blendDstAlpha = glDst;
    }
}

void OpenGLRenderDevice::setBlendFuncSeparate(BlendFactor srcRgb, BlendFactor dstRgb,
                                               BlendFactor srcAlpha, BlendFactor dstAlpha)
{
    GLenum glSrcRgb = toGlBlendFactor(srcRgb);
    GLenum glDstRgb = toGlBlendFactor(dstRgb);
    GLenum glSrcAlpha = toGlBlendFactor(srcAlpha);
    GLenum glDstAlpha = toGlBlendFactor(dstAlpha);

    if (glSrcRgb != m_blendSrcRgb || glDstRgb != m_blendDstRgb ||
        glSrcAlpha != m_blendSrcAlpha || glDstAlpha != m_blendDstAlpha)
    {
        glBlendFuncSeparate(glSrcRgb, glDstRgb, glSrcAlpha, glDstAlpha);
        m_blendSrcRgb = glSrcRgb;
        m_blendDstRgb = glDstRgb;
        m_blendSrcAlpha = glSrcAlpha;
        m_blendDstAlpha = glDstAlpha;
    }
}

void OpenGLRenderDevice::setPolygonMode(PolygonMode mode)
{
    GLenum glMode = toGlPolygonMode(mode);
    if (glMode != m_polygonMode)
    {
        glPolygonMode(GL_FRONT_AND_BACK, glMode);
        m_polygonMode = glMode;
    }
}

void OpenGLRenderDevice::invalidateStateCache()
{
    m_depthTest = false;
    m_depthWrite = true;
    m_depthFunc = GL_LEQUAL;
    m_cullEnabled = false;
    m_cullMode = GL_BACK;
    m_blendEnabled = false;
    m_blendSrcRgb = GL_ONE;
    m_blendDstRgb = GL_ZERO;
    m_blendSrcAlpha = GL_ONE;
    m_blendDstAlpha = GL_ZERO;
    m_polygonMode = GL_FILL;
    m_currentProgram = 0;
    m_currentVao = 0;
    m_boundTextures = {};
    m_activeTextureUnit = 0;
    m_viewportX = -1;
    m_viewportY = -1;
    m_viewportWidth = -1;
    m_viewportHeight = -1;
}

void OpenGLRenderDevice::drawArrays(PrimitiveTopology topology, int first, int count)
{
    glDrawArrays(toGlTopology(topology), first, count);
}

void OpenGLRenderDevice::drawElements(PrimitiveTopology topology, int count,
                                       IndexType indexType, size_t offset)
{
    glDrawElements(toGlTopology(topology), count, toGlIndexType(indexType),
                   reinterpret_cast<const void*>(offset));
}

bool OpenGLRenderDevice::hasExtension(const std::string& name) const
{
    if (name == "S3TC") return m_extS3tc;
    if (name == "anisotropic") return m_extAniso;
    return false;
}

float OpenGLRenderDevice::maxAnisotropy() const
{
    return m_maxAnisotropy;
}

void OpenGLRenderDevice::bindVao(GLuint vao)
{
    if (vao != m_currentVao)
    {
        glBindVertexArray(vao);
        m_currentVao = vao;
    }
}

void OpenGLRenderDevice::unbindVao(GLuint vao)
{
    if (vao == m_currentVao)
    {
        m_currentVao = 0;
    }
}

void OpenGLRenderDevice::useProgram(GLuint program)
{
    if (program != m_currentProgram)
    {
        glUseProgram(program);
        m_currentProgram = program;
    }
}

void OpenGLRenderDevice::activeTexture(int unit)
{
    if (unit != m_activeTextureUnit)
    {
        glActiveTexture(GL_TEXTURE0 + unit);
        m_activeTextureUnit = unit;
    }
}

void OpenGLRenderDevice::bindTexture(int unit, GLuint texture, GLenum target)
{
    if (unit < 0 || unit >= static_cast<int>(m_boundTextures.size()))
    {
        return;
    }

    if (texture != m_boundTextures[unit])
    {
        activeTexture(unit);
        glBindTexture(target, texture);
        m_boundTextures[unit] = texture;
    }
}

GLenum OpenGLRenderDevice::toGlDepthFunc(DepthFunc func)
{
    switch (func)
    {
        case DepthFunc::Less:        return GL_LESS;
        case DepthFunc::LessEqual:   return GL_LEQUAL;
        case DepthFunc::Equal:       return GL_EQUAL;
        case DepthFunc::Greater:     return GL_GREATER;
        case DepthFunc::GreaterEqual:return GL_GEQUAL;
        case DepthFunc::Always:      return GL_ALWAYS;
        case DepthFunc::Never:       return GL_NEVER;
    }
    return GL_LEQUAL;
}

GLenum OpenGLRenderDevice::toGlBlendFactor(BlendFactor factor)
{
    switch (factor)
    {
        case BlendFactor::Zero:                  return GL_ZERO;
        case BlendFactor::One:                   return GL_ONE;
        case BlendFactor::SrcColor:              return GL_SRC_COLOR;
        case BlendFactor::OneMinusSrcColor:      return GL_ONE_MINUS_SRC_COLOR;
        case BlendFactor::DstColor:              return GL_DST_COLOR;
        case BlendFactor::OneMinusDstColor:      return GL_ONE_MINUS_DST_COLOR;
        case BlendFactor::SrcAlpha:              return GL_SRC_ALPHA;
        case BlendFactor::OneMinusSrcAlpha:      return GL_ONE_MINUS_SRC_ALPHA;
        case BlendFactor::DstAlpha:              return GL_DST_ALPHA;
        case BlendFactor::OneMinusDstAlpha:      return GL_ONE_MINUS_DST_ALPHA;
        case BlendFactor::ConstantAlpha:         return GL_CONSTANT_ALPHA;
        case BlendFactor::OneMinusConstantAlpha: return GL_ONE_MINUS_CONSTANT_ALPHA;
    }
    return GL_ONE;
}

GLenum OpenGLRenderDevice::toGlTopology(PrimitiveTopology topology)
{
    switch (topology)
    {
        case PrimitiveTopology::Triangles: return GL_TRIANGLES;
        case PrimitiveTopology::Lines:     return GL_LINES;
        case PrimitiveTopology::Points:    return GL_POINTS;
    }
    return GL_TRIANGLES;
}

GLenum OpenGLRenderDevice::toGlIndexType(IndexType type)
{
    switch (type)
    {
        case IndexType::UInt16: return GL_UNSIGNED_SHORT;
        case IndexType::UInt32: return GL_UNSIGNED_INT;
    }
    return GL_UNSIGNED_SHORT;
}

GLenum OpenGLRenderDevice::toGlPolygonMode(PolygonMode mode)
{
    switch (mode)
    {
        case PolygonMode::Fill:  return GL_FILL;
        case PolygonMode::Line:  return GL_LINE;
        case PolygonMode::Point: return GL_POINT;
    }
    return GL_FILL;
}

} // namespace placeholder::renderer
