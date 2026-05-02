#include "renderer/render_state.h"

namespace placeholder::renderer
{

void applyRenderState(RenderDevice& device, const RenderStateDesc& state)
{
    device.setDepthTest(state.depthTestEnabled);
    device.setDepthWrite(state.depthWriteEnabled);
    device.setDepthFunc(state.depthFunc);
    device.setCullMode(state.cullMode);
    device.setBlendEnabled(state.blendEnabled);
    if (state.blendEnabled)
    {
        device.setBlendFunc(state.srcBlend, state.dstBlend);
    }
    device.setPolygonMode(state.polygonMode);
}

} // namespace placeholder::renderer
