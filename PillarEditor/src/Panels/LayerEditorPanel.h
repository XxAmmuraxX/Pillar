#pragma once

#include <imgui.h>
#include <string>
#include <memory>

namespace Pillar {
    class Scene;
}

namespace PillarEditor {

    /**
     * @brief Panel for managing sprite layers (add, remove, rename, reorder, visibility)
     * 
     * Provides a visual interface for managing the project's sprite layer system,
     * allowing users to organize Z-ordering through named layers.
     */
    class LayerEditorPanel
    {
    public:
        LayerEditorPanel() = default;
        ~LayerEditorPanel() = default;

        void OnImGuiRender();
        void SetScene(std::shared_ptr<Pillar::Scene> scene);
        
        // Update all sprites in scene to match their layer settings
        void RefreshAllSprites();

    private:
        void DrawLayerList();
        void DrawLayerProperties();
        void DrawAddLayerDialog();
        void UpdateSpritesInLayer(const std::string& layerName, bool visible);

        std::shared_ptr<Pillar::Scene> m_Scene;
        std::string m_SelectedLayer = "Default";
        bool m_ShowAddLayerDialog = false;
        char m_NewLayerName[64] = "New Layer";
        float m_NewLayerZIndex = 0.0f;
    };

} // namespace PillarEditor
