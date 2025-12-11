#include "InspectorPanel.h"
#include "../SelectionContext.h"
#include "../EditorLayer.h"
#include "../Commands/TransformCommand.h"
#include "ConsolePanel.h"
#include "Pillar/ECS/Components/Core/TagComponent.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Core/UUIDComponent.h"
#include "Pillar/ECS/Components/Core/HierarchyComponent.h"
#include "Pillar/ECS/Components/Rendering/SpriteComponent.h"
#include "Pillar/ECS/Components/Rendering/CameraComponent.h"
#include "Pillar/ECS/Components/Rendering/AnimationComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"
#include "Pillar/ECS/Components/Physics/RigidbodyComponent.h"
#include "Pillar/ECS/Components/Physics/ColliderComponent.h"
#include "Pillar/ECS/Components/Gameplay/BulletComponent.h"
#include "Pillar/ECS/Components/Gameplay/XPGemComponent.h"
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

    InspectorPanel::InspectorPanel(EditorLayer* editorLayer)
        : EditorPanel("Inspector"), m_EditorLayer(editorLayer)
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

        // Sprite Component
        if (entity.HasComponent<Pillar::SpriteComponent>())
        {
            DrawSpriteComponent(entity);
        }

        // Camera Component
        if (entity.HasComponent<Pillar::CameraComponent>())
        {
            DrawCameraComponent(entity);
        }

        // Animation Component
        if (entity.HasComponent<Pillar::AnimationComponent>())
        {
            DrawAnimationComponent(entity);
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

        // Bullet Component
        if (entity.HasComponent<Pillar::BulletComponent>())
        {
            DrawBulletComponent(entity);
        }

        // XP Gem Component
        if (entity.HasComponent<Pillar::XPGemComponent>())
        {
            DrawXPGemComponent(entity);
        }

        // Hierarchy Component
        if (entity.HasComponent<Pillar::HierarchyComponent>())
        {
            DrawHierarchyComponent(entity);
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
            
            // Capture old position before any editing
            if (!m_EditingPosition && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                m_OldPosition = transform.Position;
            }
            
            if (DrawVec2Control("Position", position))
            {
                m_EditingPosition = true;
                transform.Position = position;
                transform.Dirty = true;
            }
            
            // Check if any position widget was just deactivated after edit
            if (m_EditingPosition)
            {
                // Check if we're no longer editing (mouse released)
                if (!ImGui::IsAnyItemActive() && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
                {
                    m_EditingPosition = false;
                    
                    // Only create command if value actually changed
                    if (m_OldPosition != transform.Position && m_EditorLayer)
                    {
                        std::vector<TransformCommand::TransformState> oldStates;
                        std::vector<TransformCommand::TransformState> newStates;
                        
                        oldStates.push_back({
                            static_cast<entt::entity>(entity),
                            m_OldPosition,
                            transform.Rotation,
                            transform.Scale
                        });
                        
                        newStates.push_back({
                            static_cast<entt::entity>(entity),
                            transform.Position,
                            transform.Rotation,
                            transform.Scale
                        });
                        
                        auto command = std::make_unique<TransformCommand>(
                            m_EditorLayer->GetActiveScene().get(),
                            oldStates,
                            newStates,
                            "Change Position"
                        );
                        m_EditorLayer->GetCommandHistory().ExecuteCommand(std::move(command));
                    }
                }
            }

            // Rotation (convert to degrees for display)
            float rotationDegrees = glm::degrees(transform.Rotation);
            
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, 100.0f);
            ImGui::Text("Rotation");
            ImGui::NextColumn();
            
            ImGui::PushItemWidth(-1);
            
            // Capture old rotation before editing
            if (ImGui::IsItemActive() && !m_EditingRotation)
            {
                m_EditingRotation = true;
                m_OldRotation = transform.Rotation;
            }
            
            if (ImGui::DragFloat("##Rotation", &rotationDegrees, 1.0f, -180.0f, 180.0f, "%.1f deg"))
            {
                // Clamp rotation to -360 to 360 range
                if (rotationDegrees > 360.0f)
                    rotationDegrees = std::fmod(rotationDegrees, 360.0f);
                else if (rotationDegrees < -360.0f)
                    rotationDegrees = std::fmod(rotationDegrees, -360.0f);
                    
                transform.Rotation = glm::radians(rotationDegrees);
                transform.Dirty = true;
            }
            
            // Check if rotation editing ended
            if (ImGui::IsItemDeactivatedAfterEdit() && m_EditingRotation)
            {
                m_EditingRotation = false;
                
                // Only create command if value actually changed
                if (m_OldRotation != transform.Rotation && m_EditorLayer)
                {
                    std::vector<TransformCommand::TransformState> oldStates;
                    std::vector<TransformCommand::TransformState> newStates;
                    
                    oldStates.push_back({
                        static_cast<entt::entity>(entity),
                        transform.Position,
                        m_OldRotation,
                        transform.Scale
                    });
                    
                    newStates.push_back({
                        static_cast<entt::entity>(entity),
                        transform.Position,
                        transform.Rotation,
                        transform.Scale
                    });
                    
                    auto command = std::make_unique<TransformCommand>(
                        m_EditorLayer->GetActiveScene().get(),
                        oldStates,
                        newStates,
                        "Change Rotation"
                    );
                    m_EditorLayer->GetCommandHistory().ExecuteCommand(std::move(command));
                }
            }
            
            ImGui::PopItemWidth();
            ImGui::Columns(1);

            // Scale
            glm::vec2 scale = transform.Scale;
            
            // Capture old scale before any editing
            if (!m_EditingScale && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                m_OldScale = transform.Scale;
            }
            
            if (DrawVec2Control("Scale", scale, 1.0f))
            {
                m_EditingScale = true;
                transform.Scale = scale;
                transform.Dirty = true;
            }
            
            // Check if any scale widget was just deactivated after edit
            if (m_EditingScale)
            {
                // Check if we're no longer editing (mouse released)
                if (!ImGui::IsAnyItemActive() && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
                {
                    m_EditingScale = false;
                    
                    // Only create command if value actually changed
                    if (m_OldScale != transform.Scale && m_EditorLayer)
                    {
                        std::vector<TransformCommand::TransformState> oldStates;
                        std::vector<TransformCommand::TransformState> newStates;
                        
                        oldStates.push_back({
                            static_cast<entt::entity>(entity),
                            transform.Position,
                            transform.Rotation,
                            m_OldScale
                        });
                        
                        newStates.push_back({
                            static_cast<entt::entity>(entity),
                            transform.Position,
                            transform.Rotation,
                            transform.Scale
                        });
                        
                        auto command = std::make_unique<TransformCommand>(
                            m_EditorLayer->GetActiveScene().get(),
                            oldStates,
                            newStates,
                            "Change Scale"
                        );
                        m_EditorLayer->GetCommandHistory().ExecuteCommand(std::move(command));
                    }
                }
            }

            ImGui::TreePop();
        }
    }

    void InspectorPanel::DrawSpriteComponent(Pillar::Entity entity)
    {
        if (!entity.HasComponent<Pillar::SpriteComponent>())
            return;

        if (!DrawComponentHeader<Pillar::SpriteComponent>("Sprite", entity))
            return;

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
                                   ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap |
                                   ImGuiTreeNodeFlags_FramePadding;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
        bool open = ImGui::TreeNodeEx("Sprite", flags);
        ImGui::PopStyleVar();

        if (open)
        {
            auto& sprite = entity.GetComponent<Pillar::SpriteComponent>();

            // Texture Path
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, 100.0f);
            ImGui::Text("Texture");
            ImGui::NextColumn();
            
            ImGui::PushItemWidth(-150);
            char buffer[256];
            std::strncpy(buffer, sprite.TexturePath.c_str(), sizeof(buffer));
            buffer[sizeof(buffer) - 1] = '\0';
            
            if (ImGui::InputText("##TexturePath", buffer, sizeof(buffer)))
            {
                sprite.TexturePath = buffer;
            }
            ImGui::PopItemWidth();
            
            ImGui::SameLine();
            if (ImGui::Button("Load##Texture", ImVec2(70, 0)))
            {
                if (!sprite.TexturePath.empty())
                {
                    try
                    {
                        sprite.Texture = Pillar::Texture2D::Create(sprite.TexturePath);
                        ConsolePanel::Log("Loaded texture: " + sprite.TexturePath, LogLevel::Info);
                    }
                    catch (const std::exception& e)
                    {
                        ConsolePanel::Log("Failed to load texture: " + sprite.TexturePath + " - " + e.what(), 
                                        LogLevel::Error);
                        sprite.Texture = nullptr;
                    }
                }
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Clear##Texture", ImVec2(70, 0)))
            {
                sprite.Texture = nullptr;
                sprite.TexturePath.clear();
            }
            
            ImGui::Columns(1);
            
            // Show texture info if loaded
            if (sprite.Texture)
            {
                ImGui::Indent();
                ImGui::TextDisabled("Size: %dx%d", sprite.Texture->GetWidth(), sprite.Texture->GetHeight());
                ImGui::Unindent();
            }

            // Color Tint
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, 100.0f);
            ImGui::Text("Color");
            ImGui::NextColumn();
            
            ImGui::PushItemWidth(-1);
            ImGui::ColorEdit4("##Color", glm::value_ptr(sprite.Color));
            ImGui::PopItemWidth();
            ImGui::Columns(1);

            // Size
            if (DrawVec2Control("Size", sprite.Size, 1.0f))
            {
                // Size changed
            }

            // Z-Index
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, 100.0f);
            ImGui::Text("Z-Index");
            ImGui::NextColumn();
            
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat("##ZIndex", &sprite.ZIndex, 0.1f, -100.0f, 100.0f, "%.1f");
            ImGui::PopItemWidth();
            ImGui::Columns(1);

            // Flip X/Y
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, 100.0f);
            ImGui::Text("Flip");
            ImGui::NextColumn();
            
            ImGui::Checkbox("Flip X", &sprite.FlipX);
            ImGui::SameLine();
            ImGui::Checkbox("Flip Y", &sprite.FlipY);
            ImGui::Columns(1);

            ImGui::TreePop();
        }
    }

    void InspectorPanel::DrawCameraComponent(Pillar::Entity entity)
    {
        if (!entity.HasComponent<Pillar::CameraComponent>())
            return;

        if (!DrawComponentHeader<Pillar::CameraComponent>("Camera", entity))
            return;

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
                                   ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap |
                                   ImGuiTreeNodeFlags_FramePadding;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
        bool open = ImGui::TreeNodeEx("Camera", flags);
        ImGui::PopStyleVar();

        if (open)
        {
            auto& camera = entity.GetComponent<Pillar::CameraComponent>();

            // Primary Camera
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, 100.0f);
            ImGui::Text("Primary");
            ImGui::NextColumn();
            
            ImGui::Checkbox("##Primary", &camera.Primary);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("This camera will be used during play mode");
            ImGui::Columns(1);

            // Orthographic Size
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, 100.0f);
            ImGui::Text("Size");
            ImGui::NextColumn();
            
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat("##OrthoSize", &camera.OrthographicSize, 0.1f, 0.1f, 100.0f, "%.1f");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Height of the camera view in world units");
            ImGui::PopItemWidth();
            ImGui::Columns(1);

            // Near/Far Clip
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, 100.0f);
            ImGui::Text("Near Clip");
            ImGui::NextColumn();
            
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat("##NearClip", &camera.NearClip, 0.1f, -10.0f, camera.FarClip - 0.1f, "%.1f");
            ImGui::PopItemWidth();
            ImGui::Columns(1);

            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, 100.0f);
            ImGui::Text("Far Clip");
            ImGui::NextColumn();
            
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat("##FarClip", &camera.FarClip, 0.1f, camera.NearClip + 0.1f, 10.0f, "%.1f");
            ImGui::PopItemWidth();
            ImGui::Columns(1);

            // Fixed Aspect Ratio
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, 100.0f);
            ImGui::Text("Fixed Aspect");
            ImGui::NextColumn();
            
            ImGui::Checkbox("##FixedAspect", &camera.FixedAspectRatio);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Maintain aspect ratio for pixel-perfect rendering");
            ImGui::Columns(1);

            ImGui::TreePop();
        }
    }

    void InspectorPanel::DrawAnimationComponent(Pillar::Entity entity)
    {
        if (!DrawComponentHeader<Pillar::AnimationComponent>("Animation", entity))
            return;

        auto& anim = entity.GetComponent<Pillar::AnimationComponent>();

        if (ImGui::TreeNode("Animation"))
        {
            // Current Clip Name
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, 120.0f);
            ImGui::Text("Current Clip");
            ImGui::NextColumn();
            
            char clipNameBuffer[128];
            strncpy(clipNameBuffer, anim.CurrentClipName.c_str(), sizeof(clipNameBuffer) - 1);
            clipNameBuffer[sizeof(clipNameBuffer) - 1] = '\0';
            
            ImGui::PushItemWidth(-1);
            if (ImGui::InputText("##ClipName", clipNameBuffer, sizeof(clipNameBuffer)))
            {
                anim.CurrentClipName = clipNameBuffer;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Enter animation clip name (e.g., 'Idle', 'Walk', 'Jump')");
            ImGui::PopItemWidth();
            ImGui::Columns(1);
            
            ImGui::Spacing();
            ImGui::TextDisabled("Available clips are managed via Animation Manager panel");
            ImGui::TextDisabled("Clips must be loaded or created before use");
            ImGui::Spacing();

            // Playing State
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, 120.0f);
            ImGui::Text("Playing");
            ImGui::NextColumn();
            
            ImGui::Checkbox("##Playing", &anim.Playing);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Toggle animation playback");
            ImGui::Columns(1);

            // Frame Index (Read-only)
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, 120.0f);
            ImGui::Text("Frame Index");
            ImGui::NextColumn();
            
            ImGui::PushItemWidth(-1);
            ImGui::InputInt("##FrameIndex", &anim.FrameIndex, 0, 0, ImGuiInputTextFlags_ReadOnly);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Current frame in animation sequence");
            ImGui::PopItemWidth();
            ImGui::Columns(1);

            // Playback Time (Read-only)
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, 120.0f);
            ImGui::Text("Playback Time");
            ImGui::NextColumn();
            
            ImGui::PushItemWidth(-1);
            ImGui::InputFloat("##PlaybackTime", &anim.PlaybackTime, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_ReadOnly);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Time elapsed in current frame (seconds)");
            ImGui::PopItemWidth();
            ImGui::Columns(1);

            // Playback Speed
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, 120.0f);
            ImGui::Text("Playback Speed");
            ImGui::NextColumn();
            
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##PlaybackSpeed", &anim.PlaybackSpeed, 0.1f, 0.0f, 5.0f, "%.2f"))
            {
                if (anim.PlaybackSpeed < 0.0f)
                    anim.PlaybackSpeed = 0.0f;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Animation speed multiplier (1.0 = normal)");
            ImGui::PopItemWidth();
            ImGui::Columns(1);

            // Control Buttons
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("Playback Controls:");
            ImGui::Spacing();
            
            if (ImGui::Button("Play", ImVec2(60, 0)))
            {
                anim.Playing = true;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Start/resume animation playback");
                
            ImGui::SameLine();
            if (ImGui::Button("Pause", ImVec2(60, 0)))
            {
                anim.Pause();
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Pause animation (preserves frame)");
                
            ImGui::SameLine();
            if (ImGui::Button("Stop", ImVec2(60, 0)))
            {
                anim.Stop();
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Stop animation (resets to frame 0)");
                
            ImGui::SameLine();
            if (ImGui::Button("Reset", ImVec2(60, 0)))
            {
                anim.FrameIndex = 0;
                anim.PlaybackTime = 0.0f;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Reset to first frame");

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextDisabled("Tip: Animation clips use sprite sheets");
            ImGui::TextDisabled("Each frame has texture path + UV coords");
            ImGui::TextDisabled("See AnimationDemoLayer for examples");

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

            if (!entity.HasComponent<Pillar::SpriteComponent>())
            {
                if (ImGui::Selectable("Sprite"))
                {
                    entity.AddComponent<Pillar::SpriteComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }

            if (!entity.HasComponent<Pillar::CameraComponent>())
            {
                if (ImGui::Selectable("Camera"))
                {
                    entity.AddComponent<Pillar::CameraComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }

            if (!entity.HasComponent<Pillar::AnimationComponent>())
            {
                if (ImGui::Selectable("Animation"))
                {
                    entity.AddComponent<Pillar::AnimationComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }

            if (!entity.HasComponent<Pillar::VelocityComponent>())
            {
                if (ImGui::Selectable("Velocity"))
                {
                    entity.AddComponent<Pillar::VelocityComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }

            if (!entity.HasComponent<Pillar::RigidbodyComponent>())
            {
                if (ImGui::Selectable("Rigidbody"))
                {
                    entity.AddComponent<Pillar::RigidbodyComponent>();
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

            if (!entity.HasComponent<Pillar::BulletComponent>())
            {
                if (ImGui::Selectable("Bullet"))
                {
                    entity.AddComponent<Pillar::BulletComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }

            if (!entity.HasComponent<Pillar::XPGemComponent>())
            {
                if (ImGui::Selectable("XP Gem"))
                {
                    entity.AddComponent<Pillar::XPGemComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }

            if (!entity.HasComponent<Pillar::HierarchyComponent>())
            {
                if (ImGui::Selectable("Hierarchy"))
                {
                    entity.AddComponent<Pillar::HierarchyComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::EndPopup();
        }
    }

    void InspectorPanel::DrawBulletComponent(Pillar::Entity entity)
    {
        if (!entity.HasComponent<Pillar::BulletComponent>())
            return;

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
                                   ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap |
                                   ImGuiTreeNodeFlags_FramePadding;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
        
        ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();
        float lineHeight = ImGui::GetFrameHeight();
        
        bool open = ImGui::TreeNodeEx("Bullet", flags);
        
        // Remove button
        ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
        if (ImGui::Button("X##RemoveBullet", ImVec2(lineHeight, lineHeight)))
        {
            entity.RemoveComponent<Pillar::BulletComponent>();
            ImGui::PopStyleVar();
            if (open)
                ImGui::TreePop();
            return;
        }
        
        ImGui::PopStyleVar();

        if (open)
        {
            auto& bullet = entity.GetComponent<Pillar::BulletComponent>();

            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, 120.0f);

            ImGui::Text("Damage");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat("##Damage", &bullet.Damage, 0.5f, 0.0f, 1000.0f, "%.1f");
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Text("Lifetime");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat("##Lifetime", &bullet.Lifetime, 0.1f, 0.1f, 60.0f, "%.1f s");
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Text("Time Alive");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::TextDisabled("%.2f s", bullet.TimeAlive);
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Separator();
            ImGui::NextColumn();
            ImGui::NextColumn();

            ImGui::Text("Pierce");
            ImGui::NextColumn();
            ImGui::Checkbox("##Pierce", &bullet.Pierce);
            ImGui::NextColumn();

            ImGui::Text("Max Hits");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            int maxHits = static_cast<int>(bullet.MaxHits);
            if (ImGui::DragInt("##MaxHits", &maxHits, 1.0f, 1, 100))
                bullet.MaxHits = static_cast<uint32_t>(maxHits);
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Text("Hits Remaining");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::TextDisabled("%u", bullet.HitsRemaining);
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Columns(1);

            ImGui::TreePop();
        }
    }

    void InspectorPanel::DrawXPGemComponent(Pillar::Entity entity)
    {
        if (!entity.HasComponent<Pillar::XPGemComponent>())
            return;

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
                                   ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap |
                                   ImGuiTreeNodeFlags_FramePadding;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
        
        ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();
        float lineHeight = ImGui::GetFrameHeight();
        
        bool open = ImGui::TreeNodeEx("XP Gem", flags);
        
        // Remove button
        ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
        if (ImGui::Button("X##RemoveXPGem", ImVec2(lineHeight, lineHeight)))
        {
            entity.RemoveComponent<Pillar::XPGemComponent>();
            ImGui::PopStyleVar();
            if (open)
                ImGui::TreePop();
            return;
        }
        
        ImGui::PopStyleVar();

        if (open)
        {
            auto& xpGem = entity.GetComponent<Pillar::XPGemComponent>();

            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, 140.0f);

            ImGui::Text("XP Value");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::DragInt("##XPValue", &xpGem.XPValue, 1.0f, 1, 10000);
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Text("Attraction Radius");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat("##AttractionRadius", &xpGem.AttractionRadius, 0.1f, 0.1f, 50.0f, "%.1f");
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Text("Move Speed");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat("##MoveSpeed", &xpGem.MoveSpeed, 0.5f, 0.1f, 100.0f, "%.1f");
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Separator();
            ImGui::NextColumn();
            ImGui::NextColumn();

            ImGui::Text("Is Attracted");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::Checkbox("##IsAttracted", &xpGem.IsAttracted);
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Columns(1);

            ImGui::TreePop();
        }
    }

    void InspectorPanel::DrawHierarchyComponent(Pillar::Entity entity)
    {
        if (!entity.HasComponent<Pillar::HierarchyComponent>())
            return;

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
                                   ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap |
                                   ImGuiTreeNodeFlags_FramePadding;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
        
        ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();
        float lineHeight = ImGui::GetFrameHeight();
        
        bool open = ImGui::TreeNodeEx("Hierarchy", flags);
        
        // Remove button
        ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
        if (ImGui::Button("X##RemoveHierarchy", ImVec2(lineHeight, lineHeight)))
        {
            entity.RemoveComponent<Pillar::HierarchyComponent>();
            ImGui::PopStyleVar();
            if (open)
                ImGui::TreePop();
            return;
        }
        
        ImGui::PopStyleVar();

        if (open)
        {
            auto& hierarchy = entity.GetComponent<Pillar::HierarchyComponent>();

            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, 120.0f);

            ImGui::Text("Parent UUID");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::Text("%llu", hierarchy.ParentUUID);
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Columns(1);

            ImGui::Spacing();
            ImGui::TextDisabled("Note: Parent-child relationships are managed by the scene hierarchy.");
            ImGui::TextDisabled("Use the Scene Hierarchy panel to set parent/child relationships.");

            ImGui::TreePop();
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
