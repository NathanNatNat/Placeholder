#include "wowlib/casc_source.h"

#include <stdexcept>
#include <format>

namespace wowlib
{

CascSource::CascSource(TactKeys& keys, uint32_t locale)
    : m_keys(keys), m_locale(locale)
{
}

CascSource::~CascSource() = default;

bool CascSource::fileExists(uint32_t fileDataID) const
{
    auto rootIt = rootEntries.find(fileDataID);
    if (rootIt == rootEntries.end())
        return false;

    for (const auto& [rootTypeIdx, key] : rootIt->second)
    {
        const auto& rootType = rootTypes[rootTypeIdx];
        if ((rootType.localeFlags & m_locale) &&
            ((rootType.contentFlags & content_flags::LOW_VIOLENCE) == 0))
            return true;
    }
    return false;
}

std::string CascSource::getFile(uint32_t fileDataID) const
{
    auto rootIt = rootEntries.find(fileDataID);
    if (rootIt == rootEntries.end())
        throw std::runtime_error("fileDataID not in root: " + std::to_string(fileDataID));

    std::string contentKey;
    bool found = false;
    for (const auto& [rootTypeIdx, key] : rootIt->second)
    {
        const auto& rootType = rootTypes[rootTypeIdx];
        if ((rootType.localeFlags & m_locale) &&
            ((rootType.contentFlags & content_flags::LOW_VIOLENCE) == 0))
        {
            contentKey = key;
            found = true;
            break;
        }
    }

    if (!found)
        throw std::runtime_error("No root entry for locale: " + std::to_string(m_locale));

    auto encIt = encodingKeys.find(contentKey);
    if (encIt == encodingKeys.end())
        throw std::runtime_error("No encoding entry for content key: " + contentKey);

    return encIt->second;
}

std::vector<uint32_t> CascSource::getValidRootEntries() const
{
    std::vector<uint32_t> entries;
    for (const auto& [fileDataID, entry] : rootEntries)
    {
        for (const auto& [rootTypeIdx, key] : entry)
        {
            const auto& rootType = rootTypes[rootTypeIdx];
            if ((rootType.localeFlags & m_locale) &&
                ((rootType.contentFlags & content_flags::LOW_VIOLENCE) == 0))
            {
                entries.push_back(fileDataID);
                break;
            }
        }
    }
    return entries;
}

size_t CascSource::parseRootFile(DataBuffer data, const std::string& hash)
{
    BLTEReader root(std::move(data), hash, m_keys);

    const uint32_t magic = root.readUInt32LE();
    rootEntries.reserve(2'000'000);

    if (magic == ROOT_MAGIC)
    {
        // Modern format (8.2+)
        uint32_t headerSize = root.readUInt32LE();
        uint32_t version = root.readUInt32LE();

        if (headerSize != 0x18)
        {
            version = 0;
        }
        else
        {
            if (version != 1 && version != 2)
                throw std::runtime_error("Unknown root version: " + std::to_string(version));
        }

        uint32_t totalFileCount;
        uint32_t namedFileCount;

        if (version == 0)
        {
            totalFileCount = headerSize;
            namedFileCount = version;
            headerSize = 12;
        }
        else
        {
            totalFileCount = root.readUInt32LE();
            namedFileCount = root.readUInt32LE();
        }

        root.seek(headerSize);
        const bool allowNamelessFiles = totalFileCount != namedFileCount;

        while (root.remainingBytes() > 0)
        {
            const uint32_t numRecords = root.readUInt32LE();

            uint32_t contentFlags = 0;
            uint32_t localeFlags = 0;

            if (version == 0 || version == 1)
            {
                contentFlags = root.readUInt32LE();
                localeFlags = root.readUInt32LE();
            }
            else if (version == 2)
            {
                localeFlags = root.readUInt32LE();
                const uint32_t cflags1 = root.readUInt32LE();
                const uint32_t cflags2 = root.readUInt32LE();
                const uint8_t cflags3 = root.readUInt8();
                contentFlags = cflags1 | cflags2 | (static_cast<uint32_t>(cflags3) << 17);
            }

            std::vector<int32_t> fileDataIDs(numRecords);
            int32_t fileDataID = 0;
            for (uint32_t i = 0; i < numRecords; i++)
            {
                const int32_t nextID = fileDataID + root.readInt32LE();
                fileDataIDs[i] = nextID;
                fileDataID = nextID + 1;
            }

            for (uint32_t i = 0; i < numRecords; i++)
            {
                const uint32_t fdid = static_cast<uint32_t>(fileDataIDs[i]);
                rootEntries[fdid][rootTypes.size()] = root.readHexString(16);
            }

            if (!(allowNamelessFiles && (contentFlags & content_flags::NO_NAME_HASH)))
                root.move(8 * numRecords);

            rootTypes.push_back({ contentFlags, localeFlags });
        }
    }
    else
    {
        // Classic format
        root.seek(0);
        while (root.remainingBytes() > 0)
        {
            const uint32_t numRecords = root.readUInt32LE();
            const uint32_t contentFlags = root.readUInt32LE();
            const uint32_t localeFlags = root.readUInt32LE();

            std::vector<int32_t> fileDataIDs(numRecords);
            int32_t fileDataID = 0;
            for (uint32_t i = 0; i < numRecords; i++)
            {
                const int32_t nextID = fileDataID + root.readInt32LE();
                fileDataIDs[i] = nextID;
                fileDataID = nextID + 1;
            }

            for (uint32_t i = 0; i < numRecords; i++)
            {
                const std::string key = root.readHexString(16);
                root.move(8); // lookup hash
                const uint32_t fdid = static_cast<uint32_t>(fileDataIDs[i]);
                rootEntries[fdid][rootTypes.size()] = key;
            }

            rootTypes.push_back({ contentFlags, localeFlags });
        }
    }

    return rootEntries.size();
}

void CascSource::parseEncodingFile(DataBuffer data, const std::string& hash)
{
    BLTEReader encoding(std::move(data), hash, m_keys);

    const uint16_t magic = encoding.readUInt16LE();
    if (magic != ENC_MAGIC)
        throw std::runtime_error("Invalid encoding magic: " + std::to_string(magic));

    encoding.move(1); // version
    const uint8_t hashSizeCKey = encoding.readUInt8();
    const uint8_t hashSizeEKey = encoding.readUInt8();
    const int32_t cKeyPageSize = encoding.readInt16BE() * 1024;
    encoding.move(2); // eKeyPageSize
    const int32_t cKeyPageCount = encoding.readInt32BE();
    encoding.move(4 + 1); // eKeyPageCount + unk11
    const int32_t specBlockSize = encoding.readInt32BE();

    encoding.move(specBlockSize + (cKeyPageCount * (hashSizeCKey + 16)));

    encodingKeys.reserve(3'000'000);
    encodingSizes.reserve(3'000'000);

    const size_t pagesStart = encoding.offset();
    for (int32_t i = 0; i < cKeyPageCount; i++)
    {
        const size_t pageStart = pagesStart + (static_cast<size_t>(cKeyPageSize) * i);
        encoding.seek(pageStart);

        while (encoding.offset() < (pageStart + static_cast<size_t>(cKeyPageSize)))
        {
            const uint8_t keysCount = encoding.readUInt8();
            if (keysCount == 0)
                break;

            const int64_t size = encoding.readInt40BE();
            const std::string cKey = encoding.readHexString(hashSizeCKey);

            encodingSizes[cKey] = size;
            encodingKeys[cKey] = encoding.readHexString(hashSizeEKey);

            encoding.move(hashSizeEKey * (keysCount - 1));
        }
    }
}

std::string CascSource::formatCdnKey(const std::string& key)
{
    return key.substr(0, 2) + "/" + key.substr(2, 2) + "/" + key;
}

} // namespace wowlib
