#include "editor/wow_scene.h"

#include <wowlib/m2_loader.h>
#include <wowlib/blp_image.h>
#include <wowlib/blte_reader.h>
#include <spdlog/spdlog.h>

#include <format>

namespace placeholder::editor
{

WowScene::WowScene(renderer::OpenGLRenderDevice& device,
                   renderer::ShaderManager& shaderManager,
                   renderer::TextureLoader& textureLoader)
    : m_device(device), m_shaderManager(shaderManager), m_textureLoader(textureLoader)
{
}

bool WowScene::initCasc(const std::string& wowPath, const std::string& cachePath)
{
    try
    {
        m_keys = std::make_unique<wowlib::TactKeys>();
        m_casc = std::make_unique<wowlib::CascLocal>(
            wowPath, *m_keys, cachePath);
        m_casc->init();

        auto builds = m_casc->getBuildList();
        if (builds.empty())
        {
            spdlog::get("editor")->error("No WoW builds found in {}", wowPath);
            return false;
        }

        spdlog::get("editor")->info("Found {} WoW builds, loading first...", builds.size());
        m_casc->load(0);

        spdlog::get("editor")->info("CASC loaded: {} root entries, {} encoding keys",
            m_casc->rootEntries.size(), m_casc->encodingKeys.size());

        m_cascReady = true;

        // Register WoW shaders
        m_shaderManager.registerShader("wow_model", {"wow_model.vert", "wow_model.frag"});
        auto prog = m_shaderManager.createProgram("wow_model");
        m_wowShader = prog.get();
        auto progAT = m_shaderManager.createProgram("wow_model", {{"ALPHA_TEST", "1"}});
        m_wowAlphaTestShader = progAT.get();

        m_whiteTexture = m_textureLoader.createWhiteTexture();

        return true;
    }
    catch (const std::exception& e)
    {
        spdlog::get("editor")->error("Failed to init CASC: {}", e.what());
        m_cascReady = false;
        return false;
    }
}

bool WowScene::loadM2(uint32_t fileDataID)
{
    if (!m_cascReady)
        return false;

    try
    {
        spdlog::get("editor")->info("Loading M2 model: {}", fileDataID);

        // Read M2 file from CASC
        wowlib::BLTEReader blte = m_casc->getFileAsBLTE(fileDataID);
        blte.processAllBlocks();
        wowlib::DataBuffer m2Buf = wowlib::DataBuffer::from(blte.raw());

        // Parse M2
        wowlib::M2Loader loader(std::move(m2Buf));
        loader.load();
        const auto& m2 = loader.data();

        spdlog::get("editor")->info("M2 '{}': {} vertices, {} textures, {} materials, {} skins",
            m2.name, m2.vertices.size() / 3, m2.textures.size(),
            m2.materials.size(), m2.skins.size());

        if (m2.skins.empty())
        {
            spdlog::get("editor")->warn("M2 has no skin files");
            return false;
        }

        // Load first skin file
        wowlib::M2SkinData skinData;
        skinData.fileDataID = m2.skins[0].fileDataID;
        if (skinData.fileDataID == 0)
        {
            spdlog::get("editor")->warn("M2 skin file has no fileDataID");
            return false;
        }

        wowlib::BLTEReader skinBlte = m_casc->getFileAsBLTE(skinData.fileDataID);
        skinBlte.processAllBlocks();
        wowlib::DataBuffer skinBuf = wowlib::DataBuffer::from(skinBlte.raw());
        wowlib::M2Loader::loadSkin(std::move(skinBuf), skinData);

        // Convert to engine mesh
        renderer::MeshData meshData = convertM2ToMeshData(m2, skinData);
        if (meshData.vertices.empty())
        {
            spdlog::get("editor")->warn("M2 produced empty mesh data");
            return false;
        }

        LoadedModel model;
        model.mesh = std::make_unique<renderer::Mesh>(renderer::Mesh::create(m_device, meshData));

        // Create materials for each texture unit
        for (const auto& tu : skinData.textureUnits)
        {
            renderer::Material mat;

            // Select shader based on blend mode
            uint16_t matIdx = tu.materialIndex;
            uint16_t blendMode = 0;
            if (matIdx < m2.materials.size())
                blendMode = m2.materials[matIdx].blendingMode;

            switch (blendMode)
            {
                case 0: // Opaque
                    mat.blendMode = renderer::BlendMode::Opaque;
                    mat.shader = m_wowShader;
                    break;
                case 1: // Alpha key
                    mat.blendMode = renderer::BlendMode::AlphaTest;
                    mat.shader = m_wowAlphaTestShader;
                    mat.alphaTestThreshold = 0.5f;
                    break;
                case 2: // Alpha blend
                    mat.blendMode = renderer::BlendMode::AlphaBlend;
                    mat.shader = m_wowShader;
                    break;
                case 3: // Additive
                    mat.blendMode = renderer::BlendMode::Additive;
                    mat.shader = m_wowShader;
                    break;
                case 4: // Modulate
                    mat.blendMode = renderer::BlendMode::Modulate;
                    mat.shader = m_wowShader;
                    break;
                default:
                    mat.blendMode = renderer::BlendMode::Opaque;
                    mat.shader = m_wowShader;
                    break;
            }

            // Set two-sided rendering if material flag is set
            if (matIdx < m2.materials.size() && (m2.materials[matIdx].flags & 0x4))
                mat.cullMode = renderer::CullMode::None;

            // Load diffuse texture
            uint16_t texComboIdx = tu.textureComboIndex;
            if (texComboIdx < m2.textureCombos.size())
            {
                uint16_t texIdx = m2.textureCombos[texComboIdx];
                if (texIdx < m2.textures.size() && m2.textures[texIdx].fileDataID != 0)
                {
                    renderer::Texture* tex = loadBlpTexture(m2.textures[texIdx].fileDataID);
                    if (tex)
                        mat.diffuseTexture = tex;
                }
            }

            if (!mat.diffuseTexture)
                mat.diffuseTexture = m_whiteTexture.get();

            model.materials.push_back(std::move(mat));
        }

        m_models.push_back(std::move(model));
        spdlog::get("editor")->info("M2 loaded successfully with {} submeshes", skinData.subMeshes.size());
        return true;
    }
    catch (const std::exception& e)
    {
        spdlog::get("editor")->error("Failed to load M2 {}: {}", fileDataID, e.what());
        return false;
    }
}

void WowScene::submitToRenderQueue(renderer::ForwardPipeline& pipeline, const glm::mat4& modelMatrix)
{
    for (auto& model : m_models)
    {
        if (!model.mesh || !model.mesh->valid())
            continue;

        for (size_t i = 0; i < model.mesh->subMeshCount(); i++)
        {
            renderer::RenderItem item;
            item.mesh = model.mesh.get();
            item.subMeshIndex = static_cast<int>(i);
            item.modelMatrix = modelMatrix;

            // Use material matching this submesh, or fallback to first
            size_t matIdx = i < model.materials.size() ? i : 0;
            if (!model.materials.empty())
                item.material = &model.materials[matIdx];

            pipeline.submit(item);
        }
    }
}

void WowScene::clear()
{
    m_models.clear();
    m_textureCache.clear();
}

renderer::MeshData WowScene::convertM2ToMeshData(const wowlib::M2Data& m2, const wowlib::M2SkinData& skin)
{
    renderer::MeshData meshData;
    uint32_t vertCount = static_cast<uint32_t>(m2.vertices.size() / 3);

    meshData.vertices.resize(vertCount);
    for (uint32_t i = 0; i < vertCount; i++)
    {
        meshData.vertices[i].position = {
            m2.vertices[i * 3 + 0],
            m2.vertices[i * 3 + 1],
            m2.vertices[i * 3 + 2]
        };
        meshData.vertices[i].normal = {
            m2.normals[i * 3 + 0],
            m2.normals[i * 3 + 1],
            m2.normals[i * 3 + 2]
        };
        meshData.vertices[i].texCoord = {
            m2.uvCoords[i * 2 + 0],
            m2.uvCoords[i * 2 + 1]
        };
    }

    // Build indices from skin data: skin.indices maps local -> global vertex,
    // skin.triangles are local triangle indices referencing skin.indices
    meshData.indices.reserve(skin.triangles.size());
    for (uint16_t tri : skin.triangles)
    {
        if (tri < skin.indices.size())
            meshData.indices.push_back(skin.indices[tri]);
        else
            meshData.indices.push_back(0);
    }

    // Create submeshes from texture units (each texture unit = one draw call)
    for (const auto& tu : skin.textureUnits)
    {
        if (tu.skinSectionIndex >= skin.subMeshes.size())
            continue;

        const auto& sm = skin.subMeshes[tu.skinSectionIndex];
        renderer::SubMesh sub;
        sub.indexOffset = sm.triangleStart;
        sub.indexCount = sm.triangleCount;
        sub.materialIndex = static_cast<uint32_t>(meshData.subMeshes.size());
        meshData.subMeshes.push_back(sub);
    }

    return meshData;
}

renderer::Texture* WowScene::loadBlpTexture(uint32_t fileDataID)
{
    auto it = m_textureCache.find(fileDataID);
    if (it != m_textureCache.end())
        return it->second.get();

    try
    {
        wowlib::BLTEReader blte = m_casc->getFileAsBLTE(fileDataID, true);
        blte.processAllBlocks();

        wowlib::DataBuffer blpBuf = wowlib::DataBuffer::from(blte.raw());
        wowlib::BlpImage blp(std::move(blpBuf));

        std::vector<uint8_t> rgba = blp.toRGBA(0);

        renderer::TextureDesc desc;
        desc.width = blp.width();
        desc.height = blp.height();
        desc.format = renderer::TextureFormat::RGBA8;
        desc.generateMipmaps = true;

        auto tex = std::make_unique<renderer::Texture>(
            renderer::Texture::create2D(m_device, desc, rgba.data()));

        renderer::Texture* texPtr = tex.get();
        m_textureCache[fileDataID] = std::move(tex);
        return texPtr;
    }
    catch (const std::exception& e)
    {
        spdlog::get("editor")->warn("Failed to load BLP texture {}: {}", fileDataID, e.what());
        m_textureCache[fileDataID] = nullptr;
        return nullptr;
    }
}

} // namespace placeholder::editor
