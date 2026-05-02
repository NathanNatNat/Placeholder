#pragma once

#include "wowlib/data_buffer.h"

#include <cstdint>
#include <vector>

namespace wowlib
{

/// BLP texture format loader. Decodes WoW's proprietary BLP2 texture format
/// into raw RGBA pixel data suitable for GPU upload.
class BlpImage
{
public:
    explicit BlpImage(DataBuffer data);

    /// Get decoded RGBA pixel data for a given mipmap level.
    std::vector<uint8_t> toRGBA(int mipmap = 0) const;

    uint32_t width() const { return m_width; }
    uint32_t height() const { return m_height; }
    uint8_t encoding() const { return m_encoding; }
    int mipmapCount() const { return m_mipmapCount; }

private:
    void decodePalette(int mipmap, std::vector<uint8_t>& out) const;
    void decodeDXT(int mipmap, std::vector<uint8_t>& out) const;
    void decodeBGRA(int mipmap, std::vector<uint8_t>& out) const;

    DataBuffer m_data;
    uint8_t m_encoding = 0;
    uint8_t m_alphaDepth = 0;
    uint8_t m_alphaEncoding = 0;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    int m_mipmapCount = 0;
    std::vector<uint32_t> m_mipmapOffsets;
    std::vector<uint32_t> m_mipmapSizes;
    std::vector<uint8_t> m_palette;
};

} // namespace wowlib
