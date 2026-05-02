#include "wowlib/wmo_loader.h"

#include <stdexcept>
#include <format>
#include <algorithm>

namespace
{

constexpr uint32_t CHUNK_MVER = 0x5245564D;
constexpr uint32_t CHUNK_MOHD = 0x44484F4D;
constexpr uint32_t CHUNK_MOTX = 0x58544F4D;
constexpr uint32_t CHUNK_MOMT = 0x544D4F4D;
constexpr uint32_t CHUNK_MOGI = 0x49474F4D;
constexpr uint32_t CHUNK_MODS = 0x53444F4D;
constexpr uint32_t CHUNK_MODI = 0x49444F4D;
constexpr uint32_t CHUNK_MODD = 0x44444F4D;
constexpr uint32_t CHUNK_GFID = 0x44494647;

// Group chunks
constexpr uint32_t CHUNK_MOGP = 0x50474F4D;
constexpr uint32_t CHUNK_MOVI = 0x49564F4D;
constexpr uint32_t CHUNK_MOVT = 0x54564F4D;
constexpr uint32_t CHUNK_MOTV = 0x56544F4D;
constexpr uint32_t CHUNK_MONR = 0x524E4F4D;
constexpr uint32_t CHUNK_MOBA = 0x41424F4D;
constexpr uint32_t CHUNK_MOCV = 0x56434F4D;

} // anonymous namespace

namespace wowlib
{

WmoLoader::WmoLoader(DataBuffer data)
    : m_buffer(std::move(data))
{
}

void WmoLoader::load()
{
    if (m_loaded)
        return;

    m_buffer.seek(0);

    while (m_buffer.remainingBytes() > 0)
    {
        const uint32_t chunkID = m_buffer.readUInt32LE();
        const uint32_t chunkSize = m_buffer.readUInt32LE();
        const size_t nextChunkPos = m_buffer.offset() + chunkSize;

        handleChunk(chunkID, chunkSize);

        m_buffer.seek(nextChunkPos);
    }

    m_loaded = true;
}

void WmoLoader::handleChunk(uint32_t chunkID, uint32_t chunkSize)
{
    switch (chunkID)
    {
        case CHUNK_MVER: m_data.version = m_buffer.readUInt32LE(); break;
        case CHUNK_MOHD: parseMOHD(); break;
        case CHUNK_MOTX: parseMOTX(chunkSize); break;
        case CHUNK_MOMT: parseMOMT(chunkSize); break;
        case CHUNK_MOGI: parseMOGI(chunkSize); break;
        case CHUNK_MODS: parseMODS(chunkSize); break;
        case CHUNK_MODI: parseMODI(chunkSize); break;
        case CHUNK_MODD: parseMODD(chunkSize); break;
        case CHUNK_GFID: parseGFID(chunkSize); break;
        default: break;
    }
}

void WmoLoader::parseMOHD()
{
    m_data.materialCount = m_buffer.readUInt32LE();
    m_data.groupCount = m_buffer.readUInt32LE();
    m_data.portalCount = m_buffer.readUInt32LE();
    m_data.lightCount = m_buffer.readUInt32LE();
    m_buffer.readUInt32LE(); // modelCount (unused)
    m_data.doodadCount = m_buffer.readUInt32LE();
    m_data.setCount = m_buffer.readUInt32LE();
    m_data.ambientColor = m_buffer.readUInt32LE();
    m_data.wmoID = m_buffer.readUInt32LE();
    auto bb1 = m_buffer.readFloatLE(3);
    auto bb2 = m_buffer.readFloatLE(3);
    std::copy(bb1.begin(), bb1.end(), m_data.boundingBox1);
    std::copy(bb2.begin(), bb2.end(), m_data.boundingBox2);
    m_data.flags = m_buffer.readUInt16LE();
    m_data.lodCount = m_buffer.readUInt16LE();
}

void WmoLoader::parseMOMT(uint32_t chunkSize)
{
    const uint32_t count = chunkSize / 64;
    m_data.materials.resize(count);

    for (uint32_t i = 0; i < count; i++)
    {
        WMOMaterial& mat = m_data.materials[i];
        mat.flags = m_buffer.readUInt32LE();
        mat.shader = m_buffer.readUInt32LE();
        mat.blendMode = m_buffer.readUInt32LE();
        mat.texture1 = m_buffer.readUInt32LE();
        mat.color1 = m_buffer.readUInt32LE();
        mat.color1b = m_buffer.readUInt32LE();
        mat.texture2 = m_buffer.readUInt32LE();
        mat.color2 = m_buffer.readUInt32LE();
        mat.groupType = m_buffer.readUInt32LE();
        mat.texture3 = m_buffer.readUInt32LE();
        mat.color3 = m_buffer.readUInt32LE();
        mat.flags3 = m_buffer.readUInt32LE();
        for (int j = 0; j < 4; j++)
            mat.runtimeData[j] = m_buffer.readUInt32LE();
    }
}

void WmoLoader::parseMOGI(uint32_t chunkSize)
{
    const uint32_t count = chunkSize / 32;
    m_data.groupInfo.resize(count);

    for (uint32_t i = 0; i < count; i++)
    {
        WMOGroupInfo& info = m_data.groupInfo[i];
        info.flags = m_buffer.readUInt32LE();
        auto bb1 = m_buffer.readFloatLE(3);
        auto bb2 = m_buffer.readFloatLE(3);
        std::copy(bb1.begin(), bb1.end(), info.boundingBox1);
        std::copy(bb2.begin(), bb2.end(), info.boundingBox2);
        info.nameIndex = m_buffer.readInt32LE();
    }
}

void WmoLoader::parseMODS(uint32_t chunkSize)
{
    const uint32_t count = chunkSize / 32;
    m_data.doodadSets.resize(count);

    for (uint32_t i = 0; i < count; i++)
    {
        WMODoodadSet& set = m_data.doodadSets[i];
        std::string raw = m_buffer.readString(20);
        raw.erase(std::remove(raw.begin(), raw.end(), '\0'), raw.end());
        set.name = raw;
        set.firstIndex = m_buffer.readUInt32LE();
        set.count = m_buffer.readUInt32LE();
        m_buffer.move(4); // unused
    }
}

void WmoLoader::parseMODI(uint32_t chunkSize)
{
    m_data.doodadFileIDs = m_buffer.readUInt32LE(chunkSize / 4);
}

void WmoLoader::parseMODD(uint32_t chunkSize)
{
    const uint32_t count = chunkSize / 40;
    m_data.doodads.resize(count);

    for (uint32_t i = 0; i < count; i++)
    {
        WMODoodad& d = m_data.doodads[i];
        d.nameIndex = m_buffer.readUInt24LE();
        d.flags = m_buffer.readUInt8();
        auto pos = m_buffer.readFloatLE(3);
        auto rot = m_buffer.readFloatLE(4);
        d.position[0] = pos[0]; d.position[1] = pos[1]; d.position[2] = pos[2];
        d.rotation[0] = rot[0]; d.rotation[1] = rot[1];
        d.rotation[2] = rot[2]; d.rotation[3] = rot[3];
        d.scale = m_buffer.readFloatLE();
        auto col = m_buffer.readUInt8(4);
        d.color[0] = col[0]; d.color[1] = col[1]; d.color[2] = col[2]; d.color[3] = col[3];
    }
}

void WmoLoader::parseGFID(uint32_t chunkSize)
{
    m_data.groupFileIDs = m_buffer.readUInt32LE(chunkSize / 4);
}

void WmoLoader::parseMOTX(uint32_t chunkSize)
{
    const size_t endOfs = m_buffer.offset() + chunkSize;
    uint32_t ofs = 0;

    while (m_buffer.offset() < endOfs)
    {
        uint32_t startOfs = static_cast<uint32_t>(m_buffer.offset() - (endOfs - chunkSize));
        std::string name = m_buffer.readNullTerminatedString();
        if (!name.empty())
            m_data.textureNames[startOfs] = name;
    }
}

// --- Group file parsing ---

WMOGroupData WmoLoader::loadGroup(DataBuffer data)
{
    WMOGroupData group;
    data.seek(0);

    while (data.remainingBytes() > 0)
    {
        const uint32_t chunkID = data.readUInt32LE();
        const uint32_t chunkSize = data.readUInt32LE();
        const size_t nextChunkPos = data.offset() + chunkSize;

        switch (chunkID)
        {
            case CHUNK_MVER:
                group.version = data.readUInt32LE();
                break;

            case CHUNK_MOGP:
            {
                const size_t endOfs = data.offset() + chunkSize;
                data.move(4); // nameOfs
                data.move(4); // descOfs
                group.groupFlags = data.readUInt32LE();
                auto bb1 = data.readFloatLE(3);
                auto bb2 = data.readFloatLE(3);
                std::copy(bb1.begin(), bb1.end(), group.boundingBox1);
                std::copy(bb2.begin(), bb2.end(), group.boundingBox2);
                data.move(4 + 4 + 4 + 4 + 4 + 8); // portal info, batch counts, fog, liquid, groupID, unknown

                // Parse sub-chunks within MOGP
                while (data.offset() < endOfs)
                {
                    const uint32_t subID = data.readUInt32LE();
                    const uint32_t subSize = data.readUInt32LE();
                    const size_t subNext = data.offset() + subSize;

                    switch (subID)
                    {
                        case CHUNK_MOVI:
                            group.indices = data.readUInt16LE(subSize / 2);
                            break;

                        case CHUNK_MOVT:
                        {
                            const uint32_t floatCount = subSize / 4;
                            group.vertices.resize(floatCount);
                            for (uint32_t i = 0; i < floatCount; i += 3)
                            {
                                group.vertices[i]     = data.readFloatLE();
                                group.vertices[i + 2] = data.readFloatLE() * -1;
                                group.vertices[i + 1] = data.readFloatLE();
                            }
                            break;
                        }

                        case CHUNK_MOTV:
                        {
                            const uint32_t floatCount = subSize / 4;
                            group.uvCoords.resize(floatCount);
                            for (uint32_t i = 0; i < floatCount; i++)
                                group.uvCoords[i] = data.readFloatLE();
                            break;
                        }

                        case CHUNK_MONR:
                        {
                            const uint32_t floatCount = subSize / 4;
                            group.normals.resize(floatCount);
                            for (uint32_t i = 0; i < floatCount; i += 3)
                            {
                                group.normals[i]     = data.readFloatLE();
                                group.normals[i + 2] = data.readFloatLE() * -1;
                                group.normals[i + 1] = data.readFloatLE();
                            }
                            break;
                        }

                        case CHUNK_MOBA:
                        {
                            const uint32_t count = subSize / 24;
                            group.renderBatches.resize(count);
                            for (uint32_t i = 0; i < count; i++)
                            {
                                WMORenderBatch& rb = group.renderBatches[i];
                                for (int j = 0; j < 3; j++)
                                    rb.possibleBox1[j] = data.readUInt16LE();
                                for (int j = 0; j < 3; j++)
                                    rb.possibleBox2[j] = data.readUInt16LE();
                                rb.firstFace = data.readUInt32LE();
                                rb.numFaces = data.readUInt16LE();
                                rb.firstVertex = data.readUInt16LE();
                                rb.lastVertex = data.readUInt16LE();
                                rb.flags = data.readUInt8();
                                rb.materialID = data.readUInt8();
                            }
                            break;
                        }

                        case CHUNK_MOCV:
                            group.vertexColors = data.readUInt8(subSize);
                            break;
                    }

                    data.seek(subNext);
                }
                break;
            }
        }

        data.seek(nextChunkPos);
    }

    return group;
}

} // namespace wowlib
