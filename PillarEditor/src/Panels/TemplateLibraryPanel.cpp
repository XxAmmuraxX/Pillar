#include "TemplateLibraryPanel.h"
#include <imgui.h>
#include <algorithm>

namespace PillarEditor
{
    TemplateLibraryPanel::TemplateLibraryPanel()
    {
    }

    void TemplateLibraryPanel::OnImGuiRender()
    {
        ImGui::Begin("Template Library");

        if (!m_TemplateManager)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "No TemplateManager set!");
            ImGui::End();
            return;
        }

        DrawToolbar();
        DrawSearchBar();
        
        ImGui::Separator();
        
        DrawTemplateGrid();

        ImGui::End();
    }

    void TemplateLibraryPanel::DrawToolbar()
    {
        if (ImGui::Button("Refresh"))
        {
            m_TemplateManager->LoadTemplatesFromDirectory();
            m_SelectedTemplateIndex = -1;
        }

        ImGui::SameLine();
        
        if (ImGui::Button("Instantiate") && m_SelectedTemplateIndex >= 0)
        {
            InstantiateSelectedTemplate();
        }

        ImGui::SameLine();
        
        if (ImGui::Button("Delete") && m_SelectedTemplateIndex >= 0)
        {
            const auto& templates = m_TemplateManager->GetTemplates();
            if (m_SelectedTemplateIndex < templates.size())
            {
                const std::string& templateName = templates[m_SelectedTemplateIndex].Name;
                
                // Show confirmation (simple for now)
                if (m_TemplateManager->DeleteTemplate(templateName))
                {
                    m_SelectedTemplateIndex = -1;
                }
            }
        }

        ImGui::SameLine();
        ImGui::Text("| %zu templates", m_TemplateManager->GetTemplates().size());
    }

    void TemplateLibraryPanel::DrawSearchBar()
    {
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputTextWithHint("##search", "Search templates...", m_SearchBuffer, sizeof(m_SearchBuffer));
    }

    void TemplateLibraryPanel::DrawTemplateGrid()
    {
        const auto& templates = m_TemplateManager->GetTemplates();
        
        if (templates.empty())
        {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No templates available");
            ImGui::Text("Save an entity as a template from the Hierarchy panel");
            return;
        }

        // Filter templates based on search
        std::string searchStr = m_SearchBuffer;
        std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);

        // Calculate grid layout
        float windowWidth = ImGui::GetContentRegionAvail().x;
        int columns = std::max(1, (int)(windowWidth / (m_CardWidth + 10.0f)));

        int visibleIndex = 0;
        for (int i = 0; i < templates.size(); ++i)
        {
            const auto& templateData = templates[i];

            // Filter by search
            if (!searchStr.empty())
            {
                std::string nameLower = templateData.Name;
                std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
                
                if (nameLower.find(searchStr) == std::string::npos)
                {
                    continue; // Skip this template
                }
            }

            // Layout in grid
            if (visibleIndex > 0 && visibleIndex % columns != 0)
            {
                ImGui::SameLine();
            }

            DrawTemplateCard(templateData, i);
            visibleIndex++;
        }
    }

    void TemplateLibraryPanel::DrawTemplateCard(const EntityTemplate& templateData, int index)
    {
        ImGui::BeginGroup();

        // Card background
        ImVec2 cardMin = ImGui::GetCursorScreenPos();
        ImVec2 cardMax = ImVec2(cardMin.x + m_CardWidth, cardMin.y + m_CardWidth);

        bool isSelected = (m_SelectedTemplateIndex == index);
        ImU32 borderColor = isSelected ? IM_COL32(100, 150, 255, 255) : IM_COL32(60, 60, 60, 255);
        ImU32 bgColor = isSelected ? IM_COL32(40, 60, 100, 255) : IM_COL32(30, 30, 30, 255);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(cardMin, cardMax, bgColor, 4.0f);
        drawList->AddRect(cardMin, cardMax, borderColor, 4.0f, 0, 2.0f);

        // Make card selectable
        ImGui::PushID(index);
        ImGui::InvisibleButton("##card", ImVec2(m_CardWidth, m_CardWidth));
        
        if (ImGui::IsItemClicked())
        {
            m_SelectedTemplateIndex = index;
        }

        // Double-click to instantiate
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            m_SelectedTemplateIndex = index;
            InstantiateSelectedTemplate();
        }

        ImGui::PopID();

        // Draw icon placeholder (centered square)
        float iconSize = m_CardWidth * 0.5f;
        ImVec2 iconMin = ImVec2(cardMin.x + (m_CardWidth - iconSize) * 0.5f, cardMin.y + 15.0f);
        ImVec2 iconMax = ImVec2(iconMin.x + iconSize, iconMin.y + iconSize);
        drawList->AddRectFilled(iconMin, iconMax, IM_COL32(70, 70, 70, 255), 2.0f);
        
        // Draw placeholder icon (entity symbol)
        ImVec2 iconCenter = ImVec2((iconMin.x + iconMax.x) * 0.5f, (iconMin.y + iconMax.y) * 0.5f);
        drawList->AddCircleFilled(iconCenter, 15.0f, IM_COL32(120, 120, 120, 255));

        // Draw template name (below icon)
        ImVec2 textPos = ImVec2(cardMin.x + 5.0f, iconMax.y + 10.0f);
        ImGui::SetCursorScreenPos(textPos);
        
        ImGui::PushTextWrapPos(cardMax.x - 5.0f);
        ImGui::TextWrapped("%s", templateData.Name.c_str());
        ImGui::PopTextWrapPos();

        ImGui::EndGroup();

        // Tooltip on hover
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Template: %s", templateData.Name.c_str());
            if (!templateData.Description.empty())
            {
                ImGui::Separator();
                ImGui::TextWrapped("%s", templateData.Description.c_str());
            }
            ImGui::Separator();
            ImGui::Text("Double-click to instantiate");
            ImGui::EndTooltip();
        }
    }

    Pillar::Entity TemplateLibraryPanel::InstantiateSelectedTemplate()
    {
        if (!m_Scene || !m_TemplateManager || m_SelectedTemplateIndex < 0)
        {
            return Pillar::Entity();
        }

        const auto& templates = m_TemplateManager->GetTemplates();
        if (m_SelectedTemplateIndex >= templates.size())
        {
            return Pillar::Entity();
        }

        const std::string& templateName = templates[m_SelectedTemplateIndex].Name;
        return m_TemplateManager->InstantiateTemplate(templateName, m_Scene);
    }
}
