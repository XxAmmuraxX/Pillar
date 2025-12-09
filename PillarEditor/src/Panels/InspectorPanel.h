#pragma once

#include "EditorPanel.h"
#include "Pillar/ECS/Entity.h"

namespace PillarEditor {

    class InspectorPanel : public EditorPanel
    {
    public:
        InspectorPanel();

        virtual void OnImGuiRender() override;

    private:
        void DrawComponents(Pillar::Entity entity);
        void DrawAddComponentButton(Pillar::Entity entity);

        // Component-specific UI drawers
        void DrawTagComponent(Pillar::Entity entity);
        void DrawTransformComponent(Pillar::Entity entity);
        void DrawVelocityComponent(Pillar::Entity entity);
        void DrawRigidbodyComponent(Pillar::Entity entity);
        void DrawColliderComponent(Pillar::Entity entity);

        // Helper for drawing component headers with removal option
        template<typename T>
        bool DrawComponentHeader(const char* label, Pillar::Entity entity, bool canRemove = true);
    };

}
