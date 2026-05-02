#pragma once

#include "render_device.h"
#include "render_types.h"

namespace placeholder::renderer
{

/// Apply all fields of a RenderStateDesc to a RenderDevice in one call.
void applyRenderState(RenderDevice& device, const RenderStateDesc& state);

} // namespace placeholder::renderer
