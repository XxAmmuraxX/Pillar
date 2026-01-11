#pragma once

#include "EditorPanel.h"
#include "Pillar/ECS/Entity.h"
#include <glm/glm.hpp>

namespace PillarEditor {

    class EditorLayer;

    class InspectorPanel : public EditorPanel
    {
    public:
        InspectorPanel(EditorLayer* editorLayer = nullptr);

        virtual void OnImGuiRender() override;

    private:
        void DrawComponents(Pillar::Entity entity);
        void DrawAddComponentButton(Pillar::Entity entity);

        // Component-specific UI drawers
        void DrawTagComponent(Pillar::Entity entity);
        void DrawTransformComponent(Pillar::Entity entity);
        void DrawSpriteComponent(Pillar::Entity entity);
        void DrawCameraComponent(Pillar::Entity entity);
        void DrawAnimationComponent(Pillar::Entity entity);
        void DrawLight2DComponent(Pillar::Entity entity);
        void DrawShadowCaster2DComponent(Pillar::Entity entity);
        void DrawVelocityComponent(Pillar::Entity entity);
        void DrawRigidbodyComponent(Pillar::Entity entity);
        void DrawColliderComponent(Pillar::Entity entity);
        void DrawBulletComponent(Pillar::Entity entity);
        void DrawXPGemComponent(Pillar::Entity entity);
        void DrawHierarchyComponent(Pillar::Entity entity);

        // Helper for drawing component headers with removal option
        template<typename T>
        bool DrawComponentHeader(const char* label, Pillar::Entity entity, bool canRemove = true);

    private:
        EditorLayer* m_EditorLayer = nullptr;
        
        // Track property edits for undo/redo
        bool m_EditingPosition = false;
        bool m_EditingRotation = false;
        bool m_EditingScale = false;
        glm::vec2 m_OldPosition{0.0f};
        float m_OldRotation = 0.0f;
        glm::vec2 m_OldScale{1.0f};
    };

}
