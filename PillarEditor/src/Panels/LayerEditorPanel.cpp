#include "LayerEditorPanel.h"
#include "../EditorSettings.h"
#include "ConsolePanel.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Components/Rendering/SpriteComponent.h"
#include <algorithm>

namespace PillarEditor {

    void LayerEditorPanel::SetScene(std::shared_ptr<Pillar::Scene> scene)
    {
        m_Scene = scene;
        
        // Refresh all sprites to ensure ZIndex matches their layer
        if (m_Scene)
        {
            RefreshAllSprites();
        }
    }

    void LayerEditorPanel::OnImGuiRender()
    {
        ImGui::Begin("Layer Editor");

        DrawLayerList();
        ImGui::Separator();
        DrawLayerProperties();
        DrawAddLayerDialog();

        ImGui::End();
    }

    void LayerEditorPanel::DrawLayerList()
    {
        auto& layerMgr = LayerManager::Get();
        auto& layers = layerMgr.GetAllLayers();

        // Toolbar
        if (ImGui::Button("➕ Add Layer"))
        {
            m_ShowAddLayerDialog = true;
            ImGui::OpenPopup("Add New Layer");
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Create a new sprite layer");

        ImGui::SameLine();
        if (ImGui::Button("📁 Import..."))
        {
            // TODO: Import layers from another project (future feature)
            ConsolePanel::Log("Layer import not yet implemented", LogLevel::Warn);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Import layers from another project");

        ImGui::SameLine();
        if (ImGui::Button("💾 Save"))
        {
            EditorSettings::Get().Save();
            ConsolePanel::Log("Layer settings saved", LogLevel::Info);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Save layer settings to EditorSettings.json");

        ImGui::SameLine();
        if (ImGui::Button("🔄 Refresh Sprites"))
        {
            RefreshAllSprites();
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Update all sprites to match current layer settings\n(fixes Z-Index and visibility)");

        ImGui::Separator();
        ImGui::TextWrapped("💡 Tip: Use layers to organize sprite draw order. Select a layer in the Inspector's 'Layer' dropdown.");
        ImGui::Separator();

        // Layer list (scrollable)
        ImGui::BeginChild("LayerListScroll", ImVec2(0, 300), true);
        
        for (size_t i = 0; i < layers.size(); ++i)
        {
            auto& layer = layers[i];
            
            ImGui::PushID((int)i);

            // Color indicator (small square)
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(layer.color.r, layer.color.g, layer.color.b, layer.color.a));
            ImGui::Button("##ColorBox", ImVec2(16, 16));
            ImGui::PopStyleColor();
            
            ImGui::SameLine();
            
            // Visibility toggle
            bool visible = layer.visible;
            if (ImGui::Checkbox("##Visible", &visible))
            {
                layer.visible = visible;
                UpdateSpritesInLayer(layer.name, visible);
                ConsolePanel::Log(layer.name + (visible ? " shown" : " hidden"), LogLevel::Info);
                // Auto-save settings to persist layer visibility
                EditorSettings::Get().Save();
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle visibility");
            
            ImGui::SameLine();

            // Lock toggle
            bool locked = layer.locked;
            if (ImGui::Checkbox("##Locked", &locked))
            {
                layer.locked = locked;
                ConsolePanel::Log(layer.name + (locked ? " locked" : " unlocked"), LogLevel::Info);
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle locked");
            
            ImGui::SameLine();

            // Layer name (selectable)
            bool selected = (m_SelectedLayer == layer.name);
            
            std::string displayName = layer.name + " (" + std::to_string((int)layer.baseZIndex) + ")";
            if (ImGui::Selectable(displayName.c_str(), selected, ImGuiSelectableFlags_None))
            {
                m_SelectedLayer = layer.name;
            }

            // Context menu
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Rename"))
                {
                    ConsolePanel::Log("Layer rename not yet implemented", LogLevel::Warn);
                }
                if (ImGui::MenuItem("Delete", nullptr, false, layer.name != "Default"))
                {
                    layerMgr.RemoveLayer(layer.name);
                    ConsolePanel::Log("Deleted layer: " + layer.name, LogLevel::Info);
                    if (m_SelectedLayer == layer.name)
                    {
                        m_SelectedLayer = "Default";
                    }
                }
                if (ImGui::MenuItem("Move Up", nullptr, false, i > 0))
                {
                    layerMgr.MoveLayer(i, i - 1);
                }
                if (ImGui::MenuItem("Move Down", nullptr, false, i < layers.size() - 1))
                {
                    layerMgr.MoveLayer(i, i + 1);
                }
                ImGui::EndPopup();
            }

            // Drag-and-drop reordering
            if (ImGui::BeginDragDropSource())
            {
                ImGui::SetDragDropPayload("LAYER_INDEX", &i, sizeof(size_t));
                ImGui::Text("📄 %s", layer.name.c_str());
                ImGui::EndDragDropSource();
            }

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("LAYER_INDEX"))
                {
                    size_t srcIndex = *(size_t*)payload->Data;
                    layerMgr.MoveLayer(srcIndex, i);
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::PopID();
        }

        ImGui::EndChild();
    }

    void LayerEditorPanel::DrawLayerProperties()
    {
        auto& layerMgr = LayerManager::Get();
        auto* layer = layerMgr.GetLayer(m_SelectedLayer);

        if (!layer)
        {
            ImGui::Spacing();
            ImGui::TextDisabled("📝 Select a layer from the list above to edit its properties");
            ImGui::Spacing();
            ImGui::TextWrapped("Layers control sprite drawing order. Higher Z-Index = drawn on top.");
            return;
        }

        ImGui::Spacing();
        ImGui::Text("🎨 Selected Layer Properties");
        ImGui::Separator();

        // Layer name (read-only for now)
        ImGui::Text("Name:");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", layer->name.c_str());

        // Base Z-Index
        ImGui::Spacing();
        ImGui::Text("Base Z-Index:");
        if (ImGui::DragFloat("##BaseZIndex", &layer->baseZIndex, 1.0f, -200.0f, 200.0f, "%.1f"))
        {
            ConsolePanel::Log("Updated " + layer->name + " base Z-index to " + std::to_string(layer->baseZIndex), LogLevel::Info);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Base Z-index for this layer\nSprites use: baseZ + (orderInLayer * 0.01)\nHigher values draw on top");

        // Color picker
        ImGui::Spacing();
        ImGui::Text("Editor Color:");
        ImGui::ColorEdit4("##LayerColor", &layer->color.r, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
        ImGui::SameLine();
        ImGui::TextDisabled("(visual indicator)");

        // Visibility and lock
        ImGui::Spacing();
        ImGui::Checkbox("Visible", &layer->visible);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Show/hide sprites on this layer (not yet implemented in renderer)");
        
        ImGui::SameLine();
        ImGui::Checkbox("Locked", &layer->locked);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Prevent editing sprites on this layer (not yet implemented)");
    }

    void LayerEditorPanel::DrawAddLayerDialog()
    {
        if (!m_ShowAddLayerDialog)
            return;

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(350, 150), ImGuiCond_Appearing);

        if (ImGui::BeginPopupModal("Add New Layer", &m_ShowAddLayerDialog))
        {
            ImGui::Text("Create a new sprite layer");
            ImGui::Separator();

            ImGui::Text("Name:");
            ImGui::SetNextItemWidth(-1);
            ImGui::InputText("##NewLayerName", m_NewLayerName, sizeof(m_NewLayerName));

            ImGui::Text("Base Z-Index:");
            ImGui::SetNextItemWidth(-1);
            ImGui::DragFloat("##NewLayerZIndex", &m_NewLayerZIndex, 1.0f, -200.0f, 200.0f, "%.1f");

            ImGui::Separator();

            if (ImGui::Button("Create", ImVec2(120, 0)))
            {
                std::string layerName(m_NewLayerName);
                
                if (layerName.empty())
                {
                    ConsolePanel::Log("Layer name cannot be empty", LogLevel::Error);
                }
                else if (LayerManager::Get().HasLayer(layerName))
                {
                    ConsolePanel::Log("Layer already exists: " + layerName, LogLevel::Error);
                }
                else
                {
                    LayerManager::Get().AddLayer(layerName, m_NewLayerZIndex);
                    ConsolePanel::Log("Created layer: " + layerName, LogLevel::Info);
                    m_SelectedLayer = layerName;
                    
                    // Reset dialog
                    std::strcpy(m_NewLayerName, "New Layer");
                    m_NewLayerZIndex = 0.0f;
                    m_ShowAddLayerDialog = false;
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                m_ShowAddLayerDialog = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    void LayerEditorPanel::UpdateSpritesInLayer(const std::string& layerName, bool visible)
    {
        if (!m_Scene)
            return;

        // Update all sprites on this layer
        auto view = m_Scene->GetRegistry().view<Pillar::SpriteComponent>();
        int count = 0;
        for (auto entity : view)
        {
            auto& sprite = view.get<Pillar::SpriteComponent>(entity);
            if (sprite.Layer == layerName)
            {
                sprite.Visible = visible;
                count++;
            }
        }

        if (count > 0)
        {
            ConsolePanel::Log("Updated " + std::to_string(count) + " sprites in layer: " + layerName, LogLevel::Info);
        }
    }

    void LayerEditorPanel::RefreshAllSprites()
    {
        if (!m_Scene)
            return;

        auto& layerMgr = LayerManager::Get();
        auto view = m_Scene->GetRegistry().view<Pillar::SpriteComponent>();
        int count = 0;
        int visibilityUpdates = 0;
        
        for (auto entity : view)
        {
            auto& sprite = view.get<Pillar::SpriteComponent>(entity);
            
            // Compute correct ZIndex from layer
            auto* layer = layerMgr.GetLayer(sprite.Layer);
            if (layer)
            {
                float newZIndex = layer->baseZIndex + (sprite.OrderInLayer * 0.01f);
                if (sprite.ZIndex != newZIndex)
                {
                    PIL_CORE_INFO("Sprite layer '{}' updated ZIndex from {} to {} (baseZ={}, order={})", 
                                  sprite.Layer, sprite.ZIndex, newZIndex, layer->baseZIndex, sprite.OrderInLayer);
                    sprite.ZIndex = newZIndex;
                    count++;
                }
                
                // Update visibility based on layer
                bool oldVisible = sprite.Visible;
                sprite.Visible = layer->visible;
                if (oldVisible != sprite.Visible)
                {
                    visibilityUpdates++;
                }
            }
            else
            {
                // Layer doesn't exist, default to "Default" layer
                PIL_CORE_WARN("Sprite has invalid layer '{}', resetting to Default", sprite.Layer);
                sprite.Layer = "Default";
                auto* defaultLayer = layerMgr.GetLayer("Default");
                if (defaultLayer)
                {
                    sprite.ZIndex = defaultLayer->baseZIndex + (sprite.OrderInLayer * 0.01f);
                    sprite.Visible = defaultLayer->visible;
                }
            }
        }

        ConsolePanel::Log("Refreshed " + std::to_string(count) + " sprite Z-indices, " + 
                         std::to_string(visibilityUpdates) + " visibility changes", LogLevel::Info);
    }

} // namespace PillarEditor
