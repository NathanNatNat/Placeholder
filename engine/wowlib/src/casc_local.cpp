#include "wowlib/casc_local.h"
#include "wowlib/cdn_config.h"
#include "wowlib/blte_reader.h"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <format>
#include <stdexcept>
#include <filesystem>

namespace fs = std::filesystem;

namespace
{

std::vector<std::unordered_map<std::string, std::string>> parseVersionConfig(std::string_view data)
{
    std::vector<std::unordered_map<std::string, std::string>> entries;

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

    if (lines.empty())
        return entries;

    std::vector<std::string> fields;
    {
        size_t pos = 0;
        while (pos < lines[0].size())
        {
            size_t pipe = lines[0].find('|', pos);
            if (pipe == std::string_view::npos)
                pipe = lines[0].size();

            std::string_view header = lines[0].substr(pos, pipe - pos);
            size_t bang = header.find('!');
            if (bang != std::string_view::npos)
                header = header.substr(0, bang);

            std::string field(header);
            auto spacePos = field.find(' ');
            if (spacePos != std::string::npos)
                field.erase(spacePos, 1);
            fields.push_back(std::move(field));

            pos = pipe + 1;
        }
    }

    for (size_t li = 1; li < lines.size(); li++)
    {
        std::string_view entry = lines[li];
        std::string_view trimmed = entry;
        while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.front())))
            trimmed.remove_prefix(1);
        if (trimmed.empty() || entry.front() == '#')
            continue;

        std::unordered_map<std::string, std::string> node;
        size_t pos = 0;
        size_t fi = 0;
        while (pos <= entry.size())
        {
            size_t pipe = entry.find('|', pos);
            if (pipe == std::string_view::npos)
                pipe = entry.size();

            const std::string key = fi < fields.size() ? fields[fi] : "undefined";
            node[key] = std::string(entry.substr(pos, pipe - pos));
            fi++;
            pos = pipe + 1;
        }
        entries.push_back(std::move(node));
    }

    return entries;
}

} // anonymous namespace

namespace wowlib
{

CascLocal::CascLocal(const fs::path& installDir, TactKeys& keys,
                       const fs::path& cacheDir, uint32_t locale)
    : CascSource(keys, locale), m_installDir(installDir), m_cacheDir(cacheDir)
{
    m_dataDir = m_installDir / "Data";
    m_storageDir = m_dataDir / "data";
}

void CascLocal::init()
{
    auto buildInfoPath = m_installDir / ".build.info";
    if (!fs::exists(buildInfoPath))
    {
        buildInfoPath = m_installDir / "_retail_" / ".build.info";
        if (!fs::exists(buildInfoPath))
            throw std::runtime_error("Cannot find .build.info in " + m_installDir.string());
        m_dataDir = m_installDir / "_retail_" / "Data";
        m_storageDir = m_dataDir / "data";
    }

    std::ifstream ifs(buildInfoPath);
    if (!ifs.is_open())
        throw std::runtime_error("Failed to open .build.info: " + buildInfoPath.string());

    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    m_builds = parseVersionConfig(content);

    if (m_builds.empty())
        throw std::runtime_error("No builds found in .build.info");
}

std::vector<std::string> CascLocal::getBuildList() const
{
    std::vector<std::string> list;
    for (const auto& build : m_builds)
    {
        auto versionIt = build.find("Version");
        auto productIt = build.find("Product");
        std::string label;
        if (productIt != build.end())
            label = productIt->second;
        if (versionIt != build.end())
            label += " " + versionIt->second;
        list.push_back(label);
    }
    return list;
}

void CascLocal::load(int buildIndex)
{
    if (buildIndex < 0 || buildIndex >= static_cast<int>(m_builds.size()))
        throw std::runtime_error("Invalid build index: " + std::to_string(buildIndex));

    m_activeBuild = m_builds[buildIndex];

    if (!m_cacheDir.empty())
    {
        auto buildKeyIt = m_activeBuild.find("BuildKey");
        std::string buildKey = buildKeyIt != m_activeBuild.end() ? buildKeyIt->second : "default";
        m_cache = std::make_unique<BuildCache>(m_cacheDir, buildKey);
        m_cache->init();
    }

    // Load config files
    auto buildKeyIt = m_activeBuild.find("BuildKey");
    if (buildKeyIt != m_activeBuild.end())
    {
        auto configPath = formatConfigPath(buildKeyIt->second);
        if (fs::exists(configPath))
        {
            std::ifstream ifs(configPath);
            std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            buildConfig = parseCdnConfig(content);
        }
    }

    auto cdnKeyIt = m_activeBuild.find("CDNKey");
    if (cdnKeyIt != m_activeBuild.end())
    {
        auto configPath = formatConfigPath(cdnKeyIt->second);
        if (fs::exists(configPath))
        {
            std::ifstream ifs(configPath);
            std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            cdnConfig = parseCdnConfig(content);
        }
    }

    loadIndexes();
    loadEncoding();
    loadRoot();
}

BLTEReader CascLocal::getFileAsBLTE(uint32_t fileDataID, bool partialDecrypt)
{
    const std::string encodingKey = getFile(fileDataID);
    DataBuffer data = getDataFile(encodingKey);
    return BLTEReader(std::move(data), encodingKey, m_keys, partialDecrypt);
}

DataBuffer CascLocal::getDataFile(const std::string& key)
{
    auto entryIt = m_localIndexes.find(key.substr(0, 18));
    if (entryIt == m_localIndexes.end())
        throw std::runtime_error("File not in local data: " + key);

    const auto& entry = entryIt->second;
    std::string dataPath = formatDataPath(entry.index);

    std::ifstream ifs(dataPath, std::ios::binary);
    if (!ifs)
        throw std::runtime_error("Failed to open data archive: " + dataPath);

    // Skip 0x1E byte header in archive entries
    int32_t readOffset = entry.offset + 0x1E;
    int32_t readSize = entry.size - 0x1E;
    if (readSize <= 0)
        throw std::runtime_error("Invalid entry size for key: " + key);

    ifs.seekg(readOffset);
    std::vector<uint8_t> buf(readSize);
    if (!ifs.read(reinterpret_cast<char*>(buf.data()), readSize))
        throw std::runtime_error("Failed to read data for key: " + key);

    DataBuffer data(std::move(buf));

    // Verify the data isn't entirely zeroed
    if (data.isZeroed())
        throw std::runtime_error("Data file is empty/missing: " + key);

    return data;
}

void CascLocal::parseIndex(const fs::path& file)
{
    DataBuffer index = DataBuffer::readFile(file);

    const int32_t headerHashSize = index.readInt32LE();
    index.move(4); // headerHash uint32
    index.move(headerHashSize); // headerHash bytes

    // Align to next 0x10 boundary
    index.seek((8 + headerHashSize + 0x0F) & 0xFFFFFFF0);

    const int32_t dataLength = index.readInt32LE();
    index.move(4);

    const int32_t nBlocks = dataLength / 18;
    for (int32_t i = 0; i < nBlocks; i++)
    {
        const std::string key = index.readHexString(9);
        if (m_localIndexes.count(key))
        {
            index.move(1 + 4 + 4);
            continue;
        }

        const uint8_t idxHigh = index.readUInt8();
        const int32_t idxLow = index.readInt32BE();

        m_localIndexes[key] = {
            static_cast<int32_t>((idxHigh << 2) | ((static_cast<uint32_t>(idxLow) & 0xC0000000) >> 30)),
            idxLow & 0x3FFFFFFF,
            index.readInt32LE()
        };
    }
}

void CascLocal::loadIndexes()
{
    m_localIndexes.reserve(2'500'000);
    for (const auto& entry : fs::directory_iterator(m_storageDir))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".idx")
            parseIndex(entry.path());
    }
}

void CascLocal::loadEncoding()
{
    auto encIt = buildConfig.find("encoding");
    if (encIt == buildConfig.end())
        throw std::runtime_error("No encoding key in build config");

    const std::string& encStr = encIt->second;
    std::string encKey;
    auto spacePos = encStr.find(' ');
    encKey = (spacePos != std::string::npos) ? encStr.substr(spacePos + 1) : encStr;

    DataBuffer encRaw = getDataFile(encKey);
    parseEncodingFile(std::move(encRaw), encKey);
}

void CascLocal::loadRoot()
{
    auto rootIt = buildConfig.find("root");
    if (rootIt == buildConfig.end())
        throw std::runtime_error("No root key in build config");

    auto rootEncIt = encodingKeys.find(rootIt->second);
    if (rootEncIt == encodingKeys.end())
        throw std::runtime_error("No encoding entry for root key");

    const std::string& rootKey = rootEncIt->second;
    DataBuffer rootData = getDataFile(rootKey);
    parseRootFile(std::move(rootData), rootKey);
}

std::string CascLocal::formatDataPath(int32_t id) const
{
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(3) << id;
    return (m_storageDir / ("data." + oss.str())).string();
}

fs::path CascLocal::formatConfigPath(const std::string& key) const
{
    return m_dataDir / "config" / key.substr(0, 2) / key.substr(2, 2) / key;
}

} // namespace wowlib
