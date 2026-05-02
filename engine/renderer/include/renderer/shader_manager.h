#pragma once

#include "shader_program.h"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace placeholder::renderer
{

class OpenGLRenderDevice;

/// Manifest entry mapping a shader name to vertex/fragment source files.
struct ShaderManifestEntry
{
    std::string vertexFile;
    std::string fragmentFile;
};

/// Manages shader loading, caching, and hot-reload.
///
/// Shaders are registered by name in a manifest, then created via createProgram().
/// The manager tracks active programs so reloadAll() can recompile them.
class ShaderManager
{
public:
    ShaderManager(OpenGLRenderDevice& device,
                  const std::filesystem::path& shaderDirectory);

    /// Register a named shader in the manifest.
    void registerShader(const std::string& name, const ShaderManifestEntry& entry);

    /// Create a shader program from a registered name.
    /// @param defines Optional preprocessor defines for shader variants.
    /// @throws std::runtime_error if the shader name is not registered or compilation fails.
    std::unique_ptr<ShaderProgram> createProgram(
        const std::string& name,
        const std::vector<std::pair<std::string, std::string>>& defines = {});

    /// Reload all shaders from disk and recompile active programs.
    void reloadAll();

    /// Notify the manager that a program is being destroyed.
    void unregisterProgram(ShaderProgram* program, const std::string& name);

    /// @return The shader directory path.
    const std::filesystem::path& shaderDirectory() const { return m_shaderDirectory; }

private:
    struct CachedSource
    {
        std::string vertex;
        std::string fragment;
    };

    const CachedSource& loadSource(const std::string& name);

    OpenGLRenderDevice& m_device;
    std::filesystem::path m_shaderDirectory;
    std::unordered_map<std::string, ShaderManifestEntry> m_manifest;
    std::unordered_map<std::string, CachedSource> m_sourceCache;
    std::unordered_map<std::string, std::unordered_set<ShaderProgram*>> m_activePrograms;
};

} // namespace placeholder::renderer
