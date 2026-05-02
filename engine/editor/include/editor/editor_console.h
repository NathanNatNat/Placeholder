#pragma once

#include <imgui.h>

namespace placeholder::core { class Console; }

namespace placeholder::editor
{

/// ImGui frontend for the developer console.
///
/// Wraps the core::Console backend with text input, scrolling output,
/// autocomplete popup, and command history navigation.
class EditorConsole
{
public:
    /// Draw the console window.
    void draw(core::Console& console);

    bool visible = true;

private:
    static int inputCallback(ImGuiInputTextCallbackData* data);

    char m_inputBuf[512] = {};
    bool m_scrollToBottom = true;
    bool m_reclaimFocus = false;
    int m_historyPos = -1;
};

} // namespace placeholder::editor
