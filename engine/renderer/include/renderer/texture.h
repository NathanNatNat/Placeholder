#pragma once

#include <glad/gl.h>

#include <cstdint>

namespace placeholder::renderer
{

class OpenGLRenderDevice;

/// Pixel format for texture data.
enum class TextureFormat
{
    R8,
    RG8,
    RGB8,
    RGBA8,
    SRGB8,
    SRGBA8,
    DXT1,
    DXT3,
    DXT5,
};

/// Texture coordinate wrapping mode.
enum class TextureWrap
{
    Repeat,
    ClampToEdge,
    MirroredRepeat,
};

/// Texture sampling filter.
enum class TextureFilter
{
    Nearest,
    Linear,
    NearestMipmapNearest,
    LinearMipmapNearest,
    NearestMipmapLinear,
    LinearMipmapLinear,
};

/// Describes how to create a texture on the GPU.
struct TextureDesc
{
    int width = 0;
    int height = 0;
    TextureFormat format = TextureFormat::RGBA8;
    TextureFilter minFilter = TextureFilter::LinearMipmapLinear;
    TextureFilter magFilter = TextureFilter::Linear;
    TextureWrap wrapS = TextureWrap::Repeat;
    TextureWrap wrapT = TextureWrap::Repeat;
    bool generateMipmaps = true;
    float anisotropy = 0.0f;
};

/// RAII wrapper around an OpenGL texture (2D or cubemap).
///
/// Default-constructable (creates a null texture). Move-only.
class Texture
{
public:
    Texture() = default;
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    /// Create a 2D texture and upload pixel data.
    static Texture create2D(OpenGLRenderDevice& device, const TextureDesc& desc,
                            const void* pixels);

    /// Create a cubemap from 6 face images. Each face must be the same size.
    /// @param facePixels Array of 6 pixel data pointers: +X, -X, +Y, -Y, +Z, -Z.
    static Texture createCubemap(OpenGLRenderDevice& device, const TextureDesc& desc,
                                 const void* facePixels[6]);

    /// Bind this texture to a texture unit.
    void bind(OpenGLRenderDevice& device, int unit) const;

    GLuint handle() const { return m_texture; }
    GLenum target() const { return m_target; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    TextureFormat format() const { return m_format; }
    bool valid() const { return m_texture != 0; }

private:
    static GLenum toGlInternalFormat(TextureFormat fmt);
    static GLenum toGlFormat(TextureFormat fmt);
    static GLenum toGlWrap(TextureWrap wrap);
    static GLenum toGlFilter(TextureFilter filter);

    GLuint m_texture = 0;
    GLenum m_target = GL_TEXTURE_2D;
    int m_width = 0;
    int m_height = 0;
    TextureFormat m_format = TextureFormat::RGBA8;
};

} // namespace placeholder::renderer
