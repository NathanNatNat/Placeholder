#pragma once

#include <nlohmann/json.hpp>
#include <filesystem>
#include <string>
#include <vector>

namespace placeholder::core
{

/// Engine configuration backed by a JSON file with command-line overrides.
///
/// Values are accessed via dot-separated key paths (e.g. "window.width").
/// Command-line arguments in the form --key=value override file values.
class Config
{
public:
    /// Load configuration from a JSON file.
    /// @param path Filesystem path to the JSON config file.
    /// @return true if the file was loaded successfully.
    bool loadFromFile(const std::filesystem::path& path);

    /// Save the current configuration to a JSON file.
    /// @param path Filesystem path to write. Uses the original load path if empty.
    /// @return true if the file was saved successfully.
    bool saveToFile(const std::filesystem::path& path = {}) const;

    /// Parse command-line arguments and override matching config values.
    /// Accepts arguments in the form --key=value where key is a dot-separated path.
    void applyCommandLine(int argc, char* argv[]);

    /// Get a value by dot-separated key path.
    /// @return The value, or the provided default if the key doesn't exist.
    template<typename T>
    T get(const std::string& keyPath, const T& defaultValue = T{}) const;

    /// Set a value by dot-separated key path.
    template<typename T>
    void set(const std::string& keyPath, const T& value);

    /// Check whether a key path exists.
    bool has(const std::string& keyPath) const;

    /// Access the underlying JSON object.
    const nlohmann::json& raw() const { return m_data; }

private:
    /// Navigate to a nested JSON value by dot-separated key path.
    const nlohmann::json* resolve(const std::string& keyPath) const;

    /// Navigate to (or create) a nested JSON value by dot-separated key path.
    nlohmann::json& resolveOrCreate(const std::string& keyPath);

    nlohmann::json m_data;
    std::filesystem::path m_filePath;
};

// --- Template implementations ---

template<typename T>
T Config::get(const std::string& keyPath, const T& defaultValue) const
{
    const auto* node = resolve(keyPath);
    if (!node || node->is_null())
    {
        return defaultValue;
    }
    try
    {
        return node->get<T>();
    }
    catch (const nlohmann::json::exception&)
    {
        return defaultValue;
    }
}

template<typename T>
void Config::set(const std::string& keyPath, const T& value)
{
    resolveOrCreate(keyPath) = value;
}

} // namespace placeholder::core
