#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace wowlib
{

struct WMOMaterial
{
    uint32_t flags;
    uint32_t shader;
    uint32_t blendMode;
    uint32_t texture1;
    uint32_t texture2;
    uint32_t texture3;
    uint32_t color1;
    uint32_t color2;
};

struct WMOGroupInfo
{
    uint32_t flags;
    float boundingBox1[3];
    float boundingBox2[3];
    int32_t nameIndex;
};

struct WMODoodad
{
    uint32_t nameIndex;
    float position[3];
    float rotation[4];
    float scale;
    uint8_t color[4];
};

struct WMODoodadSet
{
    std::string name;
    uint32_t firstIndex;
    uint32_t count;
};

struct WMORenderBatch
{
    uint32_t firstFace;
    uint32_t numFaces;
    uint16_t firstVertex;
    uint16_t lastVertex;
    uint8_t materialID;
};

/// Parsed WMO (World Map Object) data.
struct WMOData
{
    uint32_t version = 0;
    uint32_t materialCount = 0;
    uint32_t groupCount = 0;
    uint32_t portalCount = 0;
    uint32_t lightCount = 0;
    uint32_t doodadCount = 0;
    uint32_t setCount = 0;
    uint16_t flags = 0;

    std::vector<WMOMaterial> materials;
    std::vector<WMOGroupInfo> groupInfo;
    std::vector<WMODoodadSet> doodadSets;
    std::vector<WMODoodad> doodads;
    std::vector<uint32_t> doodadFileIDs;
    std::vector<uint32_t> groupFileIDs;
};

/// Parsed WMO group geometry data.
struct WMOGroupData
{
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> uvCoords;
    std::vector<uint16_t> indices;
    std::vector<WMORenderBatch> renderBatches;
    std::vector<std::vector<uint8_t>> vertexColors;
};

} // namespace wowlib
