#pragma once

#include "mesh.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace placeholder::renderer
{

class OpenGLRenderDevice;

/// Embedded texture data extracted from a model file (e.g. glTF binary).
struct EmbeddedTexture
{
    std::vector<uint8_t> data;
    int width = 0;
    int height = 0;
    int channels = 0;
    bool compressed = false;
};

/// Material description extracted from a model file by Assimp.
struct LoadedMaterial
{
    std::string name;
    std::string diffuseTexturePath;
    int embeddedTextureIndex = -1;
    glm::vec3 diffuseColor{1.0f};
    float opacity = 1.0f;
};

/// Result of loading a model file.
struct LoadedModel
{
    std::unique_ptr<Mesh> mesh;
    std::vector<LoadedMaterial> materials;
    std::vector<EmbeddedTexture> embeddedTextures;
};

/// Loads 3D models from disk via Assimp (glTF, OBJ, FBX, etc.).
class MeshLoader
{
public:
    explicit MeshLoader(OpenGLRenderDevice& device);

    /// Load a model file. Returns null mesh on failure.
    LoadedModel loadFromFile(const std::string& path);

private:
    OpenGLRenderDevice& m_device;
};

} // namespace placeholder::renderer
