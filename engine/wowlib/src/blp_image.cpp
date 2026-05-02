#include "wowlib/blp_image.h"

#include <stdexcept>
#include <cstring>
#include <algorithm>

namespace wowlib
{

// BLP2 file magic
constexpr uint32_t BLP2_MAGIC = 0x32504c42; // "BLP2"

BlpImage::BlpImage(DataBuffer data)
    : m_data(std::move(data))
{
    m_data.seek(0);

    const uint32_t magic = m_data.readUInt32LE();
    if (magic != BLP2_MAGIC)
        throw std::runtime_error("Invalid BLP magic");

    const uint32_t type = m_data.readUInt32LE();
    if (type != 1)
        throw std::runtime_error("Unsupported BLP type: " + std::to_string(type));

    m_encoding = m_data.readUInt8();
    m_alphaDepth = m_data.readUInt8();
    m_alphaEncoding = m_data.readUInt8();
    m_data.readUInt8(); // hasMipmaps

    m_width = m_data.readUInt32LE();
    m_height = m_data.readUInt32LE();

    m_mipmapOffsets.resize(16);
    m_mipmapSizes.resize(16);
    for (int i = 0; i < 16; i++)
        m_mipmapOffsets[i] = m_data.readUInt32LE();
    for (int i = 0; i < 16; i++)
        m_mipmapSizes[i] = m_data.readUInt32LE();

    m_mipmapCount = 0;
    for (int i = 0; i < 16; i++)
    {
        if (m_mipmapSizes[i] > 0)
            m_mipmapCount = i + 1;
    }

    // Read palette for encoding type 1
    if (m_encoding == 1)
    {
        m_palette.resize(256 * 4);
        m_data.seek(148);
        for (int i = 0; i < 256; i++)
        {
            m_palette[i * 4 + 2] = m_data.readUInt8(); // B
            m_palette[i * 4 + 1] = m_data.readUInt8(); // G
            m_palette[i * 4 + 0] = m_data.readUInt8(); // R
            m_palette[i * 4 + 3] = m_data.readUInt8(); // A
        }
    }
}

std::vector<uint8_t> BlpImage::toRGBA(int mipmap) const
{
    if (mipmap >= m_mipmapCount || m_mipmapSizes[mipmap] == 0)
        throw std::runtime_error("Invalid mipmap level: " + std::to_string(mipmap));

    uint32_t w = std::max(1u, m_width >> mipmap);
    uint32_t h = std::max(1u, m_height >> mipmap);
    std::vector<uint8_t> out(w * h * 4, 255);

    switch (m_encoding)
    {
        case 1: decodePalette(mipmap, out); break;
        case 2: decodeDXT(mipmap, out); break;
        case 3: decodeBGRA(mipmap, out); break;
        default:
            throw std::runtime_error("Unsupported BLP encoding: " + std::to_string(m_encoding));
    }

    return out;
}

void BlpImage::decodePalette(int mipmap, std::vector<uint8_t>& out) const
{
    uint32_t w = std::max(1u, m_width >> mipmap);
    uint32_t h = std::max(1u, m_height >> mipmap);
    uint32_t pixelCount = w * h;

    DataBuffer buf = DataBuffer::from(m_data.raw());
    buf.seek(m_mipmapOffsets[mipmap]);

    std::vector<uint8_t> indices(pixelCount);
    for (uint32_t i = 0; i < pixelCount; i++)
        indices[i] = buf.readUInt8();

    for (uint32_t i = 0; i < pixelCount; i++)
    {
        uint8_t idx = indices[i];
        out[i * 4 + 0] = m_palette[idx * 4 + 0]; // R
        out[i * 4 + 1] = m_palette[idx * 4 + 1]; // G
        out[i * 4 + 2] = m_palette[idx * 4 + 2]; // B
        out[i * 4 + 3] = (m_alphaDepth > 0) ? buf.readUInt8() : 255;
    }
}

namespace
{

void decodeDXT1Block(const uint8_t* block, uint8_t* out, int outStride)
{
    uint16_t c0 = block[0] | (block[1] << 8);
    uint16_t c1 = block[2] | (block[3] << 8);

    uint8_t colors[4][4];

    auto unpack565 = [](uint16_t c, uint8_t* rgb)
    {
        rgb[0] = static_cast<uint8_t>(((c >> 11) & 0x1F) * 255 / 31);
        rgb[1] = static_cast<uint8_t>(((c >> 5) & 0x3F) * 255 / 63);
        rgb[2] = static_cast<uint8_t>((c & 0x1F) * 255 / 31);
    };

    unpack565(c0, colors[0]);
    colors[0][3] = 255;
    unpack565(c1, colors[1]);
    colors[1][3] = 255;

    if (c0 > c1)
    {
        for (int i = 0; i < 3; i++)
        {
            colors[2][i] = static_cast<uint8_t>((2 * colors[0][i] + colors[1][i]) / 3);
            colors[3][i] = static_cast<uint8_t>((colors[0][i] + 2 * colors[1][i]) / 3);
        }
        colors[2][3] = 255;
        colors[3][3] = 255;
    }
    else
    {
        for (int i = 0; i < 3; i++)
            colors[2][i] = static_cast<uint8_t>((colors[0][i] + colors[1][i]) / 2);
        colors[2][3] = 255;
        colors[3][0] = colors[3][1] = colors[3][2] = 0;
        colors[3][3] = 0;
    }

    uint32_t bits = block[4] | (block[5] << 8) | (block[6] << 16) | (block[7] << 24);
    for (int y = 0; y < 4; y++)
    {
        for (int x = 0; x < 4; x++)
        {
            uint8_t idx = (bits >> (2 * (y * 4 + x))) & 3;
            int ofs = y * outStride + x * 4;
            std::memcpy(out + ofs, colors[idx], 4);
        }
    }
}

void decodeDXT3Alpha(const uint8_t* alphaBlock, uint8_t* out, int outStride)
{
    for (int y = 0; y < 4; y++)
    {
        uint16_t row = alphaBlock[y * 2] | (alphaBlock[y * 2 + 1] << 8);
        for (int x = 0; x < 4; x++)
        {
            uint8_t a = ((row >> (x * 4)) & 0xF) * 17;
            out[y * outStride + x * 4 + 3] = a;
        }
    }
}

void decodeDXT5Alpha(const uint8_t* alphaBlock, uint8_t* out, int outStride)
{
    uint8_t a0 = alphaBlock[0];
    uint8_t a1 = alphaBlock[1];

    uint8_t alphas[8];
    alphas[0] = a0;
    alphas[1] = a1;
    if (a0 > a1)
    {
        for (int i = 0; i < 6; i++)
            alphas[2 + i] = static_cast<uint8_t>(((6 - i) * a0 + (1 + i) * a1) / 7);
    }
    else
    {
        for (int i = 0; i < 4; i++)
            alphas[2 + i] = static_cast<uint8_t>(((4 - i) * a0 + (1 + i) * a1) / 5);
        alphas[6] = 0;
        alphas[7] = 255;
    }

    uint64_t bits = 0;
    for (int i = 0; i < 6; i++)
        bits |= static_cast<uint64_t>(alphaBlock[2 + i]) << (8 * i);

    for (int y = 0; y < 4; y++)
    {
        for (int x = 0; x < 4; x++)
        {
            int idx = static_cast<int>((bits >> (3 * (y * 4 + x))) & 7);
            out[y * outStride + x * 4 + 3] = alphas[idx];
        }
    }
}

} // anonymous namespace

void BlpImage::decodeDXT(int mipmap, std::vector<uint8_t>& out) const
{
    uint32_t w = std::max(1u, m_width >> mipmap);
    uint32_t h = std::max(1u, m_height >> mipmap);
    int blockW = std::max(1u, (w + 3) / 4);
    int blockH = std::max(1u, (h + 3) / 4);
    int outStride = static_cast<int>(w * 4);

    const uint8_t* src = m_data.raw().data() + m_mipmapOffsets[mipmap];

    bool isDXT1 = (m_alphaDepth == 0) || (m_alphaDepth == 1 && m_alphaEncoding == 0);
    bool isDXT3 = (m_alphaDepth == 4 || (m_alphaDepth == 8 && m_alphaEncoding == 1));
    bool isDXT5 = (m_alphaDepth == 8 && m_alphaEncoding == 7);

    for (int by = 0; by < blockH; by++)
    {
        for (int bx = 0; bx < blockW; bx++)
        {
            uint8_t blockPixels[4 * 4 * 4];

            if (isDXT1)
            {
                decodeDXT1Block(src, blockPixels, 16);
                src += 8;
            }
            else if (isDXT3)
            {
                decodeDXT1Block(src + 8, blockPixels, 16);
                decodeDXT3Alpha(src, blockPixels, 16);
                src += 16;
            }
            else if (isDXT5)
            {
                decodeDXT1Block(src + 8, blockPixels, 16);
                decodeDXT5Alpha(src, blockPixels, 16);
                src += 16;
            }
            else
            {
                decodeDXT1Block(src, blockPixels, 16);
                src += 8;
            }

            for (int y = 0; y < 4 && (by * 4 + y) < static_cast<int>(h); y++)
            {
                for (int x = 0; x < 4 && (bx * 4 + x) < static_cast<int>(w); x++)
                {
                    int dstIdx = ((by * 4 + y) * w + (bx * 4 + x)) * 4;
                    int srcIdx = (y * 4 + x) * 4;
                    std::memcpy(out.data() + dstIdx, blockPixels + srcIdx, 4);
                }
            }
        }
    }
}

void BlpImage::decodeBGRA(int mipmap, std::vector<uint8_t>& out) const
{
    uint32_t w = std::max(1u, m_width >> mipmap);
    uint32_t h = std::max(1u, m_height >> mipmap);
    uint32_t pixelCount = w * h;

    const uint8_t* src = m_data.raw().data() + m_mipmapOffsets[mipmap];

    for (uint32_t i = 0; i < pixelCount; i++)
    {
        out[i * 4 + 2] = src[i * 4 + 0]; // B -> B
        out[i * 4 + 1] = src[i * 4 + 1]; // G -> G
        out[i * 4 + 0] = src[i * 4 + 2]; // R -> R
        out[i * 4 + 3] = src[i * 4 + 3]; // A -> A
    }
}

} // namespace wowlib
