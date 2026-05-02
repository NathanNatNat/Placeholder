#include "renderer/texture.h"
#include "renderer/opengl_render_device.h"

#include <utility>

namespace placeholder::renderer
{

Texture::~Texture()
{
    if (m_texture != 0)
    {
        glDeleteTextures(1, &m_texture);
    }
}

Texture::Texture(Texture&& other) noexcept
    : m_texture(other.m_texture)
    , m_target(other.m_target)
    , m_width(other.m_width)
    , m_height(other.m_height)
    , m_format(other.m_format)
{
    other.m_texture = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept
{
    if (this != &other)
    {
        if (m_texture != 0)
        {
            glDeleteTextures(1, &m_texture);
        }
        m_texture = other.m_texture;
        m_target = other.m_target;
        m_width = other.m_width;
        m_height = other.m_height;
        m_format = other.m_format;
        other.m_texture = 0;
    }
    return *this;
}

Texture Texture::create2D(OpenGLRenderDevice& device, const TextureDesc& desc,
                           const void* pixels)
{
    Texture tex;
    tex.m_target = GL_TEXTURE_2D;
    tex.m_width = desc.width;
    tex.m_height = desc.height;
    tex.m_format = desc.format;

    glGenTextures(1, &tex.m_texture);
    device.bindTexture(0, tex.m_texture, GL_TEXTURE_2D);

    GLenum internalFmt = toGlInternalFormat(desc.format);
    GLenum pixelFmt = toGlFormat(desc.format);

    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internalFmt),
                 desc.width, desc.height, 0, pixelFmt, GL_UNSIGNED_BYTE, pixels);

    if (desc.generateMipmaps && pixels)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, toGlWrap(desc.wrapS));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, toGlWrap(desc.wrapT));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, toGlFilter(desc.minFilter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, toGlFilter(desc.magFilter));

    float aniso = desc.anisotropy;
    if (aniso <= 0.0f)
    {
        aniso = device.maxAnisotropy();
    }
    if (aniso > 1.0f && device.hasExtension("GL_EXT_texture_filter_anisotropic"))
    {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
    }

    return tex;
}

Texture Texture::createCubemap(OpenGLRenderDevice& device, const TextureDesc& desc,
                                const void* facePixels[6])
{
    Texture tex;
    tex.m_target = GL_TEXTURE_CUBE_MAP;
    tex.m_width = desc.width;
    tex.m_height = desc.height;
    tex.m_format = desc.format;

    glGenTextures(1, &tex.m_texture);
    device.bindTexture(0, tex.m_texture, GL_TEXTURE_CUBE_MAP);

    GLenum internalFmt = toGlInternalFormat(desc.format);
    GLenum pixelFmt = toGlFormat(desc.format);

    for (int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
                     static_cast<GLint>(internalFmt),
                     desc.width, desc.height, 0,
                     pixelFmt, GL_UNSIGNED_BYTE, facePixels[i]);
    }

    if (desc.generateMipmaps)
    {
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                    toGlFilter(desc.minFilter));
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER,
                    toGlFilter(desc.magFilter));

    return tex;
}

void Texture::bind(OpenGLRenderDevice& device, int unit) const
{
    device.bindTexture(unit, m_texture, m_target);
}

GLenum Texture::toGlInternalFormat(TextureFormat fmt)
{
    switch (fmt)
    {
        case TextureFormat::R8:      return GL_R8;
        case TextureFormat::RG8:     return GL_RG8;
        case TextureFormat::RGB8:    return GL_RGB8;
        case TextureFormat::RGBA8:   return GL_RGBA8;
        case TextureFormat::SRGB8:   return GL_SRGB8;
        case TextureFormat::SRGBA8:  return GL_SRGB8_ALPHA8;
        case TextureFormat::DXT1:    return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        case TextureFormat::DXT3:    return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        case TextureFormat::DXT5:    return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
    }
    return GL_RGBA8;
}

GLenum Texture::toGlFormat(TextureFormat fmt)
{
    switch (fmt)
    {
        case TextureFormat::R8:      return GL_RED;
        case TextureFormat::RG8:     return GL_RG;
        case TextureFormat::RGB8:    return GL_RGB;
        case TextureFormat::SRGB8:   return GL_RGB;
        case TextureFormat::RGBA8:   return GL_RGBA;
        case TextureFormat::SRGBA8:  return GL_RGBA;
        case TextureFormat::DXT1:    return GL_RGBA;
        case TextureFormat::DXT3:    return GL_RGBA;
        case TextureFormat::DXT5:    return GL_RGBA;
    }
    return GL_RGBA;
}

GLenum Texture::toGlWrap(TextureWrap wrap)
{
    switch (wrap)
    {
        case TextureWrap::Repeat:         return GL_REPEAT;
        case TextureWrap::ClampToEdge:    return GL_CLAMP_TO_EDGE;
        case TextureWrap::MirroredRepeat: return GL_MIRRORED_REPEAT;
    }
    return GL_REPEAT;
}

GLenum Texture::toGlFilter(TextureFilter filter)
{
    switch (filter)
    {
        case TextureFilter::Nearest:               return GL_NEAREST;
        case TextureFilter::Linear:                return GL_LINEAR;
        case TextureFilter::NearestMipmapNearest:  return GL_NEAREST_MIPMAP_NEAREST;
        case TextureFilter::LinearMipmapNearest:   return GL_LINEAR_MIPMAP_NEAREST;
        case TextureFilter::NearestMipmapLinear:   return GL_NEAREST_MIPMAP_LINEAR;
        case TextureFilter::LinearMipmapLinear:    return GL_LINEAR_MIPMAP_LINEAR;
    }
    return GL_LINEAR;
}

} // namespace placeholder::renderer
