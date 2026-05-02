#include "editor/editor.h"
#include "editor/editor_panels.h"
#include "editor/editor_console.h"
#include "editor/icon_font.h"

#include "core/logging.h"
#include "renderer/forward_pipeline.h"
#include "renderer/render_types.h"
#include "input/input_manager.h"
#include "core/console.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>

namespace placeholder::editor
{

static EditorConsole s_console;

void Editor::initialize(GLFWwindow* window, float dpiScale, const std::string& rootDir)
{
    auto logger = core::getLogger("editor");
    logger->info("Initializing ImGui editor");

    m_rootDir = rootDir;
    m_currentDpiScale = dpiScale;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    loadFonts(dpiScale);
    applyStyle();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    m_initialized = true;
    logger->info("ImGui editor initialized (DPI scale: {:.2f})", dpiScale);
}

void Editor::shutdown()
{
    if (!m_initialized)
    {
        return;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    m_initialized = false;

    auto logger = core::getLogger("editor");
    logger->info("ImGui editor shut down");
}

void Editor::beginFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Editor::drawEditor(const renderer::FrameContext& ctx,
                         renderer::ForwardPipeline& pipeline,
                         input::InputManager& inputManager,
                         core::Console& console)
{
    ImGuiID dockspaceId = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(),
        ImGuiDockNodeFlags_PassthruCentralNode);
    (void)dockspaceId;

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Scene", nullptr, &m_showScene);
            ImGui::MenuItem("Properties", nullptr, &m_showProperties);
            ImGui::MenuItem("Asset Browser", nullptr, &m_showAssetBrowser);
            ImGui::MenuItem("Renderer Stats", nullptr, &m_showRendererStats);
            ImGui::MenuItem("Console", "~", &s_console.visible);
            ImGui::MenuItem("Performance", nullptr, &m_showPerformance);
            ImGui::MenuItem("ImGui Demo", nullptr, &showDemoWindow);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Debug"))
        {
            ImGui::MenuItem("Wireframe", "F1", &pipeline.wireframeEnabled);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if (m_showScene)
    {
        drawScenePanel(ctx, pipeline);
    }
    if (m_showProperties)
    {
        drawPropertiesPanel();
    }
    if (m_showAssetBrowser)
    {
        drawAssetBrowserPanel();
    }
    if (m_showRendererStats)
    {
        drawRendererStatsPanel(ctx);
    }
    if (m_showPerformance)
    {
        drawPerformanceOverlay(ctx);
    }

    s_console.draw(console);

    if (showDemoWindow)
    {
        ImGui::ShowDemoWindow(&showDemoWindow);
    }
}

void Editor::renderDrawData()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Editor::rebuildFontsForDpi(float dpiScale)
{
    if (std::abs(dpiScale - m_currentDpiScale) < 0.01f)
    {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    loadFonts(dpiScale);

    ImGui_ImplOpenGL3_DestroyDeviceObjects();
    ImGui_ImplOpenGL3_CreateDeviceObjects();

    m_currentDpiScale = dpiScale;

    auto logger = core::getLogger("editor");
    logger->info("Rebuilt fonts for DPI scale: {:.2f}", dpiScale);
}

bool Editor::wantsKeyboard() const
{
    return ImGui::GetIO().WantCaptureKeyboard;
}

bool Editor::wantsMouse() const
{
    return ImGui::GetIO().WantCaptureMouse;
}

void Editor::loadFonts(float dpiScale)
{
    ImGuiIO& io = ImGui::GetIO();

    float scaledSize = BASE_FONT_SIZE * dpiScale;
    std::string fontPath = m_rootDir + "/assets/fonts/selawk.ttf";
    std::string iconPath = m_rootDir + "/assets/fonts/fa-solid-900.ttf";

    ImFont* mainFont = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), scaledSize);
    if (!mainFont)
    {
        auto logger = core::getLogger("editor");
        logger->warn("Failed to load Selawik font, using ImGui default");
        io.Fonts->AddFontDefault();
    }

    static const ImWchar iconRanges[] = { 0xE005, 0xF8FF, 0 };
    ImFontConfig iconCfg;
    iconCfg.MergeMode = true;
    iconCfg.GlyphMinAdvanceX = scaledSize;
    iconCfg.PixelSnapH = true;
    io.Fonts->AddFontFromFileTTF(iconPath.c_str(), scaledSize, &iconCfg, iconRanges);

    io.FontGlobalScale = 1.0f / dpiScale;
}

void Editor::applyStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsDark();

    style.WindowRounding = 4.0f;
    style.FrameRounding = 2.0f;
    style.GrabRounding = 2.0f;
    style.TabRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;
    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.FramePadding = ImVec2(6.0f, 4.0f);
    style.ItemSpacing = ImVec2(8.0f, 4.0f);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]         = ImVec4(0.12f, 0.12f, 0.14f, 0.94f);
    colors[ImGuiCol_TitleBg]          = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_TitleBgActive]    = ImVec4(0.14f, 0.14f, 0.18f, 1.00f);
    colors[ImGuiCol_Tab]              = ImVec4(0.14f, 0.14f, 0.18f, 1.00f);
    colors[ImGuiCol_TabSelected]      = ImVec4(0.22f, 0.22f, 0.28f, 1.00f);
    colors[ImGuiCol_TabHovered]       = ImVec4(0.28f, 0.28f, 0.36f, 1.00f);
    colors[ImGuiCol_MenuBarBg]        = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_Header]           = ImVec4(0.20f, 0.20f, 0.25f, 0.80f);
    colors[ImGuiCol_HeaderHovered]    = ImVec4(0.26f, 0.26f, 0.32f, 0.80f);
    colors[ImGuiCol_HeaderActive]     = ImVec4(0.30f, 0.30f, 0.38f, 0.80f);

    style.ScaleAllSizes(m_currentDpiScale);
}

} // namespace placeholder::editor
