#pragma once

#include <glad/gl.h>

namespace placeholder::renderer
{

/// RAII framebuffer object with color texture and depth renderbuffer.
///
/// The color attachment is a GL_RGBA8 texture suitable for display
/// (e.g. via ImGui::Image in an editor viewport panel).
class Framebuffer
{
public:
    Framebuffer();
    ~Framebuffer();

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    /// Create or recreate the framebuffer at the given resolution.
    /// No-op if the size matches the current allocation.
    void resize(int width, int height);

    /// Release all GPU resources.
    void destroy();

    /// Bind this framebuffer as the current render target.
    void bind();

    /// Restore the default framebuffer (backbuffer).
    static void unbind();

    GLuint colorTexture() const { return m_colorTexture; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    bool valid() const { return m_fbo != 0; }

private:
    GLuint m_fbo = 0;
    GLuint m_colorTexture = 0;
    GLuint m_depthRenderbuffer = 0;
    int m_width = 0;
    int m_height = 0;
};

} // namespace placeholder::renderer
