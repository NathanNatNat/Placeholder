#pragma once

#include "wowlib/data_buffer.h"

#include <string>
#include <filesystem>
#include <optional>
#include <mutex>
#include <unordered_map>

namespace wowlib
{

/// Per-build file cache with SHA1 integrity verification.
/// Caches downloaded CASC files locally to avoid re-downloading.
class BuildCache
{
public:
    /// @param cacheDir Root cache directory
    /// @param buildKey Build key (MD5) used as subdirectory name
    explicit BuildCache(const std::filesystem::path& cacheDir, const std::string& buildKey);

    /// Initialize the cache directory and load integrity manifest.
    void init();

    /// Retrieve a cached file, verifying SHA1 integrity.
    /// @param key File key (encoding hash)
    /// @param subDir Subdirectory within the build cache (e.g., "data", "config")
    /// @return The file data, or std::nullopt if not cached or integrity check fails
    std::optional<DataBuffer> getFile(const std::string& key, const std::string& subDir = "data");

    /// Store a file in the cache.
    void storeFile(const std::string& key, const DataBuffer& data, const std::string& subDir = "data");

    /// Get the direct filesystem path for a cached file.
    std::filesystem::path getFilePath(const std::string& key, const std::string& subDir = "data") const;

private:
    void loadIntegrity();
    void saveIntegrity();

    std::filesystem::path m_cacheDir;
    std::string m_buildKey;
    std::filesystem::path m_buildDir;
    std::mutex m_mutex;
    std::unordered_map<std::string, std::string> m_integrity;
};

} // namespace wowlib
