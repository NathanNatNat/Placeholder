#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <functional>
#include <string>

namespace placeholder::platform
{

/// Configuration for creating a Window.
struct WindowConfig
{
    int width = 1920;
    int height = 1080;
    std::string title = "Placeholder Engine";
    bool fullscreen = false;
    bool vsync = true;
};

/// RAII wrapper around a GLFW window with an OpenGL 4.6 Core context.
///
/// Handles window creation, DPI awareness, resize events, and fullscreen toggling.
/// Only one Window instance should exist at a time (GLFW limitation for OpenGL contexts).
class Window
{
public:
    using ResizeCallback = std::function<void(int width, int height)>;
    using KeyCallback = std::function<void(int key, int action, int mods)>;

    /// Create and show a window with an OpenGL 4.6 Core context.
    /// @throws std::runtime_error if GLFW or GLAD initialization fails.
    explicit Window(const WindowConfig& config);

    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

    /// Poll OS events. Call once per frame.
    void pollEvents();

    /// Swap front and back buffers. Call at end of frame.
    void swapBuffers();

    /// @return true if the user requested the window to close.
    bool shouldClose() const;

    /// Request the window to close.
    void requestClose();

    /// Toggle between windowed and borderless fullscreen on the primary monitor.
    void toggleFullscreen();

    /// @return true if the window is currently in fullscreen mode.
    bool isFullscreen() const { return m_isFullscreen; }

    /// @return Current framebuffer width in pixels (for glViewport).
    int framebufferWidth() const { return m_framebufferWidth; }

    /// @return Current framebuffer height in pixels (for glViewport).
    int framebufferHeight() const { return m_framebufferHeight; }

    /// @return Current window width in screen coordinates.
    int windowWidth() const { return m_windowWidth; }

    /// @return Current window height in screen coordinates.
    int windowHeight() const { return m_windowHeight; }

    /// @return DPI scale factor (1.0 at 96 DPI, 2.0 at 192 DPI, etc.).
    float dpiScale() const { return m_dpiScale; }

    /// Set a callback for framebuffer resize events.
    void setResizeCallback(ResizeCallback callback) { m_resizeCallback = std::move(callback); }

    /// Set a callback for key events (press, release, repeat).
    void setKeyCallback(KeyCallback callback) { m_keyCallback = std::move(callback); }

    /// @return The underlying GLFW window handle (for ImGui, input, etc.).
    GLFWwindow* handle() const { return m_window; }

    /// @return true if a key is currently pressed.
    bool isKeyDown(int glfwKey) const;

    /// @return true if a mouse button is currently pressed.
    bool isMouseButtonDown(int glfwButton) const;

    /// Get the current cursor position in screen coordinates.
    void getCursorPos(double& x, double& y) const;

private:
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void windowContentScaleCallback(GLFWwindow* window, float xScale, float yScale);
    static void keyCallbackDispatch(GLFWwindow* window, int key, int scancode, int action, int mods);

    void initGlfw(const WindowConfig& config);
    void initGlad();
    void setupDebugCallback();
    void queryDpi();

    GLFWwindow* m_window = nullptr;
    bool m_isFullscreen = false;

    int m_framebufferWidth = 0;
    int m_framebufferHeight = 0;
    int m_windowWidth = 0;
    int m_windowHeight = 0;
    float m_dpiScale = 1.0f;

    // Saved windowed-mode geometry for restoring after fullscreen
    int m_savedX = 0;
    int m_savedY = 0;
    int m_savedWidth = 0;
    int m_savedHeight = 0;

    ResizeCallback m_resizeCallback;
    KeyCallback m_keyCallback;
};

} // namespace placeholder::platform
