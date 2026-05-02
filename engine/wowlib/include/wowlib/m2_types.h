#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace wowlib
{

struct M2BoundingBox
{
    float min[3];
    float max[3];
};

struct M2Material
{
    uint16_t flags;
    uint16_t blendingMode;
};

struct M2Texture
{
    uint32_t type;
    uint32_t flags;
    uint32_t fileDataID;
};

struct M2Animation
{
    uint16_t id;
    uint16_t variationIndex;
    uint32_t duration;
    float moveSpeed;
    uint32_t flags;
    int16_t frequency;
};

struct M2Bone
{
    int32_t boneID;
    uint32_t flags;
    int16_t parentBone;
    float pivot[3];
};

struct M2SubMesh
{
    uint32_t indexStart;
    uint32_t indexCount;
    uint32_t vertexStart;
    uint32_t vertexCount;
    uint16_t materialIndex;
};

/// Parsed M2 model data. Contains raw structured data for engine conversion.
struct M2Data
{
    std::string name;
    uint32_t version = 0;
    uint32_t flags = 0;

    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> uvCoords;
    std::vector<float> uvCoords2;
    std::vector<uint16_t> indices;
    std::vector<uint8_t> boneWeights;
    std::vector<uint8_t> boneIndices;

    std::vector<M2SubMesh> subMeshes;
    std::vector<M2Texture> textures;
    std::vector<M2Material> materials;
    std::vector<uint16_t> textureCombos;
    std::vector<M2Bone> bones;
    std::vector<M2Animation> animations;

    M2BoundingBox boundingBox = {};
    float boundingSphereRadius = 0.0f;
};

} // namespace wowlib
