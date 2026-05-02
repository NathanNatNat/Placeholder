#pragma once

#include "render_types.h"
#include "shader_program.h"
#include "vertex_array.h"
#include "material.h"
#include "mesh.h"
#include "skybox.h"
#include "texture.h"

#include <glm/mat4x4.hpp>

#include <functional>
#include <memory>
#include <vector>

namespace placeholder::renderer
{

class OpenGLRenderDevice;
class ShaderManager;

/// A renderable object submitted to the pipeline each frame.
struct RenderItem
{
    Mesh* mesh = nullptr;
    Material* material = nullptr;
    glm::mat4 modelMatrix{1.0f};
    int subMeshIndex = -1;
};

/// Hardcoded forward rendering pipeline.
///
/// Executes a fixed sequence of passes each frame:
/// clear -> geometry -> skybox -> debug -> ImGui.
class ForwardPipeline
{
public:
    ForwardPipeline(OpenGLRenderDevice& device, ShaderManager& shaderManager);
    ~ForwardPipeline();

    /// Create GPU resources (shaders, default geometry).
    void initialize();

    /// Execute one frame of the pipeline.
    void execute(const FrameContext& ctx);

    /// Release GPU resources.
    void shutdown();

    /// Submit a renderable for this frame's geometry pass.
    void submit(const RenderItem& item);

    /// Set the skybox cubemap texture.
    void setSkyboxTexture(Texture* cubemap);

    /// Toggle wireframe rendering.
    bool wireframeEnabled = false;

    /// Callback invoked during the debug pass (for debug draw, gizmos, etc.).
    std::function<void(const FrameContext&)> onDebugPass;

    /// Callback invoked during the ImGui pass (for editor rendering).
    std::function<void(const FrameContext&)> onImGuiPass;

private:
    void clearPass(const FrameContext& ctx);
    void geometryPass(const FrameContext& ctx);
    void skyboxPass(const FrameContext& ctx);
    void debugPass(const FrameContext& ctx);
    void imguiPass(const FrameContext& ctx);

    OpenGLRenderDevice& m_device;
    ShaderManager& m_shaderManager;

    std::unique_ptr<VertexArray> m_triangleVao;
    std::unique_ptr<ShaderProgram> m_triangleShader;
    std::unique_ptr<ShaderProgram> m_meshShader;
    std::unique_ptr<ShaderProgram> m_meshAlphaTestShader;
    std::unique_ptr<Skybox> m_skybox;

    std::vector<RenderItem> m_renderQueue;
};

} // namespace placeholder::renderer
