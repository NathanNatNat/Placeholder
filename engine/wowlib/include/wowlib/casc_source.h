#pragma once

#include "wowlib/data_buffer.h"
#include "wowlib/blte_reader.h"
#include "wowlib/tact_keys.h"

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace wowlib
{

constexpr uint16_t ENC_MAGIC = 0x4E45;
constexpr uint32_t ROOT_MAGIC = 0x4D465354;

namespace locale_flags
{
    constexpr uint32_t EN_US = 0x2;
    constexpr uint32_t KO_KR = 0x4;
    constexpr uint32_t FR_FR = 0x10;
    constexpr uint32_t DE_DE = 0x20;
    constexpr uint32_t ZH_CN = 0x40;
    constexpr uint32_t ES_ES = 0x80;
    constexpr uint32_t ZH_TW = 0x100;
    constexpr uint32_t EN_GB = 0x200;
    constexpr uint32_t ES_MX = 0x1000;
    constexpr uint32_t RU_RU = 0x2000;
    constexpr uint32_t PT_BR = 0x4000;
    constexpr uint32_t IT_IT = 0x8000;
    constexpr uint32_t PT_PT = 0x10000;
}

namespace content_flags
{
    constexpr uint32_t LOAD_ON_WINDOWS    = 0x8;
    constexpr uint32_t LOAD_ON_MACOS      = 0x10;
    constexpr uint32_t LOW_VIOLENCE       = 0x80;
    constexpr uint32_t DO_NOT_LOAD        = 0x100;
    constexpr uint32_t ENCRYPTED          = 0x8000000;
    constexpr uint32_t NO_NAME_HASH       = 0x10000000;
    constexpr uint32_t UNCOMMON_RES       = 0x20000000;
    constexpr uint32_t BUNDLE             = 0x40000000;
    constexpr uint32_t NO_COMPRESSION     = 0x80000000;
}

struct RootType
{
    uint32_t contentFlags;
    uint32_t localeFlags;
};

/// Base CASC data source. Handles root/encoding file parsing and file ID lookup.
/// Subclasses (CascLocal, CascRemote) implement actual file reading.
class CascSource
{
public:
    explicit CascSource(TactKeys& keys, uint32_t locale = locale_flags::EN_US);
    virtual ~CascSource();

    /// Check if a file exists for the current locale.
    bool fileExists(uint32_t fileDataID) const;

    /// Get the encoding key for a file by its fileDataID.
    std::string getFile(uint32_t fileDataID) const;

    /// Get a file as a BLTE reader for decompression.
    virtual BLTEReader getFileAsBLTE(uint32_t fileDataID, bool partialDecrypt = false) = 0;

    /// Get the list of valid fileDataIDs matching the current locale.
    std::vector<uint32_t> getValidRootEntries() const;

    /// Parse entries from a BLTE-wrapped root file.
    size_t parseRootFile(DataBuffer data, const std::string& hash);

    /// Parse entries from a BLTE-wrapped encoding file.
    void parseEncodingFile(DataBuffer data, const std::string& hash);

    /// Format a key for CDN URL paths (ab/cd/abcd...).
    static std::string formatCdnKey(const std::string& key);

    /// Read raw data from archives or CDN. Must be implemented by subclasses.
    virtual DataBuffer getDataFile(const std::string& file) = 0;

    // Public data
    std::unordered_map<std::string, int64_t> encodingSizes;
    std::unordered_map<std::string, std::string> encodingKeys;
    std::vector<RootType> rootTypes;
    std::unordered_map<uint32_t, std::unordered_map<size_t, std::string>> rootEntries;
    std::unordered_map<std::string, std::string> buildConfig;
    std::unordered_map<std::string, std::string> cdnConfig;

protected:
    TactKeys& m_keys;
    uint32_t m_locale;
};

} // namespace wowlib
