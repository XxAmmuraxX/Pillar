#include "Pillar/Renderer/OrthographicCameraController.h"
#include "Pillar/Input.h"
#include "Pillar/KeyCodes.h"
#include "Pillar/Logger.h"

namespace Pillar {

    OrthographicCameraController::OrthographicCameraController(float aspectRatio, bool rotation)
        : m_AspectRatio(aspectRatio),
          m_Camera(-m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel),
          m_Rotation(rotation)
    {
    }

    void OrthographicCameraController::OnUpdate(float deltaTime)
    {
        // Adjust translation speed based on zoom level (zoom out = move faster)
        // Use base speed as the multiplier so user changes persist
        m_CameraTranslationSpeed = m_BaseTranslationSpeed * m_ZoomLevel;

        // Movement input (WASD)
        if (Input::IsKeyPressed(PIL_KEY_A))
        {
            m_CameraPosition.x -= m_CameraTranslationSpeed * deltaTime;
        }
        else if (Input::IsKeyPressed(PIL_KEY_D))
        {
            m_CameraPosition.x += m_CameraTranslationSpeed * deltaTime;
        }

        if (Input::IsKeyPressed(PIL_KEY_W))
        {
            m_CameraPosition.y += m_CameraTranslationSpeed * deltaTime;
        }
        else if (Input::IsKeyPressed(PIL_KEY_S))
        {
            m_CameraPosition.y -= m_CameraTranslationSpeed * deltaTime;
        }

        // Rotation input (Q/E) - only if rotation enabled
        if (m_Rotation)
        {
            if (Input::IsKeyPressed(PIL_KEY_Q))
            {
                m_CameraRotation += m_CameraRotationSpeed * deltaTime;
            }
            else if (Input::IsKeyPressed(PIL_KEY_E))
            {
                m_CameraRotation -= m_CameraRotationSpeed * deltaTime;
            }
        }

        // Update camera transform
        m_Camera.SetPosition(m_CameraPosition);
        m_Camera.SetRotation(m_CameraRotation);
    }

    void OrthographicCameraController::OnEvent(Event& e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<MouseScrolledEvent>(std::bind(&OrthographicCameraController::OnMouseScrolled, this, std::placeholders::_1));
        dispatcher.Dispatch<WindowResizeEvent>(std::bind(&OrthographicCameraController::OnWindowResized, this, std::placeholders::_1));
    }

    bool OrthographicCameraController::OnMouseScrolled(MouseScrolledEvent& e)
    {
        m_ZoomLevel -= e.GetYOffset() * m_ZoomSpeed;
        m_ZoomLevel = std::max(m_ZoomLevel, 0.25f); // Min zoom
        m_ZoomLevel = std::min(m_ZoomLevel, 10.0f);  // Max zoom
        
        // Recalculate projection matrix with new zoom level
        m_Camera = OrthographicCamera(-m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel);
        
        // Restore camera position and rotation after projection change
        m_Camera.SetPosition(m_CameraPosition);
        m_Camera.SetRotation(m_CameraRotation);
        
        return false; // Don't consume event (allow other systems to react)
    }

    bool OrthographicCameraController::OnWindowResized(WindowResizeEvent& e)
    {
        m_AspectRatio = (float)e.GetWidth() / (float)e.GetHeight();
        
        // Recalculate projection matrix with new aspect ratio
        m_Camera = OrthographicCamera(-m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel);
        
        // Restore camera position and rotation after projection change
        m_Camera.SetPosition(m_CameraPosition);
        m_Camera.SetRotation(m_CameraRotation);
        
        return false; // Don't consume event (renderer also needs to update viewport)
    }

    void OrthographicCameraController::SetZoomLevel(float level)
    {
        m_ZoomLevel = std::max(level, 0.25f); // Min zoom
        m_ZoomLevel = std::min(m_ZoomLevel, 10.0f);  // Max zoom
        
        // Recalculate projection matrix
        m_Camera = OrthographicCamera(-m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel);
        
        // Restore camera position and rotation
        m_Camera.SetPosition(m_CameraPosition);
        m_Camera.SetRotation(m_CameraRotation);
    }

}
