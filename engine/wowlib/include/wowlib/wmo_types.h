#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>

namespace wowlib
{

struct WMOMaterial
{
    uint32_t flags = 0;
    uint32_t shader = 0;
    uint32_t blendMode = 0;
    uint32_t texture1 = 0;
    uint32_t color1 = 0;
    uint32_t color1b = 0;
    uint32_t texture2 = 0;
    uint32_t color2 = 0;
    uint32_t groupType = 0;
    uint32_t texture3 = 0;
    uint32_t color3 = 0;
    uint32_t flags3 = 0;
    uint32_t runtimeData[4] = {};
};

struct WMOGroupInfo
{
    uint32_t flags = 0;
    float boundingBox1[3] = {};
    float boundingBox2[3] = {};
    int32_t nameIndex = 0;
};

struct WMODoodad
{
    uint32_t nameIndex = 0;
    uint8_t flags = 0;
    float position[3] = {};
    float rotation[4] = {};
    float scale = 1.0f;
    uint8_t color[4] = {};
};

struct WMODoodadSet
{
    std::string name;
    uint32_t firstIndex = 0;
    uint32_t count = 0;
};

struct WMORenderBatch
{
    uint16_t possibleBox1[3] = {};
    uint16_t possibleBox2[3] = {};
    uint32_t firstFace = 0;
    uint16_t numFaces = 0;
    uint16_t firstVertex = 0;
    uint16_t lastVertex = 0;
    uint8_t flags = 0;
    uint8_t materialID = 0;
};

/// Parsed WMO root data.
struct WMOData
{
    uint32_t version = 0;
    uint32_t materialCount = 0;
    uint32_t groupCount = 0;
    uint32_t portalCount = 0;
    uint32_t lightCount = 0;
    uint32_t doodadCount = 0;
    uint32_t setCount = 0;
    uint32_t ambientColor = 0;
    uint32_t wmoID = 0;
    uint16_t flags = 0;
    uint16_t lodCount = 0;
    float boundingBox1[3] = {};
    float boundingBox2[3] = {};

    std::vector<WMOMaterial> materials;
    std::vector<WMOGroupInfo> groupInfo;
    std::vector<WMODoodadSet> doodadSets;
    std::vector<WMODoodad> doodads;
    std::vector<uint32_t> doodadFileIDs;
    std::vector<uint32_t> groupFileIDs;
    std::map<uint32_t, std::string> textureNames;
};

/// Parsed WMO group geometry data.
struct WMOGroupData
{
    uint32_t version = 0;
    uint32_t groupFlags = 0;
    float boundingBox1[3] = {};
    float boundingBox2[3] = {};

    std::vector<float> vertices;    // 3 floats per vertex, Y-up converted
    std::vector<float> normals;     // 3 floats per vertex, Y-up converted
    std::vector<float> uvCoords;    // 2 floats per vertex
    std::vector<uint16_t> indices;
    std::vector<WMORenderBatch> renderBatches;
    std::vector<uint8_t> vertexColors;
};

} // namespace wowlib
