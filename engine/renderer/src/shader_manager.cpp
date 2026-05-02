#include "renderer/shader_manager.h"
#include "renderer/shader_preprocessor.h"
#include "renderer/opengl_render_device.h"

#include "core/file_io.h"
#include "core/logging.h"

#include <stdexcept>

namespace placeholder::renderer
{

ShaderManager::ShaderManager(OpenGLRenderDevice& device,
                             const std::filesystem::path& shaderDirectory)
    : m_device(device)
    , m_shaderDirectory(shaderDirectory)
{
    auto logger = core::getLogger("renderer");
    logger->info("Shader manager initialized (directory: {})", m_shaderDirectory.string());
}

void ShaderManager::registerShader(const std::string& name, const ShaderManifestEntry& entry)
{
    m_manifest[name] = entry;
}

std::unique_ptr<ShaderProgram> ShaderManager::createProgram(
    const std::string& name,
    const std::vector<std::pair<std::string, std::string>>& defines)
{
    auto logger = core::getLogger("renderer");

    auto manifestIt = m_manifest.find(name);
    if (manifestIt == m_manifest.end())
    {
        throw std::runtime_error("Unknown shader: " + name);
    }

    const auto& entry = manifestIt->second;

    auto vertSource = core::readFileText(m_shaderDirectory / entry.vertexFile);
    if (!vertSource)
    {
        throw std::runtime_error("Failed to read vertex shader: " + entry.vertexFile);
    }

    auto fragSource = core::readFileText(m_shaderDirectory / entry.fragmentFile);
    if (!fragSource)
    {
        throw std::runtime_error("Failed to read fragment shader: " + entry.fragmentFile);
    }

    auto vertResult = preprocessShader(*vertSource, m_shaderDirectory, defines);
    if (!vertResult.success)
    {
        throw std::runtime_error("Vertex shader preprocess failed: " + vertResult.errorMessage);
    }

    auto fragResult = preprocessShader(*fragSource, m_shaderDirectory, defines);
    if (!fragResult.success)
    {
        throw std::runtime_error("Fragment shader preprocess failed: " + fragResult.errorMessage);
    }

    auto program = std::make_unique<ShaderProgram>(m_device, vertResult.source, fragResult.source);

    m_activePrograms[name].insert(program.get());

    logger->debug("Created shader program '{}' (handle={})", name, program->handle());
    return program;
}

void ShaderManager::reloadAll()
{
    auto logger = core::getLogger("renderer");
    logger->info("Reloading all shaders...");

    m_sourceCache.clear();

    size_t successCount = 0;
    size_t failCount = 0;

    for (auto& [name, programs] : m_activePrograms)
    {
        if (programs.empty())
        {
            continue;
        }

        auto manifestIt = m_manifest.find(name);
        if (manifestIt == m_manifest.end())
        {
            failCount += programs.size();
            continue;
        }

        try
        {
            const auto& entry = manifestIt->second;

            auto vertSource = core::readFileText(m_shaderDirectory / entry.vertexFile);
            auto fragSource = core::readFileText(m_shaderDirectory / entry.fragmentFile);
            if (!vertSource || !fragSource)
            {
                throw std::runtime_error("Failed to read shader files");
            }

            auto vertResult = preprocessShader(*vertSource, m_shaderDirectory);
            auto fragResult = preprocessShader(*fragSource, m_shaderDirectory);
            if (!vertResult.success || !fragResult.success)
            {
                throw std::runtime_error("Shader preprocess failed");
            }

            for (auto* program : programs)
            {
                if (program->recompile(vertResult.source, fragResult.source))
                {
                    successCount++;
                }
                else
                {
                    failCount++;
                    logger->error("Failed to recompile shader program: {}", name);
                }
            }
        }
        catch (const std::exception& e)
        {
            failCount += programs.size();
            logger->error("Failed to reload shader '{}': {}", name, e.what());
        }
    }

    logger->info("Shader reload complete: {} succeeded, {} failed", successCount, failCount);
}

void ShaderManager::unregisterProgram(ShaderProgram* program, const std::string& name)
{
    auto it = m_activePrograms.find(name);
    if (it != m_activePrograms.end())
    {
        it->second.erase(program);
    }
}

const ShaderManager::CachedSource& ShaderManager::loadSource(const std::string& name)
{
    auto cacheIt = m_sourceCache.find(name);
    if (cacheIt != m_sourceCache.end())
    {
        return cacheIt->second;
    }

    auto manifestIt = m_manifest.find(name);
    if (manifestIt == m_manifest.end())
    {
        throw std::runtime_error("Unknown shader: " + name);
    }

    const auto& entry = manifestIt->second;

    auto vertSource = core::readFileText(m_shaderDirectory / entry.vertexFile);
    if (!vertSource)
    {
        throw std::runtime_error("Failed to read vertex shader: " + entry.vertexFile);
    }

    auto fragSource = core::readFileText(m_shaderDirectory / entry.fragmentFile);
    if (!fragSource)
    {
        throw std::runtime_error("Failed to read fragment shader: " + entry.fragmentFile);
    }

    CachedSource cached{std::move(*vertSource), std::move(*fragSource)};
    auto [insertIt, _] = m_sourceCache.emplace(name, std::move(cached));
    return insertIt->second;
}

} // namespace placeholder::renderer
