#include "EditorCamera.h"
#include "Pillar/Input.h"
#include "Pillar/KeyCodes.h"
#include <algorithm>
#include <cmath>

namespace PillarEditor {

    EditorCamera::EditorCamera()
        : m_Camera(-m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel)
    {
        UpdateProjection();
        UpdateView();
    }

    EditorCamera::EditorCamera(float aspectRatio)
        : m_AspectRatio(aspectRatio),
          m_Camera(-aspectRatio * m_ZoomLevel, aspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel)
    {
        UpdateProjection();
        UpdateView();
    }

    void EditorCamera::OnUpdate(float deltaTime)
    {
        // Get current mouse position
        auto [mouseX, mouseY] = Pillar::Input::GetMousePosition();
        glm::vec2 currentMousePos = { mouseX, mouseY };

        // Middle mouse button panning
        if (Pillar::Input::IsMouseButtonPressed(2)) // Middle mouse button
        {
            if (!m_IsPanning)
            {
                m_IsPanning = true;
                m_LastMousePosition = currentMousePos;
            }
            else
            {
                glm::vec2 delta = currentMousePos - m_LastMousePosition;
                
                // Convert screen delta to world space delta
                // The conversion factor depends on viewport size and zoom level
                float worldUnitsPerPixelX = (2.0f * m_AspectRatio * m_ZoomLevel) / m_ViewportWidth;
                float worldUnitsPerPixelY = (2.0f * m_ZoomLevel) / m_ViewportHeight;

                m_Position.x -= delta.x * worldUnitsPerPixelX * m_PanSpeed;
                m_Position.y += delta.y * worldUnitsPerPixelY * m_PanSpeed;  // Y is inverted
                
                m_LastMousePosition = currentMousePos;
                UpdateView();
            }
        }
        else
        {
            m_IsPanning = false;
        }
    }

    void EditorCamera::OnEvent(Pillar::Event& e)
    {
        Pillar::EventDispatcher dispatcher(e);
        dispatcher.Dispatch<Pillar::MouseScrolledEvent>(
            [this](Pillar::MouseScrolledEvent& event) { return OnMouseScrolled(event); }
        );
    }

    bool EditorCamera::OnMouseScrolled(Pillar::MouseScrolledEvent& e)
    {
        // Zoom speed is proportional to current zoom level for smooth feel
        float zoomFactor = 1.0f - e.GetYOffset() * m_ZoomSpeed;
        zoomFactor = std::clamp(zoomFactor, 0.8f, 1.2f);  // Clamp to prevent extreme jumps
        
        float newZoom = m_ZoomLevel * zoomFactor;
        m_ZoomLevel = std::clamp(newZoom, m_MinZoom, m_MaxZoom);
        
        UpdateProjection();
        return true;  // Consume the event so it doesn't propagate
    }

    void EditorCamera::SetViewportSize(float width, float height)
    {
        if (width <= 0 || height <= 0)
            return;

        m_ViewportWidth = width;
        m_ViewportHeight = height;
        m_AspectRatio = width / height;
        UpdateProjection();
    }

    void EditorCamera::FocusOnPosition(const glm::vec2& position)
    {
        m_Position.x = position.x;
        m_Position.y = position.y;
        UpdateView();
    }

    void EditorCamera::SetZoomLevel(float level)
    {
        m_ZoomLevel = std::clamp(level, m_MinZoom, m_MaxZoom);
        UpdateProjection();
    }

    void EditorCamera::SetPosition(const glm::vec3& position)
    {
        m_Position = position;
        UpdateView();
    }

    void EditorCamera::UpdateProjection()
    {
        float left = -m_AspectRatio * m_ZoomLevel;
        float right = m_AspectRatio * m_ZoomLevel;
        float bottom = -m_ZoomLevel;
        float top = m_ZoomLevel;
        
        m_Camera.SetProjection(left, right, bottom, top);
    }

    void EditorCamera::UpdateView()
    {
        m_Camera.SetPosition(m_Position);
    }

}
