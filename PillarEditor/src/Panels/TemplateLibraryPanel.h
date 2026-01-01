#pragma once

#include "TemplateManager.h"
#include "Pillar/ECS/Scene.h"
#include <memory>

namespace PillarEditor
{
    /**
     * TemplateLibraryPanel - UI panel for browsing and managing entity templates
     */
    class TemplateLibraryPanel
    {
    public:
        TemplateLibraryPanel();
        ~TemplateLibraryPanel() = default;

        void OnImGuiRender();
        
        void SetScene(const std::shared_ptr<Pillar::Scene>& scene) { m_Scene = scene; }
        void SetTemplateManager(TemplateManager* manager) { m_TemplateManager = manager; }

        // Instantiate selected template
        Pillar::Entity InstantiateSelectedTemplate();

    private:
        void DrawTemplateGrid();
        void DrawTemplateCard(const EntityTemplate& templateData, int index);
        void DrawSearchBar();
        void DrawToolbar();

    private:
        std::shared_ptr<Pillar::Scene> m_Scene;
        TemplateManager* m_TemplateManager = nullptr;
        
        // UI state
        char m_SearchBuffer[256] = "";
        int m_SelectedTemplateIndex = -1;
        float m_CardWidth = 150.0f;
    };
}
