#include "core/logging.h"
#include "core/config.h"
#include "platform/window.h"
#include "renderer/opengl_render_device.h"
#include "renderer/shader_manager.h"
#include "renderer/forward_pipeline.h"
#include "renderer/render_types.h"
#include "renderer/texture.h"
#include "renderer/texture_loader.h"
#include "renderer/mesh.h"
#include "renderer/mesh_loader.h"
#include "renderer/material.h"
#include "input/input_manager.h"
#include "input/fly_camera.h"
#include "input/orbit_camera.h"
#include "input/camera.h"

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdlib>
#include <exception>
#include <memory>
#include <string>
#include <vector>

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
        shaderManager.registerShader("mesh", {"mesh.vert", "mesh.frag"});
        shaderManager.registerShader("skybox", {"skybox.vert", "skybox.frag"});

        placeholder::renderer::ForwardPipeline pipeline(renderDevice, shaderManager);
        pipeline.initialize();

        placeholder::renderer::TextureLoader textureLoader(renderDevice);
        placeholder::renderer::MeshLoader meshLoader(renderDevice);

        auto whiteTexture = textureLoader.createWhiteTexture();

        std::unique_ptr<placeholder::renderer::Mesh> loadedMesh;
        std::vector<placeholder::renderer::Material> materials;
        std::vector<std::unique_ptr<placeholder::renderer::Texture>> loadedTextures;

        std::string modelPath = config.get<std::string>("model", "");

        if (!modelPath.empty())
        {
            auto model = meshLoader.loadFromFile(modelPath);
            if (model.mesh)
            {
                loadedMesh = std::move(model.mesh);

                for (const auto& loadedMat : model.materials)
                {
                    placeholder::renderer::Material mat;
                    mat.diffuseColor = loadedMat.diffuseColor;
                    mat.opacity = loadedMat.opacity;
                    mat.diffuseTexture = whiteTexture.get();

                    if (!loadedMat.diffuseTexturePath.empty())
                    {
                        auto tex = textureLoader.loadFromFile(loadedMat.diffuseTexturePath);
                        if (tex)
                        {
                            mat.diffuseTexture = tex.get();
                            loadedTextures.push_back(std::move(tex));
                        }
                    }

                    if (loadedMat.opacity < 1.0f)
                    {
                        mat.blendMode = placeholder::renderer::BlendMode::AlphaBlend;
                        mat.depthWrite = false;
                    }

                    materials.push_back(mat);
                }
            }
        }

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

            if (loadedMesh)
            {
                for (size_t i = 0; i < loadedMesh->subMeshCount(); ++i)
                {
                    placeholder::renderer::RenderItem item;
                    item.mesh = loadedMesh.get();
                    item.modelMatrix = glm::mat4(1.0f);
                    item.subMeshIndex = static_cast<int>(i);

                    uint32_t matIdx = loadedMesh->subMesh(i).materialIndex;
                    if (matIdx < materials.size())
                    {
                        item.material = &materials[matIdx];
                    }

                    pipeline.submit(item);
                }
            }

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
