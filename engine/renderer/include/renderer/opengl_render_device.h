#pragma once

#include "render_device.h"

#include <glad/gl.h>

#include <array>

namespace placeholder::renderer
{

/// OpenGL 4.6 Core implementation of the RenderDevice interface.
///
/// All GL state changes are cached to avoid redundant API calls.
class OpenGLRenderDevice final : public RenderDevice
{
public:
    OpenGLRenderDevice();

    void beginFrame(const FrameContext& ctx) override;
    void endFrame() override;

    void setViewport(int x, int y, int width, int height) override;
    void setClearColor(float r, float g, float b, float a) override;
    void clear(bool color, bool depth, bool stencil) override;

    void setDepthTest(bool enabled) override;
    void setDepthWrite(bool enabled) override;
    void setDepthFunc(DepthFunc func) override;
    void setCullMode(CullMode mode) override;
    void setBlendEnabled(bool enabled) override;
    void setBlendFunc(BlendFactor src, BlendFactor dst) override;
    void setBlendFuncSeparate(BlendFactor srcRgb, BlendFactor dstRgb,
                              BlendFactor srcAlpha, BlendFactor dstAlpha) override;
    void setPolygonMode(PolygonMode mode) override;

    void invalidateStateCache() override;

    void drawArrays(PrimitiveTopology topology, int first, int count) override;
    void drawElements(PrimitiveTopology topology, int count,
                      IndexType indexType, size_t offset) override;

    bool hasExtension(const std::string& name) const override;
    float maxAnisotropy() const override;

    /// @name OpenGL-specific operations (used by VertexArray, ShaderProgram, etc.)
    /// @{
    void bindVao(GLuint vao);
    void unbindVao(GLuint vao);
    void useProgram(GLuint program);
    void activeTexture(int unit);
    void bindTexture(int unit, GLuint texture, GLenum target = GL_TEXTURE_2D);
    /// @}

private:
    void initExtensions();
    void initDefaultState();

    static GLenum toGlDepthFunc(DepthFunc func);
    static GLenum toGlBlendFactor(BlendFactor factor);
    static GLenum toGlTopology(PrimitiveTopology topology);
    static GLenum toGlIndexType(IndexType type);
    static GLenum toGlPolygonMode(PolygonMode mode);

    bool m_depthTest = false;
    bool m_depthWrite = true;
    GLenum m_depthFunc = GL_LEQUAL;
    bool m_cullEnabled = false;
    GLenum m_cullMode = GL_BACK;
    bool m_blendEnabled = false;
    GLenum m_blendSrcRgb = GL_ONE;
    GLenum m_blendDstRgb = GL_ZERO;
    GLenum m_blendSrcAlpha = GL_ONE;
    GLenum m_blendDstAlpha = GL_ZERO;
    GLenum m_polygonMode = GL_FILL;

    GLuint m_currentProgram = 0;
    GLuint m_currentVao = 0;
    std::array<GLuint, 16> m_boundTextures{};
    int m_activeTextureUnit = 0;

    int m_viewportX = 0;
    int m_viewportY = 0;
    int m_viewportWidth = 0;
    int m_viewportHeight = 0;

    bool m_extS3tc = false;
    bool m_extAniso = false;
    float m_maxAnisotropy = 1.0f;
};

} // namespace placeholder::renderer
