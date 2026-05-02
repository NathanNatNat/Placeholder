#include "wowlib/blte_reader.h"
#include "wowlib/salsa20.h"
#include "wowlib/tact_keys.h"

#include <cstring>
#include <format>
#include <span>

namespace wowlib
{

EncryptionError::EncryptionError(const std::string& key)
    : std::runtime_error("[BLTE] Missing decryption key " + key), key(key)
{
}

BLTEIntegrityError::BLTEIntegrityError(const std::string& expected, const std::string& actual)
    : std::runtime_error(std::format("[BLTE] Invalid block hash. Expected {}, got {}", expected, actual))
{
}

bool BLTEReader::check(DataBuffer& data)
{
    if (data.byteLength() < 4)
        return false;
    const uint32_t magic = data.readUInt32LE();
    data.seek(0);
    return magic == BLTE_MAGIC;
}

BLTEMetadata BLTEReader::parseBLTEHeader(DataBuffer& buf, const std::string& hash,
                                           bool verifyHash, bool restoreOffset)
{
    const size_t size = buf.byteLength();
    if (size < 8)
        throw std::runtime_error("[BLTE] Not enough data (< 8)");

    const size_t originalOffset = buf.offset();
    buf.seek(0);

    const uint32_t magic = buf.readUInt32LE();
    if (magic != BLTE_MAGIC)
        throw std::runtime_error(std::format("[BLTE] Invalid magic: {}", magic));

    const int32_t headerSize = buf.readInt32BE();
    const size_t origPos = buf.offset();

    if (verifyHash)
    {
        buf.seek(0);
        const std::string hashCheck = headerSize > 0
            ? buf.readBuffer(static_cast<size_t>(headerSize)).calculateHash()
            : buf.calculateHash();

        if (hashCheck != hash)
            throw std::runtime_error(std::format("[BLTE] Invalid MD5 hash, expected {} got {}", hash, hashCheck));

        buf.seek(static_cast<int64_t>(origPos));
    }

    int numBlocks = 1;
    size_t dataStart = 8;

    if (headerSize > 0)
    {
        if (size < 12)
            throw std::runtime_error("[BLTE] Not enough data (< 12)");

        std::vector<uint8_t> fc = buf.readUInt8(4);
        numBlocks = fc[1] << 16 | fc[2] << 8 | fc[3] << 0;

        if (fc[0] != 0x0F || numBlocks == 0)
            throw std::runtime_error("[BLTE] Invalid table format.");

        const int frameHeaderSize = 24 * numBlocks + 12;
        if (headerSize != frameHeaderSize)
            throw std::runtime_error("[BLTE] Invalid header size.");

        if (size < static_cast<size_t>(frameHeaderSize))
            throw std::runtime_error("[BLTE] Not enough data (frameHeader).");

        dataStart = static_cast<size_t>(headerSize);
    }

    std::vector<BLTEBlock> blocks(numBlocks);
    int64_t fileOffset = 0;
    size_t totalDecompSize = 0;

    for (int i = 0; i < numBlocks; i++)
    {
        BLTEBlock& block = blocks[i];
        if (headerSize != 0)
        {
            block.compSize = buf.readInt32BE();
            block.decompSize = buf.readInt32BE();
            block.hash = buf.readHexString(16);
        }
        else
        {
            block.compSize = static_cast<int32_t>(size - 8);
            block.decompSize = static_cast<int32_t>(size - 9);
            block.hash = EMPTY_HASH;
        }

        block.fileOffset = fileOffset;
        fileOffset += block.compSize;
        totalDecompSize += block.decompSize;
    }

    if (restoreOffset)
        buf.seek(static_cast<int64_t>(originalOffset));

    return BLTEMetadata{
        .blocks = std::move(blocks),
        .headerSize = headerSize,
        .dataStart = dataStart,
        .totalSize = totalDecompSize
    };
}

BLTEReader::BLTEReader(DataBuffer buf, const std::string& hash, const TactKeys& keys,
                         bool partialDecrypt)
    : DataBuffer(), m_blte(std::move(buf)), m_keys(keys), m_partialDecrypt(partialDecrypt)
{
    BLTEMetadata metadata = parseBLTEHeader(m_blte, hash, true, false);
    m_blocks = std::move(metadata.blocks);
    setCapacity(metadata.totalSize);
}

void BLTEReader::checkBounds(size_t length)
{
    DataBuffer::checkBounds(length);
    const size_t pos = offset() + length;
    while (pos > m_blockWriteIndex)
    {
        if (!processBlock())
            return;
    }
}

void BLTEReader::processAllBlocks()
{
    while (m_blockIndex < m_blocks.size())
        processBlock();
}

bool BLTEReader::processBlock()
{
    if (m_blockIndex == m_blocks.size())
        return false;

    const size_t oldPos = offset();
    seek(static_cast<int64_t>(m_blockWriteIndex));

    const BLTEBlock& block = m_blocks[m_blockIndex];
    const size_t bltePos = m_blte.offset();

    if (block.hash != EMPTY_HASH)
    {
        DataBuffer blockData = m_blte.readBuffer(block.compSize);
        const std::string blockHash = blockData.calculateHash();
        m_blte.seek(static_cast<int64_t>(bltePos));

        if (blockHash != block.hash)
            throw BLTEIntegrityError(block.hash, blockHash);
    }

    handleBlock(m_blte, bltePos + block.compSize, m_blockIndex);
    m_blte.seek(static_cast<int64_t>(bltePos + block.compSize));

    m_blockIndex++;
    m_blockWriteIndex = offset();

    seek(static_cast<int64_t>(oldPos));
    return true;
}

void BLTEReader::handleBlock(DataBuffer& block, size_t blockEnd, size_t index)
{
    const uint8_t flag = block.readUInt8();
    switch (flag)
    {
        case 0x45: // Encrypted
        {
            try
            {
                DataBuffer decrypted = decryptBlock(block, blockEnd, index);
                handleBlock(decrypted, decrypted.byteLength(), index);
            }
            catch (const EncryptionError&)
            {
                if (m_partialDecrypt)
                    move(static_cast<int64_t>(m_blocks[index].decompSize));
                else
                    throw;
            }
            break;
        }

        case 0x46: // Frame (Recursive)
            throw std::runtime_error("[BLTE] No frame decoder implemented!");

        case 0x4E: // Frame (Normal)
            writeBufferBLTE(block, blockEnd);
            break;

        case 0x5A: // Compressed
            decompressBlock(block, blockEnd, index);
            break;

        default:
            throw std::runtime_error(std::format("[BLTE] Unknown block type: 0x{:02X}", flag));
    }
}

void BLTEReader::decompressBlock(DataBuffer& data, size_t blockEnd, size_t index)
{
    DataBuffer decomp = data.readBuffer(blockEnd - data.offset(), true);
    const size_t expectedSize = static_cast<size_t>(m_blocks[index].decompSize);

    if (decomp.byteLength() > expectedSize)
        setCapacity(byteLength() + (decomp.byteLength() - expectedSize));

    writeBufferBLTE(decomp, decomp.byteLength());
}

DataBuffer BLTEReader::decryptBlock(DataBuffer& data, size_t blockEnd, size_t index)
{
    const uint8_t keyNameSize = data.readUInt8();
    if (keyNameSize == 0 || keyNameSize != 8)
        throw std::runtime_error(std::format("[BLTE] Unexpected keyNameSize: {}", keyNameSize));

    std::vector<std::string> keyNameBytes(keyNameSize);
    for (uint8_t i = 0; i < keyNameSize; i++)
        keyNameBytes[i] = data.readHexString(1);

    std::string keyName;
    for (auto it = keyNameBytes.rbegin(); it != keyNameBytes.rend(); ++it)
        keyName += *it;

    const uint8_t ivSize = data.readUInt8();
    if ((ivSize != 4 && ivSize != 8) || ivSize > 8)
        throw std::runtime_error(std::format("[BLTE] Unexpected ivSize: {}", ivSize));

    std::vector<uint8_t> ivShort = data.readUInt8(ivSize);
    if (data.remainingBytes() == 0)
        throw std::runtime_error("[BLTE] Unexpected end of data before encryption flag.");

    const uint8_t encryptType = data.readUInt8();
    if (encryptType != ENC_TYPE_SALSA20)
        throw std::runtime_error(std::format("[BLTE] Unexpected encryption type: {}", encryptType));

    for (int shift = 0, i = 0; i < 4; shift += 8, i++)
        ivShort[i] = (ivShort[i] ^ ((index >> shift) & 0xFF)) & 0xFF;

    const auto key = m_keys.getKey(keyName);
    if (!key.has_value())
        throw EncryptionError(keyName);

    std::vector<uint8_t> nonce(8, 0);
    for (size_t i = 0; i < 8; i++)
        nonce[i] = (i < ivShort.size() ? ivShort[i] : 0x0);

    Salsa20 instance(std::span<const uint8_t>(nonce), *key);
    DataBuffer encData = data.readBuffer(blockEnd - data.offset());
    return instance.process(encData);
}

void BLTEReader::writeBufferBLTE(DataBuffer& buf, size_t blockEnd)
{
    const size_t length = blockEnd - buf.offset();
    std::memcpy(raw().data() + offset(), buf.raw().data() + buf.offset(), length);
    move(static_cast<int64_t>(length));
}

} // namespace wowlib
