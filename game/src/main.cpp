#include "core/logging.h"
#include "core/config.h"
#include "platform/window.h"

#include <spdlog/spdlog.h>

#include <cstdlib>
#include <exception>

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

        window.setKeyCallback([&window](int key, int action, int /*mods*/)
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
        });

        while (!window.shouldClose())
        {
            window.pollEvents();

            glViewport(0, 0, window.framebufferWidth(), window.framebufferHeight());
            glClearColor(0.39f, 0.58f, 0.93f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            window.swapBuffers();
        }
    }
    catch (const std::exception& e)
    {
        spdlog::critical("Fatal error: {}", e.what());
        return EXIT_FAILURE;
    }

    spdlog::info("Shutdown complete");
    return EXIT_SUCCESS;
}
