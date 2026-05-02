#pragma once

#include "wowlib/data_buffer.h"

#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>

namespace wowlib
{

class TactKeys;

constexpr uint32_t BLTE_MAGIC = 0x45544c42;
constexpr uint8_t ENC_TYPE_SALSA20 = 0x53;
inline const std::string EMPTY_HASH = "00000000000000000000000000000000";

/// Thrown when a required TACT encryption key is missing.
class EncryptionError : public std::runtime_error
{
public:
    explicit EncryptionError(const std::string& key);
    std::string key;
};

/// Thrown when a BLTE block fails MD5 integrity verification.
class BLTEIntegrityError : public std::runtime_error
{
public:
    BLTEIntegrityError(const std::string& expected, const std::string& actual);
};

struct BLTEBlock
{
    int32_t compSize;
    int32_t decompSize;
    std::string hash;
    int64_t fileOffset;
};

struct BLTEMetadata
{
    std::vector<BLTEBlock> blocks;
    int32_t headerSize;
    size_t dataStart;
    size_t totalSize;
};

/// BLTE (Block Table Encoding) reader with lazy on-demand block decompression.
/// Inherits DataBuffer and overrides checkBounds to decompress blocks as they are read.
class BLTEReader : public DataBuffer
{
public:
    /// Check if the given data starts with BLTE magic.
    static bool check(DataBuffer& data);

    /// Parse BLTE header and return metadata without decompressing blocks.
    static BLTEMetadata parseBLTEHeader(DataBuffer& buf, const std::string& hash,
                                         bool verifyHash = true, bool restoreOffset = true);

    /// Construct a reader. Parses the BLTE header and allocates the decompressed buffer.
    /// @param buf Raw BLTE data
    /// @param hash Expected MD5 hash for integrity verification
    /// @param keys TACT key ring for encrypted block decryption
    /// @param partialDecrypt If true, leave zeroed data for missing keys instead of throwing
    BLTEReader(DataBuffer buf, const std::string& hash, const TactKeys& keys,
               bool partialDecrypt = false);

    /// Decompress all remaining blocks.
    void processAllBlocks();

protected:
    void checkBounds(size_t length) override;

private:
    bool processBlock();
    void handleBlock(DataBuffer& block, size_t blockEnd, size_t index);
    void decompressBlock(DataBuffer& data, size_t blockEnd, size_t index);
    DataBuffer decryptBlock(DataBuffer& data, size_t blockEnd, size_t index);
    void writeBufferBLTE(DataBuffer& buf, size_t blockEnd);

    DataBuffer m_blte;
    const TactKeys& m_keys;
    std::vector<BLTEBlock> m_blocks;
    size_t m_blockIndex = 0;
    size_t m_blockWriteIndex = 0;
    bool m_partialDecrypt = false;
};

} // namespace wowlib
