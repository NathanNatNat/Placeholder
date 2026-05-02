#include "wowlib/cdn_config.h"

#include <cctype>
#include <stdexcept>
#include <vector>

namespace
{

std::string normalizeKey(const std::string& key)
{
    std::vector<std::string> parts;
    size_t start = 0;
    for (size_t i = 0; i <= key.size(); i++)
    {
        if (i == key.size() || key[i] == '-')
        {
            parts.push_back(key.substr(start, i - start));
            start = i + 1;
        }
    }

    if (parts.size() == 1)
        return key;

    for (size_t i = 1; i < parts.size(); i++)
    {
        if (!parts[i].empty())
            parts[i][0] = static_cast<char>(std::toupper(static_cast<unsigned char>(parts[i][0])));
    }

    std::string result;
    for (const auto& part : parts)
        result += part;
    return result;
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

std::unordered_map<std::string, std::string> parseCdnConfig(std::string_view data)
{
    std::unordered_map<std::string, std::string> entries;

    std::vector<std::string_view> lines;
    size_t start = 0;
    for (size_t i = 0; i < data.size(); i++)
    {
        if (data[i] == '\n')
        {
            std::string_view line = data.substr(start, i - start);
            if (!line.empty() && line.back() == '\r')
                line.remove_suffix(1);
            lines.push_back(line);
            start = i + 1;
        }
    }
    if (start <= data.size())
    {
        std::string_view line = data.substr(start);
        if (!line.empty() && line.back() == '\r')
            line.remove_suffix(1);
        lines.push_back(line);
    }

    bool hasValidHeader = !lines.empty() && trim(lines[0]).substr(0, 2) == "# ";
    if (!hasValidHeader)
        throw std::runtime_error("Invalid CDN config: unexpected start of config");

    for (const auto& lineView : lines)
    {
        if (trim(lineView).empty() || lineView.front() == '#')
            continue;

        std::string line(lineView);
        auto eqPos = line.find('=');
        if (eqPos == std::string::npos)
            throw std::runtime_error("Invalid token in CDN config");

        std::string key(trim(std::string_view(line).substr(0, eqPos)));
        std::string value(trim(std::string_view(line).substr(eqPos + 1)));
        entries[normalizeKey(key)] = value;
    }

    return entries;
}

} // namespace wowlib
