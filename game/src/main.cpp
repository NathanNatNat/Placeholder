#include "core/logging.h"
#include "core/config.h"
#include "platform/window.h"
#include "renderer/opengl_render_device.h"
#include "renderer/shader_manager.h"
#include "renderer/forward_pipeline.h"
#include "renderer/render_types.h"

#include <spdlog/spdlog.h>

#include <cstdlib>
#include <exception>
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

        window.setKeyCallback([&window, &pipeline](int key, int action, int /*mods*/)
        {
            if (action != GLFW_PRESS)
            {
                return;
            }

            if (key == GLFW_KEY_ESCAPE)
            {
                window.requestClose();
            }
            else if (key == GLFW_KEY_F11)
            {
                window.toggleFullscreen();
            }
            else if (key == GLFW_KEY_F1)
            {
                pipeline.wireframeEnabled = !pipeline.wireframeEnabled;
                spdlog::info("Wireframe: {}", pipeline.wireframeEnabled ? "on" : "off");
            }
        });

        while (!window.shouldClose())
        {
            window.pollEvents();

            placeholder::renderer::FrameContext frameCtx;
            frameCtx.viewportWidth = window.framebufferWidth();
            frameCtx.viewportHeight = window.framebufferHeight();

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
