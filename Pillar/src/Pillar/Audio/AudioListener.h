#pragma once

#include "Pillar/Core.h"
#include <glm/glm.hpp>

namespace Pillar {

    /**
     * @brief Standalone audio listener utility class
     * 
     * Provides a convenient wrapper around AudioEngine listener functions.
     * This is optional - you can use AudioEngine static methods directly
     * or use AudioListenerComponent with AudioSystem in ECS.
     */
    class PIL_API AudioListener
    {
    public:
        AudioListener();
        ~AudioListener() = default;
        
        // Position
        void SetPosition(const glm::vec3& position);
        glm::vec3 GetPosition() const;
        
        // Velocity (for doppler effect)
        void SetVelocity(const glm::vec3& velocity);
        glm::vec3 GetVelocity() const { return m_Velocity; }
        
        // Orientation
        void SetOrientation(const glm::vec3& forward, const glm::vec3& up);
        glm::vec3 GetForward() const { return m_Forward; }
        glm::vec3 GetUp() const { return m_Up; }
        
        // Update from camera-like object
        void UpdateFromCamera(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up);
        
    private:
        glm::vec3 m_Position;
        glm::vec3 m_Velocity;
        glm::vec3 m_Forward;
        glm::vec3 m_Up;
    };

} // namespace Pillar
