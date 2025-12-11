#pragma once

#include <glm/glm.hpp>

namespace Pillar {

    /**
     * @brief Component for marking an entity as the audio listener
     * 
     * Typically attached to the camera entity. Only one listener
     * should be active at a time. The AudioSystem will automatically
     * update the OpenAL listener position/orientation based on the
     * entity's TransformComponent.
     */
    struct AudioListenerComponent
    {
        bool IsActive = true;           // Only one listener can be active
        glm::vec3 Forward = { 0.0f, 0.0f, -1.0f };
        glm::vec3 Up = { 0.0f, 1.0f, 0.0f };
        
        AudioListenerComponent() = default;
        
        AudioListenerComponent(const AudioListenerComponent& other) = default;
        AudioListenerComponent& operator=(const AudioListenerComponent& other) = default;
    };

} // namespace Pillar
