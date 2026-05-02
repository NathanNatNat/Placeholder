#include "wowlib/build_cache.h"

#include <nlohmann/json.hpp>
#include <fstream>

namespace wowlib
{

BuildCache::BuildCache(const std::filesystem::path& cacheDir, const std::string& buildKey)
    : m_cacheDir(cacheDir), m_buildKey(buildKey)
{
    m_buildDir = m_cacheDir / buildKey;
}

void BuildCache::init()
{
    std::filesystem::create_directories(m_buildDir);
    loadIntegrity();
}

std::optional<DataBuffer> BuildCache::getFile(const std::string& key, const std::string& subDir)
{
    auto path = getFilePath(key, subDir);
    if (!std::filesystem::exists(path))
        return std::nullopt;

    try
    {
        DataBuffer data = DataBuffer::readFile(path);

        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_integrity.find(key);
        if (it != m_integrity.end())
        {
            std::string hash = data.calculateHash("sha1");
            if (hash != it->second)
                return std::nullopt;
        }

        return data;
    }
    catch (...)
    {
        return std::nullopt;
    }
}

void BuildCache::storeFile(const std::string& key, const DataBuffer& data, const std::string& subDir)
{
    auto path = getFilePath(key, subDir);
    std::filesystem::create_directories(path.parent_path());

    DataBuffer copy = DataBuffer::from(data.raw());
    std::string hash = copy.calculateHash("sha1");

    {
        std::ofstream ofs(path, std::ios::binary);
        if (ofs)
            ofs.write(reinterpret_cast<const char*>(data.raw().data()),
                      static_cast<std::streamsize>(data.raw().size()));
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_integrity[key] = hash;
    }

    saveIntegrity();
}

std::filesystem::path BuildCache::getFilePath(const std::string& key, const std::string& subDir) const
{
    return m_buildDir / subDir / key;
}

void BuildCache::loadIntegrity()
{
    auto path = m_buildDir / "integrity.json";
    if (!std::filesystem::exists(path))
        return;

    try
    {
        std::ifstream ifs(path);
        nlohmann::json j = nlohmann::json::parse(ifs, nullptr, false);
        if (j.is_discarded() || !j.is_object())
            return;

        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto it = j.begin(); it != j.end(); ++it)
            m_integrity[it.key()] = it.value().get<std::string>();
    }
    catch (...)
    {
    }
}

void BuildCache::saveIntegrity()
{
    auto path = m_buildDir / "integrity.json";
    nlohmann::json j;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto& [key, hash] : m_integrity)
            j[key] = hash;
    }

    std::ofstream ofs(path);
    if (ofs)
        ofs << j.dump(1, '\t');
}

} // namespace wowlib
