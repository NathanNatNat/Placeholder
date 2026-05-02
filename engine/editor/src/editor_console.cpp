#include "editor/editor_console.h"
#include "editor/icon_font.h"

#include "core/console.h"

#include <imgui.h>

#include <algorithm>
#include <cstring>

namespace placeholder::editor
{

struct ConsoleCallbackData
{
    EditorConsole* self;
    core::Console* console;
};

void EditorConsole::draw(core::Console& console)
{
    if (!visible)
    {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(600, 300), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(ICON_FA_TERMINAL " Console", &visible))
    {
        ImGui::End();
        return;
    }

    if (ImGui::SmallButton("Clear"))
    {
        console.clearOutput();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("Type 'help' for available commands");

    ImGui::Separator();

    float footerHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    if (ImGui::BeginChild("ScrollRegion", ImVec2(0, -footerHeight), ImGuiChildFlags_None,
                          ImGuiWindowFlags_HorizontalScrollbar))
    {
        const auto& output = console.outputLog();
        for (const auto& line : output)
        {
            bool isCommand = !line.empty() && line[0] == '>';
            if (isCommand)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
            }
            else if (line.find("Error") != std::string::npos
                     || line.find("error") != std::string::npos)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
            }
            ImGui::TextUnformatted(line.c_str());
            ImGui::PopStyleColor();
        }

        if (m_scrollToBottom || ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        {
            ImGui::SetScrollHereY(1.0f);
        }
        m_scrollToBottom = false;
    }
    ImGui::EndChild();

    ImGui::Separator();

    ImGuiInputTextFlags inputFlags = ImGuiInputTextFlags_EnterReturnsTrue
                                   | ImGuiInputTextFlags_CallbackHistory
                                   | ImGuiInputTextFlags_CallbackCompletion;

    ConsoleCallbackData cbData{this, &console};

    bool reclaimFocus = m_reclaimFocus;
    m_reclaimFocus = false;

    if (ImGui::InputText("##ConsoleInput", m_inputBuf, sizeof(m_inputBuf), inputFlags,
                         inputCallback, &cbData))
    {
        std::string input(m_inputBuf);
        if (!input.empty())
        {
            console.execute(input);
            m_inputBuf[0] = '\0';
            m_scrollToBottom = true;
            m_historyPos = -1;
        }
        m_reclaimFocus = true;
    }

    ImGui::SetItemDefaultFocus();
    if (reclaimFocus)
    {
        ImGui::SetKeyboardFocusHere(-1);
    }

    ImGui::End();
}

int EditorConsole::inputCallback(ImGuiInputTextCallbackData* data)
{
    auto* cbData = static_cast<ConsoleCallbackData*>(data->UserData);
    auto* self = cbData->self;
    auto* console = cbData->console;

    if (data->EventFlag == ImGuiInputTextFlags_CallbackCompletion)
    {
        std::string prefix(data->Buf, static_cast<size_t>(data->CursorPos));
        auto candidates = console->autocomplete(prefix);
        if (candidates.size() == 1)
        {
            data->DeleteChars(0, data->BufTextLen);
            data->InsertChars(0, candidates[0].c_str());
            data->InsertChars(data->CursorPos, " ");
        }
    }
    else if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory)
    {
        const auto& history = console->history();
        if (history.empty())
        {
            return 0;
        }

        int prevPos = self->m_historyPos;

        if (data->EventKey == ImGuiKey_UpArrow)
        {
            if (self->m_historyPos == -1)
            {
                self->m_historyPos = static_cast<int>(history.size()) - 1;
            }
            else if (self->m_historyPos > 0)
            {
                self->m_historyPos--;
            }
        }
        else if (data->EventKey == ImGuiKey_DownArrow)
        {
            if (self->m_historyPos != -1)
            {
                self->m_historyPos++;
                if (self->m_historyPos >= static_cast<int>(history.size()))
                {
                    self->m_historyPos = -1;
                }
            }
        }

        if (prevPos != self->m_historyPos)
        {
            data->DeleteChars(0, data->BufTextLen);
            if (self->m_historyPos >= 0)
            {
                data->InsertChars(0, history[static_cast<size_t>(self->m_historyPos)].c_str());
            }
        }
    }

    return 0;
}

} // namespace placeholder::editor
