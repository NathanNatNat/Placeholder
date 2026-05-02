#pragma once

#include "render_types.h"
#include "texture.h"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace placeholder::renderer
{

class ShaderProgram;

/// Blend mode presets matching WoW-style material categories.
enum class BlendMode
{
    Opaque,
    AlphaTest,
    AlphaBlend,
    Additive,
    Modulate,
};

/// Describes the visual properties of a surface.
///
/// Binds a shader, up to 4 texture slots, colors, and render state.
struct Material
{
    ShaderProgram* shader = nullptr;

    Texture* diffuseTexture = nullptr;
    Texture* emissiveTexture = nullptr;
    Texture* normalTexture = nullptr;
    Texture* envTexture = nullptr;

    glm::vec3 diffuseColor{1.0f};
    glm::vec3 emissiveColor{0.0f};
    float opacity = 1.0f;
    float alphaTestThreshold = 0.5f;

    BlendMode blendMode = BlendMode::Opaque;
    CullMode cullMode = CullMode::Back;
    bool depthWrite = true;

    /// Build a RenderStateDesc from this material's blend mode and settings.
    RenderStateDesc buildRenderState() const;
};

} // namespace placeholder::renderer
