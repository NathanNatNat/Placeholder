#pragma once

#include "texture.h"

#include <array>
#include <memory>
#include <string>

namespace placeholder::renderer
{

class OpenGLRenderDevice;

/// Loads textures from disk into GPU-ready Texture objects.
///
/// Supports PNG, JPG, BMP, TGA (via stb_image), and can create
/// textures from raw pixel data.
class TextureLoader
{
public:
    explicit TextureLoader(OpenGLRenderDevice& device);

    /// Load a 2D texture from a file. Returns null on failure.
    /// @param srgb If true, uploads as SRGB for correct gamma handling.
    std::unique_ptr<Texture> loadFromFile(const std::string& path, bool srgb = true);

    /// Create a 2D texture from raw pixel data.
    std::unique_ptr<Texture> createFromData(const TextureDesc& desc, const void* pixels);

    /// Load a cubemap from 6 face images (+X, -X, +Y, -Y, +Z, -Z).
    std::unique_ptr<Texture> loadCubemap(const std::array<std::string, 6>& facePaths,
                                         bool srgb = true);

    /// Create a 1x1 white texture (placeholder for missing textures).
    std::unique_ptr<Texture> createWhiteTexture();

private:
    std::unique_ptr<Texture> loadWebP(const std::string& path, bool srgb);

    OpenGLRenderDevice& m_device;
};

} // namespace placeholder::renderer
