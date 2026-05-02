#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <filesystem>
#include <unordered_map>

namespace wowlib
{

/// Manages TACT encryption keys used for BLTE encrypted block decryption.
/// Keys are loaded from a local JSON cache file and optionally from a remote URL.
class TactKeys
{
public:
    /// Look up a decryption key by its 16-char hex name.
    std::optional<std::string> getKey(std::string_view keyName) const;

    /// Register a key. Returns false if the pair is invalid.
    bool addKey(std::string_view keyName, std::string_view key);

    /// Load keys from a local JSON file (keyName -> key hex pairs).
    void loadFromFile(const std::filesystem::path& file);

    /// Save current key ring to a JSON file.
    void saveToFile(const std::filesystem::path& file) const;

    /// Load keys from raw text data (space-separated keyName key lines).
    void loadFromText(std::string_view text);

    /// Number of keys in the ring.
    size_t size() const;

private:
    std::unordered_map<std::string, std::string> m_keyRing;
};

} // namespace wowlib
