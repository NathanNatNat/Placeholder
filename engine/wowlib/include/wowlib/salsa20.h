#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include <string_view>
#include <span>

namespace wowlib
{

class DataBuffer;

/// Salsa20 stream cipher for BLTE encrypted block decryption.
class Salsa20
{
public:
    /// @param nonce 8-byte nonce
    /// @param key Hex-encoded key string (32 or 64 hex chars = 16 or 32 bytes)
    /// @param rounds Number of rounds (default 20)
    Salsa20(std::span<const uint8_t> nonce, std::string_view key, int rounds = 20);

    void setKey(std::vector<uint8_t> key);
    void setNonce(std::span<const uint8_t> nonce);

    /// Generate raw keystream bytes.
    DataBuffer getBytes(size_t byteCount);

    /// XOR buf with keystream to decrypt/encrypt.
    DataBuffer process(DataBuffer& buf);

private:
    void reset();
    void increment();
    void generateBlock();

    int m_rounds;
    std::array<uint32_t, 4> m_sigma;
    std::array<uint32_t, 8> m_keyWords;
    std::array<uint32_t, 2> m_nonceWords;
    std::array<uint32_t, 2> m_counter;
    std::array<uint8_t, 64> m_block;
    int m_blockUsed;
};

} // namespace wowlib
