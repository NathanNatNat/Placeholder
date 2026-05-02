#pragma once

#include "render_types.h"
#include "shader_program.h"
#include "vertex_array.h"

#include <memory>

namespace placeholder::renderer
{

class OpenGLRenderDevice;
class ShaderManager;

/// Hardcoded forward rendering pipeline.
///
/// Executes a fixed sequence of passes each frame:
/// clear -> geometry -> skybox -> debug -> ImGui.
/// Stub passes log once and return until their systems are implemented.
class ForwardPipeline
{
public:
    ForwardPipeline(OpenGLRenderDevice& device, ShaderManager& shaderManager);
    ~ForwardPipeline();

    /// Create GPU resources (shaders, triangle VAO, etc.).
    void initialize();

    /// Execute one frame of the pipeline.
    void execute(const FrameContext& ctx);

    /// Release GPU resources.
    void shutdown();

    /// Toggle wireframe rendering (F1 key in the demo).
    bool wireframeEnabled = false;

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

    bool m_stubsLogged = false;
};

} // namespace placeholder::renderer
