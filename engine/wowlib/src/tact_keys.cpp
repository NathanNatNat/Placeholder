#include "wowlib/tact_keys.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm>
#include <unordered_map>

namespace
{

std::string toLower(std::string_view sv)
{
    std::string s(sv);
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

bool validateKeyPair(std::string_view keyName, std::string_view key)
{
    return keyName.length() == 16 && key.length() == 32;
}

std::string_view trim(std::string_view sv)
{
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front())))
        sv.remove_prefix(1);
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back())))
        sv.remove_suffix(1);
    return sv;
}

} // anonymous namespace

namespace wowlib
{

std::optional<std::string> TactKeys::getKey(std::string_view keyName) const
{
    std::string lower = toLower(keyName);
    auto it = m_keyRing.find(lower);
    if (it != m_keyRing.end())
        return it->second;
    return std::nullopt;
}

bool TactKeys::addKey(std::string_view keyName, std::string_view key)
{
    if (!validateKeyPair(keyName, key))
        return false;

    std::string lowerName = toLower(keyName);
    std::string lowerKey = toLower(key);
    m_keyRing[lowerName] = lowerKey;
    return true;
}

void TactKeys::loadFromFile(const std::filesystem::path& file)
{
    std::ifstream ifs(file);
    if (!ifs.is_open())
        return;

    nlohmann::json j = nlohmann::json::parse(ifs, nullptr, false);
    if (j.is_discarded())
        return;

    for (auto it = j.begin(); it != j.end(); ++it)
    {
        const std::string& keyName = it.key();
        const std::string key = it.value().get<std::string>();
        if (validateKeyPair(keyName, key))
            m_keyRing[toLower(keyName)] = toLower(key);
    }
}

void TactKeys::saveToFile(const std::filesystem::path& file) const
{
    std::filesystem::create_directories(file.parent_path());
    nlohmann::json j = nlohmann::json::object();
    for (const auto& [name, value] : m_keyRing)
        j[name] = value;

    std::ofstream ofs(file);
    if (ofs.is_open())
        ofs << j.dump(1, '\t');
}

void TactKeys::loadFromText(std::string_view text)
{
    size_t start = 0;
    while (start < text.size())
    {
        size_t lineEnd = text.find('\n', start);
        if (lineEnd == std::string_view::npos)
            lineEnd = text.size();

        std::string_view line = text.substr(start, lineEnd - start);
        if (!line.empty() && line.back() == '\r')
            line.remove_suffix(1);

        auto spacePos = line.find(' ');
        if (spacePos != std::string_view::npos)
        {
            std::string_view keyName = trim(line.substr(0, spacePos));
            std::string_view key = trim(line.substr(spacePos + 1));
            if (key.find(' ') == std::string_view::npos && validateKeyPair(keyName, key))
                m_keyRing[toLower(keyName)] = toLower(key);
        }

        start = lineEnd + 1;
    }
}

size_t TactKeys::size() const
{
    return m_keyRing.size();
}

} // namespace wowlib
