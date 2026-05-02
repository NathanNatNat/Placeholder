#include "core/logging.h"
#include "core/config.h"
#include "core/console.h"
#include "platform/window.h"
#include "renderer/opengl_render_device.h"
#include "renderer/shader_manager.h"
#include "renderer/forward_pipeline.h"
#include "renderer/render_types.h"
#include "renderer/texture.h"
#include "renderer/texture_loader.h"
#include "renderer/mesh.h"
#include "renderer/material.h"
#include "input/input_manager.h"
#include "input/fly_camera.h"
#include "input/orbit_camera.h"
#include "input/camera.h"
#include "editor/editor.h"
#include "editor/debug_draw.h"

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <cstdlib>
#include <exception>
#include <memory>
#include <string>
#include <vector>

static void registerConsoleCommands(placeholder::core::Console& console,
                                    placeholder::renderer::ForwardPipeline& pipeline,
                                    placeholder::renderer::ShaderManager& shaderManager,
                                    placeholder::editor::DebugDraw& debugDraw,
                                    placeholder::editor::Editor& editor,
                                    bool& usingFlyCamera,
                                    placeholder::input::Camera*& activeCamera,
                                    placeholder::input::FlyCamera* flyCamera,
                                    placeholder::input::OrbitCamera* orbitCamera)
{
    console.registerCommand("wireframe", "Toggle wireframe rendering [on|off]",
        [&pipeline](const std::vector<std::string>& args) -> std::string
        {
            if (!args.empty())
            {
                pipeline.wireframeEnabled = (args[0] == "on" || args[0] == "1" || args[0] == "true");
            }
            else
            {
                pipeline.wireframeEnabled = !pipeline.wireframeEnabled;
            }
            return std::string("Wireframe: ") + (pipeline.wireframeEnabled ? "on" : "off");
        });

    console.registerCommand("camera", "Switch camera mode [fly|orbit]",
        [&usingFlyCamera, &activeCamera, flyCamera, orbitCamera]
        (const std::vector<std::string>& args) -> std::string
        {
            if (!args.empty())
            {
                usingFlyCamera = (args[0] == "fly");
            }
            else
            {
                usingFlyCamera = !usingFlyCamera;
            }
            activeCamera = usingFlyCamera
                ? static_cast<placeholder::input::Camera*>(flyCamera)
                : static_cast<placeholder::input::Camera*>(orbitCamera);
            return std::string("Camera: ") + (usingFlyCamera ? "fly" : "orbit");
        });

    console.registerCommand("reload", "Reload all shaders from disk",
        [&shaderManager](const std::vector<std::string>&) -> std::string
        {
            shaderManager.reloadAll();
            return "Shaders reloaded";
        });

    console.registerCommand("grid", "Toggle debug grid [on|off]",
        [&debugDraw](const std::vector<std::string>& args) -> std::string
        {
            if (!args.empty())
            {
                debugDraw.showGrid = (args[0] == "on" || args[0] == "1" || args[0] == "true");
            }
            else
            {
                debugDraw.showGrid = !debugDraw.showGrid;
            }
            return std::string("Grid: ") + (debugDraw.showGrid ? "on" : "off");
        });

    console.registerCommand("axes", "Toggle debug axes [on|off]",
        [&debugDraw](const std::vector<std::string>& args) -> std::string
        {
            if (!args.empty())
            {
                debugDraw.showAxes = (args[0] == "on" || args[0] == "1" || args[0] == "true");
            }
            else
            {
                debugDraw.showAxes = !debugDraw.showAxes;
            }
            return std::string("Axes: ") + (debugDraw.showAxes ? "on" : "off");
        });

    console.registerCommand("debug", "Toggle all debug drawing [on|off]",
        [&debugDraw](const std::vector<std::string>& args) -> std::string
        {
            if (!args.empty())
            {
                debugDraw.enabled = (args[0] == "on" || args[0] == "1" || args[0] == "true");
            }
            else
            {
                debugDraw.enabled = !debugDraw.enabled;
            }
            return std::string("Debug drawing: ") + (debugDraw.enabled ? "on" : "off");
        });

    console.registerCommand("demo", "Toggle ImGui demo window",
        [&editor](const std::vector<std::string>&) -> std::string
        {
            editor.showDemoWindow = !editor.showDemoWindow;
            return std::string("Demo window: ") + (editor.showDemoWindow ? "on" : "off");
        });

    console.registerCommand("clear", "Clear console output",
        [&console](const std::vector<std::string>&) -> std::string
        {
            console.clearOutput();
            return "";
        });

    console.registerCommand("help", "List all commands, or show help for a specific command",
        [&console](const std::vector<std::string>& args) -> std::string
        {
            return console.helpText(args.empty() ? "" : args[0]);
        });
}

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
        shaderManager.registerShader("wow_model", {"wow_model.vert", "wow_model.frag"});
        shaderManager.registerShader("skybox", {"skybox.vert", "skybox.frag"});
        shaderManager.registerShader("debug_line", {"debug_line.vert", "debug_line.frag"});

        placeholder::renderer::ForwardPipeline pipeline(renderDevice, shaderManager);
        pipeline.initialize();

        placeholder::renderer::TextureLoader textureLoader(renderDevice);

        auto whiteTexture = textureLoader.createWhiteTexture();

        std::string skyboxDir = std::string(PLACEHOLDER_ROOT_DIR) + "/assets/textures/skybox/";
        std::array<std::string, 6> skyboxFaces = {
            skyboxDir + "right.jpg",
            skyboxDir + "left.jpg",
            skyboxDir + "top.jpg",
            skyboxDir + "bottom.jpg",
            skyboxDir + "front.jpg",
            skyboxDir + "back.jpg"
        };
        auto skyboxTexture = textureLoader.loadCubemap(skyboxFaces);
        if (skyboxTexture)
        {
            pipeline.setSkyboxTexture(skyboxTexture.get());
            spdlog::info("Skybox loaded");
        }

        placeholder::input::InputManager inputManager;
        inputManager.initialize(config, window);

        auto flyCamera = std::make_unique<placeholder::input::FlyCamera>();
        auto orbitCamera = std::make_unique<placeholder::input::OrbitCamera>();

        placeholder::input::Camera* activeCamera = orbitCamera.get();
        bool usingFlyCamera = false;

        placeholder::editor::Editor editor;
        editor.initialize(window.handle(), window.dpiScale(), PLACEHOLDER_ROOT_DIR);

        placeholder::editor::DebugDraw debugDraw(renderDevice, shaderManager);
        debugDraw.initialize();

        placeholder::core::Console console;
        registerConsoleCommands(console, pipeline, shaderManager, debugDraw, editor,
                               usingFlyCamera, activeCamera,
                               flyCamera.get(), orbitCamera.get());

        pipeline.onDebugPass = [&debugDraw](const placeholder::renderer::FrameContext& ctx)
        {
            debugDraw.render(ctx.viewMatrix, ctx.projectionMatrix);
        };

        pipeline.onImGuiPass = [&editor](const placeholder::renderer::FrameContext&)
        {
            editor.renderDrawData();
        };

        window.setKeyCallback([&window, &editor](int key, int action, int /*mods*/)
        {
            if (editor.wantsKeyboard())
            {
                return;
            }
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
            editor.beginFrame();

            inputManager.update(window);

            if (!editor.wantsKeyboard())
            {
                if (inputManager.isTriggered("ToggleWireframe"))
                {
                    pipeline.wireframeEnabled = !pipeline.wireframeEnabled;
                }

                if (inputManager.isTriggered("ToggleCamera"))
                {
                    usingFlyCamera = !usingFlyCamera;
                    activeCamera = usingFlyCamera
                        ? static_cast<placeholder::input::Camera*>(flyCamera.get())
                        : static_cast<placeholder::input::Camera*>(orbitCamera.get());
                }
            }

            if (!editor.wantsMouse())
            {
                activeCamera->update(inputManager, deltaTime);
            }

            int fbWidth = window.framebufferWidth();
            int fbHeight = window.framebufferHeight();

            placeholder::renderer::FrameContext frameCtx;
            frameCtx.viewportWidth = fbWidth;
            frameCtx.viewportHeight = fbHeight;
            frameCtx.deltaTime = deltaTime;
            frameCtx.viewMatrix = activeCamera->getViewMatrix();
            frameCtx.projectionMatrix = activeCamera->getProjectionMatrix(
                fbHeight > 0 ? static_cast<float>(fbWidth) / static_cast<float>(fbHeight) : 1.0f);

            debugDraw.clear();

            editor.drawEditor(frameCtx, pipeline, inputManager, console);
            editor.rebuildFontsForDpi(window.dpiScale());

            int vpWidth = editor.viewportWidth();
            int vpHeight = editor.viewportHeight();
            if (vpWidth > 0 && vpHeight > 0)
            {
                frameCtx.viewportWidth = vpWidth;
                frameCtx.viewportHeight = vpHeight;
                float vpAspect = static_cast<float>(vpWidth) / static_cast<float>(vpHeight);
                frameCtx.projectionMatrix = activeCamera->getProjectionMatrix(vpAspect);
            }

            pipeline.execute(frameCtx);

            window.swapBuffers();
        }

        editor.shutdown();
        debugDraw.shutdown();
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
