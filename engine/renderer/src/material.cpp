#include "renderer/material.h"

namespace placeholder::renderer
{

RenderStateDesc Material::buildRenderState() const
{
    RenderStateDesc state;
    state.depthTestEnabled = true;
    state.depthFunc = DepthFunc::LessEqual;
    state.cullMode = cullMode;
    state.depthWriteEnabled = depthWrite;

    switch (blendMode)
    {
        case BlendMode::Opaque:
            state.blendEnabled = false;
            break;

        case BlendMode::AlphaTest:
            state.blendEnabled = false;
            break;

        case BlendMode::AlphaBlend:
            state.blendEnabled = true;
            state.srcBlend = BlendFactor::SrcAlpha;
            state.dstBlend = BlendFactor::OneMinusSrcAlpha;
            break;

        case BlendMode::Additive:
            state.blendEnabled = true;
            state.srcBlend = BlendFactor::SrcAlpha;
            state.dstBlend = BlendFactor::One;
            break;

        case BlendMode::Modulate:
            state.blendEnabled = true;
            state.srcBlend = BlendFactor::DstColor;
            state.dstBlend = BlendFactor::Zero;
            break;
    }

    return state;
}

} // namespace placeholder::renderer
