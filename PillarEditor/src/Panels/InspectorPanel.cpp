#include "InspectorPanel.h"
#include "../SelectionContext.h"
#include "../EditorLayer.h"
#include "../Commands/TransformCommand.h"
#include "../EditorConstants.h"
#include "../EditorSettings.h"
#include "ConsolePanel.h"
#include "Pillar/ECS/Components/Core/TagComponent.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Core/UUIDComponent.h"
#include "Pillar/ECS/Components/Core/HierarchyComponent.h"
#include "Pillar/ECS/Components/Rendering/SpriteComponent.h"
#include "Pillar/ECS/Components/Rendering/CameraComponent.h"
#include "Pillar/ECS/Components/Rendering/AnimationComponent.h"
#include "Pillar/ECS/Components/Rendering/Light2DComponent.h"
#include "Pillar/ECS/Components/Rendering/ShadowCaster2DComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"
#include "Pillar/ECS/Components/Physics/RigidbodyComponent.h"
#include "Pillar/ECS/Components/Physics/ColliderComponent.h"
#include "Pillar/ECS/Components/Gameplay/BulletComponent.h"
#include "Pillar/ECS/Components/Gameplay/XPGemComponent.h"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>
#include <cstring>
#include <filesystem>
#include <algorithm>
#include <unordered_map>

namespace PillarEditor {

    using namespace Constants;

    // Helper for drawing vec2 with colored labels
    static bool DrawVec2Control(const char* label, glm::vec2& values, float resetValue = Inspector::RESET_VALUE_ZERO, float columnWidth = Inspector::COLUMN_WIDTH_LABEL)
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
        ImGui::PushStyleColor(ImGuiCol_Button, Inspector::Colors::BUTTON_X_NORMAL);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Inspector::Colors::BUTTON_X_HOVERED);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, Inspector::Colors::BUTTON_X_ACTIVE);
        if (ImGui::Button("X", buttonSize))
        {
            values.x = resetValue;
            modified = true;
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::SetNextItemWidth(inputWidth);
        if (ImGui::DragFloat("##X", &values.x, Inspector::DRAG_SPEED_DEFAULT, 0.0f, 0.0f, "%.2f"))
            modified = true;
        ImGui::SameLine();

        // Y
        ImGui::PushStyleColor(ImGuiCol_Button, Inspector::Colors::BUTTON_Y_NORMAL);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Inspector::Colors::BUTTON_Y_HOVERED);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, Inspector::Colors::BUTTON_Y_ACTIVE);
        if (ImGui::Button("Y", buttonSize))
        {
            values.y = resetValue;
            modified = true;
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::SetNextItemWidth(inputWidth);
        if (ImGui::DragFloat("##Y", &values.y, Inspector::DRAG_SPEED_DEFAULT, 0.0f, 0.0f, "%.2f"))
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

        // Push entity ID to make all widgets unique per entity
        ImGui::PushID(static_cast<int>(static_cast<uint32_t>(entity)));

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

        // Light 2D Component
        if (entity.HasComponent<Pillar::Light2DComponent>())
        {
            DrawLight2DComponent(entity);
        }

        // Shadow Caster 2D Component
        if (entity.HasComponent<Pillar::ShadowCaster2DComponent>())
        {
            DrawShadowCaster2DComponent(entity);
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

        // Pop entity ID
        ImGui::PopID();
    }

    void InspectorPanel::DrawTagComponent(Pillar::Entity entity)
    {
        if (!entity.HasComponent<Pillar::TagComponent>())
            return;

        ImGui::PushID("TagComponent");

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

        ImGui::PopID();
    }

    void InspectorPanel::DrawTransformComponent(Pillar::Entity entity)
    {
        if (!entity.HasComponent<Pillar::TransformComponent>())
            return;

        ImGui::PushID("TransformComponent");

        bool open = DrawComponentHeader<Pillar::TransformComponent>("Transform", entity, false); // Can't remove Transform

        if (open)
        {
            auto& transform = entity.GetComponent<Pillar::TransformComponent>();

            // === POSITION ===
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
            
            // Quick position presets
            ImGui::Indent();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            if (ImGui::SmallButton("Origin"))
            {
                transform.Position = glm::vec2(0.0f);
                transform.Dirty = true;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Move to (0, 0)");
            ImGui::PopStyleVar();
            ImGui::Unindent();
            
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

            ImGui::Spacing();

            // === ROTATION ===
            float rotationDegrees = glm::degrees(transform.Rotation);
            
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
            ImGui::Text("Rotation");
            ImGui::NextColumn();
            
            ImGui::PushItemWidth(-1);
            
            // Capture old rotation before editing
            if (ImGui::IsItemActive() && !m_EditingRotation)
            {
                m_EditingRotation = true;
                m_OldRotation = transform.Rotation;
            }
            
            if (ImGui::DragFloat("##Rotation", &rotationDegrees, Inspector::DRAG_SPEED_FAST, -180.0f, 180.0f, "%.1f°"))
            {
                // Clamp rotation to -360 to 360 range
                if (rotationDegrees > 360.0f)
                    rotationDegrees = std::fmod(rotationDegrees, 360.0f);
                else if (rotationDegrees < -360.0f)
                    rotationDegrees = std::fmod(rotationDegrees, -360.0f);
                    
                transform.Rotation = glm::radians(rotationDegrees);
                transform.Dirty = true;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Rotation in degrees (Z-axis)");
            
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

            // Quick rotation presets
            ImGui::Indent();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            if (ImGui::SmallButton("0°")) { transform.Rotation = 0.0f; transform.Dirty = true; }
            ImGui::SameLine();
            if (ImGui::SmallButton("90°")) { transform.Rotation = glm::radians(90.0f); transform.Dirty = true; }
            ImGui::SameLine();
            if (ImGui::SmallButton("180°")) { transform.Rotation = glm::radians(180.0f); transform.Dirty = true; }
            ImGui::SameLine();
            if (ImGui::SmallButton("270°")) { transform.Rotation = glm::radians(270.0f); transform.Dirty = true; }
            ImGui::PopStyleVar();
            ImGui::Unindent();

            ImGui::Spacing();

            // === SCALE ===
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

            // Uniform scale toggle
            ImGui::Indent();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            if (ImGui::SmallButton("Uniform 1"))
            {
                transform.Scale = glm::vec2(1.0f);
                transform.Dirty = true;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Reset to uniform scale (1, 1)");
            ImGui::SameLine();
            if (ImGui::SmallButton("2x"))
            {
                transform.Scale *= 2.0f;
                transform.Dirty = true;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("0.5x"))
            {
                transform.Scale *= 0.5f;
                transform.Dirty = true;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Flip X"))
            {
                transform.Scale.x *= -1.0f;
                transform.Dirty = true;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Flip Y"))
            {
                transform.Scale.y *= -1.0f;
                transform.Dirty = true;
            }
            ImGui::PopStyleVar();
            ImGui::Unindent();
            
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

        ImGui::PopID();
    }

    void InspectorPanel::DrawSpriteComponent(Pillar::Entity entity)
    {
        if (!entity.HasComponent<Pillar::SpriteComponent>())
            return;

        ImGui::PushID("SpriteComponent");

        bool open = DrawComponentHeader<Pillar::SpriteComponent>("Sprite", entity);
        if (!open && !entity.HasComponent<Pillar::SpriteComponent>())
            return; // Component was removed

        if (open)
        {
            auto& sprite = entity.GetComponent<Pillar::SpriteComponent>();

            // === TEXTURE ===
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
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
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Enter texture filename or drag from Content Browser");
            
            // Drag-and-drop target for textures
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                {
                    const char* droppedPath = (const char*)payload->Data;
                    std::string pathStr(droppedPath);
                    
                    // Check if it's an image file
                    std::filesystem::path path(pathStr);
                    std::string ext = path.extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    
                    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga")
                    {
                        // Use just the filename
                        sprite.TexturePath = path.filename().string();
                        
                        // Auto-load the texture
                        try
                        {
                            sprite.Texture = Pillar::Texture2D::Create(sprite.TexturePath);
                            ConsolePanel::Log("Loaded texture: " + sprite.TexturePath, LogLevel::Info);
                            
                            // Auto-size if enabled
                            if (EditorSettings::Get().AutoSizeSpritesOnLoad && sprite.Texture)
                            {
                                float ppu = EditorSettings::Get().PixelsPerUnit;
                                sprite.MatchTextureSize(ppu);
                                ConsolePanel::Log("Auto-sized sprite to match texture", LogLevel::Info);
                            }
                        }
                        catch (const std::exception& e)
                        {
                            ConsolePanel::Log("Failed to load texture: " + sprite.TexturePath + " - " + e.what(), 
                                            LogLevel::Error);
                            sprite.Texture = nullptr;
                        }
                    }
                    else
                    {
                        ConsolePanel::Log("Not a supported image format: " + ext, LogLevel::Warn);
                    }
                }
                ImGui::EndDragDropTarget();
            }
            
            ImGui::PopItemWidth();
            
            // Button row
            ImGui::NextColumn();
            ImGui::NextColumn();
            
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
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Load texture from file");
            
            ImGui::SameLine();
            if (ImGui::Button("Clear##Texture", ImVec2(70, 0)))
            {
                sprite.Texture = nullptr;
                sprite.TexturePath.clear();
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Clear texture");
            
            // Texture browser button
            ImGui::SameLine();
            if (ImGui::Button("Browse...##Texture", ImVec2(80, 0)))
            {
                ImGui::OpenPopup("Texture Browser");
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Browse textures in assets folder");
            
            ImGui::Columns(1);
            
            // Texture browser popup modal
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_Appearing);
            
            static char textureSearchBuffer[256] = "";
            if (ImGui::BeginPopupModal("Texture Browser", nullptr, ImGuiWindowFlags_NoScrollbar))
            {
                // Search bar
                ImGui::Text("Search:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(-1);
                ImGui::InputText("##TextureSearch", textureSearchBuffer, sizeof(textureSearchBuffer));
                
                ImGui::Separator();
                
                // Scrollable area for texture grid
                ImGui::BeginChild("TextureGrid", ImVec2(0, -30), true);
                
                // Scan textures directory
                std::vector<std::filesystem::path> texturePaths;
                std::vector<std::string> searchLocations = {
                    "Sandbox/assets/textures",
                    "assets/textures",
                    "PillarEditor/assets/textures"
                };
                
                for (const auto& searchPath : searchLocations)
                {
                    if (std::filesystem::exists(searchPath))
                    {
                        for (const auto& entry : std::filesystem::directory_iterator(searchPath))
                        {
                            if (entry.is_regular_file())
                            {
                                std::string ext = entry.path().extension().string();
                                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                                if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga")
                                {
                                    // Filter by search
                                    std::string filename = entry.path().filename().string();
                                    std::string search = textureSearchBuffer;
                                    std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
                                    std::transform(search.begin(), search.end(), search.begin(), ::tolower);
                                    
                                    if (search.empty() || filename.find(search) != std::string::npos)
                                    {
                                        texturePaths.push_back(entry.path());
                                    }
                                }
                            }
                        }
                        break; // Use first valid directory
                    }
                }
                
                // Display textures in grid
                const float thumbnailSize = 80.0f;
                const float padding = 10.0f;
                float windowWidth = ImGui::GetContentRegionAvail().x;
                int columns = std::max(1, (int)((windowWidth + padding) / (thumbnailSize + padding)));
                
                int column = 0;
                for (const auto& texPath : texturePaths)
                {
                    std::string filename = texPath.filename().string();
                    
                    ImGui::BeginGroup();
                    ImGui::PushID(filename.c_str());
                    
                    // Try to load and display thumbnail
                    static std::unordered_map<std::string, std::shared_ptr<Pillar::Texture2D>> thumbnailCache;
                    if (thumbnailCache.find(filename) == thumbnailCache.end())
                    {
                        try
                        {
                            thumbnailCache[filename] = Pillar::Texture2D::Create(filename);
                        }
                        catch (...)
                        {
                            thumbnailCache[filename] = nullptr;
                        }
                    }
                    
                    auto thumbnail = thumbnailCache[filename];
                    if (thumbnail)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.5f, 0.7f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.6f, 0.8f, 1.0f));
                        
                        if (ImGui::ImageButton(("##" + filename).c_str(), 
                                             (void*)(intptr_t)thumbnail->GetRendererID(),
                                             ImVec2(thumbnailSize, thumbnailSize),
                                             ImVec2(0, 1), ImVec2(1, 0)))
                        {
                            // Apply texture
                            sprite.TexturePath = filename;
                            sprite.Texture = thumbnail;
                            ConsolePanel::Log("Selected texture: " + filename, LogLevel::Info);
                            
                            // Auto-size if enabled
                            if (EditorSettings::Get().AutoSizeSpritesOnLoad)
                            {
                                float ppu = EditorSettings::Get().PixelsPerUnit;
                                sprite.MatchTextureSize(ppu);
                            }
                            
                            ImGui::CloseCurrentPopup();
                        }
                        
                        ImGui::PopStyleColor(3);
                        
                        // Show info on hover
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::BeginTooltip();
                            ImGui::Image((void*)(intptr_t)thumbnail->GetRendererID(),
                                       ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
                            ImGui::Text("%s", filename.c_str());
                            ImGui::Text("%dx%d", thumbnail->GetWidth(), thumbnail->GetHeight());
                            ImGui::EndTooltip();
                        }
                    }
                    else
                    {
                        // Placeholder for failed texture
                        ImGui::Button("?", ImVec2(thumbnailSize, thumbnailSize));
                    }
                    
                    // Filename below thumbnail (truncated)
                    std::string displayName = filename;
                    if (displayName.length() > 12)
                    {
                        displayName = displayName.substr(0, 9) + "...";
                    }
                    ImGui::Text("%s", displayName.c_str());
                    
                    ImGui::PopID();
                    ImGui::EndGroup();
                    
                    column++;
                    if (column < columns)
                    {
                        ImGui::SameLine();
                    }
                    else
                    {
                        column = 0;
                    }
                }
                
                if (texturePaths.empty())
                {
                    ImGui::TextDisabled("No textures found in assets/textures/");
                }
                
                ImGui::EndChild();
                
                ImGui::Separator();
                if (ImGui::Button("Close", ImVec2(120, 0)))
                {
                    ImGui::CloseCurrentPopup();
                }
                
                ImGui::EndPopup();
            }
            
            // Texture preview and info
            if (sprite.Texture)
            {
                ImGui::Indent();
                ImGui::TextDisabled("📐 Size: %dx%d", sprite.Texture->GetWidth(), sprite.Texture->GetHeight());
                
                // Show texture preview (small thumbnail)
                ImGui::Text("Preview:");
                ImGui::SameLine();
                ImVec2 thumbnailSize(64, 64);
                ImGui::Image((void*)(intptr_t)sprite.Texture->GetRendererID(), thumbnailSize, 
                           ImVec2(0, 1), ImVec2(1, 0));
                if (ImGui::IsItemHovered())
                {
                    // Show larger preview on hover
                    ImGui::BeginTooltip();
                    ImVec2 largeSize(256, 256);
                    ImGui::Image((void*)(intptr_t)sprite.Texture->GetRendererID(), largeSize,
                               ImVec2(0, 1), ImVec2(1, 0));
                    ImGui::EndTooltip();
                }
                ImGui::Unindent();
            }
            else
            {
                ImGui::Indent();
                ImGui::TextDisabled("⚠ No texture loaded");
                ImGui::Unindent();
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // === COLOR TINT ===
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
            ImGui::Text("Color Tint");
            ImGui::NextColumn();
            
            ImGui::PushItemWidth(-1);
            ImGui::ColorEdit4("##Color", glm::value_ptr(sprite.Color), 
                            ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview);
            ImGui::PopItemWidth();
            ImGui::Columns(1);

            // Color presets
            ImGui::Indent();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            if (ImGui::SmallButton("White")) { sprite.Color = glm::vec4(1.0f); }
            ImGui::SameLine();
            if (ImGui::SmallButton("Red")) { sprite.Color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f); }
            ImGui::SameLine();
            if (ImGui::SmallButton("Green")) { sprite.Color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f); }
            ImGui::SameLine();
            if (ImGui::SmallButton("Blue")) { sprite.Color = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f); }
            ImGui::SameLine();
            if (ImGui::SmallButton("Yellow")) { sprite.Color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f); }
            ImGui::PopStyleVar();
            ImGui::Unindent();

            ImGui::Spacing();

            // === SIZE ===
            // Get PPU from editor settings
            float ppu = EditorSettings::Get().PixelsPerUnit;
            
            // Static aspect ratio lock state (per sprite)
            static std::unordered_map<uint64_t, bool> s_AspectRatioLocked;
            uint64_t spriteID = (uint64_t)entity.GetComponent<Pillar::UUIDComponent>().UUID;
            bool& aspectLocked = s_AspectRatioLocked[spriteID];
            
            // Store original aspect ratio if locked
            static std::unordered_map<uint64_t, float> s_AspectRatios;
            if (aspectLocked && s_AspectRatios.find(spriteID) == s_AspectRatios.end())
            {
                s_AspectRatios[spriteID] = sprite.Size.x / (sprite.Size.y != 0.0f ? sprite.Size.y : 1.0f);
            }
            
            // Aspect ratio lock checkbox
            ImGui::Checkbox("Lock Aspect Ratio", &aspectLocked);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Maintain proportions when resizing");
            
            // Update aspect ratio when locked changes
            if (aspectLocked)
            {
                s_AspectRatios[spriteID] = sprite.Size.x / (sprite.Size.y != 0.0f ? sprite.Size.y : 1.0f);
            }
            
            ImGui::Spacing();
            
            // Draw world units size control with aspect ratio locking
            glm::vec2 originalSize = sprite.Size;
            if (DrawVec2Control("Size (World)", sprite.Size, 1.0f))
            {
                // If aspect locked and size changed, maintain aspect ratio
                if (aspectLocked && s_AspectRatios.find(spriteID) != s_AspectRatios.end())
                {
                    float aspectRatio = s_AspectRatios[spriteID];
                    // Determine which component changed more
                    float xChange = std::abs(sprite.Size.x - originalSize.x);
                    float yChange = std::abs(sprite.Size.y - originalSize.y);
                    
                    if (xChange > yChange)
                    {
                        // X changed, adjust Y
                        sprite.Size.y = sprite.Size.x / aspectRatio;
                    }
                    else
                    {
                        // Y changed, adjust X
                        sprite.Size.x = sprite.Size.y * aspectRatio;
                    }
                }
            }
            
            // Display pixel size as read-only info
            glm::vec2 pixelSize = sprite.GetSizeInPixels(ppu);
            ImGui::Indent();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            ImGui::Text("📐 Pixel Size: %.0f x %.0f px (at %.0f PPU)", 
                       pixelSize.x, pixelSize.y, ppu);
            ImGui::PopStyleColor();
            ImGui::Unindent();

            // Size presets
            ImGui::Indent();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::Text("Quick Sizes:");
            ImGui::SameLine();
            if (ImGui::SmallButton("1x1")) { sprite.Size = glm::vec2(1.0f); }
            ImGui::SameLine();
            if (ImGui::SmallButton("32px"))
            {
                sprite.SetSizeInPixels(32.0f, 32.0f, ppu);
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("32x32 pixels");
            ImGui::SameLine();
            if (ImGui::SmallButton("64px"))
            {
                sprite.SetSizeInPixels(64.0f, 64.0f, ppu);
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("64x64 pixels");
            ImGui::SameLine();
            if (ImGui::SmallButton("Match Texture"))
            {
                if (sprite.Texture)
                {
                    sprite.MatchTextureSize(ppu);
                    if (aspectLocked)
                    {
                        s_AspectRatios[spriteID] = sprite.Size.x / (sprite.Size.y != 0.0f ? sprite.Size.y : 1.0f);
                    }
                }
            }
            if (ImGui::IsItemHovered())
            {
                if (sprite.Texture)
                {
                    ImGui::SetTooltip("Set size to match texture: %dx%d pixels", 
                                    sprite.Texture->GetWidth(), sprite.Texture->GetHeight());
                }
                else
                {
                    ImGui::SetTooltip("No texture loaded");
                }
            }
            
            // New row for scaling buttons
            ImGui::Text("Scale:");
            ImGui::SameLine();
            if (ImGui::SmallButton("Half"))
            {
                sprite.Size *= 0.5f;
                if (aspectLocked)
                {
                    s_AspectRatios[spriteID] = sprite.Size.x / (sprite.Size.y != 0.0f ? sprite.Size.y : 1.0f);
                }
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Reduce size by 50%");
            ImGui::SameLine();
            if (ImGui::SmallButton("Double"))
            {
                sprite.Size *= 2.0f;
                if (aspectLocked)
                {
                    s_AspectRatios[spriteID] = sprite.Size.x / (sprite.Size.y != 0.0f ? sprite.Size.y : 1.0f);
                }
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Increase size by 200%");
            ImGui::SameLine();
            if (ImGui::SmallButton("Reset (1x1)"))
            {
                sprite.Size = glm::vec2(1.0f);
                if (aspectLocked)
                {
                    s_AspectRatios[spriteID] = 1.0f;
                }
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Reset to 1x1 world units");
            
            ImGui::PopStyleVar();
            ImGui::Unindent();
            
            // === SIZE VALIDATION WARNINGS ===
            ImGui::Spacing();
            bool hasWarnings = false;
            
            // Check for zero or near-zero size (invisible sprite)
            if (sprite.Size.x < 0.01f || sprite.Size.y < 0.01f)
            {
                hasWarnings = true;
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f)); // Red
                ImGui::TextWrapped("⚠ Sprite size is too small (%.3f x %.3f) - sprite will be invisible!", 
                                  sprite.Size.x, sprite.Size.y);
                ImGui::PopStyleColor();
                
                ImGui::SameLine();
                if (ImGui::SmallButton("Fix##ZeroSize"))
                {
                    if (sprite.Texture)
                    {
                        sprite.MatchTextureSize(ppu);
                        ConsolePanel::Log("Auto-sized sprite to match texture", LogLevel::Info);
                    }
                    else
                    {
                        sprite.Size = glm::vec2(1.0f);
                        ConsolePanel::Log("Reset sprite size to 1x1", LogLevel::Info);
                    }
                    if (aspectLocked)
                    {
                        s_AspectRatios[spriteID] = sprite.Size.x / (sprite.Size.y != 0.0f ? sprite.Size.y : 1.0f);
                    }
                }
                if (ImGui::IsItemHovered())
                {
                    if (sprite.Texture)
                        ImGui::SetTooltip("Set size to match texture dimensions");
                    else
                        ImGui::SetTooltip("Reset to 1x1 world units");
                }
            }
            
            // Check for very large size (likely a mistake)
            if (sprite.Size.x > 1000.0f || sprite.Size.y > 1000.0f)
            {
                hasWarnings = true;
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.7f, 0.2f, 1.0f)); // Orange
                ImGui::TextWrapped("⚠ Sprite size is very large (%.0f x %.0f) - is this intentional?", 
                                  sprite.Size.x, sprite.Size.y);
                ImGui::PopStyleColor();
                
                ImGui::SameLine();
                if (ImGui::SmallButton("Fix##LargeSize"))
                {
                    if (sprite.Texture)
                    {
                        sprite.MatchTextureSize(ppu);
                        ConsolePanel::Log("Auto-sized sprite to match texture", LogLevel::Info);
                    }
                    else
                    {
                        sprite.Size = glm::vec2(1.0f);
                        ConsolePanel::Log("Reset sprite size to 1x1", LogLevel::Info);
                    }
                    if (aspectLocked)
                    {
                        s_AspectRatios[spriteID] = sprite.Size.x / (sprite.Size.y != 0.0f ? sprite.Size.y : 1.0f);
                    }
                }
                if (ImGui::IsItemHovered())
                {
                    if (sprite.Texture)
                        ImGui::SetTooltip("Set size to match texture dimensions");
                    else
                        ImGui::SetTooltip("Reset to 1x1 world units");
                }
            }
            
            // Check for texture loaded but size is still default 1x1 (forgot to match?)
            if (sprite.Texture && 
                std::abs(sprite.Size.x - 1.0f) < 0.01f && 
                std::abs(sprite.Size.y - 1.0f) < 0.01f &&
                (sprite.Texture->GetWidth() != 100 || sprite.Texture->GetHeight() != 100)) // Not 100x100 at 100 PPU
            {
                hasWarnings = true;
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.7f, 1.0f, 1.0f)); // Blue
                ImGui::TextWrapped("💡 Texture loaded but size is 1x1 - did you forget to match texture size?");
                ImGui::PopStyleColor();
                
                ImGui::SameLine();
                if (ImGui::SmallButton("Match Now##DefaultSize"))
                {
                    sprite.MatchTextureSize(ppu);
                    ConsolePanel::Log("Auto-sized sprite to match texture", LogLevel::Info);
                    if (aspectLocked)
                    {
                        s_AspectRatios[spriteID] = sprite.Size.x / (sprite.Size.y != 0.0f ? sprite.Size.y : 1.0f);
                    }
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Set size to %dx%d pixels", sprite.Texture->GetWidth(), sprite.Texture->GetHeight());
            }
            
            // Check for extreme aspect ratio (very stretched)
            float aspectRatio = sprite.Size.x / (sprite.Size.y != 0.0f ? sprite.Size.y : 1.0f);
            if (aspectRatio > 10.0f || aspectRatio < 0.1f)
            {
                hasWarnings = true;
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.7f, 0.2f, 1.0f)); // Orange
                ImGui::TextWrapped("⚠ Sprite is extremely stretched (aspect ratio: %.2f) - this may look distorted", 
                                  aspectRatio);
                ImGui::PopStyleColor();
                
                ImGui::SameLine();
                if (ImGui::SmallButton("Fix##Stretched"))
                {
                    if (sprite.Texture)
                    {
                        sprite.MatchTextureSize(ppu);
                        ConsolePanel::Log("Auto-sized sprite to match texture aspect ratio", LogLevel::Info);
                    }
                    else
                    {
                        // Make square
                        float avg = (sprite.Size.x + sprite.Size.y) * 0.5f;
                        sprite.Size = glm::vec2(avg);
                        ConsolePanel::Log("Fixed sprite aspect ratio to 1:1", LogLevel::Info);
                    }
                    if (aspectLocked)
                    {
                        s_AspectRatios[spriteID] = sprite.Size.x / (sprite.Size.y != 0.0f ? sprite.Size.y : 1.0f);
                    }
                }
                if (ImGui::IsItemHovered())
                {
                    if (sprite.Texture)
                        ImGui::SetTooltip("Restore texture's original aspect ratio");
                    else
                        ImGui::SetTooltip("Make sprite square (1:1 aspect ratio)");
                }
            }
            
            if (!hasWarnings)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 1.0f, 0.4f, 1.0f)); // Green
                ImGui::Text("✓ Sprite size looks good");
                ImGui::PopStyleColor();
            }

            ImGui::Spacing();

            // === FLIP & LAYER SYSTEM ===
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
            
            ImGui::Text("Flip");
            ImGui::NextColumn();
            ImGui::Checkbox("Flip X", &sprite.FlipX);
            ImGui::SameLine();
            ImGui::Checkbox("Flip Y", &sprite.FlipY);
            ImGui::NextColumn();

            // === LAYER DROPDOWN ===
            ImGui::Text("Layer");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Named layer for organized Z-ordering");
            ImGui::NextColumn();
            
            auto& layerMgr = LayerManager::Get();
            auto& layers = layerMgr.GetAllLayers();
            
            ImGui::PushItemWidth(-1);
            if (ImGui::BeginCombo("##Layer", sprite.Layer.c_str()))
            {
                for (const auto& layer : layers)
                {
                    bool selected = (sprite.Layer == layer.name);
                    if (ImGui::Selectable(layer.name.c_str(), selected))
                    {
                        sprite.Layer = layer.name;
                        
                        // Update ZIndex based on layer's base Z-index
                        sprite.ZIndex = layer.baseZIndex + (sprite.OrderInLayer * 0.01f);
                        
                        // Update visibility to match layer's visibility
                        sprite.Visible = layer.visible;
                    }
                    if (selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            // === ORDER IN LAYER ===
            ImGui::Text("Order in Layer");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Fine control within layer\nHigher = drawn on top");
            ImGui::NextColumn();
            
            ImGui::PushItemWidth(-80);
            if (ImGui::DragInt("##OrderInLayer", &sprite.OrderInLayer, 1.0f, -100, 100))
            {
                // Update ZIndex based on layer's base Z-index
                auto* layer = layerMgr.GetLayer(sprite.Layer);
                if (layer)
                {
                    sprite.ZIndex = layer->baseZIndex + (sprite.OrderInLayer * 0.01f);
                }
            }
            ImGui::PopItemWidth();
            
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Sprites within same layer are sorted by this value");
            
            ImGui::NextColumn();

            // === COMPUTED Z-INDEX (READ-ONLY) ===
            ImGui::Text("Final Z-Index");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Computed from layer base + order\n(This value is used for rendering)");
            ImGui::NextColumn();
            
            float finalZ = sprite.GetFinalZIndex();
            auto* currentLayer = layerMgr.GetLayer(sprite.Layer);
            if (currentLayer)
            {
                finalZ = currentLayer->baseZIndex + (sprite.OrderInLayer * 0.01f);
            }
            
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            ImGui::Text("%.2f", finalZ);
            ImGui::PopStyleColor();
            
            ImGui::Columns(1);

            // Layer quick-select presets (optional, can be removed if layer dropdown is sufficient)
            ImGui::Indent();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::Text("Quick Select:");
            ImGui::SameLine();
            if (ImGui::SmallButton("Background"))
            {
                sprite.Layer = "Background";
                auto* layer = layerMgr.GetLayer("Background");
                if (layer) sprite.ZIndex = layer->baseZIndex + (sprite.OrderInLayer * 0.01f);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Default"))
            {
                sprite.Layer = "Default";
                auto* layer = layerMgr.GetLayer("Default");
                if (layer) sprite.ZIndex = layer->baseZIndex + (sprite.OrderInLayer * 0.01f);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Player"))
            {
                sprite.Layer = "Player";
                auto* layer = layerMgr.GetLayer("Player");
                if (layer) sprite.ZIndex = layer->baseZIndex + (sprite.OrderInLayer * 0.01f);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("UI Foreground"))
            {
                sprite.Layer = "UI Foreground";
                auto* layer = layerMgr.GetLayer("UI Foreground");
                if (layer) sprite.ZIndex = layer->baseZIndex + (sprite.OrderInLayer * 0.01f);
            }
            ImGui::PopStyleVar();
            ImGui::Unindent();

            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    void InspectorPanel::DrawCameraComponent(Pillar::Entity entity)
    {
        if (!entity.HasComponent<Pillar::CameraComponent>())
            return;

        ImGui::PushID("CameraComponent");

        bool open = DrawComponentHeader<Pillar::CameraComponent>("Camera", entity);
        if (!open && !entity.HasComponent<Pillar::CameraComponent>())
            return; // Component was removed

        if (open)
        {
            auto& camera = entity.GetComponent<Pillar::CameraComponent>();

            // === PRIMARY CAMERA ===
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
            ImGui::Text("Primary");
            ImGui::NextColumn();
            
            ImGui::Checkbox("##Primary", &camera.Primary);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("🎥 This camera will be used during play mode");
            ImGui::Columns(1);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("📷 Orthographic Settings");
            ImGui::Separator();
            ImGui::Spacing();

            // === ORTHOGRAPHIC SIZE ===
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
            ImGui::Text("Size");
            ImGui::NextColumn();
            
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat("##OrthoSize", &camera.OrthographicSize, Inspector::DRAG_SPEED_DEFAULT, 0.1f, 100.0f, "%.1f");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Height of the camera view in world units\n(larger = more visible area)");
            ImGui::PopItemWidth();
            ImGui::Columns(1);

            // Size presets
            ImGui::Indent();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::Text("Presets:");
            ImGui::SameLine();
            if (ImGui::SmallButton("5")) { camera.OrthographicSize = 5.0f; }
            ImGui::SameLine();
            if (ImGui::SmallButton("10")) { camera.OrthographicSize = 10.0f; }
            ImGui::SameLine();
            if (ImGui::SmallButton("20")) { camera.OrthographicSize = 20.0f; }
            ImGui::SameLine();
            if (ImGui::SmallButton("50")) { camera.OrthographicSize = 50.0f; }
            ImGui::PopStyleVar();
            ImGui::Unindent();

            ImGui::Spacing();

            // === CLIP PLANES ===
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
            ImGui::Text("Near Clip");
            ImGui::NextColumn();
            
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat("##NearClip", &camera.NearClip, Inspector::DRAG_SPEED_DEFAULT, -10.0f, camera.FarClip - 0.1f, "%.1f");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Objects closer than this won't be rendered");
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Text("Far Clip");
            ImGui::NextColumn();
            
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat("##FarClip", &camera.FarClip, Inspector::DRAG_SPEED_DEFAULT, camera.NearClip + 0.1f, 10.0f, "%.1f");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Objects farther than this won't be rendered");
            ImGui::PopItemWidth();
            ImGui::Columns(1);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // === ASPECT RATIO ===
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
            ImGui::Text("Fixed Aspect");
            ImGui::NextColumn();
            
            ImGui::Checkbox("##FixedAspect", &camera.FixedAspectRatio);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("🔒 Maintain aspect ratio for pixel-perfect rendering\nUseful for retro-style games");
            ImGui::Columns(1);

            // Info box
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.15f, 0.8f));
            if (ImGui::BeginChild("CameraInfo", ImVec2(0, 60), true))
            {
                ImGui::TextDisabled("💡 Camera Tips:");
                ImGui::BulletText("Only one Primary camera should be active");
                ImGui::BulletText("Use arrow keys / WASD to move camera in editor");
                ImGui::BulletText("Mouse wheel to zoom in/out");
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();

            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    void InspectorPanel::DrawAnimationComponent(Pillar::Entity entity)
    {
        ImGui::PushID("AnimationComponent");

        bool open = DrawComponentHeader<Pillar::AnimationComponent>("Animation", entity);
        if (!open && !entity.HasComponent<Pillar::AnimationComponent>())
            return; // Component was removed

        if (!entity.HasComponent<Pillar::AnimationComponent>())
            return;

        auto& anim = entity.GetComponent<Pillar::AnimationComponent>();

        if (open)
        {
            // === ANIMATION CLIP ===
            ImGui::Separator();
            ImGui::Text("🎬 Animation Clip");
            ImGui::Separator();
            ImGui::Spacing();

            // Current Clip Name
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL_WIDE);
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
            ImGui::TextDisabled("💡 Available clips are managed via Animation Manager panel");
            ImGui::Spacing();

            // === PLAYBACK STATUS ===
            ImGui::Separator();
            ImGui::Text("▶ Playback Status");
            ImGui::Separator();
            ImGui::Spacing();

            // Playing State
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL_WIDE);
            ImGui::Text("Playing");
            ImGui::NextColumn();
            
            // Visual status indicator
            if (anim.Playing)
            {
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.7f, 0.2f, 0.5f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 1.0f, 0.2f, 1.0f));
                ImGui::Checkbox("##Playing", &anim.Playing);
                ImGui::SameLine();
                ImGui::TextDisabled("▶ Playing");
                ImGui::PopStyleColor(2);
            }
            else
            {
                ImGui::Checkbox("##Playing", &anim.Playing);
                ImGui::SameLine();
                ImGui::TextDisabled("⏸ Paused");
            }
            
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Toggle animation playback");
            ImGui::Columns(1);

            ImGui::Spacing();

            // Frame Index (Read-only)
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL_WIDE);
            ImGui::Text("Frame Index");
            ImGui::NextColumn();
            
            ImGui::PushItemWidth(-1);
            ImGui::InputInt("##FrameIndex", &anim.FrameIndex, 0, 0, ImGuiInputTextFlags_ReadOnly);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Current frame in animation sequence");
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            // Playback Time (Read-only)
            ImGui::Text("Playback Time");
            ImGui::NextColumn();
            
            ImGui::PushItemWidth(-1);
            ImGui::InputFloat("##PlaybackTime", &anim.PlaybackTime, 0.0f, 0.0f, "%.3f s", ImGuiInputTextFlags_ReadOnly);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Time elapsed in current frame (seconds)");
            ImGui::PopItemWidth();
            ImGui::Columns(1);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("⚙ Playback Settings");
            ImGui::Separator();
            ImGui::Spacing();

            // Playback Speed
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL_WIDE);
            ImGui::Text("Playback Speed");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Animation speed multiplier\n1.0 = normal speed");
            ImGui::NextColumn();
            
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##PlaybackSpeed", &anim.PlaybackSpeed, Inspector::DRAG_SPEED_DEFAULT, 0.0f, 5.0f, "%.2f"))
            {
                if (anim.PlaybackSpeed < 0.0f)
                    anim.PlaybackSpeed = 0.0f;
            }
            ImGui::PopItemWidth();
            ImGui::Columns(1);

            // Playback speed presets
            ImGui::Indent();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            if (ImGui::SmallButton("Slow (0.5x)")) { anim.PlaybackSpeed = 0.5f; }
            ImGui::SameLine();
            if (ImGui::SmallButton("Normal (1.0x)")) { anim.PlaybackSpeed = 1.0f; }
            ImGui::SameLine();
            if (ImGui::SmallButton("Fast (1.5x)")) { anim.PlaybackSpeed = 1.5f; }
            ImGui::SameLine();
            if (ImGui::SmallButton("Very Fast (2.0x)")) { anim.PlaybackSpeed = 2.0f; }
            ImGui::PopStyleVar();
            ImGui::Unindent();

            // Control Buttons
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("🎮 Playback Controls:");
            ImGui::Separator();
            ImGui::Spacing();
            
            if (ImGui::Button("▶ Play", ImVec2(80, 25)))
            {
                anim.Playing = true;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Start/resume animation playback");
                
            ImGui::SameLine();
            if (ImGui::Button("⏸ Pause", ImVec2(80, 25)))
            {
                anim.Pause();
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Pause animation (preserves frame)");
                
            ImGui::SameLine();
            if (ImGui::Button("⏹ Stop", ImVec2(80, 25)))
            {
                anim.Stop();
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Stop animation (resets to frame 0)");
                
            ImGui::SameLine();
            if (ImGui::Button("🔄 Reset", ImVec2(80, 25)))
            {
                anim.FrameIndex = 0;
                anim.PlaybackTime = 0.0f;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Reset to first frame");

            ImGui::Spacing();
            
            // Info box
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.2f, 0.3f, 0.7f, 0.2f));
            ImGui::BeginChild("AnimationInfo", ImVec2(0, 80), true);
            ImGui::TextWrapped("💡 Animation Tips:");
            ImGui::BulletText("Animation clips use sprite sheets with UV coordinates");
            ImGui::BulletText("Create clips in the Animation Manager panel");
            ImGui::BulletText("Each frame has texture path + UV coords");
            ImGui::EndChild();
            ImGui::PopStyleColor();

            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    void InspectorPanel::DrawVelocityComponent(Pillar::Entity entity)
    {
        if (!entity.HasComponent<Pillar::VelocityComponent>())
            return;

        ImGui::PushID("VelocityComponent");

        bool open = DrawComponentHeader<Pillar::VelocityComponent>("Velocity", entity);
        if (!open && !entity.HasComponent<Pillar::VelocityComponent>())
            return; // Component was removed

        if (open)
        {
            auto& velocity = entity.GetComponent<Pillar::VelocityComponent>();

            // === VELOCITY ===
            glm::vec2 vel = velocity.Velocity;
            if (DrawVec2Control("Velocity", vel))
            {
                velocity.Velocity = vel;
            }

            // Show magnitude
            float magnitude = glm::length(velocity.Velocity);
            ImGui::Indent();
            ImGui::TextDisabled("📊 Magnitude: %.2f units/sec", magnitude);
            ImGui::Unindent();

            // Quick velocity presets
            ImGui::Indent();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            if (ImGui::SmallButton("Stop"))
            {
                velocity.Velocity = glm::vec2(0.0f);
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Set velocity to zero");
            ImGui::SameLine();
            if (ImGui::SmallButton("→ Right"))
            {
                velocity.Velocity = glm::vec2(5.0f, 0.0f);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("← Left"))
            {
                velocity.Velocity = glm::vec2(-5.0f, 0.0f);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("↑ Up"))
            {
                velocity.Velocity = glm::vec2(0.0f, 5.0f);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("↓ Down"))
            {
                velocity.Velocity = glm::vec2(0.0f, -5.0f);
            }
            ImGui::PopStyleVar();
            ImGui::Unindent();

            ImGui::Spacing();

            // === ACCELERATION ===
            glm::vec2 accel = velocity.Acceleration;
            if (DrawVec2Control("Acceleration", accel))
            {
                velocity.Acceleration = accel;
            }

            ImGui::Spacing();

            // === PHYSICS PROPERTIES ===
            ImGui::Separator();
            ImGui::Text("⚙ Physics Properties");
            ImGui::Separator();

            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
            
            ImGui::Text("Drag");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Air resistance / friction (0 = no drag, higher = more resistance)");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat("##Drag", &velocity.Drag, Inspector::DRAG_SPEED_PRECISE, 0.0f, 10.0f, "%.2f");
            ImGui::PopItemWidth();
            ImGui::NextColumn();
            
            ImGui::Text("Max Speed");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Maximum velocity magnitude (0 = unlimited)");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat("##MaxSpeed", &velocity.MaxSpeed, Inspector::DRAG_SPEED_FAST, 0.0f, 10000.0f, "%.0f");
            ImGui::PopItemWidth();
            
            ImGui::Columns(1);

            // Max speed presets
            ImGui::Indent();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::Text("Presets:");
            ImGui::SameLine();
            if (ImGui::SmallButton("Unlimited")) { velocity.MaxSpeed = 0.0f; }
            ImGui::SameLine();
            if (ImGui::SmallButton("Slow (50)")) { velocity.MaxSpeed = 50.0f; }
            ImGui::SameLine();
            if (ImGui::SmallButton("Normal (100)")) { velocity.MaxSpeed = 100.0f; }
            ImGui::SameLine();
            if (ImGui::SmallButton("Fast (200)")) { velocity.MaxSpeed = 200.0f; }
            ImGui::PopStyleVar();
            ImGui::Unindent();

            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    void InspectorPanel::DrawRigidbodyComponent(Pillar::Entity entity)
    {
        if (!entity.HasComponent<Pillar::RigidbodyComponent>())
            return;

        ImGui::PushID("RigidbodyComponent");

        bool open = DrawComponentHeader<Pillar::RigidbodyComponent>("Rigidbody", entity);
        if (!open && !entity.HasComponent<Pillar::RigidbodyComponent>())
            return; // Component was removed

        if (open)
        {
            auto& rb = entity.GetComponent<Pillar::RigidbodyComponent>();
            bool isPlaying = m_EditorLayer && m_EditorLayer->GetEditorState() == EditorState::Play;

            // === HELP BUTTON ===
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - 30);
            if (ImGui::SmallButton("?"))
                ImGui::OpenPopup("RigidbodyHelp");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Rigidbody Component Help");
            
            // Help popup
            if (ImGui::BeginPopup("RigidbodyHelp"))
            {
                ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "Rigidbody Component Guide");
                ImGui::Separator();
                ImGui::Spacing();
                
                ImGui::TextWrapped("The Rigidbody component makes an entity respond to physics.");
                ImGui::Spacing();
                
                ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.3f, 1.0f), "Body Types:");
                ImGui::BulletText("Static: Immovable objects (walls, floors)");
                ImGui::BulletText("Kinematic: Manually controlled (moving platforms)");
                ImGui::BulletText("Dynamic: Fully simulated physics (players, enemies)");
                ImGui::Spacing();
                
                ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.3f, 1.0f), "Common Issues:");
                ImGui::BulletText("No collision? Add a Collider component");
                ImGui::BulletText("Objects fall through? Enable Bullet Mode");
                ImGui::BulletText("Too bouncy? Increase damping values");
                ImGui::BulletText("Won't rotate? Disable Fixed Rotation");
                ImGui::Spacing();
                
                ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.3f, 1.0f), "Tips:");
                ImGui::BulletText("Use presets for quick setup");
                ImGui::BulletText("Apply impulses for instant velocity changes");
                ImGui::BulletText("Sleeping bodies save performance");
                ImGui::BulletText("Press X in viewport to see physics gizmos");
                
                ImGui::EndPopup();
            }

            // === PERFORMANCE INDICATORS (Play Mode Only) ===
            if (isPlaying)
            {
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.1f, 0.25f, 0.7f));
                if (ImGui::BeginChild("PerfIndicators", ImVec2(0, 60), true))
                {
                    ImGui::TextDisabled("Performance Metrics");
                    ImGui::Spacing();
                    
                    // Count bodies in scene
                    auto& scene = m_EditorLayer->GetActiveScene();
                    auto view = scene->GetRegistry().view<Pillar::RigidbodyComponent>();
                    int totalBodies = 0;
                    int awakeBodies = 0;
                    int sleepingBodies = 0;
                    int inactiveBodies = 0;
                    
                    for (auto entityID : view)
                    {
                        auto& component = view.get<Pillar::RigidbodyComponent>(entityID);
                        if (component.Body)
                        {
                            totalBodies++;
                            if (!component.Body->IsEnabled())
                                inactiveBodies++;
                            else if (component.Body->IsAwake())
                                awakeBodies++;
                            else
                                sleepingBodies++;
                        }
                    }
                    
                    ImGui::Columns(3);
                    ImGui::Text("Total Bodies: %d", totalBodies);
                    ImGui::NextColumn();
                    ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.3f, 1.0f), "Awake: %d", awakeBodies);
                    ImGui::NextColumn();
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Sleeping: %d", sleepingBodies);
                    ImGui::Columns(1);
                    
                    if (inactiveBodies > 0)
                    {
                        ImGui::TextColored(ImVec4(0.9f, 0.4f, 0.4f, 1.0f), "Inactive: %d", inactiveBodies);
                    }
                    
                    // Performance tip
                    if (sleepingBodies > 0)
                    {
                        ImGui::SameLine();
                        ImGui::TextDisabled("(✓ Sleeping bodies save CPU)");
                    }
                }
                ImGui::EndChild();
                ImGui::PopStyleColor();
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
            }

            // === HELP BUTTON ===
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - 30);
            if (ImGui::SmallButton("?"))
                ImGui::OpenPopup("RigidbodyHelp");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Rigidbody Component Help");
            
            // Help popup
            if (ImGui::BeginPopup("RigidbodyHelp"))
            {
                ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "Rigidbody Component Guide");
                ImGui::Separator();
                ImGui::Spacing();
                
                ImGui::TextWrapped("The Rigidbody component makes an entity respond to physics.");
                ImGui::Spacing();
                
                ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.3f, 1.0f), "Body Types:");
                ImGui::BulletText("Static: Immovable objects (walls, floors)");
                ImGui::BulletText("Kinematic: Manually controlled (moving platforms)");
                ImGui::BulletText("Dynamic: Fully simulated physics (players, enemies)");
                ImGui::Spacing();
                
                ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.3f, 1.0f), "Common Issues:");
                ImGui::BulletText("No collision? Add a Collider component");
                ImGui::BulletText("Objects fall through? Enable Bullet Mode");
                ImGui::BulletText("Too bouncy? Increase damping values");
                ImGui::BulletText("Won't rotate? Disable Fixed Rotation");
                ImGui::Spacing();
                
                ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.3f, 1.0f), "Tips:");
                ImGui::BulletText("Use presets for quick setup");
                ImGui::BulletText("Apply impulses for instant velocity changes");
                ImGui::BulletText("Sleeping bodies save performance");
                ImGui::BulletText("Press X in viewport to see physics gizmos");
                
                ImGui::EndPopup();
            }

            // === PERFORMANCE INDICATORS (Play Mode Only) ===
            if (isPlaying)
            {
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.1f, 0.25f, 0.7f));
                if (ImGui::BeginChild("PerfIndicators", ImVec2(0, 60), true))
                {
                    ImGui::TextDisabled("Performance Metrics");
                    ImGui::Spacing();
                    
                    // Count bodies in scene
                    auto& scene = m_EditorLayer->GetActiveScene();
                    auto view = scene->GetRegistry().view<Pillar::RigidbodyComponent>();
                    int totalBodies = 0;
                    int awakeBodies = 0;
                    int sleepingBodies = 0;
                    int inactiveBodies = 0;
                    
                    for (auto entityID : view)
                    {
                        auto& component = view.get<Pillar::RigidbodyComponent>(entityID);
                        if (component.Body)
                        {
                            totalBodies++;
                            if (!component.Body->IsEnabled())
                                inactiveBodies++;
                            else if (component.Body->IsAwake())
                                awakeBodies++;
                            else
                                sleepingBodies++;
                        }
                    }
                    
                    ImGui::Columns(3);
                    ImGui::Text("Total Bodies: %d", totalBodies);
                    ImGui::NextColumn();
                    ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.3f, 1.0f), "Awake: %d", awakeBodies);
                    ImGui::NextColumn();
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Sleeping: %d", sleepingBodies);
                    ImGui::Columns(1);
                    
                    if (inactiveBodies > 0)
                    {
                        ImGui::TextColored(ImVec4(0.9f, 0.4f, 0.4f, 1.0f), "Inactive: %d", inactiveBodies);
                    }
                    
                    // Performance tip
                    if (sleepingBodies > 0)
                    {
                        ImGui::SameLine();
                        ImGui::TextDisabled("(✓ Sleeping bodies save CPU)");
                    }
                }
                ImGui::EndChild();
                ImGui::PopStyleColor();
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
            }

            // === BODY STATE INDICATOR (Play Mode Only) ===
            if (isPlaying && rb.Body)
            {
                bool isAwake = rb.Body->IsAwake();
                bool isEnabled = rb.Body->IsEnabled();
                
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.12f, 0.15f, 0.8f));
                if (ImGui::BeginChild("BodyState", ImVec2(0, 50), true))
                {
                    // State badge
                    if (!isEnabled)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
                        ImGui::TextUnformatted("● INACTIVE");
                        ImGui::PopStyleColor();
                    }
                    else if (isAwake)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.9f, 0.3f, 1.0f));
                        ImGui::TextUnformatted("● AWAKE");
                        ImGui::PopStyleColor();
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                        ImGui::TextUnformatted("● SLEEPING");
                        ImGui::PopStyleColor();
                    }

                    ImGui::SameLine();
                    ImGui::TextDisabled("(Physics State)");

                    // Control buttons
                    ImGui::Spacing();
                    if (isEnabled)
                    {
                        if (isAwake)
                        {
                            if (ImGui::SmallButton("Allow Sleep"))
                                rb.Body->SetSleepingAllowed(true);
                        }
                        else
                        {
                            if (ImGui::SmallButton("Wake Up"))
                                rb.Body->SetAwake(true);
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Disable"))
                            rb.Body->SetEnabled(false);
                    }
                    else
                    {
                        if (ImGui::SmallButton("Enable"))
                            rb.Body->SetEnabled(true);
                    }
                }
                ImGui::EndChild();
                ImGui::PopStyleColor();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
            }

            // === PHYSICS PRESETS ===
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.12f, 0.16f, 0.9f));
            if (ImGui::BeginChild("PhysicsPresets", ImVec2(0, 100), true))
            {
                ImGui::TextDisabled("Quick Presets");
                ImGui::Spacing();

                // Row 1: Player, Enemy, Projectile
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 4));
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.35f, 0.5f, 0.8f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.45f, 0.6f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.3f, 0.45f, 1.0f));

                float buttonWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 2) / 3.0f;

                if (ImGui::Button("Player", ImVec2(buttonWidth, 0)))
                {
                    rb.BodyType = b2_dynamicBody;
                    rb.FixedRotation = true;
                    rb.GravityScale = 1.0f;
                    rb.LinearDamping = 0.5f;
                    rb.AngularDamping = 0.5f;
                    rb.IsBullet = false;
                    rb.IsEnabled = true;
                    if (rb.Body) {
                        rb.Body->SetLinearDamping(0.5f);
                        rb.Body->SetAngularDamping(0.5f);
                        rb.Body->SetBullet(false);
                    }
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Dynamic body for player characters\nFixed rotation, moderate damping");

                ImGui::SameLine();
                if (ImGui::Button("Enemy", ImVec2(buttonWidth, 0)))
                {
                    rb.BodyType = b2_dynamicBody;
                    rb.FixedRotation = false;
                    rb.GravityScale = 1.0f;
                    rb.LinearDamping = 0.3f;
                    rb.AngularDamping = 0.3f;
                    rb.IsBullet = false;
                    rb.IsEnabled = true;
                    if (rb.Body) {
                        rb.Body->SetLinearDamping(0.3f);
                        rb.Body->SetAngularDamping(0.3f);
                        rb.Body->SetBullet(false);
                    }
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Dynamic body for enemies\nCan rotate, light damping");

                ImGui::SameLine();
                if (ImGui::Button("Projectile", ImVec2(buttonWidth, 0)))
                {
                    rb.BodyType = b2_dynamicBody;
                    rb.FixedRotation = false;
                    rb.GravityScale = 0.0f;
                    rb.LinearDamping = 0.0f;
                    rb.AngularDamping = 0.0f;
                    rb.IsBullet = true;
                    rb.IsEnabled = true;
                    if (rb.Body) {
                        rb.Body->SetLinearDamping(0.0f);
                        rb.Body->SetAngularDamping(0.0f);
                        rb.Body->SetBullet(true);
                    }
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Fast-moving projectile\nNo gravity, bullet mode enabled");

                // Row 2: Crate, Platform, Wall
                if (ImGui::Button("Crate", ImVec2(buttonWidth, 0)))
                {
                    rb.BodyType = b2_dynamicBody;
                    rb.FixedRotation = false;
                    rb.GravityScale = 1.0f;
                    rb.LinearDamping = 0.1f;
                    rb.AngularDamping = 0.1f;
                    rb.IsBullet = false;
                    rb.IsEnabled = true;
                    if (rb.Body) {
                        rb.Body->SetLinearDamping(0.1f);
                        rb.Body->SetAngularDamping(0.1f);
                        rb.Body->SetBullet(false);
                    }
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Physics object like boxes/barrels\nFully simulated with slight damping");

                ImGui::SameLine();
                if (ImGui::Button("Platform", ImVec2(buttonWidth, 0)))
                {
                    rb.BodyType = b2_kinematicBody;
                    rb.FixedRotation = true;
                    rb.GravityScale = 0.0f;
                    rb.LinearDamping = 0.0f;
                    rb.AngularDamping = 0.0f;
                    rb.IsBullet = false;
                    rb.IsEnabled = true;
                    if (rb.Body) {
                        rb.Body->SetLinearDamping(0.0f);
                        rb.Body->SetAngularDamping(0.0f);
                        rb.Body->SetBullet(false);
                    }
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Moving platform\nKinematic body, script-controlled");

                ImGui::SameLine();
                if (ImGui::Button("Wall", ImVec2(buttonWidth, 0)))
                {
                    rb.BodyType = b2_staticBody;
                    rb.FixedRotation = true;
                    rb.GravityScale = 1.0f;
                    rb.LinearDamping = 0.0f;
                    rb.AngularDamping = 0.0f;
                    rb.IsBullet = false;
                    rb.IsEnabled = true;
                    if (rb.Body) {
                        rb.Body->SetLinearDamping(0.0f);
                        rb.Body->SetAngularDamping(0.0f);
                        rb.Body->SetBullet(false);
                    }
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Static immovable object\nWalls, floors, terrain");

                ImGui::PopStyleColor(3);
                ImGui::PopStyleVar();
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // === BODY TYPE ===
            const char* bodyTypes[] = { "Static", "Kinematic", "Dynamic" };
            int currentType = static_cast<int>(rb.BodyType);
            
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
            
            ImGui::Text("Body Type");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Static: Immovable (walls, ground)\nKinematic: Moves via script (platforms)\nDynamic: Fully simulated (player, enemies)");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::Combo("##BodyType", &currentType, bodyTypes, 3))
            {
                rb.BodyType = static_cast<b2BodyType>(currentType);
                // Body recreation needed - will happen automatically in physics system
            }
            ImGui::PopItemWidth();
            ImGui::Columns(1);

            // Body type description
            ImGui::Indent();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            if (rb.BodyType == b2_staticBody)
                ImGui::TextWrapped("🧱 Static: Zero velocity, infinite mass, doesn't move");
            else if (rb.BodyType == b2_kinematicBody)
                ImGui::TextWrapped("🎮 Kinematic: Can be moved via velocity, not affected by forces");
            else
                ImGui::TextWrapped("⚙ Dynamic: Fully physics-simulated, affected by forces and gravity");
            ImGui::PopStyleColor();
            ImGui::Unindent();

            ImGui::Spacing();

            // === VALIDATION WARNINGS ===
            bool hasWarnings = false;
            
            // Check if dynamic body has no collider
            if (rb.BodyType == b2_dynamicBody && !entity.HasComponent<Pillar::ColliderComponent>())
            {
                hasWarnings = true;
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.3f, 0.2f, 0.0f, 0.6f));
                if (ImGui::BeginChild("Warning_NoCollider", ImVec2(0, 50), true))
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.0f, 1.0f));
                    ImGui::TextWrapped("⚠ Dynamic body has no Collider");
                    ImGui::PopStyleColor();
                    ImGui::TextWrapped("Add a Collider component for collision detection");
                }
                ImGui::EndChild();
                ImGui::PopStyleColor();
                ImGui::Spacing();
            }
            
            // Check if kinematic body has collider with density (unnecessary)
            if (isPlaying && rb.BodyType == b2_kinematicBody && entity.HasComponent<Pillar::ColliderComponent>())
            {
                auto& collider = entity.GetComponent<Pillar::ColliderComponent>();
                if (collider.Density > 0.0f)
                {
                    hasWarnings = true;
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.2f, 0.25f, 0.3f, 0.6f));
                    if (ImGui::BeginChild("Info_KinematicDensity", ImVec2(0, 50), true))
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.8f, 1.0f, 1.0f));
                        ImGui::TextWrapped("ℹ Kinematic bodies ignore density");
                        ImGui::PopStyleColor();
                        ImGui::TextWrapped("Density only affects dynamic bodies");
                    }
                    ImGui::EndChild();
                    ImGui::PopStyleColor();
                    ImGui::Spacing();
                }
            }
            
            // Info tooltip if body is sleeping
            if (isPlaying && rb.Body && !rb.Body->IsAwake() && rb.Body->IsEnabled())
            {
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.25f, 0.15f, 0.6f));
                if (ImGui::BeginChild("Info_Sleeping", ImVec2(0, 50), true))
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.9f, 0.5f, 1.0f));
                    ImGui::TextWrapped("✓ Body is sleeping (performance win!)");
                    ImGui::PopStyleColor();
                    ImGui::TextWrapped("Inactive bodies don't consume physics CPU time");
                }
                ImGui::EndChild();
                ImGui::PopStyleColor();
                ImGui::Spacing();
            }

            ImGui::Separator();
            ImGui::Spacing();

            // === PHYSICS PROPERTIES ===
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);

            ImGui::Text("Gravity Scale");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Multiplier for world gravity (1.0 = normal, 0.0 = floating)");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat("##GravityScale", &rb.GravityScale, Inspector::DRAG_SPEED_DEFAULT, 0.0f, 10.0f, "%.1f");
            ImGui::PopItemWidth();
            ImGui::Columns(1);

            // Gravity presets
            ImGui::Indent();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            if (ImGui::SmallButton("No Gravity (0)")) { rb.GravityScale = 0.0f; }
            ImGui::SameLine();
            if (ImGui::SmallButton("Half (0.5)")) { rb.GravityScale = 0.5f; }
            ImGui::SameLine();
            if (ImGui::SmallButton("Normal (1)")) { rb.GravityScale = 1.0f; }
            ImGui::SameLine();
            if (ImGui::SmallButton("Double (2)")) { rb.GravityScale = 2.0f; }
            ImGui::PopStyleVar();
            ImGui::Unindent();

            ImGui::Spacing();

            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
            ImGui::Text("Fixed Rotation");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("🔒 Prevent the body from rotating\nUseful for player characters");
            ImGui::NextColumn();
            ImGui::Checkbox("##FixedRotation", &rb.FixedRotation);
            ImGui::Columns(1);

            // === DAMPING CONTROLS ===
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::TextDisabled("Damping (Air Resistance)");
            
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
            
            ImGui::Text("Linear Damping");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Reduces linear velocity over time\n0 = no damping, higher = more resistance");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            bool linearDampingChanged = ImGui::DragFloat("##LinearDamping", &rb.LinearDamping, 0.01f, 0.0f, 5.0f, "%.2f");
            if (linearDampingChanged && rb.Body)
                rb.Body->SetLinearDamping(rb.LinearDamping);
            ImGui::PopItemWidth();
            ImGui::Columns(1);

            // Linear damping presets
            ImGui::Indent();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            if (ImGui::SmallButton("None (0)")) { 
                rb.LinearDamping = 0.0f;
                if (rb.Body) rb.Body->SetLinearDamping(0.0f);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Light (0.5)")) { 
                rb.LinearDamping = 0.5f;
                if (rb.Body) rb.Body->SetLinearDamping(0.5f);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Heavy (2.0)")) { 
                rb.LinearDamping = 2.0f;
                if (rb.Body) rb.Body->SetLinearDamping(2.0f);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Water (5.0)")) { 
                rb.LinearDamping = 5.0f;
                if (rb.Body) rb.Body->SetLinearDamping(5.0f);
            }
            ImGui::PopStyleVar();
            ImGui::Unindent();

            ImGui::Spacing();

            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
            
            ImGui::Text("Angular Damping");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Reduces rotational velocity over time\n0 = no damping, higher = more resistance");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            bool angularDampingChanged = ImGui::DragFloat("##AngularDamping", &rb.AngularDamping, 0.01f, 0.0f, 5.0f, "%.2f");
            if (angularDampingChanged && rb.Body)
                rb.Body->SetAngularDamping(rb.AngularDamping);
            ImGui::PopItemWidth();
            ImGui::Columns(1);

            // Angular damping presets
            ImGui::Indent();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            if (ImGui::SmallButton("None##Ang (0)")) { 
                rb.AngularDamping = 0.0f;
                if (rb.Body) rb.Body->SetAngularDamping(0.0f);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Light##Ang (0.5)")) { 
                rb.AngularDamping = 0.5f;
                if (rb.Body) rb.Body->SetAngularDamping(0.5f);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Heavy##Ang (2.0)")) { 
                rb.AngularDamping = 2.0f;
                if (rb.Body) rb.Body->SetAngularDamping(2.0f);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Water##Ang (5.0)")) { 
                rb.AngularDamping = 5.0f;
                if (rb.Body) rb.Body->SetAngularDamping(5.0f);
            }
            ImGui::PopStyleVar();
            ImGui::Unindent();

            // === ADVANCED SETTINGS ===
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::TextDisabled("Advanced Settings");

            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
            
            ImGui::Text("Bullet Mode");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("🚀 Enable continuous collision detection\nPrevents fast objects from tunneling through thin walls\nUse for bullets, projectiles, or high-speed objects");
            ImGui::NextColumn();
            bool bulletChanged = ImGui::Checkbox("##IsBullet", &rb.IsBullet);
            if (bulletChanged && rb.Body)
                rb.Body->SetBullet(rb.IsBullet);
            ImGui::Columns(1);

            // Velocity warning for bullet mode
            if (!rb.IsBullet && isPlaying && rb.Body && rb.BodyType == b2_dynamicBody)
            {
                b2Vec2 linearVel = rb.Body->GetLinearVelocity();
                float speed = linearVel.Length();
                if (speed > 20.0f) // Threshold for "fast" objects
                {
                    ImGui::Indent();
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.0f, 1.0f));
                    ImGui::TextWrapped("⚠ High velocity detected! Consider enabling Bullet Mode.");
                    ImGui::PopStyleColor();
                    ImGui::Unindent();
                }
            }

            ImGui::Spacing();

            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
            
            ImGui::Text("Enabled");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Temporarily disable physics simulation\nUseful for cutscenes or special states");
            ImGui::NextColumn();
            bool enabledChanged = ImGui::Checkbox("##IsEnabled", &rb.IsEnabled);
            if (enabledChanged && rb.Body)
                rb.Body->SetEnabled(rb.IsEnabled);
            ImGui::Columns(1);

            // === MASS & INERTIA INFO (Play Mode Only) ===
            if (isPlaying && rb.Body)
            {
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::TextDisabled("Mass & Inertia");
                
                ImGui::Indent();
                float mass = rb.Body->GetMass();
                float inertia = rb.Body->GetInertia();
                b2Vec2 centerOfMass = rb.Body->GetWorldCenter();

                ImGui::Columns(2);
                ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
                
                ImGui::Text("Mass");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Computed from fixture densities");
                ImGui::NextColumn();
                ImGui::TextDisabled("%.2f kg", mass);
                
                ImGui::NextColumn();
                ImGui::Text("Inertia");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Rotational mass");
                ImGui::NextColumn();
                ImGui::TextDisabled("%.2f", inertia);
                
                ImGui::NextColumn();
                ImGui::Text("Center of Mass");
                ImGui::NextColumn();
                ImGui::TextDisabled("(%.2f, %.2f)", centerOfMass.x, centerOfMass.y);
                
                ImGui::Columns(1);
                ImGui::Unindent();
            }

            // === VELOCITY CONTROLS (Play Mode Only) ===
            if (isPlaying && rb.Body && rb.BodyType == b2_dynamicBody)
            {
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::TextDisabled("Velocity (Runtime)");
                
                ImGui::Indent();
                
                // Linear velocity
                b2Vec2 linearVel = rb.Body->GetLinearVelocity();
                glm::vec2 linearVelGlm(linearVel.x, linearVel.y);
                float linearSpeed = glm::length(linearVelGlm);

                ImGui::Columns(2);
                ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
                
                ImGui::Text("Linear");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Linear velocity (X, Y)");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                
                // Display velocity with magnitude
                ImGui::Text("(%.2f, %.2f)", linearVel.x, linearVel.y);
                ImGui::SameLine();
                ImGui::TextDisabled("[%.2f m/s]", linearSpeed);
                
                ImGui::PopItemWidth();
                ImGui::Columns(1);

                // Angular velocity
                float angularVel = rb.Body->GetAngularVelocity();

                ImGui::Columns(2);
                ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
                
                ImGui::Text("Angular");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Angular velocity (rotation speed)");
                ImGui::NextColumn();
                ImGui::Text("%.2f rad/s", angularVel);
                ImGui::Columns(1);

                // Control buttons
                ImGui::Spacing();
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                
                if (ImGui::SmallButton("Reset Velocity"))
                {
                    rb.Body->SetLinearVelocity(b2Vec2(0.0f, 0.0f));
                    rb.Body->SetAngularVelocity(0.0f);
                }
                ImGui::SameLine();
                
                // Apply impulse controls
                static float impulseX = 10.0f;
                static float impulseY = 0.0f;
                
                ImGui::TextDisabled("Quick Impulse:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(60);
                ImGui::DragFloat("##ImpulseX", &impulseX, 1.0f, -100.0f, 100.0f, "X:%.0f");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(60);
                ImGui::DragFloat("##ImpulseY", &impulseY, 1.0f, -100.0f, 100.0f, "Y:%.0f");
                ImGui::SameLine();
                
                if (ImGui::SmallButton("Apply"))
                {
                    rb.Body->ApplyLinearImpulseToCenter(b2Vec2(impulseX, impulseY), true);
                }
                
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Apply impulse to center of body");
                
                ImGui::PopStyleVar();
                ImGui::Unindent();
            }

            ImGui::Spacing();

            // Info box
            if (rb.Body)
            {
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.15f, 0.1f, 0.8f));
                if (ImGui::BeginChild("RBInfo", ImVec2(0, 40), true))
                {
                    ImGui::TextDisabled("✓ Physics body active");
                    ImGui::TextDisabled("Add a Collider component for collision detection");
                }
                ImGui::EndChild();
                ImGui::PopStyleColor();
            }
            else
            {
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.1f, 0.1f, 0.8f));
                if (ImGui::BeginChild("RBWarning", ImVec2(0, 40), true))
                {
                    ImGui::TextDisabled("⚠ Physics body not created yet");
                    ImGui::TextDisabled("Enter play mode to activate");
                }
                ImGui::EndChild();
                ImGui::PopStyleColor();
            }

            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    void InspectorPanel::DrawColliderComponent(Pillar::Entity entity)
    {
        if (!entity.HasComponent<Pillar::ColliderComponent>())
            return;

        ImGui::PushID("ColliderComponent");

        bool open = DrawComponentHeader<Pillar::ColliderComponent>("Collider", entity);
        if (!open && !entity.HasComponent<Pillar::ColliderComponent>())
            return; // Component was removed

        if (open)
        {
            auto& collider = entity.GetComponent<Pillar::ColliderComponent>();

            // === COLLIDER TYPE ===
            const char* colliderTypes[] = { "Circle", "Box", "Polygon" };
            int currentType = static_cast<int>(collider.Type);
            
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
            ImGui::Text("Shape Type");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::Combo("##ColliderType", &currentType, colliderTypes, 3))
            {
                collider.Type = static_cast<Pillar::ColliderType>(currentType);
            }
            ImGui::PopItemWidth();
            ImGui::Columns(1);

            // Auto-fit button
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.45f, 0.75f, 1.0f));
            if (ImGui::Button("🎯 Auto-fit to Bounds", ImVec2(-1, 0)))
            {
                // Get entity's visual bounds
                glm::vec2 boundsSize = glm::vec2(1.0f, 1.0f);
                
                if (entity.HasComponent<Pillar::SpriteComponent>())
                {
                    auto& sprite = entity.GetComponent<Pillar::SpriteComponent>();
                    boundsSize = sprite.Size;
                }
                
                // Apply transform scale to get actual rendered size
                if (entity.HasComponent<Pillar::TransformComponent>())
                {
                    auto& transform = entity.GetComponent<Pillar::TransformComponent>();
                    boundsSize *= transform.Scale;
                }
                
                // Apply to collider based on type
                switch (collider.Type)
                {
                case Pillar::ColliderType::Circle:
                    // Use the larger dimension for radius
                    collider.Radius = glm::max(boundsSize.x, boundsSize.y) * 0.5f;
                    break;
                    
                case Pillar::ColliderType::Box:
                    // Set half extents to match scaled sprite size
                    collider.HalfExtents = boundsSize * 0.5f;
                    break;
                    
                case Pillar::ColliderType::Polygon:
                    // Generate rectangular polygon matching scaled bounds
                    {
                        float halfW = boundsSize.x * 0.5f;
                        float halfH = boundsSize.y * 0.5f;
                        collider.Vertices.clear();
                        collider.Vertices.push_back(glm::vec2(-halfW, -halfH)); // Bottom-left
                        collider.Vertices.push_back(glm::vec2( halfW, -halfH)); // Bottom-right
                        collider.Vertices.push_back(glm::vec2( halfW,  halfH)); // Top-right
                        collider.Vertices.push_back(glm::vec2(-halfW,  halfH)); // Top-left
                    }
                    break;
                }
                
                collider.Offset = glm::vec2(0.0f); // Reset offset to center
            }
            ImGui::PopStyleColor(3);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Automatically size collider to match sprite bounds\n(includes transform scale)");

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("📏 Shape Parameters");
            ImGui::Separator();
            ImGui::Spacing();

            // === SHAPE PARAMETERS ===
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);

            if (collider.Type == Pillar::ColliderType::Circle)
            {
                ImGui::Text("Radius");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Circle radius in world units");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                ImGui::DragFloat("##Radius", &collider.Radius, Inspector::DRAG_SPEED_ROTATION, 0.01f, 100.0f, "%.2f");
                ImGui::PopItemWidth();
                ImGui::Columns(1);

                // Radius presets
                ImGui::Indent();
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                if (ImGui::SmallButton("0.5")) { collider.Radius = 0.5f; }
                ImGui::SameLine();
                if (ImGui::SmallButton("1.0")) { collider.Radius = 1.0f; }
                ImGui::SameLine();
                if (ImGui::SmallButton("2.0")) { collider.Radius = 2.0f; }
                ImGui::PopStyleVar();
                ImGui::Unindent();

                ImGui::Spacing();
                ImGui::Columns(2);
                ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
            }
            else if (collider.Type == Pillar::ColliderType::Box)
            {
                ImGui::Text("Half Extents");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Half width and half height\n(full size = half extents × 2)");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                ImGui::DragFloat2("##HalfExtents", glm::value_ptr(collider.HalfExtents), Inspector::DRAG_SPEED_ROTATION, 0.01f, 100.0f, "%.2f");
                ImGui::PopItemWidth();
                ImGui::Columns(1);

                // Box size presets
                ImGui::Indent();
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                if (ImGui::SmallButton("0.5x0.5")) { collider.HalfExtents = glm::vec2(0.5f); }
                ImGui::SameLine();
                if (ImGui::SmallButton("1x1")) { collider.HalfExtents = glm::vec2(1.0f); }
                ImGui::SameLine();
                if (ImGui::SmallButton("1x2")) { collider.HalfExtents = glm::vec2(1.0f, 2.0f); }
                ImGui::SameLine();
                if (ImGui::SmallButton("2x1")) { collider.HalfExtents = glm::vec2(2.0f, 1.0f); }
                ImGui::PopStyleVar();
                ImGui::Unindent();

                ImGui::Spacing();
                ImGui::Columns(2);
                ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
            }
            else if (collider.Type == Pillar::ColliderType::Polygon)
            {
                ImGui::Columns(1);
                
                // Polygon vertices editor
                ImGui::Text("Vertices");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Polygon vertices (3-8 points)\nMust be convex and counter-clockwise");

                ImGui::Spacing();

                // Vertex list
                int vertexToRemove = -1;
                for (size_t i = 0; i < collider.Vertices.size(); ++i)
                {
                    ImGui::PushID((int)i);
                    ImGui::Indent();
                    
                    // Vertex label
                    ImGui::Text("V%d", (int)i);
                    ImGui::SameLine();
                    
                    // Vertex input
                    ImGui::PushItemWidth(-60);
                    ImGui::DragFloat2("##Vertex", glm::value_ptr(collider.Vertices[i]), Inspector::DRAG_SPEED_ROTATION, -10.0f, 10.0f, "%.2f");
                    ImGui::PopItemWidth();
                    
                    // Delete button
                    ImGui::SameLine();
                    if (ImGui::SmallButton("X"))
                    {
                        vertexToRemove = (int)i;
                    }
                    
                    ImGui::Unindent();
                    ImGui::PopID();
                }

                // Remove vertex if requested (do this after iteration to avoid iterator invalidation)
                if (vertexToRemove >= 0 && collider.Vertices.size() > 3)
                {
                    collider.Vertices.erase(collider.Vertices.begin() + vertexToRemove);
                }

                ImGui::Spacing();

                // Add vertex button
                if (collider.Vertices.size() < 8)
                {
                    if (ImGui::Button("+ Add Vertex", ImVec2(-1, 0)))
                    {
                        // Add vertex at (0, 0) by default
                        collider.Vertices.push_back(glm::vec2(0.0f, 0.0f));
                    }
                }
                else
                {
                    ImGui::TextDisabled("⚠ Max 8 vertices (Box2D limit)");
                }

                // Validation warnings
                if (collider.Vertices.size() < 3)
                {
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "⚠ Need at least 3 vertices!");
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Text("Regular Polygon Presets:");
                ImGui::Indent();
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                
                if (ImGui::SmallButton("Triangle"))
                {
                    collider = Pillar::ColliderComponent::RegularPolygon(3, 0.5f);
                    collider.Offset = glm::vec2(0.0f);
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("Square"))
                {
                    collider = Pillar::ColliderComponent::RegularPolygon(4, 0.5f);
                    collider.Offset = glm::vec2(0.0f);
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("Pentagon"))
                {
                    collider = Pillar::ColliderComponent::RegularPolygon(5, 0.5f);
                    collider.Offset = glm::vec2(0.0f);
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("Hexagon"))
                {
                    collider = Pillar::ColliderComponent::RegularPolygon(6, 0.5f);
                    collider.Offset = glm::vec2(0.0f);
                }
                
                ImGui::PopStyleVar();
                ImGui::Unindent();

                ImGui::Spacing();
                ImGui::Columns(2);
                ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
            }

            ImGui::Text("Offset");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Local offset from entity center");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat2("##Offset", glm::value_ptr(collider.Offset), Inspector::DRAG_SPEED_ROTATION, -100.0f, 100.0f, "%.2f");
            ImGui::PopItemWidth();
            ImGui::Columns(1);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("🔧 Physics Material");
            ImGui::Separator();
            ImGui::Spacing();

            // === PHYSICS MATERIAL ===
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);

            ImGui::Text("Density");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Mass per unit area (affects weight)");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat("##Density", &collider.Density, Inspector::DRAG_SPEED_DEFAULT, 0.0f, 100.0f, "%.1f");
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Text("Friction");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Surface friction (0 = ice, 1 = rubber)");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat("##Friction", &collider.Friction, Inspector::DRAG_SPEED_PRECISE, 0.0f, 1.0f, "%.2f");
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Text("Restitution");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Bounciness (0 = no bounce, 1 = perfect bounce)");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat("##Restitution", &collider.Restitution, Inspector::DRAG_SPEED_PRECISE, 0.0f, 1.0f, "%.2f");
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Text("Is Sensor");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("🚪 Trigger only (no physical collision)");
            ImGui::NextColumn();
            ImGui::Checkbox("##IsSensor", &collider.IsSensor);
            
            ImGui::Columns(1);

            // Material presets
            ImGui::Spacing();
            ImGui::Text("Material Presets:");
            ImGui::Indent();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            if (ImGui::SmallButton("Default"))
            {
                collider.Density = 1.0f;
                collider.Friction = 0.3f;
                collider.Restitution = 0.0f;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Bouncy Ball"))
            {
                collider.Density = 0.5f;
                collider.Friction = 0.2f;
                collider.Restitution = 0.8f;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Ice"))
            {
                collider.Density = 0.5f;
                collider.Friction = 0.05f;
                collider.Restitution = 0.1f;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Heavy"))
            {
                collider.Density = 10.0f;
                collider.Friction = 0.5f;
                collider.Restitution = 0.0f;
            }
            ImGui::PopStyleVar();
            ImGui::Unindent();

            ImGui::TreePop();
        }

        ImGui::PopID();
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
                    auto& sprite = entity.AddComponent<Pillar::SpriteComponent>();
                    // Initialize visibility from Default layer
                    auto& layerMgr = LayerManager::Get();
                    auto* defaultLayer = layerMgr.GetLayer("Default");
                    if (defaultLayer)
                    {
                        sprite.Visible = defaultLayer->visible;
                        sprite.ZIndex = defaultLayer->baseZIndex;
                    }
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

            ImGui::Separator();
            ImGui::TextDisabled("Lighting:");

            if (!entity.HasComponent<Pillar::Light2DComponent>())
            {
                if (ImGui::Selectable("Light 2D"))
                {
                    entity.AddComponent<Pillar::Light2DComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }

            if (!entity.HasComponent<Pillar::ShadowCaster2DComponent>())
            {
                if (ImGui::Selectable("Shadow Caster 2D"))
                {
                    auto& caster = entity.AddComponent<Pillar::ShadowCaster2DComponent>();
                    caster.Points = {
                        { -0.5f, -0.5f },
                        {  0.5f, -0.5f },
                        {  0.5f,  0.5f },
                        { -0.5f,  0.5f }
                    };
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::EndPopup();
        }
    }

    void InspectorPanel::DrawLight2DComponent(Pillar::Entity entity)
    {
        if (!entity.HasComponent<Pillar::Light2DComponent>())
            return;

        ImGui::PushID("Light2DComponent");

        bool open = DrawComponentHeader<Pillar::Light2DComponent>("Light 2D", entity);
        if (!open && !entity.HasComponent<Pillar::Light2DComponent>())
            return;

        if (open)
        {
            auto& light = entity.GetComponent<Pillar::Light2DComponent>();

            auto DrawLayerMaskHelper = [](const char* popupId, uint32_t& mask)
            {
                float fullWidth = ImGui::CalcItemWidth();
                float buttonWidth = 28.0f;
                float inputWidth = std::max(1.0f, fullWidth - buttonWidth - 6.0f);

                ImGui::SetNextItemWidth(inputWidth);
                ImGui::InputScalar("##Mask", ImGuiDataType_U32, &mask, nullptr, nullptr, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
                ImGui::SameLine();
                if (ImGui::SmallButton("..."))
                    ImGui::OpenPopup(popupId);

                if (ImGui::BeginPopup(popupId))
                {
                    if (ImGui::SmallButton("All")) mask = 0xFFFFFFFFu;
                    ImGui::SameLine();
                    if (ImGui::SmallButton("None")) mask = 0u;
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Invert")) mask = ~mask;

                    ImGui::Separator();

                    ImGui::Columns(4, nullptr, false);
                    for (uint32_t bit = 0; bit < 32; ++bit)
                    {
                        ImGui::PushID(static_cast<int>(bit));
                        bool enabled = (mask & (1u << bit)) != 0u;
                        if (ImGui::Checkbox("##L", &enabled))
                        {
                            if (enabled) mask |= (1u << bit);
                            else mask &= ~(1u << bit);
                        }
                        ImGui::SameLine();
                        ImGui::Text("%u", bit);
                        ImGui::NextColumn();
                        ImGui::PopID();
                    }
                    ImGui::Columns(1);

                    ImGui::EndPopup();
                }
            };

            // Type
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
            ImGui::Text("Type");
            ImGui::NextColumn();
            const char* typeNames[] = { "Point", "Spot" };
            int currentType = static_cast<int>(light.Type);
            ImGui::PushItemWidth(-1);
            if (ImGui::Combo("##LightType", &currentType, typeNames, IM_ARRAYSIZE(typeNames)))
                light.Type = static_cast<Pillar::Light2DType>(currentType);
            ImGui::PopItemWidth();
            ImGui::Columns(1);

            // Color
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
            ImGui::Text("Color");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::ColorEdit3("##LightColor", &light.Color.x, ImGuiColorEditFlags_Float);
            ImGui::PopItemWidth();
            ImGui::Columns(1);

            // Intensity
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
            ImGui::Text("Intensity");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat("##LightIntensity", &light.Intensity, 0.05f, 0.0f, 10.0f, "%.2f");
            ImGui::PopItemWidth();
            ImGui::Columns(1);

            // Radius
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
            ImGui::Text("Radius");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat("##LightRadius", &light.Radius, 0.1f, 0.1f, 100.0f, "%.2f");
            ImGui::PopItemWidth();
            ImGui::Columns(1);

            if (light.Type == Pillar::Light2DType::Spot)
            {
                float innerDeg = glm::degrees(light.InnerAngleRadians);
                float outerDeg = glm::degrees(light.OuterAngleRadians);

                ImGui::Columns(2);
                ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
                ImGui::Text("Inner Angle");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                if (ImGui::DragFloat("##InnerAngle", &innerDeg, 0.5f, 1.0f, 89.0f, "%.1f\xC2\xB0"))
                {
                    light.InnerAngleRadians = glm::radians(innerDeg);
                    if (light.InnerAngleRadians > light.OuterAngleRadians)
                        light.OuterAngleRadians = light.InnerAngleRadians;
                }
                ImGui::PopItemWidth();
                ImGui::Columns(1);

                ImGui::Columns(2);
                ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
                ImGui::Text("Outer Angle");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                if (ImGui::DragFloat("##OuterAngle", &outerDeg, 0.5f, 1.0f, 90.0f, "%.1f\xC2\xB0"))
                {
                    light.OuterAngleRadians = glm::radians(outerDeg);
                    if (light.OuterAngleRadians < light.InnerAngleRadians)
                        light.InnerAngleRadians = light.OuterAngleRadians;
                }
                ImGui::PopItemWidth();
                ImGui::Columns(1);
            }

            // Shadows
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
            ImGui::Text("Cast Shadows");
            ImGui::NextColumn();
            ImGui::Checkbox("##CastShadows", &light.CastShadows);
            ImGui::Columns(1);

            if (light.CastShadows)
            {
                ImGui::Columns(2);
                ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
                ImGui::Text("Shadow Strength");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                ImGui::SliderFloat("##ShadowStrength", &light.ShadowStrength, 0.0f, 1.0f, "%.2f");
                ImGui::PopItemWidth();
                ImGui::Columns(1);
            }

            // Layer Mask
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
            ImGui::Text("Layer Mask");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::PushID("LightLayerMask");
            DrawLayerMaskHelper("LightLayerMaskPopup", light.LayerMask);
            ImGui::PopID();
            ImGui::PopItemWidth();
            ImGui::Columns(1);
        }

        if (open)
            ImGui::TreePop();

        ImGui::PopID();
    }

    void InspectorPanel::DrawShadowCaster2DComponent(Pillar::Entity entity)
    {
        if (!entity.HasComponent<Pillar::ShadowCaster2DComponent>())
            return;

        ImGui::PushID("ShadowCaster2DComponent");

        bool open = DrawComponentHeader<Pillar::ShadowCaster2DComponent>("Shadow Caster 2D", entity);
        if (!open && !entity.HasComponent<Pillar::ShadowCaster2DComponent>())
            return;

        if (open)
        {
            auto& caster = entity.GetComponent<Pillar::ShadowCaster2DComponent>();

            auto DrawLayerMaskHelper = [](const char* popupId, uint32_t& mask)
            {
                float fullWidth = ImGui::CalcItemWidth();
                float buttonWidth = 28.0f;
                float inputWidth = std::max(1.0f, fullWidth - buttonWidth - 6.0f);

                ImGui::SetNextItemWidth(inputWidth);
                ImGui::InputScalar("##Mask", ImGuiDataType_U32, &mask, nullptr, nullptr, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
                ImGui::SameLine();
                if (ImGui::SmallButton("..."))
                    ImGui::OpenPopup(popupId);

                if (ImGui::BeginPopup(popupId))
                {
                    if (ImGui::SmallButton("All")) mask = 0xFFFFFFFFu;
                    ImGui::SameLine();
                    if (ImGui::SmallButton("None")) mask = 0u;
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Invert")) mask = ~mask;

                    ImGui::Separator();

                    ImGui::Columns(4, nullptr, false);
                    for (uint32_t bit = 0; bit < 32; ++bit)
                    {
                        ImGui::PushID(static_cast<int>(bit));
                        bool enabled = (mask & (1u << bit)) != 0u;
                        if (ImGui::Checkbox("##L", &enabled))
                        {
                            if (enabled) mask |= (1u << bit);
                            else mask &= ~(1u << bit);
                        }
                        ImGui::SameLine();
                        ImGui::Text("%u", bit);
                        ImGui::NextColumn();
                        ImGui::PopID();
                    }
                    ImGui::Columns(1);

                    ImGui::EndPopup();
                }
            };

            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
            ImGui::Text("Closed");
            ImGui::NextColumn();
            ImGui::Checkbox("##Closed", &caster.Closed);
            ImGui::Columns(1);

            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
            ImGui::Text("Two Sided");
            ImGui::NextColumn();
            ImGui::Checkbox("##TwoSided", &caster.TwoSided);
            ImGui::Columns(1);

            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
            ImGui::Text("Layer Mask");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::PushID("CasterLayerMask");
            DrawLayerMaskHelper("CasterLayerMaskPopup", caster.LayerMask);
            ImGui::PopID();
            ImGui::PopItemWidth();
            ImGui::Columns(1);

            ImGui::Separator();
            ImGui::Text("Points (%zu)", caster.Points.size());

            int removeIndex = -1;
            for (size_t i = 0; i < caster.Points.size(); i++)
            {
                ImGui::PushID(static_cast<int>(i));

                ImGui::Text("%zu", i);
                ImGui::SameLine();

                ImGui::PushItemWidth(80);
                ImGui::DragFloat("##X", &caster.Points[i].x, 0.01f, -100.0f, 100.0f, "%.2f");
                ImGui::SameLine();
                ImGui::DragFloat("##Y", &caster.Points[i].y, 0.01f, -100.0f, 100.0f, "%.2f");
                ImGui::PopItemWidth();

                ImGui::SameLine();
                if (ImGui::SmallButton("X"))
                    removeIndex = static_cast<int>(i);

                ImGui::PopID();
            }

            if (removeIndex >= 0 && caster.Points.size() > 2)
                caster.Points.erase(caster.Points.begin() + removeIndex);

            if (ImGui::Button("+ Add Point"))
            {
                glm::vec2 newPoint = caster.Points.empty() ? glm::vec2(0.0f) : (caster.Points.back() + glm::vec2(0.5f, 0.0f));
                caster.Points.push_back(newPoint);
            }

            ImGui::SameLine();
            if (ImGui::Button("Reset to Box"))
            {
                caster.Points = {
                    { -0.5f, -0.5f },
                    {  0.5f, -0.5f },
                    {  0.5f,  0.5f },
                    { -0.5f,  0.5f }
                };
            }

            if (entity.HasComponent<Pillar::ColliderComponent>())
            {
                ImGui::SameLine();
                if (ImGui::Button("From Collider"))
                {
                    auto& collider = entity.GetComponent<Pillar::ColliderComponent>();
                    if (collider.Type == Pillar::ColliderType::Box)
                    {
                        glm::vec2 h = collider.HalfExtents;
                        caster.Points = {
                            { -h.x, -h.y },
                            {  h.x, -h.y },
                            {  h.x,  h.y },
                            { -h.x,  h.y }
                        };
                    }
                    else if (collider.Type == Pillar::ColliderType::Polygon)
                    {
                        caster.Points = collider.Vertices;
                    }
                }
            }
        }

        if (open)
            ImGui::TreePop();

        ImGui::PopID();
    }

    void InspectorPanel::DrawBulletComponent(Pillar::Entity entity)
    {
        if (!entity.HasComponent<Pillar::BulletComponent>())
            return;

        ImGui::PushID("BulletComponent");

        bool open = DrawComponentHeader<Pillar::BulletComponent>("Bullet", entity);
        if (!open && !entity.HasComponent<Pillar::BulletComponent>())
            return; // Component was removed

        if (open)
        {
            auto& bullet = entity.GetComponent<Pillar::BulletComponent>();

            // === BULLET STATS ===
            ImGui::Separator();
            ImGui::Text("💥 Bullet Statistics");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL_WIDE);

            ImGui::Text("Damage");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Damage dealt per hit");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat("##Damage", &bullet.Damage, Inspector::DRAG_SPEED_SLOW, 0.0f, 1000.0f, "%.1f");
            ImGui::PopItemWidth();
            ImGui::Columns(1);

            // Damage presets
            ImGui::Indent();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            if (ImGui::SmallButton("Light (10)")) { bullet.Damage = 10.0f; }
            ImGui::SameLine();
            if (ImGui::SmallButton("Normal (25)")) { bullet.Damage = 25.0f; }
            ImGui::SameLine();
            if (ImGui::SmallButton("Heavy (50)")) { bullet.Damage = 50.0f; }
            ImGui::SameLine();
            if (ImGui::SmallButton("Devastating (100)")) { bullet.Damage = 100.0f; }
            ImGui::PopStyleVar();
            ImGui::Unindent();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("⏱ Lifetime");
            ImGui::Separator();
            ImGui::Spacing();

            // === LIFETIME ===
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL_WIDE);

            ImGui::Text("Max Lifetime");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Total seconds before bullet is destroyed");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat("##Lifetime", &bullet.Lifetime, Inspector::DRAG_SPEED_DEFAULT, 0.1f, 60.0f, "%.1f s");
            ImGui::PopItemWidth();
            ImGui::Columns(1);

            // Lifetime presets
            ImGui::Indent();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            if (ImGui::SmallButton("Short (1s)")) { bullet.Lifetime = 1.0f; }
            ImGui::SameLine();
            if (ImGui::SmallButton("Normal (3s)")) { bullet.Lifetime = 3.0f; }
            ImGui::SameLine();
            if (ImGui::SmallButton("Long (5s)")) { bullet.Lifetime = 5.0f; }
            ImGui::SameLine();
            if (ImGui::SmallButton("Very Long (10s)")) { bullet.Lifetime = 10.0f; }
            ImGui::PopStyleVar();
            ImGui::Unindent();

            ImGui::Spacing();
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL_WIDE);

            ImGui::Text("Time Alive");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Current age of this bullet");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            
            // Lifetime progress bar
            float lifePercent = bullet.Lifetime > 0.0f ? (bullet.TimeAlive / bullet.Lifetime) : 0.0f;
            ImGui::ProgressBar(lifePercent, ImVec2(-1, 0), "");
            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
            ImGui::TextDisabled("%.2f / %.2f s", bullet.TimeAlive, bullet.Lifetime);
            
            ImGui::PopItemWidth();
            ImGui::Columns(1);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("🎯 Pierce Settings");
            ImGui::Separator();
            ImGui::Spacing();

            // === PIERCE SETTINGS ===
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL_WIDE);

            ImGui::Text("Pierce");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("✓ Bullet passes through enemies\n✗ Bullet destroyed on first hit");
            ImGui::NextColumn();
            ImGui::Checkbox("##Pierce", &bullet.Pierce);
            ImGui::NextColumn();

            if (bullet.Pierce)
            {
                ImGui::Text("Max Hits");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Maximum number of enemies this bullet can hit\nbefore being destroyed");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                int maxHits = static_cast<int>(bullet.MaxHits);
                if (ImGui::DragInt("##MaxHits", &maxHits, Inspector::DRAG_SPEED_FAST, 1, 100))
                    bullet.MaxHits = static_cast<uint32_t>(maxHits);
                ImGui::PopItemWidth();
                ImGui::Columns(1);

                // Max hits presets
                ImGui::Indent();
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                if (ImGui::SmallButton("2 hits")) { bullet.MaxHits = 2; }
                ImGui::SameLine();
                if (ImGui::SmallButton("3 hits")) { bullet.MaxHits = 3; }
                ImGui::SameLine();
                if (ImGui::SmallButton("5 hits")) { bullet.MaxHits = 5; }
                ImGui::SameLine();
                if (ImGui::SmallButton("Unlimited")) { bullet.MaxHits = 999; }
                ImGui::PopStyleVar();
                ImGui::Unindent();

                ImGui::Spacing();
                ImGui::Columns(2);
                ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL_WIDE);

                ImGui::Text("Hits Remaining");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                ImGui::ProgressBar(static_cast<float>(bullet.HitsRemaining) / static_cast<float>(bullet.MaxHits), 
                                   ImVec2(-1, 0), "");
                ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
                ImGui::TextDisabled("%u / %u", bullet.HitsRemaining, bullet.MaxHits);
                ImGui::PopItemWidth();
            }

            ImGui::Columns(1);

            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    void InspectorPanel::DrawXPGemComponent(Pillar::Entity entity)
    {
        if (!entity.HasComponent<Pillar::XPGemComponent>())
            return;

        ImGui::PushID("XPGemComponent");

        bool open = DrawComponentHeader<Pillar::XPGemComponent>("XP Gem", entity);
        if (!open && !entity.HasComponent<Pillar::XPGemComponent>())
            return; // Component was removed

        if (open)
        {
            auto& xpGem = entity.GetComponent<Pillar::XPGemComponent>();

            // === XP VALUE ===
            ImGui::Separator();
            ImGui::Text("💎 Experience Value");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL_EXTRA_WIDE);

            ImGui::Text("XP Value");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Amount of XP awarded when collected");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::DragInt("##XPValue", &xpGem.XPValue, Inspector::DRAG_SPEED_FAST, 1, 10000);
            ImGui::PopItemWidth();
            ImGui::Columns(1);

            // XP value presets
            ImGui::Indent();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            if (ImGui::SmallButton("Tiny (1)")) { xpGem.XPValue = 1; }
            ImGui::SameLine();
            if (ImGui::SmallButton("Small (5)")) { xpGem.XPValue = 5; }
            ImGui::SameLine();
            if (ImGui::SmallButton("Medium (10)")) { xpGem.XPValue = 10; }
            ImGui::SameLine();
            if (ImGui::SmallButton("Large (25)")) { xpGem.XPValue = 25; }
            ImGui::SameLine();
            if (ImGui::SmallButton("Epic (100)")) { xpGem.XPValue = 100; }
            ImGui::PopStyleVar();
            ImGui::Unindent();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("🧲 Attraction Settings");
            ImGui::Separator();
            ImGui::Spacing();

            // === ATTRACTION SETTINGS ===
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL_EXTRA_WIDE);

            ImGui::Text("Attraction Radius");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Distance at which the gem starts moving toward the player");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat("##AttractionRadius", &xpGem.AttractionRadius, Inspector::DRAG_SPEED_DEFAULT, 0.1f, 50.0f, "%.1f");
            ImGui::PopItemWidth();
            ImGui::Columns(1);

            // Attraction radius presets
            ImGui::Indent();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            if (ImGui::SmallButton("Close (2)")) { xpGem.AttractionRadius = 2.0f; }
            ImGui::SameLine();
            if (ImGui::SmallButton("Normal (5)")) { xpGem.AttractionRadius = 5.0f; }
            ImGui::SameLine();
            if (ImGui::SmallButton("Far (10)")) { xpGem.AttractionRadius = 10.0f; }
            ImGui::SameLine();
            if (ImGui::SmallButton("Very Far (15)")) { xpGem.AttractionRadius = 15.0f; }
            ImGui::PopStyleVar();
            ImGui::Unindent();

            ImGui::Spacing();
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL_EXTRA_WIDE);

            ImGui::Text("Move Speed");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("How fast the gem moves toward the player");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat("##MoveSpeed", &xpGem.MoveSpeed, Inspector::DRAG_SPEED_SLOW, 0.1f, 100.0f, "%.1f");
            ImGui::PopItemWidth();
            ImGui::Columns(1);

            // Move speed presets
            ImGui::Indent();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            if (ImGui::SmallButton("Slow (5)")) { xpGem.MoveSpeed = 5.0f; }
            ImGui::SameLine();
            if (ImGui::SmallButton("Normal (10)")) { xpGem.MoveSpeed = 10.0f; }
            ImGui::SameLine();
            if (ImGui::SmallButton("Fast (20)")) { xpGem.MoveSpeed = 20.0f; }
            ImGui::SameLine();
            if (ImGui::SmallButton("Instant (50)")) { xpGem.MoveSpeed = 50.0f; }
            ImGui::PopStyleVar();
            ImGui::Unindent();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("📊 Status");
            ImGui::Separator();
            ImGui::Spacing();

            // === STATUS ===
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL_EXTRA_WIDE);

            ImGui::Text("Is Attracted");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Currently being pulled toward the player");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            
            // Visual indicator
            if (xpGem.IsAttracted)
            {
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.7f, 0.2f, 0.5f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 1.0f, 0.2f, 1.0f));
                ImGui::TextDisabled("🧲 Being Attracted");
                ImGui::PopStyleColor(2);
            }
            else
            {
                ImGui::TextDisabled("⏸ Idle");
            }
            
            ImGui::PopItemWidth();
            ImGui::Columns(1);

            ImGui::Spacing();
            
            // Info box
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.2f, 0.3f, 0.7f, 0.2f));
            ImGui::BeginChild("XPGemInfo", ImVec2(0, 60), true);
            ImGui::TextWrapped("💡 XP Gem Tips:");
            ImGui::BulletText("Higher XP value = more player progression");
            ImGui::BulletText("Larger attraction radius = easier to collect");
            ImGui::EndChild();
            ImGui::PopStyleColor();

            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    void InspectorPanel::DrawHierarchyComponent(Pillar::Entity entity)
    {
        if (!entity.HasComponent<Pillar::HierarchyComponent>())
            return;

        ImGui::PushID("HierarchyComponent");

        bool open = DrawComponentHeader<Pillar::HierarchyComponent>("Hierarchy", entity, false); // Can't remove Hierarchy manually

        if (open)
        {
            auto& hierarchy = entity.GetComponent<Pillar::HierarchyComponent>();

            ImGui::Separator();
            ImGui::Text("🌳 Hierarchy Information");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL_WIDE);

            ImGui::Text("Parent UUID");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Unique identifier of the parent entity");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (hierarchy.ParentUUID != 0)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 1.0f, 0.2f, 1.0f));
                ImGui::Text("%llu", hierarchy.ParentUUID);
                ImGui::PopStyleColor();
            }
            else
            {
                ImGui::TextDisabled("None (Root Entity)");
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Columns(1);

            ImGui::Spacing();

            // Status indicator
            if (hierarchy.ParentUUID != 0)
            {
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.2f, 0.7f, 0.2f, 0.2f));
                ImGui::BeginChild("HierarchyStatus", ImVec2(0, 40), true);
                ImGui::TextWrapped("✓ This entity is a child of another entity");
                ImGui::EndChild();
                ImGui::PopStyleColor();
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.7f, 0.7f, 0.2f, 0.2f));
                ImGui::BeginChild("HierarchyStatus", ImVec2(0, 40), true);
                ImGui::TextWrapped("⚠ This is a root entity (no parent)");
                ImGui::EndChild();
                ImGui::PopStyleColor();
            }

            ImGui::Spacing();

            // Info box
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.2f, 0.3f, 0.7f, 0.2f));
            ImGui::BeginChild("HierarchyInfo", ImVec2(0, 80), true);
            ImGui::TextWrapped("💡 Hierarchy Tips:");
            ImGui::BulletText("Use the Scene Hierarchy panel to set parent-child relationships");
            ImGui::BulletText("Child entities inherit their parent's transform");
            ImGui::BulletText("Deleting a parent also deletes all children");
            ImGui::EndChild();
            ImGui::PopStyleColor();

            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    template<typename T>
    bool InspectorPanel::DrawComponentHeader(const char* label, Pillar::Entity entity, bool canRemove)
    {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
                                   ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap |
                                   ImGuiTreeNodeFlags_FramePadding;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
        
        ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();
        float lineHeight = ImGui::GetFrameHeight();
        
        bool open = ImGui::TreeNodeEx(label, flags);
        
        // Remove button (if allowed)
        if (canRemove)
        {
            ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
            std::string buttonLabel = "X##Remove";
            buttonLabel += label;
            if (ImGui::Button(buttonLabel.c_str(), ImVec2(lineHeight, lineHeight)))
            {
                entity.RemoveComponent<T>();
                ImGui::PopStyleVar();
                if (open)
                    ImGui::TreePop();
                ImGui::PopID(); // Pop the component ID that was pushed before calling this
                return false; // Indicate component was removed
            }
        }
        
        ImGui::PopStyleVar();
        
        return open; // Return whether the tree node is open
    }

    // Explicit template instantiations
    template bool InspectorPanel::DrawComponentHeader<Pillar::TransformComponent>(const char*, Pillar::Entity, bool);
    template bool InspectorPanel::DrawComponentHeader<Pillar::SpriteComponent>(const char*, Pillar::Entity, bool);
    template bool InspectorPanel::DrawComponentHeader<Pillar::CameraComponent>(const char*, Pillar::Entity, bool);
    template bool InspectorPanel::DrawComponentHeader<Pillar::AnimationComponent>(const char*, Pillar::Entity, bool);
    template bool InspectorPanel::DrawComponentHeader<Pillar::VelocityComponent>(const char*, Pillar::Entity, bool);
    template bool InspectorPanel::DrawComponentHeader<Pillar::RigidbodyComponent>(const char*, Pillar::Entity, bool);
    template bool InspectorPanel::DrawComponentHeader<Pillar::ColliderComponent>(const char*, Pillar::Entity, bool);
    template bool InspectorPanel::DrawComponentHeader<Pillar::Light2DComponent>(const char*, Pillar::Entity, bool);
    template bool InspectorPanel::DrawComponentHeader<Pillar::ShadowCaster2DComponent>(const char*, Pillar::Entity, bool);
    template bool InspectorPanel::DrawComponentHeader<Pillar::BulletComponent>(const char*, Pillar::Entity, bool);
    template bool InspectorPanel::DrawComponentHeader<Pillar::XPGemComponent>(const char*, Pillar::Entity, bool);
    template bool InspectorPanel::DrawComponentHeader<Pillar::HierarchyComponent>(const char*, Pillar::Entity, bool);

}











