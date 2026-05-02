#include "wowlib/m2_loader.h"

#include <stdexcept>
#include <format>

namespace wowlib
{

M2Loader::M2Loader(DataBuffer data)
    : m_buffer(std::move(data))
{
}

void M2Loader::load()
{
    if (m_loaded)
        return;

    m_buffer.seek(0);

    while (m_buffer.remainingBytes() > 0)
    {
        const uint32_t chunkID = m_buffer.readUInt32LE();
        const uint32_t chunkSize = m_buffer.readUInt32LE();
        const size_t nextChunkPos = m_buffer.offset() + chunkSize;

        switch (chunkID)
        {
            case CHUNK_MD21: parseMD21(); break;
            case CHUNK_SFID: parseSFID(chunkSize); break;
            case CHUNK_TXID: parseTXID(); break;
            case CHUNK_SKID: m_data.skeletonFileID = m_buffer.readUInt32LE(); break;
            case CHUNK_BFID: parseBFID(chunkSize); break;
            case CHUNK_AFID: parseAFID(chunkSize); break;
        }

        m_buffer.seek(nextChunkPos);
    }

    m_loaded = true;
}

void M2Loader::loadSkin(DataBuffer skinData, M2SkinData& skin)
{
    skinData.seek(0);

    const uint32_t magic = skinData.readUInt32LE();
    if (magic != SKIN_MAGIC)
        throw std::runtime_error(std::format("Invalid skin magic: 0x{:08X}", magic));

    const uint32_t indicesCount = skinData.readUInt32LE();
    const uint32_t indicesOfs = skinData.readUInt32LE();
    const uint32_t trianglesCount = skinData.readUInt32LE();
    const uint32_t trianglesOfs = skinData.readUInt32LE();
    skinData.move(8); // properties count + offset
    const uint32_t subMeshCount = skinData.readUInt32LE();
    const uint32_t subMeshOfs = skinData.readUInt32LE();
    const uint32_t texUnitsCount = skinData.readUInt32LE();
    const uint32_t texUnitsOfs = skinData.readUInt32LE();
    skin.bones = skinData.readUInt32LE();

    // Indices
    skinData.seek(indicesOfs);
    skin.indices = skinData.readUInt16LE(indicesCount);

    // Triangles
    skinData.seek(trianglesOfs);
    skin.triangles = skinData.readUInt16LE(trianglesCount);

    // SubMeshes
    skinData.seek(subMeshOfs);
    skin.subMeshes.resize(subMeshCount);
    for (uint32_t i = 0; i < subMeshCount; i++)
    {
        M2SubMesh& sm = skin.subMeshes[i];
        sm.submeshID = skinData.readUInt16LE();
        sm.level = skinData.readUInt16LE();
        sm.vertexStart = skinData.readUInt32LE();
        sm.vertexCount = skinData.readUInt16LE();
        sm.triangleStart = skinData.readUInt32LE();
        sm.triangleCount = skinData.readUInt16LE();
        sm.boneCount = skinData.readUInt16LE();
        sm.boneStart = skinData.readUInt16LE();
        sm.boneInfluences = skinData.readUInt16LE();
        sm.centerBoneIndex = skinData.readUInt16LE();
        sm.centerPosition[0] = skinData.readFloatLE();
        sm.centerPosition[1] = skinData.readFloatLE();
        sm.centerPosition[2] = skinData.readFloatLE();
        sm.sortCenterPosition[0] = skinData.readFloatLE();
        sm.sortCenterPosition[1] = skinData.readFloatLE();
        sm.sortCenterPosition[2] = skinData.readFloatLE();
        sm.sortRadius = skinData.readFloatLE();

        // Combine level bits with triangle start
        sm.triangleStart += static_cast<uint32_t>(sm.level) << 16;
    }

    // Texture units
    skinData.seek(texUnitsOfs);
    skin.textureUnits.resize(texUnitsCount);
    for (uint32_t i = 0; i < texUnitsCount; i++)
    {
        M2TextureUnit& tu = skin.textureUnits[i];
        tu.flags = skinData.readUInt16LE();
        tu.priority = skinData.readUInt16LE();
        tu.shaderID = skinData.readUInt16LE();
        tu.skinSectionIndex = skinData.readUInt16LE();
        tu.geosetIndex = skinData.readUInt16LE();
        tu.colorIndex = skinData.readUInt16LE();
        tu.materialIndex = skinData.readUInt16LE();
        tu.materialLayer = skinData.readUInt16LE();
        tu.textureCount = skinData.readUInt16LE();
        tu.textureComboIndex = skinData.readUInt16LE();
        tu.textureCoordComboIndex = skinData.readUInt16LE();
        tu.textureWeightComboIndex = skinData.readUInt16LE();
        tu.textureTransformComboIndex = skinData.readUInt16LE();
    }

    skin.loaded = true;
}

// --- Outer chunk parsing ---

void M2Loader::parseSFID(uint32_t chunkSize)
{
    if (!m_md21Parsed)
        throw std::runtime_error("SFID chunk before MD21");

    const uint32_t totalEntries = chunkSize / 4;
    const uint32_t lodSkinCount = (totalEntries >= m_data.viewCount)
        ? (totalEntries - m_data.viewCount) : 0;

    m_data.skins.resize(m_data.viewCount);
    for (uint32_t i = 0; i < m_data.viewCount; i++)
        m_data.skins[i].fileDataID = m_buffer.readUInt32LE();

    m_data.lodSkins.resize(lodSkinCount);
    for (uint32_t i = 0; i < lodSkinCount; i++)
        m_data.lodSkins[i].fileDataID = m_buffer.readUInt32LE();
}

void M2Loader::parseTXID()
{
    if (!m_md21Parsed)
        throw std::runtime_error("TXID chunk before MD21");

    for (size_t i = 0; i < m_data.textures.size(); i++)
        m_data.textures[i].fileDataID = m_buffer.readUInt32LE();
}

void M2Loader::parseBFID(uint32_t chunkSize)
{
    m_data.boneFileIDs = m_buffer.readUInt32LE(chunkSize / 4);
}

void M2Loader::parseAFID(uint32_t chunkSize)
{
    const uint32_t entryCount = chunkSize / 8;
    m_data.animFileIDs.resize(entryCount);
    for (uint32_t i = 0; i < entryCount; i++)
    {
        m_data.animFileIDs[i].animID = m_buffer.readUInt16LE();
        m_data.animFileIDs[i].subAnimID = m_buffer.readUInt16LE();
        m_data.animFileIDs[i].fileDataID = m_buffer.readUInt32LE();
    }
}

// --- MD21 interior parsing ---

void M2Loader::parseMD21()
{
    const uint32_t ofs = static_cast<uint32_t>(m_buffer.offset());

    const uint32_t magic = m_buffer.readUInt32LE();
    if (magic != CHUNK_MD20)
        throw std::runtime_error(std::format("Invalid M2 magic: 0x{:08X}", magic));

    m_data.version = m_buffer.readUInt32LE();
    parseMD21_modelName(ofs);
    m_data.flags = m_buffer.readUInt32LE();
    parseMD21_globalLoops(ofs);
    parseMD21_animations(ofs);
    parseMD21_animationLookup(ofs);
    parseMD21_bones(ofs);
    m_buffer.move(8); // key bone lookup
    parseMD21_vertices(ofs);
    m_data.viewCount = m_buffer.readUInt32LE();
    parseMD21_colors(ofs);
    parseMD21_textures(ofs);
    m_buffer.move(8); // texture weights (animation tracks - Phase 9)
    m_buffer.move(8); // texture transforms (animation tracks - Phase 9)
    parseMD21_replaceableTextureLookup(ofs);
    parseMD21_materials(ofs);
    m_buffer.move(8); // bone combos
    parseMD21_textureCombos(ofs);
    m_buffer.move(8); // texture transform bone map
    parseMD21_transparencyLookup(ofs);
    parseMD21_textureTransformLookup(ofs);
    parseMD21_collision(ofs);

    m_md21Parsed = true;
}

void M2Loader::parseMD21_modelName(uint32_t ofs)
{
    const uint32_t nameLen = m_buffer.readUInt32LE();
    const uint32_t nameOfs = m_buffer.readUInt32LE();

    const size_t base = m_buffer.offset();
    m_buffer.seek(nameOfs + ofs);
    m_data.name = m_buffer.readString(nameLen > 0 ? nameLen - 1 : 0);
    m_buffer.seek(base);
}

void M2Loader::parseMD21_globalLoops(uint32_t ofs)
{
    const uint32_t count = m_buffer.readUInt32LE();
    const uint32_t dataOfs = m_buffer.readUInt32LE();

    const size_t base = m_buffer.offset();
    m_buffer.seek(dataOfs + ofs);
    m_data.globalLoops = m_buffer.readInt32LE(count);
    m_buffer.seek(base);
}

void M2Loader::parseMD21_animations(uint32_t ofs)
{
    const uint32_t count = m_buffer.readUInt32LE();
    const uint32_t dataOfs = m_buffer.readUInt32LE();

    const size_t base = m_buffer.offset();
    m_buffer.seek(dataOfs + ofs);

    m_data.animations.resize(count);
    for (uint32_t i = 0; i < count; i++)
    {
        M2Animation& a = m_data.animations[i];
        a.id = m_buffer.readUInt16LE();
        a.variationIndex = m_buffer.readUInt16LE();
        a.duration = m_buffer.readUInt32LE();
        a.moveSpeed = m_buffer.readFloatLE();
        a.flags = m_buffer.readUInt32LE();
        a.frequency = m_buffer.readInt16LE();
        a.padding = m_buffer.readUInt16LE();
        a.replayMin = m_buffer.readUInt32LE();
        a.replayMax = m_buffer.readUInt32LE();
        a.blendTimeIn = m_buffer.readUInt16LE();
        a.blendTimeOut = m_buffer.readUInt16LE();
        auto bmin = m_buffer.readFloatLE(3);
        auto bmax = m_buffer.readFloatLE(3);
        std::copy(bmin.begin(), bmin.end(), a.boxPosMin);
        std::copy(bmax.begin(), bmax.end(), a.boxPosMax);
        a.boxRadius = m_buffer.readFloatLE();
        a.variationNext = m_buffer.readInt16LE();
        a.aliasNext = m_buffer.readUInt16LE();
    }

    m_buffer.seek(base);
}

void M2Loader::parseMD21_animationLookup(uint32_t ofs)
{
    const uint32_t count = m_buffer.readUInt32LE();
    const uint32_t dataOfs = m_buffer.readUInt32LE();

    const size_t base = m_buffer.offset();
    m_buffer.seek(dataOfs + ofs);
    m_data.animationLookup = m_buffer.readInt16LE(count);
    m_buffer.seek(base);
}

void M2Loader::parseMD21_bones(uint32_t ofs)
{
    const uint32_t count = m_buffer.readUInt32LE();
    const uint32_t dataOfs = m_buffer.readUInt32LE();

    const size_t base = m_buffer.offset();
    m_buffer.seek(dataOfs + ofs);

    m_data.bones.resize(count);
    for (uint32_t i = 0; i < count; i++)
    {
        M2Bone& bone = m_data.bones[i];
        bone.boneID = m_buffer.readInt32LE();
        bone.flags = m_buffer.readUInt32LE();
        bone.parentBone = m_buffer.readInt16LE();
        bone.subMeshID = m_buffer.readUInt16LE();
        bone.boneNameCRC = m_buffer.readUInt32LE();

        // Skip animation tracks (3 tracks: translation, rotation, scale)
        // Each track header is: interpolation(2) + globalSeq(2) + timestamps(count4+ofs4) + values(count4+ofs4) = 20 bytes
        m_buffer.move(20 * 3);

        auto pivot = m_buffer.readFloatLE(3);
        // WoW Y-up to engine Y-up coordinate conversion
        bone.pivot[0] = pivot[0];
        bone.pivot[1] = pivot[2];
        bone.pivot[2] = pivot[1] * -1;
    }

    m_buffer.seek(base);
}

void M2Loader::parseMD21_vertices(uint32_t ofs)
{
    const uint32_t count = m_buffer.readUInt32LE();
    const uint32_t dataOfs = m_buffer.readUInt32LE();

    const size_t base = m_buffer.offset();
    m_buffer.seek(dataOfs + ofs);

    m_data.vertices.resize(count * 3);
    m_data.normals.resize(count * 3);
    m_data.uvCoords.resize(count * 2);
    m_data.uvCoords2.resize(count * 2);
    m_data.boneWeights.resize(count * 4);
    m_data.boneIndices.resize(count * 4);

    for (uint32_t i = 0; i < count; i++)
    {
        // Position (WoW Z-up to engine Y-up)
        float px = m_buffer.readFloatLE();
        float py = m_buffer.readFloatLE();
        float pz = m_buffer.readFloatLE();
        m_data.vertices[i * 3 + 0] = px;
        m_data.vertices[i * 3 + 1] = pz;
        m_data.vertices[i * 3 + 2] = py * -1;

        // Bone weights and indices
        for (int x = 0; x < 4; x++)
            m_data.boneWeights[i * 4 + x] = m_buffer.readUInt8();
        for (int x = 0; x < 4; x++)
            m_data.boneIndices[i * 4 + x] = m_buffer.readUInt8();

        // Normal (same coordinate conversion)
        float nx = m_buffer.readFloatLE();
        float ny = m_buffer.readFloatLE();
        float nz = m_buffer.readFloatLE();
        m_data.normals[i * 3 + 0] = nx;
        m_data.normals[i * 3 + 1] = nz;
        m_data.normals[i * 3 + 2] = ny * -1;

        // UV coordinates
        m_data.uvCoords[i * 2 + 0] = m_buffer.readFloatLE();
        m_data.uvCoords[i * 2 + 1] = m_buffer.readFloatLE();
        m_data.uvCoords2[i * 2 + 0] = m_buffer.readFloatLE();
        m_data.uvCoords2[i * 2 + 1] = m_buffer.readFloatLE();
    }

    m_buffer.seek(base);
}

void M2Loader::parseMD21_colors(uint32_t ofs)
{
    // Color animation tracks — deferred to Phase 9
    m_buffer.move(8);
}

void M2Loader::parseMD21_textures(uint32_t ofs)
{
    const uint32_t count = m_buffer.readUInt32LE();
    const uint32_t dataOfs = m_buffer.readUInt32LE();

    const size_t base = m_buffer.offset();
    m_buffer.seek(dataOfs + ofs);

    m_data.textures.resize(count);
    m_data.textureTypes.resize(count);

    for (uint32_t i = 0; i < count; i++)
    {
        m_data.textureTypes[i] = m_buffer.readUInt32LE();
        m_data.textures[i].type = m_data.textureTypes[i];
        m_data.textures[i].flags = m_buffer.readUInt32LE();

        const uint32_t nameLen = m_buffer.readUInt32LE();
        const uint32_t nameOfs = m_buffer.readUInt32LE();

        if (m_data.textureTypes[i] == 0 && nameOfs > 0 && nameLen > 0)
        {
            const size_t pos = m_buffer.offset();
            m_buffer.seek(nameOfs + ofs);
            m_data.textures[i].fileName = m_buffer.readString(nameLen);
            m_buffer.seek(pos);
        }
    }

    m_buffer.seek(base);
}

void M2Loader::parseMD21_materials(uint32_t ofs)
{
    const uint32_t count = m_buffer.readUInt32LE();
    const uint32_t dataOfs = m_buffer.readUInt32LE();

    const size_t base = m_buffer.offset();
    m_buffer.seek(dataOfs + ofs);

    m_data.materials.resize(count);
    for (uint32_t i = 0; i < count; i++)
    {
        m_data.materials[i].flags = m_buffer.readUInt16LE();
        m_data.materials[i].blendingMode = m_buffer.readUInt16LE();
    }

    m_buffer.seek(base);
}

void M2Loader::parseMD21_textureCombos(uint32_t ofs)
{
    const uint32_t count = m_buffer.readUInt32LE();
    const uint32_t dataOfs = m_buffer.readUInt32LE();

    const size_t base = m_buffer.offset();
    m_buffer.seek(dataOfs + ofs);
    m_data.textureCombos = m_buffer.readUInt16LE(count);
    m_buffer.seek(base);
}

void M2Loader::parseMD21_transparencyLookup(uint32_t ofs)
{
    const uint32_t count = m_buffer.readUInt32LE();
    const uint32_t dataOfs = m_buffer.readUInt32LE();

    const size_t base = m_buffer.offset();
    m_buffer.seek(dataOfs + ofs);
    m_data.transparencyLookup = m_buffer.readUInt16LE(count);
    m_buffer.seek(base);
}

void M2Loader::parseMD21_textureTransformLookup(uint32_t ofs)
{
    const uint32_t count = m_buffer.readUInt32LE();
    const uint32_t dataOfs = m_buffer.readUInt32LE();

    const size_t base = m_buffer.offset();
    m_buffer.seek(dataOfs + ofs);
    m_data.textureTransformsLookup = m_buffer.readUInt16LE(count);
    m_buffer.seek(base);
}

void M2Loader::parseMD21_replaceableTextureLookup(uint32_t ofs)
{
    const uint32_t count = m_buffer.readUInt32LE();
    const uint32_t dataOfs = m_buffer.readUInt32LE();

    const size_t base = m_buffer.offset();
    m_buffer.seek(dataOfs + ofs);
    m_data.replaceableTextureLookup = m_buffer.readInt16LE(count);
    m_buffer.seek(base);
}

void M2Loader::parseMD21_collision(uint32_t ofs)
{
    // Bounding boxes
    auto bbmin = m_buffer.readFloatLE(3);
    auto bbmax = m_buffer.readFloatLE(3);
    m_data.boundingBox.min[0] = bbmin[0]; m_data.boundingBox.min[1] = bbmin[2]; m_data.boundingBox.min[2] = bbmin[1] * -1;
    m_data.boundingBox.max[0] = bbmax[0]; m_data.boundingBox.max[1] = bbmax[2]; m_data.boundingBox.max[2] = bbmax[1] * -1;
    m_data.boundingSphereRadius = m_buffer.readFloatLE();

    auto cbmin = m_buffer.readFloatLE(3);
    auto cbmax = m_buffer.readFloatLE(3);
    m_data.collisionBox.min[0] = cbmin[0]; m_data.collisionBox.min[1] = cbmin[2]; m_data.collisionBox.min[2] = cbmin[1] * -1;
    m_data.collisionBox.max[0] = cbmax[0]; m_data.collisionBox.max[1] = cbmax[2]; m_data.collisionBox.max[2] = cbmax[1] * -1;
    m_data.collisionSphereRadius = m_buffer.readFloatLE();

    // Collision indices
    const uint32_t indicesCount = m_buffer.readUInt32LE();
    const uint32_t indicesOfs = m_buffer.readUInt32LE();

    // Collision positions
    const uint32_t posCount = m_buffer.readUInt32LE();
    const uint32_t posOfs = m_buffer.readUInt32LE();

    // Collision normals
    const uint32_t normCount = m_buffer.readUInt32LE();
    const uint32_t normOfs = m_buffer.readUInt32LE();

    const size_t base = m_buffer.offset();

    m_buffer.seek(indicesOfs + ofs);
    m_data.collisionIndices = m_buffer.readUInt16LE(indicesCount);

    m_buffer.seek(posOfs + ofs);
    m_data.collisionPositions.resize(posCount * 3);
    for (uint32_t i = 0; i < posCount; i++)
    {
        float x = m_buffer.readFloatLE();
        float y = m_buffer.readFloatLE();
        float z = m_buffer.readFloatLE();
        m_data.collisionPositions[i * 3 + 0] = x;
        m_data.collisionPositions[i * 3 + 1] = z;
        m_data.collisionPositions[i * 3 + 2] = y * -1;
    }

    m_buffer.seek(normOfs + ofs);
    m_data.collisionNormals.resize(normCount * 3);
    for (uint32_t i = 0; i < normCount; i++)
    {
        float x = m_buffer.readFloatLE();
        float y = m_buffer.readFloatLE();
        float z = m_buffer.readFloatLE();
        m_data.collisionNormals[i * 3 + 0] = x;
        m_data.collisionNormals[i * 3 + 1] = z;
        m_data.collisionNormals[i * 3 + 2] = y * -1;
    }

    m_buffer.seek(base);
}

} // namespace wowlib
