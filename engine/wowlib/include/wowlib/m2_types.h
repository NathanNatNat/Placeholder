#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace wowlib
{

struct M2BoundingBox
{
    float min[3] = {};
    float max[3] = {};
};

struct M2Material
{
    uint16_t flags = 0;
    uint16_t blendingMode = 0;
};

struct M2Texture
{
    uint32_t type = 0;
    uint32_t flags = 0;
    uint32_t fileDataID = 0;
    std::string fileName;
};

struct M2Animation
{
    uint16_t id = 0;
    uint16_t variationIndex = 0;
    uint32_t duration = 0;
    float moveSpeed = 0.0f;
    uint32_t flags = 0;
    int16_t frequency = 0;
    uint16_t padding = 0;
    uint32_t replayMin = 0;
    uint32_t replayMax = 0;
    uint16_t blendTimeIn = 0;
    uint16_t blendTimeOut = 0;
    float boxPosMin[3] = {};
    float boxPosMax[3] = {};
    float boxRadius = 0.0f;
    int16_t variationNext = 0;
    uint16_t aliasNext = 0;
};

struct M2Bone
{
    int32_t boneID = 0;
    uint32_t flags = 0;
    int16_t parentBone = -1;
    uint16_t subMeshID = 0;
    uint32_t boneNameCRC = 0;
    float pivot[3] = {};
};

struct M2AnimFileEntry
{
    uint16_t animID = 0;
    uint16_t subAnimID = 0;
    uint32_t fileDataID = 0;
};

/// Skin submesh — defines a subset of vertices/triangles for one draw call.
struct M2SubMesh
{
    uint16_t submeshID = 0;
    uint16_t level = 0;
    uint32_t vertexStart = 0;
    uint16_t vertexCount = 0;
    uint32_t triangleStart = 0;
    uint16_t triangleCount = 0;
    uint16_t boneCount = 0;
    uint16_t boneStart = 0;
    uint16_t boneInfluences = 0;
    uint16_t centerBoneIndex = 0;
    float centerPosition[3] = {};
    float sortCenterPosition[3] = {};
    float sortRadius = 0.0f;
};

/// Skin texture unit — maps a submesh to textures and materials.
struct M2TextureUnit
{
    uint16_t flags = 0;
    uint16_t priority = 0;
    uint16_t shaderID = 0;
    uint16_t skinSectionIndex = 0;
    uint16_t geosetIndex = 0;
    uint16_t colorIndex = 0;
    uint16_t materialIndex = 0;
    uint16_t materialLayer = 0;
    uint16_t textureCount = 0;
    uint16_t textureComboIndex = 0;
    uint16_t textureCoordComboIndex = 0;
    uint16_t textureWeightComboIndex = 0;
    uint16_t textureTransformComboIndex = 0;
};

/// Skin data — loaded from a separate .skin file referenced by SFID chunk.
struct M2SkinData
{
    uint32_t fileDataID = 0;
    bool loaded = false;

    std::vector<uint16_t> indices;
    std::vector<uint16_t> triangles;
    std::vector<M2SubMesh> subMeshes;
    std::vector<M2TextureUnit> textureUnits;
    uint32_t bones = 0;
};

/// Parsed M2 model data. Contains raw structured data for engine conversion.
struct M2Data
{
    std::string name;
    uint32_t version = 0;
    uint32_t flags = 0;

    // Vertex data (WoW coordinate system already converted to engine Y-up)
    std::vector<float> vertices;    // 3 floats per vertex
    std::vector<float> normals;     // 3 floats per vertex
    std::vector<float> uvCoords;    // 2 floats per vertex (UV set 0)
    std::vector<float> uvCoords2;   // 2 floats per vertex (UV set 1)
    std::vector<uint8_t> boneWeights; // 4 bytes per vertex
    std::vector<uint8_t> boneIndices; // 4 bytes per vertex

    // Render data
    uint32_t viewCount = 0;
    std::vector<M2SkinData> skins;
    std::vector<M2SkinData> lodSkins;
    std::vector<M2Texture> textures;
    std::vector<uint32_t> textureTypes;
    std::vector<M2Material> materials;
    std::vector<uint16_t> textureCombos;
    std::vector<uint16_t> transparencyLookup;
    std::vector<uint16_t> textureTransformsLookup;
    std::vector<int16_t> replaceableTextureLookup;

    // Skeleton
    std::vector<M2Bone> bones;
    std::vector<M2Animation> animations;
    std::vector<int16_t> animationLookup;
    std::vector<int32_t> globalLoops;
    std::vector<M2AnimFileEntry> animFileIDs;
    std::vector<uint32_t> boneFileIDs;
    uint32_t skeletonFileID = 0;

    // Collision
    M2BoundingBox boundingBox = {};
    float boundingSphereRadius = 0.0f;
    M2BoundingBox collisionBox = {};
    float collisionSphereRadius = 0.0f;
    std::vector<uint16_t> collisionIndices;
    std::vector<float> collisionPositions;
    std::vector<float> collisionNormals;
};

} // namespace wowlib
