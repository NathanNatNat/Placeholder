#include "renderer/texture_loader.h"
#include "renderer/opengl_render_device.h"

#include "core/logging.h"
#include "core/file_io.h"

#include <stb_image.h>
#include <webp/decode.h>

#include <algorithm>

namespace placeholder::renderer
{

namespace
{

bool hasExtension(const std::string& path, const std::string& ext)
{
    if (path.size() < ext.size())
    {
        return false;
    }
    auto pathExt = path.substr(path.size() - ext.size());
    std::string lower;
    lower.resize(pathExt.size());
    std::transform(pathExt.begin(), pathExt.end(), lower.begin(), ::tolower);
    return lower == ext;
}

} // namespace

TextureLoader::TextureLoader(OpenGLRenderDevice& device)
    : m_device(device)
{
}

std::unique_ptr<Texture> TextureLoader::loadFromFile(const std::string& path, bool srgb)
{
    auto logger = core::getLogger("renderer");

    if (hasExtension(path, ".webp"))
    {
        return loadWebP(path, srgb);
    }

    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* pixels = stbi_load(path.c_str(), &width, &height, &channels, 0);

    if (!pixels)
    {
        logger->error("Failed to load texture '{}': {}", path, stbi_failure_reason());
        return nullptr;
    }

    TextureDesc desc;
    desc.width = width;
    desc.height = height;

    if (channels == 1)
    {
        desc.format = TextureFormat::R8;
    }
    else if (channels == 2)
    {
        desc.format = TextureFormat::RG8;
    }
    else if (channels == 3)
    {
        desc.format = srgb ? TextureFormat::SRGB8 : TextureFormat::RGB8;
    }
    else
    {
        desc.format = srgb ? TextureFormat::SRGBA8 : TextureFormat::RGBA8;
    }

    auto texture = std::make_unique<Texture>(
        Texture::create2D(m_device, desc, pixels));

    stbi_image_free(pixels);

    logger->debug("Loaded texture '{}' ({}x{}, {} channels)", path, width, height, channels);
    return texture;
}

std::unique_ptr<Texture> TextureLoader::createFromData(const TextureDesc& desc,
                                                        const void* pixels)
{
    return std::make_unique<Texture>(Texture::create2D(m_device, desc, pixels));
}

std::unique_ptr<Texture> TextureLoader::loadCubemap(
    const std::array<std::string, 6>& facePaths, bool srgb)
{
    auto logger = core::getLogger("renderer");

    int width = 0, height = 0;
    unsigned char* faceData[6] = {};

    stbi_set_flip_vertically_on_load(false);

    for (int i = 0; i < 6; ++i)
    {
        int w, h, channels;
        faceData[i] = stbi_load(facePaths[i].c_str(), &w, &h, &channels, 4);

        if (!faceData[i])
        {
            logger->error("Failed to load cubemap face '{}': {}",
                          facePaths[i], stbi_failure_reason());
            for (int j = 0; j < i; ++j)
            {
                stbi_image_free(faceData[j]);
            }
            return nullptr;
        }

        if (i == 0)
        {
            width = w;
            height = h;
        }
        else if (w != width || h != height)
        {
            logger->error("Cubemap face '{}' size {}x{} differs from first face {}x{}",
                          facePaths[i], w, h, width, height);
            for (int j = 0; j <= i; ++j)
            {
                stbi_image_free(faceData[j]);
            }
            return nullptr;
        }
    }

    TextureDesc desc;
    desc.width = width;
    desc.height = height;
    desc.format = srgb ? TextureFormat::SRGBA8 : TextureFormat::RGBA8;

    const void* facePixels[6];
    for (int i = 0; i < 6; ++i)
    {
        facePixels[i] = faceData[i];
    }

    auto texture = std::make_unique<Texture>(
        Texture::createCubemap(m_device, desc, facePixels));

    for (int i = 0; i < 6; ++i)
    {
        stbi_image_free(faceData[i]);
    }

    logger->debug("Loaded cubemap ({}x{}) from 6 faces", width, height);
    return texture;
}

std::unique_ptr<Texture> TextureLoader::createWhiteTexture()
{
    uint32_t white = 0xFFFFFFFF;
    TextureDesc desc;
    desc.width = 1;
    desc.height = 1;
    desc.format = TextureFormat::RGBA8;
    desc.generateMipmaps = false;
    desc.minFilter = TextureFilter::Nearest;
    desc.magFilter = TextureFilter::Nearest;
    return std::make_unique<Texture>(Texture::create2D(m_device, desc, &white));
}

std::unique_ptr<Texture> TextureLoader::loadWebP(const std::string& path, bool srgb)
{
    auto logger = core::getLogger("renderer");

    auto fileData = core::readFileBinary(path);
    if (!fileData)
    {
        logger->error("Failed to read WebP file '{}'", path);
        return nullptr;
    }

    int width, height;
    if (!WebPGetInfo(fileData->data(), fileData->size(), &width, &height))
    {
        logger->error("Invalid WebP file '{}'", path);
        return nullptr;
    }

    uint8_t* pixels = WebPDecodeRGBA(fileData->data(), fileData->size(), &width, &height);
    if (!pixels)
    {
        logger->error("Failed to decode WebP file '{}'", path);
        return nullptr;
    }

    TextureDesc desc;
    desc.width = width;
    desc.height = height;
    desc.format = srgb ? TextureFormat::SRGBA8 : TextureFormat::RGBA8;

    auto texture = std::make_unique<Texture>(
        Texture::create2D(m_device, desc, pixels));

    WebPFree(pixels);

    logger->debug("Loaded WebP texture '{}' ({}x{})", path, width, height);
    return texture;
}

} // namespace placeholder::renderer
