#include "SceneHierarchyPanel.h"
#include "../SelectionContext.h"
#include "../TemplateManager.h"
#include "Pillar/ECS/Components/Core/TagComponent.h"
#include "Pillar/ECS/Components/Core/UUIDComponent.h"
#include "Pillar/ECS/Components/Rendering/AnimationComponent.h"
#include "Pillar/ECS/Components/Rendering/SpriteComponent.h"
#include "ConsolePanel.h"
#include <imgui.h>
#include <algorithm>
#include <cctype>
#include <filesystem>

namespace PillarEditor {

    SceneHierarchyPanel::SceneHierarchyPanel()
        : EditorPanel("Scene Hierarchy")
    {
    }

    void SceneHierarchyPanel::OnImGuiRender()
    {
        ImGui::Begin("Scene Hierarchy");

        if (m_Scene)
        {
            // Scene name header (editable)
            char sceneNameBuffer[256];
            strncpy(sceneNameBuffer, m_Scene->GetName().c_str(), sizeof(sceneNameBuffer) - 1);
            sceneNameBuffer[sizeof(sceneNameBuffer) - 1] = '\0';
            
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.17f, 1.0f));
            if (ImGui::InputText("##SceneName", sceneNameBuffer, sizeof(sceneNameBuffer)))
            {
                m_Scene->SetName(sceneNameBuffer);
            }
            ImGui::PopStyleColor();
            
            ImGui::Separator();
            ImGui::Spacing();

            // Search box
            ImGui::PushItemWidth(-1);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.12f, 0.14f, 1.0f));
            if (ImGui::InputTextWithHint("##EntitySearch", "🔍 Search entities...", 
                m_SearchBuffer, IM_ARRAYSIZE(m_SearchBuffer)))
            {
                // Update search state
                m_IsSearching = (m_SearchBuffer[0] != '\0');
            }
            ImGui::PopStyleColor();
            ImGui::PopItemWidth();
            
            // Clear button and shortcut hint
            if (m_SearchBuffer[0] != '\0')
            {
                ImGui::SameLine();
                if (ImGui::SmallButton("X"))
                {
                    m_SearchBuffer[0] = '\0';
                    m_IsSearching = false;
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Clear search (ESC)");
            }
            
            // ESC to clear search
            if (m_IsSearching && ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                m_SearchBuffer[0] = '\0';
                m_IsSearching = false;
            }
            
            // Ctrl+F to focus search
            if (ImGui::IsWindowFocused() && !ImGui::GetIO().WantTextInput && 
                ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_F))
            {
                ImGui::SetKeyboardFocusHere(-1);  // Focus previous item (search box)
            }
            
            ImGui::Spacing();

            // Entity count
            size_t entityCount = m_Scene->GetEntityCount();
            ImGui::TextDisabled("%zu entities", entityCount);
            ImGui::Spacing();

            // Entity list
            auto entities = m_Scene->GetAllEntities();
            for (auto& entity : entities)
            {
                if (entity)
                {
                    DrawEntityNode(entity);
                }
            }

            // Deselect when clicking on empty space
            if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
            {
                if (m_SelectionContext)
                    m_SelectionContext->ClearSelection();
            }

            // Right-click context menu for creating entities (on empty space)
            if (ImGui::BeginPopupContextWindow("SceneHierarchyPopup", ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
            {
                DrawCreateEntityMenu();
                ImGui::EndPopup();
            }
        }
        else
        {
            ImGui::TextDisabled("No scene loaded");
        }

        ImGui::End();
        
        // Draw template save dialog (needs to be outside main window)
        DrawSaveTemplateDialog();
    }

    void SceneHierarchyPanel::DrawEntityNode(Pillar::Entity entity)
    {
        if (!entity || !entity.HasComponent<Pillar::TagComponent>())
            return;

        auto& tag = entity.GetComponent<Pillar::TagComponent>();
        
        // Filter entities based on search
        if (m_IsSearching)
        {
            std::string tagLower = tag.Tag;
            std::string searchLower = m_SearchBuffer;
            
            // Convert to lowercase for case-insensitive search
            std::transform(tagLower.begin(), tagLower.end(), tagLower.begin(), 
                [](unsigned char c){ return std::tolower(c); });
            std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), 
                [](unsigned char c){ return std::tolower(c); });
            
            // Skip if not matching
            if (tagLower.find(searchLower) == std::string::npos)
                return;
        }

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | 
                                   ImGuiTreeNodeFlags_SpanAvailWidth |
                                   ImGuiTreeNodeFlags_Leaf;  // No children for now
        
        // Highlight selected entities
        if (m_SelectionContext && m_SelectionContext->IsSelected(entity))
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        // Use entity UUID as unique ID for tree node
        uint64_t uuid = 0;
        if (entity.HasComponent<Pillar::UUIDComponent>())
        {
            uuid = entity.GetUUID();
        }
        else
        {
            uuid = static_cast<uint64_t>(static_cast<uint32_t>(entity));
        }
        
        // Add icon based on entity type
        const char* icon = GetEntityIcon(tag.Tag);
        std::string displayName = std::string(icon) + " " + tag.Tag;
        
        bool opened = ImGui::TreeNodeEx((void*)uuid, flags, "%s", displayName.c_str());

        // Handle selection
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        {
            if (m_SelectionContext)
            {
                if (ImGui::GetIO().KeyCtrl)
                {
                    // Multi-select with Ctrl
                    if (m_SelectionContext->IsSelected(entity))
                        m_SelectionContext->RemoveFromSelection(entity);
                    else
                        m_SelectionContext->AddToSelection(entity);
                }
                else
                {
                    m_SelectionContext->Select(entity);
                }
            }
        }

        // Context menu for individual entity
        DrawEntityContextMenu(entity);

        // Drag source for potential drag-drop reordering (future feature)
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
        {
            ImGui::SetDragDropPayload("ENTITY_PAYLOAD", &uuid, sizeof(uint64_t));
            ImGui::Text("%s", tag.Tag.c_str());
            ImGui::EndDragDropSource();
        }

        // Drag-drop target for animation files
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
            {
                std::string droppedPath = static_cast<const char*>(payload->Data);
                
                // Check if it's an animation file
                if (droppedPath.size() >= 10 && droppedPath.substr(droppedPath.size() - 10) == ".anim.json")
                {
                    // Add SpriteComponent if not already present (required for animations)
                    if (!entity.HasComponent<Pillar::SpriteComponent>())
                    {
                        entity.AddComponent<Pillar::SpriteComponent>();
                        ConsolePanel::Log("Auto-added SpriteComponent to entity '" + tag.Tag + "'", 
                            LogLevel::Info);
                    }
                    
                    // Add AnimationComponent if not already present
                    if (!entity.HasComponent<Pillar::AnimationComponent>())
                    {
                        entity.AddComponent<Pillar::AnimationComponent>();
                        ConsolePanel::Log("Auto-added AnimationComponent to entity '" + tag.Tag + "'", 
                            LogLevel::Info);
                    }
                    
                    // Extract clip name from filename (remove path and extension)
                    std::filesystem::path filepath(droppedPath);
                    std::string clipName = filepath.stem().string(); // Removes .json
                    if (clipName.size() > 5 && clipName.substr(clipName.size() - 5) == ".anim")
                    {
                        clipName = clipName.substr(0, clipName.size() - 5); // Remove .anim
                    }
                    
                    // Assign clip to component
                    auto& anim = entity.GetComponent<Pillar::AnimationComponent>();
                    anim.CurrentClipName = clipName;
                    
                    // Make sure clip is loaded (should already be via AnimationLibraryManager)
                    // If not, we could load it here, but that's handled automatically by the manager
                    
                    ConsolePanel::Log("Assigned animation '" + clipName + "' to entity '" + tag.Tag + "'", 
                        LogLevel::Info);
                }
            }
            ImGui::EndDragDropTarget();
        }

        if (opened)
        {
            // Future: Draw child entities here for hierarchy support
            ImGui::TreePop();
        }
    }

    void SceneHierarchyPanel::DrawEntityContextMenu(Pillar::Entity entity)
    {
        if (ImGui::BeginPopupContextItem())
        {
            // Select this entity if not already selected
            if (m_SelectionContext && !m_SelectionContext->IsSelected(entity))
            {
                m_SelectionContext->Select(entity);
            }

            if (ImGui::MenuItem("Duplicate", "Ctrl+D"))
            {
                auto duplicated = m_Scene->DuplicateEntity(entity);
                if (m_SelectionContext)
                    m_SelectionContext->Select(duplicated);
            }

            if (ImGui::MenuItem("Delete", "Delete"))
            {
                if (m_SelectionContext && m_SelectionContext->IsSelected(entity))
                {
                    m_SelectionContext->RemoveFromSelection(entity);
                }
                m_Scene->DestroyEntity(entity);
                
                // Validate selection to ensure no dead entities remain
                if (m_SelectionContext)
                    m_SelectionContext->ValidateSelection();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Save as Template...") && m_TemplateManager)
            {
                // Store entity and show dialog
                m_EntityToSaveAsTemplate = entity;
                m_ShowSaveTemplateDialog = true;
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Focus", "F"))
            {
                // This would need to communicate with the viewport
                // For now, just select it
                if (m_SelectionContext)
                    m_SelectionContext->Select(entity);
            }

            ImGui::EndPopup();
        }
    }

    void SceneHierarchyPanel::DrawSaveTemplateDialog()
    {
        // Open modal when requested
        if (m_ShowSaveTemplateDialog)
        {
            ImGui::OpenPopup("Save as Template");
            m_ShowSaveTemplateDialog = false;
        }
        
        if (ImGui::BeginPopupModal("Save as Template", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Save entity as template");
            ImGui::Separator();

            ImGui::InputText("Template Name", m_TemplateNameBuffer, sizeof(m_TemplateNameBuffer));
            ImGui::InputTextMultiline("Description", m_TemplateDescBuffer, sizeof(m_TemplateDescBuffer), ImVec2(300, 80));

            ImGui::Separator();

            if (ImGui::Button("Save", ImVec2(120, 0)))
            {
                if (strlen(m_TemplateNameBuffer) > 0 && m_TemplateManager && m_EntityToSaveAsTemplate.IsValid())
                {
                    std::string templateName = m_TemplateNameBuffer;
                    std::string templateDesc = m_TemplateDescBuffer;
                    
                    if (m_TemplateManager->SaveEntityAsTemplate(m_EntityToSaveAsTemplate, templateName, templateDesc))
                    {
                        // Success - clear buffers and close
                        m_TemplateNameBuffer[0] = '\0';
                        m_TemplateDescBuffer[0] = '\0';
                        m_EntityToSaveAsTemplate = Pillar::Entity();
                        ImGui::CloseCurrentPopup();
                    }
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                m_TemplateNameBuffer[0] = '\0';
                m_TemplateDescBuffer[0] = '\0';
                m_EntityToSaveAsTemplate = Pillar::Entity();
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    void SceneHierarchyPanel::DrawCreateEntityMenu()
    {
        if (ImGui::MenuItem("Create Empty Entity"))
        {
            auto newEntity = m_Scene->CreateEntity("New Entity");
            if (m_SelectionContext)
                m_SelectionContext->Select(newEntity);
        }

        ImGui::Separator();

        if (ImGui::BeginMenu("Create..."))
        {
            if (ImGui::MenuItem("Player"))
            {
                auto entity = m_Scene->CreateEntity("Player");
                if (m_SelectionContext)
                    m_SelectionContext->Select(entity);
            }
            
            if (ImGui::MenuItem("Enemy"))
            {
                auto entity = m_Scene->CreateEntity("Enemy");
                auto& transform = entity.GetComponent<Pillar::TransformComponent>();
                transform.Scale = { 0.8f, 0.8f };
                if (m_SelectionContext)
                    m_SelectionContext->Select(entity);
            }
            
            if (ImGui::MenuItem("Ground"))
            {
                auto entity = m_Scene->CreateEntity("Ground");
                auto& transform = entity.GetComponent<Pillar::TransformComponent>();
                transform.Position.y = -3.0f;
                transform.Scale = { 10.0f, 1.0f };
                if (m_SelectionContext)
                    m_SelectionContext->Select(entity);
            }
            
            if (ImGui::MenuItem("Wall"))
            {
                auto entity = m_Scene->CreateEntity("Wall");
                auto& transform = entity.GetComponent<Pillar::TransformComponent>();
                transform.Scale = { 1.0f, 5.0f };
                if (m_SelectionContext)
                    m_SelectionContext->Select(entity);
            }
            
            if (ImGui::MenuItem("Camera"))
            {
                auto entity = m_Scene->CreateEntity("Camera");
                if (m_SelectionContext)
                    m_SelectionContext->Select(entity);
            }

            ImGui::EndMenu();
        }
    }

    const char* SceneHierarchyPanel::GetEntityIcon(const std::string& tag)
    {
        // Return simple text icons based on entity type
        if (tag == "Player" || tag.find("Player") != std::string::npos)
            return "[P]";
        if (tag == "Enemy" || tag.find("Enemy") != std::string::npos)
            return "[E]";
        if (tag == "Camera" || tag.find("Camera") != std::string::npos)
            return "[C]";
        if (tag == "Ground" || tag.find("Ground") != std::string::npos)
            return "[G]";
        if (tag == "Wall" || tag.find("Wall") != std::string::npos)
            return "[W]";
        if (tag.find("XP") != std::string::npos || tag.find("Gem") != std::string::npos)
            return "[*]";
        if (tag.find("Bullet") != std::string::npos)
            return "[>]";

        return "[ ]";
    }

}
