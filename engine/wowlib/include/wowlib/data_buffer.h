#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>
#include <span>
#include <filesystem>

#include <nlohmann/json_fwd.hpp>

namespace wowlib
{

/// Binary buffer with sequential read/write cursor and endianness support.
/// Adapted from wow.export's BufferWrapper. Virtual _checkBounds allows
/// subclasses (BLTEReader) to lazily decompress blocks on demand.
class DataBuffer
{
public:
    // ----- static factories -----

    static DataBuffer alloc(size_t length);
    static DataBuffer from(std::span<const uint8_t> source);
    static DataBuffer from(std::vector<uint8_t>&& source);
    static DataBuffer fromBase64(std::string_view source);
    static DataBuffer concat(const std::vector<DataBuffer>& buffers);
    static DataBuffer readFile(const std::filesystem::path& file);

    // ----- constructors -----

    DataBuffer();
    explicit DataBuffer(std::vector<uint8_t> buf);
    DataBuffer(std::vector<uint8_t> buf, size_t offset);

    DataBuffer(DataBuffer&& other) noexcept;
    DataBuffer& operator=(DataBuffer&& other) noexcept;
    DataBuffer(const DataBuffer& other) = default;
    DataBuffer& operator=(const DataBuffer& other) = default;
    virtual ~DataBuffer();

    // ----- properties -----

    size_t byteLength() const;
    size_t remainingBytes() const;
    size_t offset() const;
    const std::vector<uint8_t>& raw() const;
    std::vector<uint8_t>& raw();
    const uint8_t* data() const;

    // ----- positioning -----

    void seek(int64_t ofs);
    void move(int64_t ofs);

    // ----- variable-length integer reads -----

    int64_t readIntLE(size_t byteLen);
    std::vector<int64_t> readIntLE(size_t byteLen, size_t count);
    uint64_t readUIntLE(size_t byteLen);
    std::vector<uint64_t> readUIntLE(size_t byteLen, size_t count);
    int64_t readIntBE(size_t byteLen);
    std::vector<int64_t> readIntBE(size_t byteLen, size_t count);
    uint64_t readUIntBE(size_t byteLen);
    std::vector<uint64_t> readUIntBE(size_t byteLen, size_t count);

    // ----- fixed-width integer reads (8-bit) -----

    int8_t readInt8();
    std::vector<int8_t> readInt8(size_t count);
    uint8_t readUInt8();
    std::vector<uint8_t> readUInt8(size_t count);

    // ----- fixed-width integer reads (16-bit) -----

    int16_t readInt16LE();
    std::vector<int16_t> readInt16LE(size_t count);
    uint16_t readUInt16LE();
    std::vector<uint16_t> readUInt16LE(size_t count);
    int16_t readInt16BE();
    std::vector<int16_t> readInt16BE(size_t count);
    uint16_t readUInt16BE();
    std::vector<uint16_t> readUInt16BE(size_t count);

    // ----- fixed-width integer reads (24-bit, stored in 32-bit) -----

    int32_t readInt24LE();
    std::vector<int32_t> readInt24LE(size_t count);
    uint32_t readUInt24LE();
    std::vector<uint32_t> readUInt24LE(size_t count);
    int32_t readInt24BE();
    std::vector<int32_t> readInt24BE(size_t count);
    uint32_t readUInt24BE();
    std::vector<uint32_t> readUInt24BE(size_t count);

    // ----- fixed-width integer reads (32-bit) -----

    int32_t readInt32LE();
    std::vector<int32_t> readInt32LE(size_t count);
    uint32_t readUInt32LE();
    std::vector<uint32_t> readUInt32LE(size_t count);
    int32_t readInt32BE();
    std::vector<int32_t> readInt32BE(size_t count);
    uint32_t readUInt32BE();
    std::vector<uint32_t> readUInt32BE(size_t count);

    // ----- fixed-width integer reads (40-bit, stored in 64-bit) -----

    int64_t readInt40LE();
    std::vector<int64_t> readInt40LE(size_t count);
    uint64_t readUInt40LE();
    std::vector<uint64_t> readUInt40LE(size_t count);
    int64_t readInt40BE();
    std::vector<int64_t> readInt40BE(size_t count);
    uint64_t readUInt40BE();
    std::vector<uint64_t> readUInt40BE(size_t count);

    // ----- fixed-width integer reads (48-bit, stored in 64-bit) -----

    int64_t readInt48LE();
    std::vector<int64_t> readInt48LE(size_t count);
    uint64_t readUInt48LE();
    std::vector<uint64_t> readUInt48LE(size_t count);
    int64_t readInt48BE();
    std::vector<int64_t> readInt48BE(size_t count);
    uint64_t readUInt48BE();
    std::vector<uint64_t> readUInt48BE(size_t count);

    // ----- fixed-width integer reads (64-bit) -----

    int64_t readInt64LE();
    std::vector<int64_t> readInt64LE(size_t count);
    uint64_t readUInt64LE();
    std::vector<uint64_t> readUInt64LE(size_t count);
    int64_t readInt64BE();
    std::vector<int64_t> readInt64BE(size_t count);
    uint64_t readUInt64BE();
    std::vector<uint64_t> readUInt64BE(size_t count);

    // ----- float / double reads -----

    float readFloatLE();
    std::vector<float> readFloatLE(size_t count);
    float readFloatBE();
    std::vector<float> readFloatBE(size_t count);
    double readDoubleLE();
    std::vector<double> readDoubleLE(size_t count);
    double readDoubleBE();
    std::vector<double> readDoubleBE(size_t count);

    // ----- string / buffer reads -----

    std::string readHexString(size_t length);
    DataBuffer readBuffer();
    DataBuffer readBuffer(size_t length, bool inflate = false);
    std::vector<uint8_t> readBufferRaw(size_t length, bool inflate = false);
    std::string readString();
    std::string readString(size_t length, std::string_view encoding = "utf8");
    std::string readNullTerminatedString(std::string_view encoding = "utf8");
    bool startsWith(std::string_view input, std::string_view encoding = "utf8");
    nlohmann::json readJSON();
    nlohmann::json readJSON(size_t length, std::string_view encoding = "utf8");
    std::vector<std::string> readLines(std::string_view encoding = "utf8");

    // ----- write methods (integers) -----

    void writeInt8(int8_t value);
    void writeUInt8(uint8_t value);
    void writeInt16LE(int16_t value);
    void writeUInt16LE(uint16_t value);
    void writeInt16BE(int16_t value);
    void writeUInt16BE(uint16_t value);
    void writeInt32LE(int32_t value);
    void writeUInt32LE(uint32_t value);
    void writeInt32BE(int32_t value);
    void writeUInt32BE(uint32_t value);
    void writeInt64LE(int64_t value);
    void writeUInt64LE(uint64_t value);
    void writeInt64BE(int64_t value);
    void writeUInt64BE(uint64_t value);
    void writeFloatLE(float value);
    void writeFloatBE(float value);

    // ----- write methods (buffer) -----

    void writeBuffer(DataBuffer& buf, size_t copyLength = 0);
    void writeBuffer(std::span<const uint8_t> buf, size_t copyLength = 0);
    void writeToFile(const std::filesystem::path& file);

    // ----- search -----

    int64_t indexOf(uint8_t byte);
    int64_t indexOf(uint8_t byte, size_t start);

    // ----- utility -----

    void fill(uint8_t value);
    void fill(uint8_t value, size_t length);
    void setCapacity(size_t capacity);
    std::string calculateHash(std::string_view hash = "md5", std::string_view encoding = "hex");
    bool isZeroed() const;
    DataBuffer deflate() const;

protected:
    virtual void checkBounds(size_t length);

    size_t m_offset = 0;
    std::vector<uint8_t> m_buffer;
};

} // namespace wowlib
