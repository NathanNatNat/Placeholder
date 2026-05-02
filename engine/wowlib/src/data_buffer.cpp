#include "wowlib/data_buffer.h"

#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <fstream>
#include <array>
#include <format>

#include <zlib.h>
#include <openssl/evp.h>
#include <nlohmann/json.hpp>

namespace
{

template<size_t N>
uint64_t readUintLE(const uint8_t* p)
{
    uint64_t val = 0;
    for (size_t i = 0; i < N; i++)
        val |= static_cast<uint64_t>(p[i]) << (i * 8);
    return val;
}

template<size_t N>
uint64_t readUintBE(const uint8_t* p)
{
    uint64_t val = 0;
    for (size_t i = 0; i < N; i++)
        val |= static_cast<uint64_t>(p[i]) << ((N - 1 - i) * 8);
    return val;
}

template<size_t N>
int64_t signExtend(uint64_t val)
{
    constexpr uint64_t signBit = 1ULL << (N * 8 - 1);
    constexpr uint64_t mask = []() constexpr -> uint64_t
    {
        if constexpr (N < 8) return (1ULL << (N * 8)) - 1;
        else return ~0ULL;
    }();
    val &= mask;
    if (val & signBit)
        val |= ~mask;
    return static_cast<int64_t>(val);
}

template<size_t N, bool LE>
uint64_t readUint(const uint8_t* p)
{
    if constexpr (LE) return readUintLE<N>(p);
    else return readUintBE<N>(p);
}

template<size_t N>
void writeUintLE(uint8_t* p, uint64_t val)
{
    for (size_t i = 0; i < N; i++)
        p[i] = static_cast<uint8_t>(val >> (i * 8));
}

template<size_t N>
void writeUintBE(uint8_t* p, uint64_t val)
{
    for (size_t i = 0; i < N; i++)
        p[i] = static_cast<uint8_t>(val >> ((N - 1 - i) * 8));
}

template<size_t N, bool LE>
void writeUint(uint8_t* p, uint64_t val)
{
    if constexpr (LE) writeUintLE<N>(p, val);
    else writeUintBE<N>(p, val);
}

uint64_t readVarUintLE(const uint8_t* p, size_t n)
{
    uint64_t val = 0;
    for (size_t i = 0; i < n; i++)
        val |= static_cast<uint64_t>(p[i]) << (i * 8);
    return val;
}

uint64_t readVarUintBE(const uint8_t* p, size_t n)
{
    uint64_t val = 0;
    for (size_t i = 0; i < n; i++)
        val |= static_cast<uint64_t>(p[i]) << ((n - 1 - i) * 8);
    return val;
}

int64_t signExtendVar(uint64_t val, size_t n)
{
    if (n >= 8) return static_cast<int64_t>(val);
    uint64_t signBit = 1ULL << (n * 8 - 1);
    uint64_t mask = (1ULL << (n * 8)) - 1;
    val &= mask;
    if (val & signBit)
        val |= ~mask;
    return static_cast<int64_t>(val);
}

constexpr char BASE64_CHARS[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64Encode(const uint8_t* data, size_t len)
{
    std::string result;
    result.reserve((len + 2) / 3 * 4);
    for (size_t i = 0; i < len; i += 3)
    {
        uint32_t n = static_cast<uint32_t>(data[i]) << 16;
        if (i + 1 < len) n |= static_cast<uint32_t>(data[i + 1]) << 8;
        if (i + 2 < len) n |= static_cast<uint32_t>(data[i + 2]);
        result += BASE64_CHARS[(n >> 18) & 0x3f];
        result += BASE64_CHARS[(n >> 12) & 0x3f];
        result += (i + 1 < len) ? BASE64_CHARS[(n >> 6) & 0x3f] : '=';
        result += (i + 2 < len) ? BASE64_CHARS[n & 0x3f] : '=';
    }
    return result;
}

std::vector<uint8_t> base64Decode(std::string_view str)
{
    static const auto& getTable = []() -> const std::array<uint8_t, 256>&
    {
        static const auto table = []()
        {
            std::array<uint8_t, 256> t{};
            t.fill(255);
            for (uint8_t i = 0; i < 64; i++)
                t[static_cast<uint8_t>(BASE64_CHARS[i])] = i;
            return t;
        }();
        return table;
    };
    const auto& table = getTable();
    std::vector<uint8_t> result;
    result.reserve(str.size() * 3 / 4);
    uint32_t val = 0;
    int bits = 0;
    for (char c : str)
    {
        if (c == '=' || c == '\n' || c == '\r') continue;
        uint8_t d = table[static_cast<uint8_t>(c)];
        if (d == 255) continue;
        val = (val << 6) | d;
        bits += 6;
        if (bits >= 8)
        {
            bits -= 8;
            result.push_back(static_cast<uint8_t>((val >> bits) & 0xff));
        }
    }
    return result;
}

std::string bytesToHex(const uint8_t* data, size_t length)
{
    constexpr char hexChars[] = "0123456789abcdef";
    std::string out;
    out.reserve(length * 2);
    for (size_t i = 0; i < length; ++i)
    {
        out.push_back(hexChars[data[i] >> 4]);
        out.push_back(hexChars[data[i] & 0x0F]);
    }
    return out;
}

void appendUtf8FromCodepoint(std::string& out, uint32_t cp)
{
    if (cp < 0x80)
    {
        out.push_back(static_cast<char>(cp));
    }
    else if (cp < 0x800)
    {
        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    else if (cp < 0x10000)
    {
        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    else
    {
        out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
}

std::string decodeUtf16leToUtf8(const uint8_t* data, size_t length)
{
    std::string out;
    out.reserve(length);
    size_t i = 0;
    while (i + 1 < length)
    {
        const uint16_t w1 = static_cast<uint16_t>(data[i] | (static_cast<uint16_t>(data[i + 1]) << 8));
        i += 2;
        if (w1 >= 0xD800 && w1 <= 0xDBFF && i + 1 < length)
        {
            const uint16_t w2 = static_cast<uint16_t>(data[i] | (static_cast<uint16_t>(data[i + 1]) << 8));
            if (w2 >= 0xDC00 && w2 <= 0xDFFF)
            {
                i += 2;
                const uint32_t cp = 0x10000 + (((w1 - 0xD800) << 10) | (w2 - 0xDC00));
                appendUtf8FromCodepoint(out, cp);
                continue;
            }
        }
        appendUtf8FromCodepoint(out, w1);
    }
    return out;
}

std::vector<uint8_t> evpHash(const uint8_t* data, size_t len, const EVP_MD* md)
{
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx)
        throw std::runtime_error("EVP_MD_CTX_new failed");
    if (EVP_DigestInit_ex(ctx, md, nullptr) != 1 ||
        EVP_DigestUpdate(ctx, data, len) != 1)
    {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Hash computation failed");
    }
    std::vector<uint8_t> digest(EVP_MD_size(md));
    unsigned int digestLen = 0;
    if (EVP_DigestFinal_ex(ctx, digest.data(), &digestLen) != 1)
    {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Hash finalization failed");
    }
    EVP_MD_CTX_free(ctx);
    digest.resize(digestLen);
    return digest;
}

const EVP_MD* hashNameToMd(std::string_view name)
{
    if (name == "md5")    return EVP_md5();
    if (name == "sha1")   return EVP_sha1();
    if (name == "sha256") return EVP_sha256();
    if (name == "sha384") return EVP_sha384();
    if (name == "sha512") return EVP_sha512();
    return nullptr;
}

std::vector<uint8_t> zlibInflate(const uint8_t* data, size_t len)
{
    z_stream strm{};
    if (inflateInit(&strm) != Z_OK)
        throw std::runtime_error("zlib inflateInit failed");

    strm.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(data));
    strm.avail_in = static_cast<uInt>(len);

    std::vector<uint8_t> result;
    uint8_t chunk[16384];

    int ret;
    do
    {
        strm.next_out = chunk;
        strm.avail_out = sizeof(chunk);
        ret = inflate(&strm, Z_NO_FLUSH);
        if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR)
        {
            inflateEnd(&strm);
            throw std::runtime_error("zlib inflate failed");
        }
        result.insert(result.end(), chunk, chunk + sizeof(chunk) - strm.avail_out);
    }
    while (ret != Z_STREAM_END);

    inflateEnd(&strm);
    return result;
}

std::vector<uint8_t> zlibDeflate(const uint8_t* data, size_t len)
{
    z_stream strm{};
    if (deflateInit(&strm, Z_DEFAULT_COMPRESSION) != Z_OK)
        throw std::runtime_error("zlib deflateInit failed");

    strm.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(data));
    strm.avail_in = static_cast<uInt>(len);

    std::vector<uint8_t> result;
    uint8_t chunk[16384];

    int ret;
    do
    {
        strm.next_out = chunk;
        strm.avail_out = sizeof(chunk);
        ret = ::deflate(&strm, Z_FINISH);
        if (ret == Z_STREAM_ERROR)
        {
            deflateEnd(&strm);
            throw std::runtime_error("zlib deflate failed");
        }
        result.insert(result.end(), chunk, chunk + sizeof(chunk) - strm.avail_out);
    }
    while (ret != Z_STREAM_END);

    deflateEnd(&strm);
    return result;
}

} // anonymous namespace

namespace wowlib
{

// Macros for generating repetitive read/write methods
#define IMPL_READ_U(RetType, Name, N, LE)                                        \
    RetType DataBuffer::Name()                                                   \
    {                                                                            \
        checkBounds(N);                                                          \
        auto v = static_cast<RetType>(readUint<N, LE>(m_buffer.data() + m_offset)); \
        m_offset += N;                                                           \
        return v;                                                                \
    }                                                                            \
    std::vector<RetType> DataBuffer::Name(size_t count)                          \
    {                                                                            \
        checkBounds(N * count);                                                  \
        std::vector<RetType> values(count);                                      \
        for (size_t i = 0; i < count; i++)                                       \
        {                                                                        \
            values[i] = static_cast<RetType>(readUint<N, LE>(m_buffer.data() + m_offset)); \
            m_offset += N;                                                       \
        }                                                                        \
        return values;                                                           \
    }

#define IMPL_READ_S(RetType, Name, N, LE)                                        \
    RetType DataBuffer::Name()                                                   \
    {                                                                            \
        checkBounds(N);                                                          \
        auto v = static_cast<RetType>(signExtend<N>(readUint<N, LE>(m_buffer.data() + m_offset))); \
        m_offset += N;                                                           \
        return v;                                                                \
    }                                                                            \
    std::vector<RetType> DataBuffer::Name(size_t count)                          \
    {                                                                            \
        checkBounds(N * count);                                                  \
        std::vector<RetType> values(count);                                      \
        for (size_t i = 0; i < count; i++)                                       \
        {                                                                        \
            values[i] = static_cast<RetType>(signExtend<N>(readUint<N, LE>(m_buffer.data() + m_offset))); \
            m_offset += N;                                                       \
        }                                                                        \
        return values;                                                           \
    }

#define IMPL_WRITE_INT(ParamType, Name, N, LE)                                   \
    void DataBuffer::Name(ParamType value)                                       \
    {                                                                            \
        checkBounds(N);                                                          \
        writeUint<N, LE>(m_buffer.data() + m_offset, static_cast<uint64_t>(value)); \
        m_offset += N;                                                           \
    }

// ----- static factories -----

DataBuffer DataBuffer::alloc(size_t length)
{
    return DataBuffer(std::vector<uint8_t>(length, 0));
}

DataBuffer DataBuffer::from(std::span<const uint8_t> source)
{
    return DataBuffer(std::vector<uint8_t>(source.begin(), source.end()));
}

DataBuffer DataBuffer::from(std::vector<uint8_t>&& source)
{
    return DataBuffer(std::move(source));
}

DataBuffer DataBuffer::fromBase64(std::string_view source)
{
    return DataBuffer(base64Decode(source));
}

DataBuffer DataBuffer::concat(const std::vector<DataBuffer>& buffers)
{
    size_t total = 0;
    for (const auto& b : buffers)
        total += b.byteLength();
    std::vector<uint8_t> combined;
    combined.reserve(total);
    for (const auto& b : buffers)
        combined.insert(combined.end(), b.m_buffer.begin(), b.m_buffer.end());
    return DataBuffer(std::move(combined));
}

DataBuffer DataBuffer::readFile(const std::filesystem::path& file)
{
    std::ifstream ifs(file, std::ios::binary | std::ios::ate);
    if (!ifs)
        throw std::runtime_error("Failed to open file: " + file.string());
    auto size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    std::vector<uint8_t> buf(static_cast<size_t>(size));
    if (!ifs.read(reinterpret_cast<char*>(buf.data()), size))
        throw std::runtime_error("Failed to read file: " + file.string());
    return DataBuffer(std::move(buf));
}

// ----- constructors -----

DataBuffer::DataBuffer() = default;

DataBuffer::DataBuffer(std::vector<uint8_t> buf)
    : m_buffer(std::move(buf))
{
}

DataBuffer::DataBuffer(std::vector<uint8_t> buf, size_t offset)
    : m_offset(offset), m_buffer(std::move(buf))
{
}

DataBuffer::DataBuffer(DataBuffer&& other) noexcept
    : m_offset(other.m_offset), m_buffer(std::move(other.m_buffer))
{
    other.m_offset = 0;
}

DataBuffer& DataBuffer::operator=(DataBuffer&& other) noexcept
{
    if (this != &other)
    {
        m_offset = other.m_offset;
        m_buffer = std::move(other.m_buffer);
        other.m_offset = 0;
    }
    return *this;
}

DataBuffer::~DataBuffer() = default;

// ----- properties -----

size_t DataBuffer::byteLength() const { return m_buffer.size(); }
size_t DataBuffer::remainingBytes() const { return m_buffer.size() - m_offset; }
size_t DataBuffer::offset() const { return m_offset; }
const std::vector<uint8_t>& DataBuffer::raw() const { return m_buffer; }
std::vector<uint8_t>& DataBuffer::raw() { return m_buffer; }
const uint8_t* DataBuffer::data() const { return m_buffer.data(); }

// ----- positioning -----

void DataBuffer::seek(int64_t ofs)
{
    int64_t pos = ofs < 0 ? static_cast<int64_t>(byteLength()) + ofs : ofs;
    if (pos < 0 || static_cast<size_t>(pos) > byteLength())
        throw std::runtime_error(std::format("seek() out of bounds: {} -> {} / {}", ofs, pos, byteLength()));
    m_offset = static_cast<size_t>(pos);
}

void DataBuffer::move(int64_t ofs)
{
    int64_t pos = static_cast<int64_t>(m_offset) + ofs;
    if (pos < 0 || static_cast<size_t>(pos) > byteLength())
        throw std::runtime_error(std::format("move() out of bounds: {} -> {} / {}", ofs, pos, byteLength()));
    m_offset = static_cast<size_t>(pos);
}

// ----- variable-length integer reads -----

int64_t DataBuffer::readIntLE(size_t byteLen)
{
    checkBounds(byteLen);
    uint64_t val = readVarUintLE(m_buffer.data() + m_offset, byteLen);
    m_offset += byteLen;
    return signExtendVar(val, byteLen);
}

std::vector<int64_t> DataBuffer::readIntLE(size_t byteLen, size_t count)
{
    checkBounds(byteLen * count);
    std::vector<int64_t> values(count);
    for (size_t i = 0; i < count; i++)
    {
        values[i] = signExtendVar(readVarUintLE(m_buffer.data() + m_offset, byteLen), byteLen);
        m_offset += byteLen;
    }
    return values;
}

uint64_t DataBuffer::readUIntLE(size_t byteLen)
{
    checkBounds(byteLen);
    uint64_t val = readVarUintLE(m_buffer.data() + m_offset, byteLen);
    m_offset += byteLen;
    return val;
}

std::vector<uint64_t> DataBuffer::readUIntLE(size_t byteLen, size_t count)
{
    checkBounds(byteLen * count);
    std::vector<uint64_t> values(count);
    for (size_t i = 0; i < count; i++)
    {
        values[i] = readVarUintLE(m_buffer.data() + m_offset, byteLen);
        m_offset += byteLen;
    }
    return values;
}

int64_t DataBuffer::readIntBE(size_t byteLen)
{
    checkBounds(byteLen);
    uint64_t val = readVarUintBE(m_buffer.data() + m_offset, byteLen);
    m_offset += byteLen;
    return signExtendVar(val, byteLen);
}

std::vector<int64_t> DataBuffer::readIntBE(size_t byteLen, size_t count)
{
    checkBounds(byteLen * count);
    std::vector<int64_t> values(count);
    for (size_t i = 0; i < count; i++)
    {
        values[i] = signExtendVar(readVarUintBE(m_buffer.data() + m_offset, byteLen), byteLen);
        m_offset += byteLen;
    }
    return values;
}

uint64_t DataBuffer::readUIntBE(size_t byteLen)
{
    checkBounds(byteLen);
    uint64_t val = readVarUintBE(m_buffer.data() + m_offset, byteLen);
    m_offset += byteLen;
    return val;
}

std::vector<uint64_t> DataBuffer::readUIntBE(size_t byteLen, size_t count)
{
    checkBounds(byteLen * count);
    std::vector<uint64_t> values(count);
    for (size_t i = 0; i < count; i++)
    {
        values[i] = readVarUintBE(m_buffer.data() + m_offset, byteLen);
        m_offset += byteLen;
    }
    return values;
}

// ----- fixed-width integer reads -----

IMPL_READ_S(int8_t,  readInt8,  1, true)
IMPL_READ_U(uint8_t, readUInt8, 1, true)

IMPL_READ_S(int16_t,  readInt16LE,  2, true)
IMPL_READ_U(uint16_t, readUInt16LE, 2, true)
IMPL_READ_S(int16_t,  readInt16BE,  2, false)
IMPL_READ_U(uint16_t, readUInt16BE, 2, false)

IMPL_READ_S(int32_t,  readInt24LE,  3, true)
IMPL_READ_U(uint32_t, readUInt24LE, 3, true)
IMPL_READ_S(int32_t,  readInt24BE,  3, false)
IMPL_READ_U(uint32_t, readUInt24BE, 3, false)

IMPL_READ_S(int32_t,  readInt32LE,  4, true)
IMPL_READ_U(uint32_t, readUInt32LE, 4, true)
IMPL_READ_S(int32_t,  readInt32BE,  4, false)
IMPL_READ_U(uint32_t, readUInt32BE, 4, false)

IMPL_READ_S(int64_t,  readInt40LE,  5, true)
IMPL_READ_U(uint64_t, readUInt40LE, 5, true)
IMPL_READ_S(int64_t,  readInt40BE,  5, false)
IMPL_READ_U(uint64_t, readUInt40BE, 5, false)

IMPL_READ_S(int64_t,  readInt48LE,  6, true)
IMPL_READ_U(uint64_t, readUInt48LE, 6, true)
IMPL_READ_S(int64_t,  readInt48BE,  6, false)
IMPL_READ_U(uint64_t, readUInt48BE, 6, false)

IMPL_READ_S(int64_t,  readInt64LE,  8, true)
IMPL_READ_U(uint64_t, readUInt64LE, 8, true)
IMPL_READ_S(int64_t,  readInt64BE,  8, false)
IMPL_READ_U(uint64_t, readUInt64BE, 8, false)

// ----- float / double reads -----

float DataBuffer::readFloatLE()
{
    checkBounds(4);
    uint32_t bits = static_cast<uint32_t>(readUintLE<4>(m_buffer.data() + m_offset));
    m_offset += 4;
    float f;
    std::memcpy(&f, &bits, sizeof(f));
    return f;
}

std::vector<float> DataBuffer::readFloatLE(size_t count)
{
    checkBounds(4 * count);
    std::vector<float> values(count);
    for (size_t i = 0; i < count; i++)
    {
        uint32_t bits = static_cast<uint32_t>(readUintLE<4>(m_buffer.data() + m_offset));
        m_offset += 4;
        std::memcpy(&values[i], &bits, sizeof(float));
    }
    return values;
}

float DataBuffer::readFloatBE()
{
    checkBounds(4);
    uint32_t bits = static_cast<uint32_t>(readUintBE<4>(m_buffer.data() + m_offset));
    m_offset += 4;
    float f;
    std::memcpy(&f, &bits, sizeof(f));
    return f;
}

std::vector<float> DataBuffer::readFloatBE(size_t count)
{
    checkBounds(4 * count);
    std::vector<float> values(count);
    for (size_t i = 0; i < count; i++)
    {
        uint32_t bits = static_cast<uint32_t>(readUintBE<4>(m_buffer.data() + m_offset));
        m_offset += 4;
        std::memcpy(&values[i], &bits, sizeof(float));
    }
    return values;
}

double DataBuffer::readDoubleLE()
{
    checkBounds(8);
    uint64_t bits = readUintLE<8>(m_buffer.data() + m_offset);
    m_offset += 8;
    double d;
    std::memcpy(&d, &bits, sizeof(d));
    return d;
}

std::vector<double> DataBuffer::readDoubleLE(size_t count)
{
    checkBounds(8 * count);
    std::vector<double> values(count);
    for (size_t i = 0; i < count; i++)
    {
        uint64_t bits = readUintLE<8>(m_buffer.data() + m_offset);
        m_offset += 8;
        std::memcpy(&values[i], &bits, sizeof(double));
    }
    return values;
}

double DataBuffer::readDoubleBE()
{
    checkBounds(8);
    uint64_t bits = readUintBE<8>(m_buffer.data() + m_offset);
    m_offset += 8;
    double d;
    std::memcpy(&d, &bits, sizeof(d));
    return d;
}

std::vector<double> DataBuffer::readDoubleBE(size_t count)
{
    checkBounds(8 * count);
    std::vector<double> values(count);
    for (size_t i = 0; i < count; i++)
    {
        uint64_t bits = readUintBE<8>(m_buffer.data() + m_offset);
        m_offset += 8;
        std::memcpy(&values[i], &bits, sizeof(double));
    }
    return values;
}

// ----- string / buffer reads -----

std::string DataBuffer::readHexString(size_t length)
{
    checkBounds(length);
    std::string hex = bytesToHex(m_buffer.data() + m_offset, length);
    m_offset += length;
    return hex;
}

DataBuffer DataBuffer::readBuffer()
{
    return readBuffer(remainingBytes(), false);
}

DataBuffer DataBuffer::readBuffer(size_t length, bool doInflate)
{
    checkBounds(length);
    std::vector<uint8_t> buf(m_buffer.begin() + m_offset, m_buffer.begin() + m_offset + length);
    m_offset += length;
    if (doInflate)
        buf = zlibInflate(buf.data(), buf.size());
    return DataBuffer(std::move(buf));
}

std::vector<uint8_t> DataBuffer::readBufferRaw(size_t length, bool doInflate)
{
    checkBounds(length);
    std::vector<uint8_t> buf(m_buffer.begin() + m_offset, m_buffer.begin() + m_offset + length);
    m_offset += length;
    if (doInflate)
        buf = zlibInflate(buf.data(), buf.size());
    return buf;
}

std::string DataBuffer::readString()
{
    return readString(remainingBytes());
}

std::string DataBuffer::readString(size_t length, std::string_view encoding)
{
    if (length == 0) return "";
    checkBounds(length);
    const uint8_t* ptr = m_buffer.data() + m_offset;
    std::string str;

    if (encoding == "ascii")
    {
        str.resize(length);
        for (size_t i = 0; i < length; ++i)
            str[i] = static_cast<char>(ptr[i] & 0x7F);
    }
    else if (encoding == "utf8" || encoding == "utf-8" || encoding == "latin1" || encoding == "binary")
    {
        str.assign(reinterpret_cast<const char*>(ptr), length);
    }
    else if (encoding == "base64")
    {
        str = base64Encode(ptr, length);
    }
    else if (encoding == "hex")
    {
        str = bytesToHex(ptr, length);
    }
    else if (encoding == "utf16le" || encoding == "utf-16le" || encoding == "ucs2" || encoding == "ucs-2")
    {
        str = decodeUtf16leToUtf8(ptr, length);
    }
    else
    {
        throw std::runtime_error(std::format("readString: unsupported encoding '{}'", encoding));
    }

    m_offset += length;
    return str;
}

std::string DataBuffer::readNullTerminatedString(std::string_view encoding)
{
    size_t startPos = m_offset;
    size_t length = 0;
    while (remainingBytes() > 0)
    {
        if (readUInt8() == 0x0)
            break;
        length++;
    }
    seek(static_cast<int64_t>(startPos));
    std::string str = readString(length, encoding);
    move(1);
    return str;
}

bool DataBuffer::startsWith(std::string_view input, std::string_view encoding)
{
    seek(0);
    return readString(input.size(), encoding) == input;
}

nlohmann::json DataBuffer::readJSON()
{
    return readJSON(remainingBytes());
}

nlohmann::json DataBuffer::readJSON(size_t length, std::string_view encoding)
{
    return nlohmann::json::parse(readString(length, encoding));
}

std::vector<std::string> DataBuffer::readLines(std::string_view encoding)
{
    size_t savedOfs = m_offset;
    seek(0);
    std::string str = readString(remainingBytes(), encoding);
    seek(static_cast<int64_t>(savedOfs));

    std::vector<std::string> lines;
    size_t start = 0;
    for (size_t i = 0; i < str.size(); i++)
    {
        if (str[i] == '\n')
        {
            size_t end = (i > 0 && str[i - 1] == '\r') ? i - 1 : i;
            lines.emplace_back(str, start, end - start);
            start = i + 1;
        }
    }
    lines.emplace_back(str, start);
    return lines;
}

// ----- fill -----

void DataBuffer::fill(uint8_t value)
{
    fill(value, remainingBytes());
}

void DataBuffer::fill(uint8_t value, size_t length)
{
    checkBounds(length);
    std::memset(m_buffer.data() + m_offset, value, length);
    m_offset += length;
}

// ----- fixed-width integer writes -----

IMPL_WRITE_INT(int8_t,   writeInt8,     1, true)
IMPL_WRITE_INT(uint8_t,  writeUInt8,    1, true)
IMPL_WRITE_INT(int16_t,  writeInt16LE,  2, true)
IMPL_WRITE_INT(uint16_t, writeUInt16LE, 2, true)
IMPL_WRITE_INT(int16_t,  writeInt16BE,  2, false)
IMPL_WRITE_INT(uint16_t, writeUInt16BE, 2, false)
IMPL_WRITE_INT(int32_t,  writeInt32LE,  4, true)
IMPL_WRITE_INT(uint32_t, writeUInt32LE, 4, true)
IMPL_WRITE_INT(int32_t,  writeInt32BE,  4, false)
IMPL_WRITE_INT(uint32_t, writeUInt32BE, 4, false)
IMPL_WRITE_INT(int64_t,  writeInt64LE,  8, true)
IMPL_WRITE_INT(uint64_t, writeUInt64LE, 8, true)
IMPL_WRITE_INT(int64_t,  writeInt64BE,  8, false)
IMPL_WRITE_INT(uint64_t, writeUInt64BE, 8, false)

void DataBuffer::writeFloatLE(float value)
{
    checkBounds(4);
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    writeUintLE<4>(m_buffer.data() + m_offset, bits);
    m_offset += 4;
}

void DataBuffer::writeFloatBE(float value)
{
    checkBounds(4);
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    writeUintBE<4>(m_buffer.data() + m_offset, bits);
    m_offset += 4;
}

// ----- buffer writes -----

void DataBuffer::writeBuffer(DataBuffer& buf, size_t copyLength)
{
    size_t startIndex = buf.offset();
    if (copyLength == 0)
        copyLength = buf.remainingBytes();
    else
        buf.checkBounds(copyLength);
    checkBounds(copyLength);
    std::memcpy(m_buffer.data() + m_offset, buf.m_buffer.data() + startIndex, copyLength);
    m_offset += copyLength;
    buf.m_offset += copyLength;
}

void DataBuffer::writeBuffer(std::span<const uint8_t> buf, size_t copyLength)
{
    if (copyLength == 0)
        copyLength = buf.size();
    checkBounds(copyLength);
    std::memcpy(m_buffer.data() + m_offset, buf.data(), copyLength);
    m_offset += copyLength;
}

void DataBuffer::writeToFile(const std::filesystem::path& file)
{
    std::filesystem::create_directories(file.parent_path());
    std::ofstream ofs(file, std::ios::binary);
    if (!ofs)
        throw std::runtime_error("Failed to open file for writing: " + file.string());
    ofs.write(reinterpret_cast<const char*>(m_buffer.data()), static_cast<std::streamsize>(m_buffer.size()));
    if (!ofs)
        throw std::runtime_error("Failed to write file: " + file.string());
}

// ----- search -----

int64_t DataBuffer::indexOf(uint8_t byte)
{
    return indexOf(byte, m_offset);
}

int64_t DataBuffer::indexOf(uint8_t byte, size_t start)
{
    for (size_t i = start; i < m_buffer.size(); i++)
    {
        if (m_buffer[i] == byte)
            return static_cast<int64_t>(i);
    }
    return -1;
}

// ----- utility -----

void DataBuffer::setCapacity(size_t capacity)
{
    if (capacity == byteLength()) return;
    std::vector<uint8_t> buf(capacity, 0);
    size_t copyLen = std::min(capacity, byteLength());
    std::memcpy(buf.data(), m_buffer.data(), copyLen);
    m_buffer = std::move(buf);
}

std::string DataBuffer::calculateHash(std::string_view hash, std::string_view encoding)
{
    const EVP_MD* md = hashNameToMd(hash);
    if (!md)
        throw std::runtime_error("calculateHash: unsupported algorithm '" + std::string(hash) + "'");
    auto digest = evpHash(m_buffer.data(), m_buffer.size(), md);
    if (encoding == "hex")
        return bytesToHex(digest.data(), digest.size());
    if (encoding == "base64")
        return base64Encode(digest.data(), digest.size());
    throw std::runtime_error("calculateHash: unsupported encoding '" + std::string(encoding) + "'");
}

bool DataBuffer::isZeroed() const
{
    for (size_t i = 0; i < m_buffer.size(); i++)
    {
        if (m_buffer[i] != 0x0)
            return false;
    }
    return true;
}

DataBuffer DataBuffer::deflate() const
{
    return DataBuffer(zlibDeflate(m_buffer.data(), m_buffer.size()));
}

void DataBuffer::checkBounds(size_t length)
{
    if (remainingBytes() < length)
        throw std::runtime_error(std::format("Buffer out-of-bounds: need {} bytes, {} remaining", length, remainingBytes()));
}

#undef IMPL_READ_U
#undef IMPL_READ_S
#undef IMPL_WRITE_INT

} // namespace wowlib
