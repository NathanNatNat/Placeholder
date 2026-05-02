#include "platform/window.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <stdexcept>
#include <string>

namespace placeholder::platform
{

// --- OpenGL debug callback ---

static void GLAPIENTRY glDebugCallback(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei /*length*/,
    const GLchar* message,
    const void* /*userParam*/)
{
    // Skip noise — buffer info, shader stats, etc.
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
    {
        return;
    }

    const char* srcStr = "Unknown";
    switch (source)
    {
        case GL_DEBUG_SOURCE_API:             srcStr = "API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   srcStr = "Window"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: srcStr = "Shader"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     srcStr = "ThirdParty"; break;
        case GL_DEBUG_SOURCE_APPLICATION:     srcStr = "App"; break;
        case GL_DEBUG_SOURCE_OTHER:           srcStr = "Other"; break;
    }

    const char* typeStr = "Unknown";
    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR:               typeStr = "Error"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: typeStr = "Deprecated"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  typeStr = "UB"; break;
        case GL_DEBUG_TYPE_PORTABILITY:         typeStr = "Portability"; break;
        case GL_DEBUG_TYPE_PERFORMANCE:         typeStr = "Performance"; break;
        case GL_DEBUG_TYPE_MARKER:              typeStr = "Marker"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          typeStr = "PushGroup"; break;
        case GL_DEBUG_TYPE_POP_GROUP:           typeStr = "PopGroup"; break;
        case GL_DEBUG_TYPE_OTHER:               typeStr = "Other"; break;
    }

    auto logger = spdlog::get("platform");
    if (!logger)
    {
        logger = spdlog::default_logger();
    }

    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:
            logger->error("GL [{}][{}] ({}): {}", srcStr, typeStr, id, message);
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            logger->warn("GL [{}][{}] ({}): {}", srcStr, typeStr, id, message);
            break;
        case GL_DEBUG_SEVERITY_LOW:
            logger->info("GL [{}][{}] ({}): {}", srcStr, typeStr, id, message);
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            logger->debug("GL [{}][{}] ({}): {}", srcStr, typeStr, id, message);
            break;
    }
}

// --- GLFW callbacks ---

void Window::framebufferSizeCallback(GLFWwindow* glfwWindow, int width, int height)
{
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
    if (!self)
    {
        return;
    }

    self->m_framebufferWidth = width;
    self->m_framebufferHeight = height;

    glfwGetWindowSize(glfwWindow, &self->m_windowWidth, &self->m_windowHeight);

    if (self->m_resizeCallback)
    {
        self->m_resizeCallback(width, height);
    }
}

void Window::windowContentScaleCallback(GLFWwindow* glfwWindow, float xScale, float /*yScale*/)
{
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
    if (!self)
    {
        return;
    }

    float newScale = std::clamp(xScale, 0.5f, 4.0f);
    if (newScale != self->m_dpiScale)
    {
        self->m_dpiScale = newScale;
        spdlog::get("platform")->info("DPI scale changed to {:.2f}", newScale);
    }
}

void Window::keyCallbackDispatch(GLFWwindow* glfwWindow, int key, int /*scancode*/, int action, int mods)
{
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
    if (self && self->m_keyCallback)
    {
        self->m_keyCallback(key, action, mods);
    }
}

void Window::mouseButtonCallbackDispatch(GLFWwindow* glfwWindow, int button, int action, int mods)
{
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
    if (self && self->m_mouseButtonCallback)
    {
        self->m_mouseButtonCallback(button, action, mods);
    }
}

void Window::scrollCallbackDispatch(GLFWwindow* glfwWindow, double xOffset, double yOffset)
{
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
    if (self && self->m_scrollCallback)
    {
        self->m_scrollCallback(xOffset, yOffset);
    }
}

// --- Construction / destruction ---

Window::Window(const WindowConfig& config)
{
    initGlfw(config);
    initGlad();
    setupDebugCallback();
    queryDpi();

    auto logger = spdlog::get("platform");
    logger->info("OpenGL {}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    logger->info("Renderer: {}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    logger->info("Window: {}x{}, framebuffer: {}x{}, DPI scale: {:.2f}",
        m_windowWidth, m_windowHeight, m_framebufferWidth, m_framebufferHeight, m_dpiScale);
}

Window::~Window()
{
    if (m_window)
    {
        glfwDestroyWindow(m_window);
    }
    glfwTerminate();
}

// --- Public API ---

void Window::pollEvents()
{
    glfwPollEvents();
}

void Window::swapBuffers()
{
    glfwSwapBuffers(m_window);
}

bool Window::shouldClose() const
{
    return glfwWindowShouldClose(m_window) != 0;
}

void Window::requestClose()
{
    glfwSetWindowShouldClose(m_window, GLFW_TRUE);
}

void Window::toggleFullscreen()
{
    if (m_isFullscreen)
    {
        glfwSetWindowMonitor(m_window, nullptr,
            m_savedX, m_savedY, m_savedWidth, m_savedHeight, GLFW_DONT_CARE);
        m_isFullscreen = false;
    }
    else
    {
        glfwGetWindowPos(m_window, &m_savedX, &m_savedY);
        glfwGetWindowSize(m_window, &m_savedWidth, &m_savedHeight);

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        m_isFullscreen = true;
    }
}

bool Window::isKeyDown(int glfwKey) const
{
    return glfwGetKey(m_window, glfwKey) == GLFW_PRESS;
}

bool Window::isMouseButtonDown(int glfwButton) const
{
    return glfwGetMouseButton(m_window, glfwButton) == GLFW_PRESS;
}

void Window::getCursorPos(double& x, double& y) const
{
    glfwGetCursorPos(m_window, &x, &y);
}

void Window::setCursorMode(int glfwCursorMode)
{
    glfwSetInputMode(m_window, GLFW_CURSOR, glfwCursorMode);
}

// --- Initialization helpers ---

void Window::initGlfw(const WindowConfig& config)
{
    glfwSetErrorCallback([](int code, const char* desc)
    {
        auto logger = spdlog::get("platform");
        if (logger)
        {
            logger->error("GLFW error {}: {}", code, desc);
        }
    });

    if (!glfwInit())
    {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

    GLFWmonitor* monitor = nullptr;
    int width = config.width;
    int height = config.height;

    if (config.fullscreen)
    {
        monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        width = mode->width;
        height = mode->height;
    }

    m_window = glfwCreateWindow(width, height, config.title.c_str(), monitor, nullptr);
    if (!m_window)
    {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    m_isFullscreen = config.fullscreen;

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(config.vsync ? 1 : 0);

    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);
    glfwSetWindowContentScaleCallback(m_window, windowContentScaleCallback);
    glfwSetKeyCallback(m_window, keyCallbackDispatch);
    glfwSetMouseButtonCallback(m_window, mouseButtonCallbackDispatch);
    glfwSetScrollCallback(m_window, scrollCallbackDispatch);

    glfwGetFramebufferSize(m_window, &m_framebufferWidth, &m_framebufferHeight);
    glfwGetWindowSize(m_window, &m_windowWidth, &m_windowHeight);
}

void Window::initGlad()
{
    int version = gladLoadGL(glfwGetProcAddress);
    if (!version)
    {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
        glfwTerminate();
        throw std::runtime_error("Failed to initialize OpenGL loader (GLAD)");
    }

    int major = GLAD_VERSION_MAJOR(version);
    int minor = GLAD_VERSION_MINOR(version);
    if (major < 4 || (major == 4 && minor < 6))
    {
        std::string msg = "OpenGL 4.6 required, got " + std::to_string(major) + "." + std::to_string(minor);
        glfwDestroyWindow(m_window);
        m_window = nullptr;
        glfwTerminate();
        throw std::runtime_error(msg);
    }
}

void Window::setupDebugCallback()
{
#ifndef NDEBUG
    GLint flags = 0;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(glDebugCallback, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        spdlog::get("platform")->info("OpenGL debug output enabled");
    }
#endif
}

void Window::queryDpi()
{
    float xScale = 1.0f;
    float yScale = 1.0f;
    glfwGetWindowContentScale(m_window, &xScale, &yScale);
    m_dpiScale = std::clamp(xScale, 0.5f, 4.0f);
}

} // namespace placeholder::platform
