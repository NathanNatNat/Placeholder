#include "renderer/forward_pipeline.h"
#include "renderer/opengl_render_device.h"
#include "renderer/render_state.h"
#include "renderer/shader_manager.h"

#include "core/logging.h"

#include <glm/gtc/type_ptr.hpp>

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

    float vertices[] = {
        -0.5f, -0.5f, 0.0f,   1.0f, 0.0f, 0.0f,
         0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,
         0.0f,  0.5f, 0.0f,   0.0f, 0.0f, 1.0f
    };

    m_triangleVao->setVertexData(vertices, sizeof(vertices));
    m_triangleVao->setAttribute(0, 3, VertexAttribType::Float, 24, 0);
    m_triangleVao->setAttribute(1, 3, VertexAttribType::Float, 24, 12);

    m_meshShader = m_shaderManager.createProgram("mesh");
    m_meshAlphaTestShader = m_shaderManager.createProgram("mesh", {{"ALPHA_TEST", "1"}});

    m_skybox = std::make_unique<Skybox>(m_device, m_shaderManager);
    m_skybox->initialize();

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

    m_renderQueue.clear();

    m_device.endFrame();
}

void ForwardPipeline::shutdown()
{
    m_skybox.reset();
    m_meshAlphaTestShader.reset();
    m_meshShader.reset();
    m_triangleShader.reset();
    m_triangleVao.reset();
}

void ForwardPipeline::submit(const RenderItem& item)
{
    m_renderQueue.push_back(item);
}

void ForwardPipeline::setSkyboxTexture(Texture* cubemap)
{
    if (m_skybox)
    {
        m_skybox->setTexture(cubemap);
    }
}

void ForwardPipeline::clearPass(const FrameContext& /*ctx*/)
{
    m_device.setClearColor(0.39f, 0.58f, 0.93f, 1.0f);
    m_device.clear(true, true, false);
}

void ForwardPipeline::geometryPass(const FrameContext& ctx)
{
    if (wireframeEnabled)
    {
        m_device.setPolygonMode(PolygonMode::Line);
    }

    if (!m_renderQueue.empty())
    {
        glm::vec3 lightDir = glm::normalize(glm::vec3(-0.5f, -0.8f, -0.3f));
        glm::vec3 lightColor{0.9f, 0.88f, 0.85f};
        glm::vec3 skyColor{0.65f, 0.7f, 0.75f};
        glm::vec3 groundColor{0.3f, 0.28f, 0.25f};
        glm::vec3 fillLightDir = glm::normalize(glm::vec3(0.4f, 0.5f, 0.6f));
        glm::vec3 fillLightColor{0.35f, 0.35f, 0.38f};

        for (const auto& item : m_renderQueue)
        {
            if (!item.mesh || !item.material)
            {
                continue;
            }

            RenderStateDesc state = item.material->buildRenderState();
            applyRenderState(m_device, state);

            if (wireframeEnabled)
            {
                m_device.setPolygonMode(PolygonMode::Line);
            }

            ShaderProgram* shader = item.material->shader;
            if (!shader)
            {
                shader = (item.material->blendMode == BlendMode::AlphaTest)
                    ? m_meshAlphaTestShader.get()
                    : m_meshShader.get();
            }
            shader->bind();
            shader->setUniformMat4("u_model", glm::value_ptr(item.modelMatrix));
            shader->setUniformMat4("u_view", glm::value_ptr(ctx.viewMatrix));
            shader->setUniformMat4("u_projection", glm::value_ptr(ctx.projectionMatrix));
            shader->setUniformVec3("u_diffuseColor", &item.material->diffuseColor.x);
            shader->setUniform("u_opacity", item.material->opacity);
            shader->setUniformVec3("u_lightDir", &lightDir.x);
            shader->setUniformVec3("u_lightColor", &lightColor.x);
            shader->setUniformVec3("u_skyColor", &skyColor.x);
            shader->setUniformVec3("u_groundColor", &groundColor.x);
            shader->setUniformVec3("u_fillLightDir", &fillLightDir.x);
            shader->setUniformVec3("u_fillLightColor", &fillLightColor.x);

            if (item.material->blendMode == BlendMode::AlphaTest)
            {
                shader->setUniform("u_alphaThreshold", item.material->alphaTestThreshold);
            }

            if (item.material->diffuseTexture)
            {
                item.material->diffuseTexture->bind(m_device, 0);
            }
            shader->setUniform("u_diffuseTexture", 0);

            if (item.subMeshIndex >= 0)
            {
                item.mesh->drawSubMesh(static_cast<size_t>(item.subMeshIndex));
            }
            else
            {
                item.mesh->drawAll();
            }
        }
    }
    else
    {
        m_device.setDepthTest(true);
        m_device.setDepthWrite(true);
        m_device.setCullMode(CullMode::None);
        m_device.setBlendEnabled(false);

        m_triangleShader->bind();
        m_triangleShader->setUniformMat4("u_view", glm::value_ptr(ctx.viewMatrix));
        m_triangleShader->setUniformMat4("u_projection", glm::value_ptr(ctx.projectionMatrix));
        m_triangleVao->drawArrays(PrimitiveTopology::Triangles, 0, 3);
    }

    if (wireframeEnabled)
    {
        m_device.setPolygonMode(PolygonMode::Fill);
    }
}

void ForwardPipeline::skyboxPass(const FrameContext& ctx)
{
    if (m_skybox)
    {
        m_skybox->render(glm::value_ptr(ctx.viewMatrix),
                         glm::value_ptr(ctx.projectionMatrix));
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
