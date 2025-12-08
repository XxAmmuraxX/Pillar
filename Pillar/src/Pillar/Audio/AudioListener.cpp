#include "AudioListener.h"
#include "AudioEngine.h"

namespace Pillar {

    AudioListener::AudioListener()
        : m_Position(0.0f, 0.0f, 0.0f),
          m_Velocity(0.0f, 0.0f, 0.0f),
          m_Forward(0.0f, 0.0f, -1.0f),
          m_Up(0.0f, 1.0f, 0.0f)
    {
    }
    
    void AudioListener::SetPosition(const glm::vec3& position)
    {
        m_Position = position;
        AudioEngine::SetListenerPosition(position);
    }
    
    glm::vec3 AudioListener::GetPosition() const
    {
        return AudioEngine::GetListenerPosition();
    }
    
    void AudioListener::SetVelocity(const glm::vec3& velocity)
    {
        m_Velocity = velocity;
        AudioEngine::SetListenerVelocity(velocity);
    }
    
    void AudioListener::SetOrientation(const glm::vec3& forward, const glm::vec3& up)
    {
        m_Forward = forward;
        m_Up = up;
        AudioEngine::SetListenerOrientation(forward, up);
    }
    
    void AudioListener::UpdateFromCamera(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up)
    {
        SetPosition(position);
        SetOrientation(forward, up);
    }

} // namespace Pillar
