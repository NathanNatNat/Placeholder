#include "editor/editor_panels.h"
#include "editor/icon_font.h"

#include "renderer/forward_pipeline.h"
#include "renderer/render_types.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <array>
#include <numeric>

namespace placeholder::editor
{

static constexpr size_t FPS_HISTORY_SIZE = 120;
static std::array<float, FPS_HISTORY_SIZE> s_frameTimeHistory = {};
static size_t s_frameTimeIndex = 0;
static float s_fpsSmoothed = 0.0f;

void drawScenePanel(const renderer::FrameContext& /*ctx*/,
                    renderer::ForwardPipeline& /*pipeline*/)
{
    if (!ImGui::Begin(ICON_FA_CUBES " Scene"))
    {
        ImGui::End();
        return;
    }

    ImGui::TextDisabled("(Scene hierarchy will be populated with ECS entities)");
    ImGui::Separator();
    ImGui::Text("Render items submitted per frame shown in Renderer Stats.");

    ImGui::End();
}

void drawPropertiesPanel()
{
    if (!ImGui::Begin(ICON_FA_COG " Properties"))
    {
        ImGui::End();
        return;
    }

    ImGui::TextDisabled("Select an item in the Scene panel to inspect its properties.");

    ImGui::End();
}

void drawAssetBrowserPanel()
{
    if (!ImGui::Begin(ICON_FA_FOLDER_OPEN " Asset Browser"))
    {
        ImGui::End();
        return;
    }

    ImGui::TextDisabled("(Asset browser will list loaded resources from the ResourcePool)");

    ImGui::End();
}

void drawRendererStatsPanel(const renderer::FrameContext& ctx)
{
    if (!ImGui::Begin(ICON_FA_CHART_BAR " Renderer Stats"))
    {
        ImGui::End();
        return;
    }

    ImGui::SeparatorText("Viewport");
    ImGui::Text("Resolution: %d x %d", ctx.viewportWidth, ctx.viewportHeight);

    ImGui::SeparatorText("View Matrix");
    const float* v = glm::value_ptr(ctx.viewMatrix);
    ImGui::Text("%.2f  %.2f  %.2f  %.2f", v[0], v[4], v[8],  v[12]);
    ImGui::Text("%.2f  %.2f  %.2f  %.2f", v[1], v[5], v[9],  v[13]);
    ImGui::Text("%.2f  %.2f  %.2f  %.2f", v[2], v[6], v[10], v[14]);
    ImGui::Text("%.2f  %.2f  %.2f  %.2f", v[3], v[7], v[11], v[15]);

    ImGui::End();
}

void drawCameraInfoPanel(const renderer::FrameContext& /*ctx*/,
                         input::InputManager& /*inputManager*/)
{
    if (!ImGui::Begin(ICON_FA_CAMERA " Camera"))
    {
        ImGui::End();
        return;
    }

    ImGui::TextDisabled("(Camera details will be populated when camera exposes its state)");

    ImGui::End();
}

void drawPerformanceOverlay(const renderer::FrameContext& ctx)
{
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
                           | ImGuiWindowFlags_AlwaysAutoResize
                           | ImGuiWindowFlags_NoFocusOnAppearing
                           | ImGuiWindowFlags_NoNav;

    const float padding = 10.0f;
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 pos(viewport->WorkPos.x + viewport->WorkSize.x - padding,
               viewport->WorkPos.y + padding);
    ImVec2 pivot(1.0f, 0.0f);
    ImGui::SetNextWindowPos(pos, ImGuiCond_Always, pivot);
    ImGui::SetNextWindowBgAlpha(0.6f);

    if (!ImGui::Begin("##PerfOverlay", nullptr, flags))
    {
        ImGui::End();
        return;
    }

    float dt = ctx.deltaTime;
    s_frameTimeHistory[s_frameTimeIndex] = dt * 1000.0f;
    s_frameTimeIndex = (s_frameTimeIndex + 1) % FPS_HISTORY_SIZE;

    float avgDt = std::accumulate(s_frameTimeHistory.begin(), s_frameTimeHistory.end(), 0.0f)
                  / static_cast<float>(FPS_HISTORY_SIZE);
    float currentFps = (dt > 0.0f) ? 1.0f / dt : 0.0f;
    s_fpsSmoothed = s_fpsSmoothed * 0.95f + currentFps * 0.05f;

    ImGui::Text(ICON_FA_GAUGE_HIGH " %.0f FPS (%.1f ms)", s_fpsSmoothed, avgDt);

    float maxTime = *std::max_element(s_frameTimeHistory.begin(), s_frameTimeHistory.end());
    maxTime = std::max(maxTime, 1.0f);

    ImGui::PlotLines("##FrameTime", s_frameTimeHistory.data(),
                     static_cast<int>(FPS_HISTORY_SIZE),
                     static_cast<int>(s_frameTimeIndex),
                     nullptr, 0.0f, maxTime * 1.2f,
                     ImVec2(200.0f, 40.0f));

    ImGui::Text("Viewport: %d x %d", ctx.viewportWidth, ctx.viewportHeight);

    ImGui::End();
}

} // namespace placeholder::editor
