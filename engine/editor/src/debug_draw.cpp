#include "editor/debug_draw.h"

#include "renderer/opengl_render_device.h"
#include "renderer/shader_manager.h"
#include "renderer/shader_program.h"
#include "renderer/vertex_array.h"

#include "core/logging.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>

#include <cmath>

namespace placeholder::editor
{

DebugDraw::DebugDraw(renderer::OpenGLRenderDevice& device,
                     renderer::ShaderManager& shaderManager)
    : m_device(device)
    , m_shaderManager(shaderManager)
{
}

DebugDraw::~DebugDraw()
{
    shutdown();
}

void DebugDraw::initialize()
{
    m_shader = m_shaderManager.createProgram("debug_line");
    m_vao = std::make_unique<renderer::VertexArray>(m_device);

    m_vao->setVertexData(nullptr, MAX_VERTICES * sizeof(Vertex),
                         renderer::BufferUsage::Dynamic);
    m_vao->setAttribute(0, 3, renderer::VertexAttribType::Float,
                        sizeof(Vertex), offsetof(Vertex, position));
    m_vao->setAttribute(1, 3, renderer::VertexAttribType::Float,
                        sizeof(Vertex), offsetof(Vertex, color));

    m_initialized = true;

    auto logger = core::getLogger("editor");
    logger->info("Debug draw system initialized");
}

void DebugDraw::clear()
{
    m_vertices.clear();
}

void DebugDraw::addLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color)
{
    if (m_vertices.size() + 2 > MAX_VERTICES)
    {
        return;
    }
    m_vertices.push_back({from, color});
    m_vertices.push_back({to, color});
}

void DebugDraw::addAABB(const glm::vec3& min, const glm::vec3& max, const glm::vec3& color)
{
    glm::vec3 v[8] = {
        {min.x, min.y, min.z}, {max.x, min.y, min.z},
        {max.x, max.y, min.z}, {min.x, max.y, min.z},
        {min.x, min.y, max.z}, {max.x, min.y, max.z},
        {max.x, max.y, max.z}, {min.x, max.y, max.z}
    };

    // Bottom face
    addLine(v[0], v[1], color); addLine(v[1], v[2], color);
    addLine(v[2], v[3], color); addLine(v[3], v[0], color);
    // Top face
    addLine(v[4], v[5], color); addLine(v[5], v[6], color);
    addLine(v[6], v[7], color); addLine(v[7], v[4], color);
    // Verticals
    addLine(v[0], v[4], color); addLine(v[1], v[5], color);
    addLine(v[2], v[6], color); addLine(v[3], v[7], color);
}

void DebugDraw::addAxes(const glm::vec3& origin, float size)
{
    addLine(origin, origin + glm::vec3(size, 0.0f, 0.0f), {1.0f, 0.0f, 0.0f});
    addLine(origin, origin + glm::vec3(0.0f, size, 0.0f), {0.0f, 1.0f, 0.0f});
    addLine(origin, origin + glm::vec3(0.0f, 0.0f, size), {0.0f, 0.0f, 1.0f});
}

void DebugDraw::addSphere(const glm::vec3& center, float radius, const glm::vec3& color,
                           int segments)
{
    float step = glm::two_pi<float>() / static_cast<float>(segments);

    for (int i = 0; i < segments; ++i)
    {
        float a0 = step * static_cast<float>(i);
        float a1 = step * static_cast<float>(i + 1);

        // XY circle
        addLine(center + glm::vec3(std::cos(a0), std::sin(a0), 0.0f) * radius,
                center + glm::vec3(std::cos(a1), std::sin(a1), 0.0f) * radius, color);
        // XZ circle
        addLine(center + glm::vec3(std::cos(a0), 0.0f, std::sin(a0)) * radius,
                center + glm::vec3(std::cos(a1), 0.0f, std::sin(a1)) * radius, color);
        // YZ circle
        addLine(center + glm::vec3(0.0f, std::cos(a0), std::sin(a0)) * radius,
                center + glm::vec3(0.0f, std::cos(a1), std::sin(a1)) * radius, color);
    }
}

void DebugDraw::render(const glm::mat4& view, const glm::mat4& projection)
{
    if (!m_initialized || !enabled)
    {
        return;
    }

    if (showGrid)
    {
        addGridLines();
    }
    if (showAxes)
    {
        addAxes(glm::vec3(0.0f), 1.0f);
    }

    if (m_vertices.empty())
    {
        return;
    }

    m_vao->bind();
    m_vao->setVertexData(m_vertices.data(),
                         m_vertices.size() * sizeof(Vertex),
                         renderer::BufferUsage::Dynamic);

    m_shader->bind();
    m_shader->setUniformMat4("u_view", glm::value_ptr(view));
    m_shader->setUniformMat4("u_projection", glm::value_ptr(projection));

    m_device.setDepthTest(true);
    m_device.setDepthWrite(false);
    m_device.setBlendEnabled(true);
    m_device.setBlendFunc(renderer::BlendFactor::SrcAlpha,
                          renderer::BlendFactor::OneMinusSrcAlpha);

    m_vao->drawArrays(renderer::PrimitiveTopology::Lines, 0,
                      static_cast<int>(m_vertices.size()));

    m_device.setDepthWrite(true);
    m_device.setBlendEnabled(false);
}

void DebugDraw::shutdown()
{
    m_shader.reset();
    m_vao.reset();
    m_initialized = false;
}

void DebugDraw::addGridLines()
{
    const float extent = 10.0f;
    const float step = 1.0f;
    const glm::vec3 gridColor(0.3f, 0.3f, 0.3f);

    for (float x = -extent; x <= extent; x += step)
    {
        float brightness = (x == 0.0f) ? 0.0f : 1.0f;
        glm::vec3 c = gridColor * brightness;
        if (brightness > 0.0f)
        {
            addLine(glm::vec3(x, 0.0f, -extent), glm::vec3(x, 0.0f, extent), c);
        }
    }
    for (float z = -extent; z <= extent; z += step)
    {
        float brightness = (z == 0.0f) ? 0.0f : 1.0f;
        glm::vec3 c = gridColor * brightness;
        if (brightness > 0.0f)
        {
            addLine(glm::vec3(-extent, 0.0f, z), glm::vec3(extent, 0.0f, z), c);
        }
    }
}

} // namespace placeholder::editor
