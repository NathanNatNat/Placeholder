#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace placeholder::core
{

/// Read an entire file as a byte buffer.
std::optional<std::vector<uint8_t>> readFileBinary(const std::filesystem::path& path);

/// Read an entire file as a UTF-8 string.
std::optional<std::string> readFileText(const std::filesystem::path& path);

/// Write a byte buffer to a file (creates or overwrites).
bool writeFileBinary(const std::filesystem::path& path, const void* data, size_t size);

/// Write a string to a file (creates or overwrites).
bool writeFileText(const std::filesystem::path& path, const std::string& text);

/// Normalize a path (resolve `.`, `..`, convert separators).
std::filesystem::path normalizePath(const std::filesystem::path& path);

/// Get the file extension in lowercase without the leading dot.
std::string getExtension(const std::filesystem::path& path);

} // namespace placeholder::core
