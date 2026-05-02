#pragma once

#include <string>

struct GLFWwindow;

namespace placeholder::core { class Console; }
namespace placeholder::renderer
{
    class OpenGLRenderDevice;
    class ShaderManager;
    class ForwardPipeline;
    struct FrameContext;
}
namespace placeholder::input { class InputManager; }

namespace placeholder::editor
{

/// ImGui-based editor shell.
///
/// Owns the ImGui context, font atlas (FreeType + icon font),
/// and all editor panels (scene, properties, console, stats, debug draw).
class Editor
{
public:
    /// Initialize ImGui context, load fonts with DPI scaling, install backends.
    /// @param window  Raw GLFW window handle.
    /// @param dpiScale  Current monitor DPI scale factor (1.0 = 96 DPI).
    /// @param rootDir  Path to the project root (for locating font files).
    void initialize(GLFWwindow* window, float dpiScale, const std::string& rootDir);

    /// Shut down ImGui and release all resources.
    void shutdown();

    /// Begin a new ImGui frame. Call after GLFW event polling, before game logic.
    void beginFrame();

    /// Draw all editor panels. Call after game logic, before pipeline execute.
    void drawEditor(const renderer::FrameContext& ctx,
                    renderer::ForwardPipeline& pipeline,
                    input::InputManager& inputManager,
                    core::Console& console);

    /// Render ImGui draw data via OpenGL. Called by the forward pipeline's imguiPass.
    void renderDrawData();

    /// Rebuild the font atlas for a new DPI scale (e.g. after moving to a different monitor).
    void rebuildFontsForDpi(float dpiScale);

    /// @return true if ImGui wants to capture keyboard input (e.g. typing in a text field).
    bool wantsKeyboard() const;

    /// @return true if ImGui wants to capture mouse input (e.g. hovering over a window).
    bool wantsMouse() const;

    bool showDemoWindow = false;

private:
    void loadFonts(float dpiScale);
    void applyStyle();

    std::string m_rootDir;
    bool m_initialized = false;
    float m_currentDpiScale = 1.0f;

    bool m_showScene = true;
    bool m_showProperties = true;
    bool m_showAssetBrowser = false;
    bool m_showRendererStats = true;
    bool m_showPerformance = true;

    static constexpr float BASE_FONT_SIZE = 16.0f;
};

} // namespace placeholder::editor
