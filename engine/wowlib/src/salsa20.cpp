#include "wowlib/salsa20.h"
#include "wowlib/data_buffer.h"

#include <stdexcept>

namespace
{

constexpr std::array<uint32_t, 4> SIGMA_32 = { 0x61707865, 0x3320646e, 0x79622d32, 0x6b206574 };
constexpr std::array<uint32_t, 4> SIGMA_16 = { 0x61707865, 0x3120646e, 0x79622d36, 0x6b206574 };

inline uint8_t hexCharToNibble(char c)
{
    if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
    if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return static_cast<uint8_t>(c - 'A' + 10);
    throw std::runtime_error("Invalid hex character");
}

std::vector<uint8_t> hexToBytes(std::string_view hex)
{
    std::vector<uint8_t> bytes;
    bytes.reserve(hex.size() / 2);
    for (size_t i = 0; i + 1 < hex.size(); i += 2)
        bytes.push_back(static_cast<uint8_t>((hexCharToNibble(hex[i]) << 4) | hexCharToNibble(hex[i + 1])));
    return bytes;
}

inline uint32_t rotl32(uint32_t v, int n)
{
    return (v << n) | (v >> (32 - n));
}

} // anonymous namespace

namespace wowlib
{

Salsa20::Salsa20(std::span<const uint8_t> nonce, std::string_view key, int rounds)
    : m_rounds(rounds), m_blockUsed(64)
{
    if (nonce.size() != 8)
        throw std::runtime_error("Salsa20: nonce must be 8 bytes, got " + std::to_string(nonce.size()));
    if (key.size() != 32 && key.size() != 64)
        throw std::runtime_error("Salsa20: key must be 16 or 32 bytes (hex), got " + std::to_string(key.size()));

    m_sigma = (key.size() == 32) ? SIGMA_16 : SIGMA_32;
    m_keyWords = {};
    m_nonceWords = { 0, 0 };
    m_counter = { 0, 0 };
    m_block = {};

    setKey(hexToBytes(key));
    setNonce(nonce);
}

void Salsa20::setKey(std::vector<uint8_t> key)
{
    if (key.size() == 16)
    {
        key.resize(32);
        for (int i = 0; i < 16; i++)
            key[16 + i] = key[i];
    }

    for (int i = 0, j = 0; i < 8; i++, j += 4)
    {
        m_keyWords[i] = (key[j] & 0xFF) | ((key[j+1] & 0xFF) << 8)
                      | ((key[j+2] & 0xFF) << 16) | ((key[j+3] & 0xFF) << 24);
    }

    reset();
}

void Salsa20::setNonce(std::span<const uint8_t> nonce)
{
    m_nonceWords[0] = (nonce[0] & 0xFF) | ((nonce[1] & 0xFF) << 8)
                    | ((nonce[2] & 0xFF) << 16) | ((nonce[3] & 0xFF) << 24);
    m_nonceWords[1] = (nonce[4] & 0xFF) | ((nonce[5] & 0xFF) << 8)
                    | ((nonce[6] & 0xFF) << 16) | ((nonce[7] & 0xFF) << 24);
    reset();
}

DataBuffer Salsa20::getBytes(size_t byteCount)
{
    DataBuffer out = DataBuffer::alloc(byteCount);
    for (size_t i = 0; i < byteCount; i++)
    {
        if (m_blockUsed == 64)
        {
            generateBlock();
            increment();
            m_blockUsed = 0;
        }
        out.writeUInt8(m_block[m_blockUsed]);
        m_blockUsed++;
    }
    out.seek(0);
    return out;
}

DataBuffer Salsa20::process(DataBuffer& buf)
{
    DataBuffer out = DataBuffer::alloc(buf.byteLength());
    DataBuffer bytes = getBytes(buf.byteLength());

    buf.seek(0);
    for (size_t i = 0, n = buf.byteLength(); i < n; i++)
        out.writeUInt8(bytes.readUInt8() ^ buf.readUInt8());

    out.seek(0);
    return out;
}

void Salsa20::reset()
{
    m_counter[0] = 0;
    m_counter[1] = 0;
    m_blockUsed = 64;
}

void Salsa20::increment()
{
    m_counter[0] = (m_counter[0] + 1) & 0xffffffff;
    if (m_counter[0] == 0)
        m_counter[1] = (m_counter[1] + 1) & 0xffffffff;
}

void Salsa20::generateBlock()
{
    const uint32_t j0 = m_sigma[0], j1 = m_keyWords[0], j2 = m_keyWords[1], j3 = m_keyWords[2],
        j4 = m_keyWords[3], j5 = m_sigma[1], j6 = m_nonceWords[0], j7 = m_nonceWords[1],
        j8 = m_counter[0], j9 = m_counter[1], j10 = m_sigma[2],
        j11 = m_keyWords[4], j12 = m_keyWords[5], j13 = m_keyWords[6],
        j14 = m_keyWords[7], j15 = m_sigma[3];

    uint32_t x0 = j0, x1 = j1, x2 = j2, x3 = j3, x4 = j4, x5 = j5, x6 = j6, x7 = j7,
        x8 = j8, x9 = j9, x10 = j10, x11 = j11, x12 = j12, x13 = j13, x14 = j14, x15 = j15;

    uint32_t u;
    for (int i = 0; i < m_rounds; i += 2)
    {
        u = x0 + x12; x4  ^= rotl32(u, 7);
        u = x4 + x0;  x8  ^= rotl32(u, 9);
        u = x8 + x4;  x12 ^= rotl32(u, 13);
        u = x12 + x8; x0  ^= rotl32(u, 18);

        u = x5 + x1;  x9  ^= rotl32(u, 7);
        u = x9 + x5;  x13 ^= rotl32(u, 9);
        u = x13 + x9; x1  ^= rotl32(u, 13);
        u = x1 + x13; x5  ^= rotl32(u, 18);

        u = x10 + x6; x14 ^= rotl32(u, 7);
        u = x14 + x10; x2 ^= rotl32(u, 9);
        u = x2 + x14; x6  ^= rotl32(u, 13);
        u = x6 + x2;  x10 ^= rotl32(u, 18);

        u = x15 + x11; x3 ^= rotl32(u, 7);
        u = x3 + x15; x7  ^= rotl32(u, 9);
        u = x7 + x3;  x11 ^= rotl32(u, 13);
        u = x11 + x7; x15 ^= rotl32(u, 18);

        u = x0 + x3;  x1  ^= rotl32(u, 7);
        u = x1 + x0;  x2  ^= rotl32(u, 9);
        u = x2 + x1;  x3  ^= rotl32(u, 13);
        u = x3 + x2;  x0  ^= rotl32(u, 18);

        u = x5 + x4;  x6  ^= rotl32(u, 7);
        u = x6 + x5;  x7  ^= rotl32(u, 9);
        u = x7 + x6;  x4  ^= rotl32(u, 13);
        u = x4 + x7;  x5  ^= rotl32(u, 18);

        u = x10 + x9; x11 ^= rotl32(u, 7);
        u = x11 + x10; x8 ^= rotl32(u, 9);
        u = x8 + x11; x9  ^= rotl32(u, 13);
        u = x9 + x8;  x10 ^= rotl32(u, 18);

        u = x15 + x14; x12 ^= rotl32(u, 7);
        u = x12 + x15; x13 ^= rotl32(u, 9);
        u = x13 + x12; x14 ^= rotl32(u, 13);
        u = x14 + x13; x15 ^= rotl32(u, 18);
    }

    x0 += j0; x1 += j1; x2 += j2; x3 += j3;
    x4 += j4; x5 += j5; x6 += j6; x7 += j7;
    x8 += j8; x9 += j9; x10 += j10; x11 += j11;
    x12 += j12; x13 += j13; x14 += j14; x15 += j15;

    auto store = [this](int idx, uint32_t x)
    {
        m_block[idx]   = static_cast<uint8_t>((x >>  0) & 0xFF);
        m_block[idx+1] = static_cast<uint8_t>((x >>  8) & 0xFF);
        m_block[idx+2] = static_cast<uint8_t>((x >> 16) & 0xFF);
        m_block[idx+3] = static_cast<uint8_t>((x >> 24) & 0xFF);
    };
    store(0,  x0);  store(4,  x1);  store(8,  x2);  store(12, x3);
    store(16, x4);  store(20, x5);  store(24, x6);  store(28, x7);
    store(32, x8);  store(36, x9);  store(40, x10); store(44, x11);
    store(48, x12); store(52, x13); store(56, x14); store(60, x15);
}

} // namespace wowlib
