#include "renderer/shader_preprocessor.h"

#include "core/file_io.h"

#include <sstream>
#include <unordered_set>

namespace placeholder::renderer
{

static constexpr int MAX_INCLUDE_DEPTH = 8;

static ShaderPreprocessResult resolveIncludes(
    const std::string& source,
    const std::filesystem::path& basePath,
    std::unordered_set<std::string>& includedFiles,
    int depth)
{
    if (depth > MAX_INCLUDE_DEPTH)
    {
        return {"", false, "Maximum #include depth exceeded"};
    }

    std::istringstream stream(source);
    std::ostringstream output;
    std::string line;

    while (std::getline(stream, line))
    {
        size_t pos = line.find("#include");
        if (pos == std::string::npos)
        {
            output << line << '\n';
            continue;
        }

        size_t quoteStart = line.find('"', pos);
        if (quoteStart == std::string::npos)
        {
            output << line << '\n';
            continue;
        }

        size_t quoteEnd = line.find('"', quoteStart + 1);
        if (quoteEnd == std::string::npos)
        {
            return {"", false, "Unterminated #include directive: " + line};
        }

        std::string filename = line.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
        std::filesystem::path includePath = basePath / filename;
        std::string normalizedPath = includePath.lexically_normal().string();

        if (includedFiles.count(normalizedPath))
        {
            continue;
        }
        includedFiles.insert(normalizedPath);

        auto fileContent = core::readFileText(includePath);
        if (!fileContent)
        {
            return {"", false, "Failed to read #include file: " + includePath.string()};
        }

        auto nested = resolveIncludes(*fileContent, includePath.parent_path(),
                                      includedFiles, depth + 1);
        if (!nested.success)
        {
            return nested;
        }

        output << nested.source;
    }

    return {output.str(), true, ""};
}

ShaderPreprocessResult preprocessShader(
    const std::string& source,
    const std::filesystem::path& basePath,
    const std::vector<std::pair<std::string, std::string>>& defines)
{
    if (source.empty())
    {
        return {"", false, "Empty shader source"};
    }

    std::istringstream stream(source);
    std::ostringstream output;
    std::string line;
    bool versionFound = false;

    while (std::getline(stream, line))
    {
        output << line << '\n';

        if (!versionFound && line.find("#version") != std::string::npos)
        {
            versionFound = true;
            for (const auto& [name, value] : defines)
            {
                if (value.empty())
                {
                    output << "#define " << name << '\n';
                }
                else
                {
                    output << "#define " << name << ' ' << value << '\n';
                }
            }
        }
    }

    std::string withDefines = output.str();

    std::unordered_set<std::string> includedFiles;
    return resolveIncludes(withDefines, basePath, includedFiles, 0);
}

} // namespace placeholder::renderer
