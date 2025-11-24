#pragma once

#include "Pillar/Core.h"
#include "Pillar/Renderer/OrthographicCamera.h"
#include "Pillar/Events/Event.h"
#include "Pillar/Events/MouseEvent.h"
#include "Pillar/Events/ApplicationEvent.h"

namespace Pillar {

    /**
     * @brief Controller for OrthographicCamera that handles input and automatic updates
     * 
     * Provides WASD movement, optional Q/E rotation, and mouse wheel zoom.
     * Automatically updates camera projection on window resize events.
     */
    class PIL_API OrthographicCameraController
    {
    public:
        /**
         * @brief Construct a new OrthographicCameraController
         * 
         * @param aspectRatio The aspect ratio of the viewport (width/height)
         * @param rotation Enable camera rotation with Q/E keys (default: false)
         */
        OrthographicCameraController(float aspectRatio, bool rotation = false);

        /**
         * @brief Update camera based on input (call every frame)
         * 
         * @param deltaTime Time since last frame in seconds
         */
        void OnUpdate(float deltaTime);

        /**
         * @brief Handle input events (mouse scroll, window resize)
         * 
         * @param e The event to process
         */
        void OnEvent(Event& e);

        /**
         * @brief Get the camera instance (for rendering)
         */
        OrthographicCamera& GetCamera() { return m_Camera; }
        
        /**
         * @brief Get the camera instance (const)
         */
        const OrthographicCamera& GetCamera() const { return m_Camera; }

        /**
         * @brief Get the current zoom level (1.0 = normal)
         */
        float GetZoomLevel() const { return m_ZoomLevel; }
        
        /**
         * @brief Set the zoom level (1.0 = normal, 2.0 = zoomed out 2x)
         * 
         * @param level The new zoom level (clamped to 0.25 - 10.0)
         */
        void SetZoomLevel(float level);

        /**
         * @brief Set the camera translation speed
         * 
         * @param speed Movement speed in units per second
         */
        void SetTranslationSpeed(float speed) { m_CameraTranslationSpeed = speed; }
        
        /**
         * @brief Get the camera translation speed
         */
        float GetTranslationSpeed() const { return m_CameraTranslationSpeed; }

        /**
         * @brief Set the camera rotation speed
         * 
         * @param speed Rotation speed in degrees per second
         */
        void SetRotationSpeed(float speed) { m_CameraRotationSpeed = speed; }
        
        /**
         * @brief Get the camera rotation speed
         */
        float GetRotationSpeed() const { return m_CameraRotationSpeed; }

        /**
         * @brief Set the zoom sensitivity
         * 
         * @param sensitivity Zoom change per mouse wheel tick
         */
        void SetZoomSpeed(float sensitivity) { m_ZoomSpeed = sensitivity; }
        
        /**
         * @brief Get the zoom sensitivity
         */
        float GetZoomSpeed() const { return m_ZoomSpeed; }

    private:
        bool OnMouseScrolled(MouseScrolledEvent& e);
        bool OnWindowResized(WindowResizeEvent& e);

    private:
        float m_AspectRatio;
        float m_ZoomLevel = 1.0f;
        OrthographicCamera m_Camera;

        bool m_Rotation;

        glm::vec3 m_CameraPosition = { 0.0f, 0.0f, 0.0f };
        float m_CameraRotation = 0.0f; // In degrees, counter-clockwise

        float m_CameraTranslationSpeed = 5.0f;
        float m_CameraRotationSpeed = 180.0f;
        float m_ZoomSpeed = 0.25f;
    };

}
