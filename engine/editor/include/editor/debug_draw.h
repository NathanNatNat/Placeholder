#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include <vector>
#include <memory>

namespace placeholder::renderer
{
    class OpenGLRenderDevice;
    class ShaderManager;
    class ShaderProgram;
    class VertexArray;
}

namespace placeholder::editor
{

/// Immediate-mode debug drawing system.
///
/// Submit lines, boxes, and other debug primitives each frame.
/// Geometry is batched into a single VBO and drawn in the debug pass.
class DebugDraw
{
public:
    DebugDraw(renderer::OpenGLRenderDevice& device, renderer::ShaderManager& shaderManager);
    ~DebugDraw();

    /// Initialize GPU resources (shader, VAO).
    void initialize();

    /// Clear all submitted primitives. Call at the start of each frame.
    void clear();

    /// Submit a line segment.
    void addLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color);

    /// Submit an axis-aligned bounding box as 12 line segments.
    void addAABB(const glm::vec3& min, const glm::vec3& max, const glm::vec3& color);

    /// Submit a coordinate axis gizmo (red=X, green=Y, blue=Z).
    void addAxes(const glm::vec3& origin, float size);

    /// Submit a wireframe sphere approximation.
    void addSphere(const glm::vec3& center, float radius, const glm::vec3& color, int segments = 16);

    /// Upload batched geometry and draw all primitives.
    void render(const glm::mat4& view, const glm::mat4& projection);

    /// Release GPU resources.
    void shutdown();

    bool enabled = true;
    bool showGrid = true;
    bool showAxes = true;

private:
    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 color;
    };

    void addGridLines();

    renderer::OpenGLRenderDevice& m_device;
    renderer::ShaderManager& m_shaderManager;

    std::unique_ptr<renderer::ShaderProgram> m_shader;
    std::unique_ptr<renderer::VertexArray> m_vao;

    std::vector<Vertex> m_vertices;
    bool m_initialized = false;

    static constexpr size_t MAX_VERTICES = 65536;
};

} // namespace placeholder::editor
