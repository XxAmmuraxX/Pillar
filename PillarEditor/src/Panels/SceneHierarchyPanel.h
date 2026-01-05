#pragma once

#include "EditorPanel.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include <string>

namespace PillarEditor {

    class TemplateManager;

    class SceneHierarchyPanel : public EditorPanel
    {
    public:
        SceneHierarchyPanel();

        virtual void OnImGuiRender() override;

        void SetTemplateManager(TemplateManager* manager) { m_TemplateManager = manager; }

    private:
        void DrawEntityNode(Pillar::Entity entity);
        void DrawEntityContextMenu(Pillar::Entity entity);
        void DrawCreateEntityMenu();
        const char* GetEntityIcon(const std::string& tag);
        void DrawSaveTemplateDialog();

    private:
        TemplateManager* m_TemplateManager = nullptr;
        
        // Search/filter state
        char m_SearchBuffer[256] = "";
        bool m_IsSearching = false;
        
        // Template save state
        Pillar::Entity m_EntityToSaveAsTemplate;
        bool m_ShowSaveTemplateDialog = false;
        char m_TemplateNameBuffer[128] = "";
        char m_TemplateDescBuffer[256] = "";
    };

}
