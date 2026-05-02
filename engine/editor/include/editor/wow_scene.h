#pragma once

#include <renderer/mesh.h>
#include <renderer/material.h>
#include <renderer/texture.h>
#include <renderer/texture_loader.h>
#include <renderer/shader_manager.h>
#include <renderer/forward_pipeline.h>
#include <wowlib/m2_types.h>
#include <wowlib/wmo_types.h>
#include <wowlib/casc_local.h>
#include <wowlib/blp_image.h>

#include <memory>
#include <vector>
#include <unordered_map>

namespace placeholder::editor
{

/// Manages loading and rendering WoW models via the CASC archive.
/// Bridges wowlib parsed data with the engine's mesh/material system.
class WowScene
{
public:
    WowScene(renderer::OpenGLRenderDevice& device,
             renderer::ShaderManager& shaderManager,
             renderer::TextureLoader& textureLoader);

    /// Initialize CASC from a WoW installation directory.
    /// @return true if CASC was loaded successfully
    bool initCasc(const std::string& wowPath, const std::string& cachePath = "");

    /// Load an M2 model by fileDataID and prepare it for rendering.
    /// @return true if the model was loaded successfully
    bool loadM2(uint32_t fileDataID);

    /// Submit all loaded models to the forward pipeline for rendering.
    void submitToRenderQueue(renderer::ForwardPipeline& pipeline, const glm::mat4& modelMatrix);

    /// Clear all loaded models and textures.
    void clear();

    bool isCascReady() const { return m_cascReady; }

private:
    /// Convert M2 parsed data into engine MeshData.
    renderer::MeshData convertM2ToMeshData(const wowlib::M2Data& m2, const wowlib::M2SkinData& skin);

    /// Load a BLP texture from CASC by fileDataID and upload to GPU.
    renderer::Texture* loadBlpTexture(uint32_t fileDataID);

    renderer::OpenGLRenderDevice& m_device;
    renderer::ShaderManager& m_shaderManager;
    renderer::TextureLoader& m_textureLoader;

    // CASC source
    std::unique_ptr<wowlib::TactKeys> m_keys;
    std::unique_ptr<wowlib::CascLocal> m_casc;
    bool m_cascReady = false;

    // Loaded model data
    struct LoadedModel
    {
        std::unique_ptr<renderer::Mesh> mesh;
        std::vector<renderer::Material> materials;
    };
    std::vector<LoadedModel> m_models;

    // Texture cache (fileDataID -> texture)
    std::unordered_map<uint32_t, std::unique_ptr<renderer::Texture>> m_textureCache;

    // Shader programs
    renderer::ShaderProgram* m_wowShader = nullptr;
    renderer::ShaderProgram* m_wowAlphaTestShader = nullptr;

    // Fallback white texture
    std::unique_ptr<renderer::Texture> m_whiteTexture;
};

} // namespace placeholder::editor
