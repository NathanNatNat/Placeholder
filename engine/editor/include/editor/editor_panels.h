#pragma once

#include "renderer/render_types.h"

namespace placeholder::renderer { class ForwardPipeline; }
namespace placeholder::input { class InputManager; }

namespace placeholder::editor
{

/// Scene hierarchy panel — lists submitted render items.
void drawScenePanel(const renderer::FrameContext& ctx,
                    renderer::ForwardPipeline& pipeline);

/// Properties panel — inspect the selected entity/item.
void drawPropertiesPanel();

/// Asset browser panel — shows loaded resources.
void drawAssetBrowserPanel();

/// Renderer statistics panel — draw calls, GPU state, shader info.
void drawRendererStatsPanel(const renderer::FrameContext& ctx);

/// Camera info panel — position, orientation, mode.
void drawCameraInfoPanel(const renderer::FrameContext& ctx,
                         input::InputManager& inputManager);

/// Performance overlay — FPS, frame time graph, memory, resource counts.
void drawPerformanceOverlay(const renderer::FrameContext& ctx);

} // namespace placeholder::editor
