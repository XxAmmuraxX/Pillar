#include "InspectorPanel.h"
#include "../SelectionContext.h"
#include "Pillar/ECS/Components/Core/TagComponent.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Core/UUIDComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"
#include "Pillar/ECS/Components/Physics/RigidbodyComponent.h"
#include "Pillar/ECS/Components/Physics/ColliderComponent.h"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <cstring>

namespace PillarEditor {

    // Helper for drawing vec2 with colored labels
    static bool DrawVec2Control(const char* label, glm::vec2& values, float resetValue = 0.0f, float columnWidth = 100.0f)
    {
        bool modified = false;
        
        ImGui::PushID(label);

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text("%s", label);
        ImGui::NextColumn();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

        float lineHeight = ImGui::GetFrameHeight();
        ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };
        float inputWidth = (ImGui::GetContentRegionAvail().x - buttonSize.x * 2) / 2.0f;

        // X
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("X", buttonSize))
        {
            values.x = resetValue;
            modified = true;
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::SetNextItemWidth(inputWidth);
        if (ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f"))
            modified = true;
        ImGui::SameLine();

        // Y
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        if (ImGui::Button("Y", buttonSize))
        {
            values.y = resetValue;
            modified = true;
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::SetNextItemWidth(inputWidth);
        if (ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f"))
            modified = true;

        ImGui::PopStyleVar();
        ImGui::Columns(1);

        ImGui::PopID();
        
        return modified;
    }

    InspectorPanel::InspectorPanel()
        : EditorPanel("Inspector")
    {
    }

    void InspectorPanel::OnImGuiRender()
    {
        ImGui::Begin("Inspector");

        if (m_SelectionContext && m_SelectionContext->HasSelection())
        {
            Pillar::Entity selectedEntity = m_SelectionContext->GetPrimarySelection();
            if (selectedEntity)
            {
                DrawComponents(selectedEntity);
            }
            else
            {
                ImGui::TextDisabled("Invalid entity selected");
            }
        }
        else
        {
            ImGui::TextDisabled("No entity selected");
            ImGui::Spacing();
            ImGui::TextWrapped("Select an entity from the Scene Hierarchy to view and edit its components.");
        }

        ImGui::End();
    }

    void InspectorPanel::DrawComponents(Pillar::Entity entity)
    {
        if (!entity)
            return;

        // UUID (read-only)
        if (entity.HasComponent<Pillar::UUIDComponent>())
        {
            auto& uuid = entity.GetComponent<Pillar::UUIDComponent>();
            ImGui::TextDisabled("UUID: %llu", uuid.UUID);
            ImGui::Separator();
        }

        // Tag Component (always present, can't remove)
        DrawTagComponent(entity);

        ImGui::Spacing();

        // Transform Component
        if (entity.HasComponent<Pillar::TransformComponent>())
        {
            DrawTransformComponent(entity);
        }

        // Velocity Component
        if (entity.HasComponent<Pillar::VelocityComponent>())
        {
            DrawVelocityComponent(entity);
        }

        // Rigidbody Component
        if (entity.HasComponent<Pillar::RigidbodyComponent>())
        {
            DrawRigidbodyComponent(entity);
        }

        // Collider Component
        if (entity.HasComponent<Pillar::ColliderComponent>())
        {
            DrawColliderComponent(entity);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        DrawAddComponentButton(entity);
    }

    void InspectorPanel::DrawTagComponent(Pillar::Entity entity)
    {
        if (!entity.HasComponent<Pillar::TagComponent>())
            return;

        auto& tag = entity.GetComponent<Pillar::TagComponent>();

        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        strncpy(buffer, tag.Tag.c_str(), sizeof(buffer) - 1);

        ImGui::PushItemWidth(-1);
        if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
        {
            tag.Tag = std::string(buffer);
        }
        ImGui::PopItemWidth();
    }

    void InspectorPanel::DrawTransformComponent(Pillar::Entity entity)
    {
        if (!entity.HasComponent<Pillar::TransformComponent>())
            return;

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
                                   ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap |
                                   ImGuiTreeNodeFlags_FramePadding;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
        bool open = ImGui::TreeNodeEx("Transform", flags);
        ImGui::PopStyleVar();

        if (open)
        {
            auto& transform = entity.GetComponent<Pillar::TransformComponent>();

            // Position
            glm::vec2 position = transform.Position;
            if (DrawVec2Control("Position", position))
            {
                transform.Position = position;
                transform.Dirty = true;
            }

            // Rotation (convert to degrees for display)
            float rotationDegrees = glm::degrees(transform.Rotation);
            
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, 100.0f);
            ImGui::Text("Rotation");
            ImGui::NextColumn();
            
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##Rotation", &rotationDegrees, 1.0f, -180.0f, 180.0f, "%.1f deg"))
            {
                transform.Rotation = glm::radians(rotationDegrees);
                transform.Dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::Columns(1);

            // Scale
            glm::vec2 scale = transform.Scale;
            if (DrawVec2Control("Scale", scale, 1.0f))
            {
                transform.Scale = scale;
                transform.Dirty = true;
            }

            ImGui::TreePop();
        }
    }

    void InspectorPanel::DrawVelocityComponent(Pillar::Entity entity)
    {
        if (!entity.HasComponent<Pillar::VelocityComponent>())
            return;

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
                                   ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap |
                                   ImGuiTreeNodeFlags_FramePadding;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
        
        ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();
        float lineHeight = ImGui::GetFrameHeight();
        
        bool open = ImGui::TreeNodeEx("Velocity", flags);
        
        // Remove button
        ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
        if (ImGui::Button("X##RemoveVelocity", ImVec2(lineHeight, lineHeight)))
        {
            entity.RemoveComponent<Pillar::VelocityComponent>();
            ImGui::PopStyleVar();
            if (open)
                ImGui::TreePop();
            return;
        }
        
        ImGui::PopStyleVar();

        if (open)
        {
            auto& velocity = entity.GetComponent<Pillar::VelocityComponent>();

            glm::vec2 vel = velocity.Velocity;
            if (DrawVec2Control("Velocity", vel))
            {
                velocity.Velocity = vel;
            }

            glm::vec2 accel = velocity.Acceleration;
            if (DrawVec2Control("Acceleration", accel))
            {
                velocity.Acceleration = accel;
            }

            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, 100.0f);
            
            ImGui::Text("Drag");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat("##Drag", &velocity.Drag, 0.01f, 0.0f, 10.0f, "%.2f");
            ImGui::PopItemWidth();
            ImGui::NextColumn();
            
            ImGui::Text("Max Speed");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat("##MaxSpeed", &velocity.MaxSpeed, 1.0f, 0.0f, 10000.0f, "%.0f");
            ImGui::PopItemWidth();
            
            ImGui::Columns(1);

            ImGui::TreePop();
        }
    }

    void InspectorPanel::DrawRigidbodyComponent(Pillar::Entity entity)
    {
        if (!entity.HasComponent<Pillar::RigidbodyComponent>())
            return;

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
                                   ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap |
                                   ImGuiTreeNodeFlags_FramePadding;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
        
        bool open = ImGui::TreeNodeEx("Rigidbody", flags);
        
        // Note: Rigidbody can't be removed easily due to physics system dependencies
        
        ImGui::PopStyleVar();

        if (open)
        {
            auto& rb = entity.GetComponent<Pillar::RigidbodyComponent>();

            // Body Type - use Box2D's b2BodyType enum
            const char* bodyTypes[] = { "Static", "Kinematic", "Dynamic" };
            int currentType = static_cast<int>(rb.BodyType);
            
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, 100.0f);
            
            ImGui::Text("Body Type");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::Combo("##BodyType", &currentType, bodyTypes, 3))
            {
                rb.BodyType = static_cast<b2BodyType>(currentType);
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Text("Gravity Scale");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat("##GravityScale", &rb.GravityScale, 0.1f, 0.0f, 10.0f, "%.1f");
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Text("Fixed Rotation");
            ImGui::NextColumn();
            ImGui::Checkbox("##FixedRotation", &rb.FixedRotation);
            
            ImGui::Columns(1);

            ImGui::TreePop();
        }
    }

    void InspectorPanel::DrawColliderComponent(Pillar::Entity entity)
    {
        if (!entity.HasComponent<Pillar::ColliderComponent>())
            return;

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
                                   ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap |
                                   ImGuiTreeNodeFlags_FramePadding;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
        
        ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();
        float lineHeight = ImGui::GetFrameHeight();
        
        bool open = ImGui::TreeNodeEx("Collider", flags);
        
        // Remove button
        ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
        if (ImGui::Button("X##RemoveCollider", ImVec2(lineHeight, lineHeight)))
        {
            entity.RemoveComponent<Pillar::ColliderComponent>();
            ImGui::PopStyleVar();
            if (open)
                ImGui::TreePop();
            return;
        }
        
        ImGui::PopStyleVar();

        if (open)
        {
            auto& collider = entity.GetComponent<Pillar::ColliderComponent>();

            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, 100.0f);

            // Collider Type
            const char* colliderTypes[] = { "Circle", "Box", "Polygon" };
            int currentType = static_cast<int>(collider.Type);
            
            ImGui::Text("Type");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::Combo("##ColliderType", &currentType, colliderTypes, 3))
            {
                collider.Type = static_cast<Pillar::ColliderType>(currentType);
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            // Shape parameters based on type
            if (collider.Type == Pillar::ColliderType::Circle)
            {
                ImGui::Text("Radius");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                ImGui::DragFloat("##Radius", &collider.Radius, 0.05f, 0.01f, 100.0f, "%.2f");
                ImGui::PopItemWidth();
                ImGui::NextColumn();
            }
            else if (collider.Type == Pillar::ColliderType::Box)
            {
                ImGui::Text("Half Extents");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                ImGui::DragFloat2("##HalfExtents", glm::value_ptr(collider.HalfExtents), 0.05f, 0.01f, 100.0f, "%.2f");
                ImGui::PopItemWidth();
                ImGui::NextColumn();
            }

            ImGui::Text("Offset");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat2("##Offset", glm::value_ptr(collider.Offset), 0.05f, -100.0f, 100.0f, "%.2f");
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Separator();
            ImGui::NextColumn();
            ImGui::NextColumn();

            ImGui::Text("Density");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat("##Density", &collider.Density, 0.1f, 0.0f, 100.0f, "%.1f");
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Text("Friction");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat("##Friction", &collider.Friction, 0.01f, 0.0f, 1.0f, "%.2f");
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Text("Restitution");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat("##Restitution", &collider.Restitution, 0.01f, 0.0f, 1.0f, "%.2f");
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Text("Is Sensor");
            ImGui::NextColumn();
            ImGui::Checkbox("##IsSensor", &collider.IsSensor);
            
            ImGui::Columns(1);

            ImGui::TreePop();
        }
    }

    void InspectorPanel::DrawAddComponentButton(Pillar::Entity entity)
    {
        if (!entity)
            return;

        float buttonWidth = ImGui::GetContentRegionAvail().x;

        if (ImGui::Button("Add Component", ImVec2(buttonWidth, 30)))
        {
            ImGui::OpenPopup("AddComponentPopup");
        }

        if (ImGui::BeginPopup("AddComponentPopup"))
        {
            ImGui::TextDisabled("Available Components:");
            ImGui::Separator();

            if (!entity.HasComponent<Pillar::VelocityComponent>())
            {
                if (ImGui::Selectable("Velocity"))
                {
                    entity.AddComponent<Pillar::VelocityComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }

            if (!entity.HasComponent<Pillar::ColliderComponent>())
            {
                if (ImGui::Selectable("Collider (Box)"))
                {
                    entity.AddComponent<Pillar::ColliderComponent>(Pillar::ColliderComponent::Box({0.5f, 0.5f}));
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::Selectable("Collider (Circle)"))
                {
                    entity.AddComponent<Pillar::ColliderComponent>(Pillar::ColliderComponent::Circle(0.5f));
                    ImGui::CloseCurrentPopup();
                }
            }

            // Note: RigidbodyComponent has special requirements and should be added through physics system

            ImGui::EndPopup();
        }
    }

    template<typename T>
    bool InspectorPanel::DrawComponentHeader(const char* label, Pillar::Entity entity, bool canRemove)
    {
        // This template is now unused as we handle headers inline
        return true;
    }

    // Explicit template instantiations (no longer needed but kept for compatibility)
    template bool InspectorPanel::DrawComponentHeader<Pillar::TransformComponent>(const char*, Pillar::Entity, bool);
    template bool InspectorPanel::DrawComponentHeader<Pillar::VelocityComponent>(const char*, Pillar::Entity, bool);
    template bool InspectorPanel::DrawComponentHeader<Pillar::RigidbodyComponent>(const char*, Pillar::Entity, bool);
    template bool InspectorPanel::DrawComponentHeader<Pillar::ColliderComponent>(const char*, Pillar::Entity, bool);

}
