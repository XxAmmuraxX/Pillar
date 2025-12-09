#pragma once

#include "EditorPanel.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include <string>

namespace PillarEditor {

    class SceneHierarchyPanel : public EditorPanel
    {
    public:
        SceneHierarchyPanel();

        virtual void OnImGuiRender() override;

    private:
        void DrawEntityNode(Pillar::Entity entity);
        void DrawEntityContextMenu(Pillar::Entity entity);
        void DrawCreateEntityMenu();
        const char* GetEntityIcon(const std::string& tag);
    };

}
