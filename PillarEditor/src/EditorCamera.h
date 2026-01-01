#pragma once

#include "Pillar/Renderer/OrthographicCamera.h"
#include "Pillar/Events/Event.h"
#include "Pillar/Events/MouseEvent.h"
#include <glm/glm.hpp>

namespace PillarEditor {

    class EditorCamera
    {
    public:
        EditorCamera();
        EditorCamera(float aspectRatio);

        void OnUpdate(float deltaTime);
        void OnEvent(Pillar::Event& e);

        void SetViewportSize(float width, float height);
        void FocusOnPosition(const glm::vec2& position);

        const Pillar::OrthographicCamera& GetCamera() const { return m_Camera; }
        Pillar::OrthographicCamera& GetCamera() { return m_Camera; }

        float GetZoomLevel() const { return m_ZoomLevel; }
        void SetZoomLevel(float level);

        const glm::vec3& GetPosition() const { return m_Position; }
        void SetPosition(const glm::vec3& position);

        // Settings
        void SetPanSpeed(float speed) { m_PanSpeed = speed; }
        void SetZoomSpeed(float speed) { m_ZoomSpeed = speed; }
        float GetPanSpeed() const { return m_PanSpeed; }
        float GetZoomSpeed() const { return m_ZoomSpeed; }

    private:
        bool OnMouseScrolled(Pillar::MouseScrolledEvent& e);
        void UpdateProjection();
        void UpdateView();

    private:
        Pillar::OrthographicCamera m_Camera;
        
        glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };
        float m_ZoomLevel = 5.0f;  // Default zoom to see more of the scene
        float m_AspectRatio = 16.0f / 9.0f;

        float m_ViewportWidth = 1280.0f;
        float m_ViewportHeight = 720.0f;

        // Camera controls
        float m_PanSpeed = 1.0f;
        float m_ZoomSpeed = 0.15f;  // Increased for better responsiveness
        float m_MinZoom = 0.5f;
        float m_MaxZoom = 50.0f;

        // Mouse state for panning
        bool m_IsPanning = false;
        glm::vec2 m_LastMousePosition = { 0.0f, 0.0f };
    };

}
