#include "core/file_io.h"
#include "core/logging.h"

#include <algorithm>
#include <fstream>

namespace placeholder::core
{

std::optional<std::vector<uint8_t>> readFileBinary(const std::filesystem::path& path)
{
    auto log = getLogger("core");

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        log->error("Failed to open file for binary read: {}", path.string());
        return std::nullopt;
    }

    auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
    {
        log->error("Failed to read file: {}", path.string());
        return std::nullopt;
    }

    return buffer;
}

std::optional<std::string> readFileText(const std::filesystem::path& path)
{
    auto log = getLogger("core");

    std::ifstream file(path);
    if (!file.is_open())
    {
        log->error("Failed to open file for text read: {}", path.string());
        return std::nullopt;
    }

    std::string content(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    return content;
}

bool writeFileBinary(const std::filesystem::path& path, const void* data, size_t size)
{
    auto log = getLogger("core");

    std::filesystem::create_directories(path.parent_path());

    std::ofstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        log->error("Failed to open file for binary write: {}", path.string());
        return false;
    }

    file.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
    return file.good();
}

bool writeFileText(const std::filesystem::path& path, const std::string& text)
{
    return writeFileBinary(path, text.data(), text.size());
}

std::filesystem::path normalizePath(const std::filesystem::path& path)
{
    return std::filesystem::weakly_canonical(path);
}

std::string getExtension(const std::filesystem::path& path)
{
    auto ext = path.extension().string();
    if (!ext.empty() && ext[0] == '.')
    {
        ext = ext.substr(1);
    }
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
}

} // namespace placeholder::core
