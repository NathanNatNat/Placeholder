#pragma once

#include "wowlib/casc_source.h"
#include "wowlib/build_cache.h"
#include "wowlib/cdn_resolver.h"

#include <string>
#include <filesystem>
#include <unordered_map>
#include <memory>

namespace wowlib
{

struct LocalIndexEntry
{
    int32_t index;
    int32_t offset;
    int32_t size;
};

/// CASC source using a local WoW installation directory.
class CascLocal : public CascSource
{
public:
    /// @param installDir Path to WoW installation (e.g., "C:/Program Files/World of Warcraft")
    /// @param keys TACT key ring
    /// @param cacheDir Directory for caching downloaded files
    /// @param locale Locale flags (default: enUS)
    CascLocal(const std::filesystem::path& installDir, TactKeys& keys,
              const std::filesystem::path& cacheDir = "",
              uint32_t locale = locale_flags::EN_US);

    /// Initialize by reading .build.info and listing available builds.
    void init();

    /// Get the list of available build descriptions.
    std::vector<std::string> getBuildList() const;

    /// Load a specific build by index. Parses indexes, encoding, and root files.
    void load(int buildIndex);

    /// Get a file as BLTE reader by fileDataID.
    BLTEReader getFileAsBLTE(uint32_t fileDataID, bool partialDecrypt = false) override;

    /// Read raw data from local archives by encoding key.
    DataBuffer getDataFile(const std::string& key) override;

private:
    void parseIndex(const std::filesystem::path& file);
    void loadIndexes();
    void loadEncoding();
    void loadRoot();
    std::string formatDataPath(int32_t id) const;
    std::filesystem::path formatConfigPath(const std::string& key) const;

    std::filesystem::path m_installDir;
    std::filesystem::path m_dataDir;
    std::filesystem::path m_storageDir;
    std::filesystem::path m_cacheDir;

    std::vector<std::unordered_map<std::string, std::string>> m_builds;
    std::unordered_map<std::string, std::string> m_activeBuild;
    std::unordered_map<std::string, LocalIndexEntry> m_localIndexes;
    std::unique_ptr<BuildCache> m_cache;
};

} // namespace wowlib
