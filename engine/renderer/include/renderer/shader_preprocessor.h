#pragma once

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace placeholder::renderer
{

/// Result of preprocessing a GLSL shader source string.
struct ShaderPreprocessResult
{
    std::string source;
    bool success = false;
    std::string errorMessage;
};

/// Preprocess GLSL source: inject \#define directives after the \#version line,
/// and resolve \#include "filename" directives relative to a base directory.
///
/// @param source      Raw GLSL source text.
/// @param basePath    Directory for resolving \#include paths.
/// @param defines     Pairs of (name, value). Value may be empty for flag defines.
/// @return Processed source or error information.
ShaderPreprocessResult preprocessShader(
    const std::string& source,
    const std::filesystem::path& basePath,
    const std::vector<std::pair<std::string, std::string>>& defines = {});

} // namespace placeholder::renderer
