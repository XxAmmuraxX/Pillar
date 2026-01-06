#pragma once

#include "EditorPanel.h"
#include "../EditorCamera.h"
#include "Pillar/Renderer/Framebuffer.h"
#include "Pillar/Application.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>

struct ImVec2;  // Forward declaration

namespace PillarEditor {

    class EditorLayer;  // Forward declaration

    enum class GizmoMode
    {
        None = -1,
        Translate = 0,
        Rotate = 1,
        Scale = 2
    };

    class ViewportPanel : public EditorPanel
    {
    public:
        ViewportPanel(EditorLayer* editorLayer = nullptr);

        virtual void OnImGuiRender() override;
        virtual void OnUpdate(float deltaTime) override;
        virtual void OnEvent(Pillar::Event& e) override;

        void RenderScene();  // Called before OnImGuiRender to render to framebuffer
        void ResetCamera();  // Reset camera to origin

        EditorCamera& GetCamera() { return m_EditorCamera; }
        const EditorCamera& GetCamera() const { return m_EditorCamera; }

        bool IsViewportFocused() const { return m_ViewportFocused; }
        bool IsViewportHovered() const { return m_ViewportHovered; }

        glm::vec2 GetViewportSize() const { return m_ViewportSize; }
        const glm::vec2* GetViewportBounds() const { return m_ViewportBounds; }

        // Gizmo controls
        void SetGizmoMode(GizmoMode mode) { m_GizmoMode = mode; }
        GizmoMode GetGizmoMode() const { return m_GizmoMode; }

    private:
        void DrawGrid();
        void DrawGizmos();
        void DrawGizmoToolbar();
        void DrawEntityLabels();
        void DrawEntityNameLabel(const glm::vec2& worldPos, const std::string& name);
        void DrawColliderGizmos();
        void DrawRigidbodyGizmos();
        void DrawWireBox(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color);
        ImVec2 WorldToScreenImGui(const glm::vec2& worldPos);
        glm::vec4 GetEntityColor(const std::string& tag);
        glm::vec2 GetEntitySize(const std::string& tag, const glm::vec2& scale);
        
        void ApplyNudge(const glm::vec2& nudge);
        
        // Entity picking
        bool OnMouseButtonPressed(Pillar::MouseButtonPressedEvent& e);
        glm::vec2 ScreenToWorld(const glm::vec2& screenPos);
        Pillar::Entity GetEntityAtWorldPosition(const glm::vec2& worldPos);

    private:
        std::shared_ptr<Pillar::Framebuffer> m_Framebuffer;
        EditorCamera m_EditorCamera;
        Pillar::OrthographicCamera m_GameCamera{ -10.0f, 10.0f, -10.0f, 10.0f };  // Camera for play mode

        glm::vec2 m_ViewportSize = { 1280.0f, 720.0f };
        glm::vec2 m_ViewportBounds[2] = {};

        bool m_ViewportFocused = false;
        bool m_ViewportHovered = false;
        
        // Display options
        bool m_ShowEntityLabels = true;
        bool m_ShowColliderGizmos = true;
        bool m_ShowRigidbodyGizmos = true;
        
        // Gizmo state
        GizmoMode m_GizmoMode = GizmoMode::Translate;
        bool m_GizmoInUse = false;
        glm::vec2 m_GizmoStartPosition;
        float m_GizmoStartRotation;
        glm::vec2 m_GizmoStartScale;
        
        EditorLayer* m_EditorLayer = nullptr;
    };

}
