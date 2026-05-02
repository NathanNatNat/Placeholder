#pragma once

#include "texture.h"
#include "shader_program.h"
#include "vertex_array.h"

#include <memory>

namespace placeholder::renderer
{

class OpenGLRenderDevice;
class ShaderManager;

/// Renders a cubemap skybox behind all other geometry.
///
/// Uses a unit cube VAO and a cubemap texture. Depth is written
/// as 1.0 (maximum) so the skybox only fills pixels with no geometry.
class Skybox
{
public:
    Skybox(OpenGLRenderDevice& device, ShaderManager& shaderManager);
    ~Skybox();

    /// Initialize GPU resources (cube VAO, shader).
    void initialize();

    /// Set the cubemap texture to render.
    void setTexture(Texture* cubemap);

    /// Render the skybox. Call after the geometry pass.
    void render(const float* viewMatrix, const float* projectionMatrix);

    /// Release GPU resources.
    void shutdown();

private:
    OpenGLRenderDevice& m_device;
    ShaderManager& m_shaderManager;
    std::unique_ptr<VertexArray> m_cubeVao;
    std::unique_ptr<ShaderProgram> m_shader;
    Texture* m_cubemap = nullptr;
};

} // namespace placeholder::renderer
