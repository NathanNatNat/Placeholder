#include "renderer/skybox.h"
#include "renderer/opengl_render_device.h"
#include "renderer/shader_manager.h"

#include "core/logging.h"

#include <glad/gl.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace placeholder::renderer
{

namespace
{

// clang-format off
constexpr float CUBE_VERTICES[] = {
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
};
// clang-format on

} // namespace

Skybox::Skybox(OpenGLRenderDevice& device, ShaderManager& shaderManager)
    : m_device(device)
    , m_shaderManager(shaderManager)
{
}

Skybox::~Skybox()
{
    shutdown();
}

void Skybox::initialize()
{
    auto logger = core::getLogger("renderer");

    m_shader = m_shaderManager.createProgram("skybox");
    if (!m_shader || !m_shader->isValid())
    {
        logger->warn("Skybox shader failed to compile — skybox disabled");
        return;
    }

    m_cubeVao = std::make_unique<VertexArray>(m_device);
    m_cubeVao->setVertexData(CUBE_VERTICES, sizeof(CUBE_VERTICES));
    m_cubeVao->setAttribute(AttribLocation::POSITION, 3, VertexAttribType::Float, 12, 0);

    logger->info("Skybox initialized");
}

void Skybox::setTexture(Texture* cubemap)
{
    m_cubemap = cubemap;
}

void Skybox::render(const float* viewMatrix, const float* projectionMatrix)
{
    if (!m_shader || !m_shader->isValid() || !m_cubemap)
    {
        return;
    }

    m_device.setDepthWrite(false);
    m_device.setDepthFunc(DepthFunc::LessEqual);
    m_device.setCullMode(CullMode::None);
    m_device.setBlendEnabled(false);

    m_shader->bind();

    glm::mat4 view = glm::mat4(glm::mat3(glm::make_mat4(viewMatrix)));
    m_shader->setUniformMat4("u_view", glm::value_ptr(view));
    m_shader->setUniformMat4("u_projection", projectionMatrix);
    m_shader->setUniform("u_skybox", 0);

    m_cubemap->bind(m_device, 0);

    m_cubeVao->drawArrays(PrimitiveTopology::Triangles, 0, 36);

    m_device.setDepthWrite(true);
}

void Skybox::shutdown()
{
    m_shader.reset();
    m_cubeVao.reset();
    m_cubemap = nullptr;
}

} // namespace placeholder::renderer
