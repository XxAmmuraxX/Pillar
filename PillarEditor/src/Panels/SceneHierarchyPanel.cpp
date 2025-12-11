#include "SceneHierarchyPanel.h"
#include "../SelectionContext.h"
#include "Pillar/ECS/Components/Core/TagComponent.h"
#include "Pillar/ECS/Components/Core/UUIDComponent.h"
#include <imgui.h>

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
    }

    void SceneHierarchyPanel::DrawEntityNode(Pillar::Entity entity)
    {
        if (!entity || !entity.HasComponent<Pillar::TagComponent>())
            return;

        auto& tag = entity.GetComponent<Pillar::TagComponent>();

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
