#pragma once

#include "render_types.h"

#include <string>

namespace placeholder::renderer
{

/// Abstract rendering device interface.
///
/// Covers pipeline-level operations (state management, viewport, draw calls).
/// Resource creation (buffers, shaders, textures) is done through concrete
/// backend types (e.g. VertexArray, ShaderProgram) which take the concrete
/// device by reference.
class RenderDevice
{
public:
    virtual ~RenderDevice() = default;

    /// Called at the start of each frame.
    virtual void beginFrame(const FrameContext& ctx) = 0;

    /// Called at the end of each frame.
    virtual void endFrame() = 0;

    /// Set the viewport rectangle in pixels.
    virtual void setViewport(int x, int y, int width, int height) = 0;

    /// Set the color used by clear().
    virtual void setClearColor(float r, float g, float b, float a) = 0;

    /// Clear framebuffer attachments.
    virtual void clear(bool color, bool depth, bool stencil) = 0;

    /// @name Render State
    /// @{
    virtual void setDepthTest(bool enabled) = 0;
    virtual void setDepthWrite(bool enabled) = 0;
    virtual void setDepthFunc(DepthFunc func) = 0;
    virtual void setCullMode(CullMode mode) = 0;
    virtual void setBlendEnabled(bool enabled) = 0;
    virtual void setBlendFunc(BlendFactor src, BlendFactor dst) = 0;
    virtual void setBlendFuncSeparate(BlendFactor srcRgb, BlendFactor dstRgb,
                                      BlendFactor srcAlpha, BlendFactor dstAlpha) = 0;
    virtual void setPolygonMode(PolygonMode mode) = 0;
    /// @}

    /// Invalidate cached render state. Call before rendering when an external
    /// system (e.g. ImGui) may have modified GPU state behind the cache.
    virtual void invalidateStateCache() = 0;

    /// @name Draw Commands
    /// @{
    virtual void drawArrays(PrimitiveTopology topology, int first, int count) = 0;
    virtual void drawElements(PrimitiveTopology topology, int count,
                              IndexType indexType, size_t offset) = 0;
    /// @}

    /// @return true if the named GL extension is available.
    virtual bool hasExtension(const std::string& name) const = 0;

    /// @return Maximum supported anisotropic filtering level.
    virtual float maxAnisotropy() const = 0;
};

} // namespace placeholder::renderer
