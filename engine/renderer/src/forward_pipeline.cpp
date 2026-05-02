#include "renderer/forward_pipeline.h"
#include "renderer/opengl_render_device.h"
#include "renderer/shader_manager.h"

#include "core/logging.h"

namespace placeholder::renderer
{

ForwardPipeline::ForwardPipeline(OpenGLRenderDevice& device, ShaderManager& shaderManager)
    : m_device(device)
    , m_shaderManager(shaderManager)
{
}

ForwardPipeline::~ForwardPipeline()
{
    shutdown();
}

void ForwardPipeline::initialize()
{
    auto logger = core::getLogger("renderer");
    logger->info("Initializing forward pipeline");

    m_triangleShader = m_shaderManager.createProgram("triangle");

    m_triangleVao = std::make_unique<VertexArray>(m_device);

    // Interleaved vertex data: position (vec3) + color (vec3)
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,   1.0f, 0.0f, 0.0f,
         0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,
         0.0f,  0.5f, 0.0f,   0.0f, 0.0f, 1.0f
    };

    m_triangleVao->setVertexData(vertices, sizeof(vertices));
    m_triangleVao->setAttribute(0, 3, VertexAttribType::Float, 24, 0);
    m_triangleVao->setAttribute(1, 3, VertexAttribType::Float, 24, 12);

    logger->info("Forward pipeline initialized");
}

void ForwardPipeline::execute(const FrameContext& ctx)
{
    m_device.beginFrame(ctx);
    m_device.setViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);

    clearPass(ctx);
    geometryPass(ctx);
    skyboxPass(ctx);
    debugPass(ctx);
    imguiPass(ctx);

    m_device.endFrame();
}

void ForwardPipeline::shutdown()
{
    m_triangleShader.reset();
    m_triangleVao.reset();
}

void ForwardPipeline::clearPass(const FrameContext& /*ctx*/)
{
    m_device.setClearColor(0.39f, 0.58f, 0.93f, 1.0f);
    m_device.clear(true, true, false);
}

void ForwardPipeline::geometryPass(const FrameContext& /*ctx*/)
{
    m_device.setDepthTest(true);
    m_device.setDepthWrite(true);
    m_device.setCullMode(CullMode::None);
    m_device.setBlendEnabled(false);

    if (wireframeEnabled)
    {
        m_device.setPolygonMode(PolygonMode::Line);
    }
    else
    {
        m_device.setPolygonMode(PolygonMode::Fill);
    }

    m_triangleShader->bind();
    m_triangleVao->drawArrays(PrimitiveTopology::Triangles, 0, 3);

    if (wireframeEnabled)
    {
        m_device.setPolygonMode(PolygonMode::Fill);
    }
}

void ForwardPipeline::skyboxPass(const FrameContext& /*ctx*/)
{
    if (!m_stubsLogged)
    {
        auto logger = core::getLogger("renderer");
        logger->trace("Skybox pass: stub");
    }
}

void ForwardPipeline::debugPass(const FrameContext& /*ctx*/)
{
    if (!m_stubsLogged)
    {
        auto logger = core::getLogger("renderer");
        logger->trace("Debug pass: stub");
    }
}

void ForwardPipeline::imguiPass(const FrameContext& /*ctx*/)
{
    if (!m_stubsLogged)
    {
        auto logger = core::getLogger("renderer");
        logger->trace("ImGui pass: stub");
        m_stubsLogged = true;
    }
}

} // namespace placeholder::renderer
