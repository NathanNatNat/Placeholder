#include "core/logging.h"
#include "core/config.h"
#include "platform/window.h"
#include "renderer/opengl_render_device.h"
#include "renderer/shader_manager.h"
#include "renderer/forward_pipeline.h"
#include "renderer/render_types.h"
#include "input/input_manager.h"
#include "input/fly_camera.h"
#include "input/orbit_camera.h"
#include "input/camera.h"

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include <cstdlib>
#include <exception>
#include <memory>
#include <string>

int main(int argc, char* argv[])
{
    placeholder::core::initLogging();
    spdlog::info("Placeholder Engine v0.1");

    placeholder::core::Config config;
    config.loadFromFile(PLACEHOLDER_ROOT_DIR "/assets/config/engine.json");
    config.applyCommandLine(argc, argv);

    try
    {
        placeholder::platform::WindowConfig windowConfig;
        windowConfig.width = config.get<int>("window.width", 1920);
        windowConfig.height = config.get<int>("window.height", 1080);
        windowConfig.title = config.get<std::string>("window.title", "Placeholder Engine");
        windowConfig.fullscreen = config.get<bool>("window.fullscreen", false);
        windowConfig.vsync = config.get<bool>("rendering.vsync", true);

        placeholder::platform::Window window(windowConfig);

        placeholder::renderer::OpenGLRenderDevice renderDevice;
        placeholder::renderer::ShaderManager shaderManager(
            renderDevice,
            std::string(PLACEHOLDER_ROOT_DIR) + "/assets/shaders");

        shaderManager.registerShader("triangle", {"triangle.vert", "triangle.frag"});

        placeholder::renderer::ForwardPipeline pipeline(renderDevice, shaderManager);
        pipeline.initialize();

        placeholder::input::InputManager inputManager;
        inputManager.initialize(config, window);

        auto flyCamera = std::make_unique<placeholder::input::FlyCamera>();
        auto orbitCamera = std::make_unique<placeholder::input::OrbitCamera>();
        placeholder::input::Camera* activeCamera = flyCamera.get();
        bool usingFlyCamera = true;

        window.setKeyCallback([&window](int key, int action, int /*mods*/)
        {
            if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE)
            {
                window.requestClose();
            }
            else if (action == GLFW_PRESS && key == GLFW_KEY_F11)
            {
                window.toggleFullscreen();
            }
        });

        double lastTime = glfwGetTime();

        while (!window.shouldClose())
        {
            double currentTime = glfwGetTime();
            float deltaTime = static_cast<float>(currentTime - lastTime);
            lastTime = currentTime;

            window.pollEvents();
            inputManager.update(window);

            if (inputManager.isTriggered("ToggleWireframe"))
            {
                pipeline.wireframeEnabled = !pipeline.wireframeEnabled;
                spdlog::info("Wireframe: {}", pipeline.wireframeEnabled ? "on" : "off");
            }

            if (inputManager.isTriggered("ToggleCamera"))
            {
                usingFlyCamera = !usingFlyCamera;
                activeCamera = usingFlyCamera
                    ? static_cast<placeholder::input::Camera*>(flyCamera.get())
                    : static_cast<placeholder::input::Camera*>(orbitCamera.get());
                spdlog::info("Camera: {}", usingFlyCamera ? "fly" : "orbit");
            }

            activeCamera->update(inputManager, deltaTime);

            int fbWidth = window.framebufferWidth();
            int fbHeight = window.framebufferHeight();
            float aspect = fbHeight > 0 ? static_cast<float>(fbWidth) / static_cast<float>(fbHeight) : 1.0f;

            placeholder::renderer::FrameContext frameCtx;
            frameCtx.viewportWidth = fbWidth;
            frameCtx.viewportHeight = fbHeight;
            frameCtx.deltaTime = deltaTime;
            frameCtx.viewMatrix = activeCamera->getViewMatrix();
            frameCtx.projectionMatrix = activeCamera->getProjectionMatrix(aspect);

            pipeline.execute(frameCtx);

            window.swapBuffers();
        }

        pipeline.shutdown();
    }
    catch (const std::exception& e)
    {
        spdlog::critical("Fatal error: {}", e.what());
        return EXIT_FAILURE;
    }

    spdlog::info("Shutdown complete");
    return EXIT_SUCCESS;
}
