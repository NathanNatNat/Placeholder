#pragma once

#include "mesh.h"

#include <memory>
#include <string>
#include <vector>

namespace placeholder::renderer
{

class OpenGLRenderDevice;

/// Material description extracted from a model file by Assimp.
struct LoadedMaterial
{
    std::string name;
    std::string diffuseTexturePath;
    glm::vec3 diffuseColor{1.0f};
    float opacity = 1.0f;
};

/// Result of loading a model file.
struct LoadedModel
{
    std::unique_ptr<Mesh> mesh;
    std::vector<LoadedMaterial> materials;
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
