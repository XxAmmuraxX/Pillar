#include "ConsolePanel.h"
#include <imgui.h>

namespace PillarEditor {

    std::vector<LogMessage> ConsolePanel::s_Messages;

    ConsolePanel::ConsolePanel()
        : EditorPanel("Console")
    {
    }

    void ConsolePanel::Log(const std::string& message, LogLevel level)
    {
        s_Messages.push_back({ message, level });
        
        // Limit message count
        if (s_Messages.size() > s_MaxMessages)
        {
            s_Messages.erase(s_Messages.begin());
        }
    }

    void ConsolePanel::Clear()
    {
        s_Messages.clear();
    }

    void ConsolePanel::OnImGuiRender()
    {
        ImGui::Begin("Console");

        // Toolbar
        if (ImGui::Button("Clear"))
        {
            Clear();
        }
        
        ImGui::SameLine();
        ImGui::Checkbox("Auto-scroll", &m_AutoScroll);
        
        ImGui::SameLine();
        ImGui::Separator();
        
        ImGui::SameLine();
        ImGui::Checkbox("Trace", &m_ShowTrace);
        
        ImGui::SameLine();
        ImGui::Checkbox("Info", &m_ShowInfo);
        
        ImGui::SameLine();
        ImGui::Checkbox("Warn", &m_ShowWarn);
        
        ImGui::SameLine();
        ImGui::Checkbox("Error", &m_ShowError);

        ImGui::Separator();

        // Message list
        ImGui::BeginChild("LogMessages", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        for (const auto& msg : s_Messages)
        {
            // Filter by level
            bool shouldShow = false;
            ImVec4 color;

            switch (msg.Level)
            {
                case LogLevel::Trace:
                    shouldShow = m_ShowTrace;
                    color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
                    break;
                case LogLevel::Info:
                    shouldShow = m_ShowInfo;
                    color = ImVec4(0.4f, 0.8f, 0.4f, 1.0f);
                    break;
                case LogLevel::Warn:
                    shouldShow = m_ShowWarn;
                    color = ImVec4(0.8f, 0.8f, 0.2f, 1.0f);
                    break;
                case LogLevel::Error:
                    shouldShow = m_ShowError;
                    color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                    break;
            }

            if (shouldShow)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, color);
                // Use Selectable to allow clicking and copying
                ImGui::Selectable(msg.Message.c_str(), false);
                
                // Add context menu for copying
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("Copy"))
                    {
                        ImGui::SetClipboardText(msg.Message.c_str());
                    }
                    ImGui::EndPopup();
                }
                ImGui::PopStyleColor();
            }
        }

        // Auto-scroll to bottom
        if (m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        {
            ImGui::SetScrollHereY(1.0f);
        }

        ImGui::EndChild();
        ImGui::End();
    }

}
