#pragma once

#include <glm/glm.hpp>
#include <random>
#include <cmath>

namespace Game {

    class CameraShake
    {
    public:
        void Shake(float duration, float intensity)
        {
            // Stack shakes for more impact
            m_Duration = std::max(m_Duration, duration);
            m_Intensity = std::max(m_Intensity, intensity);
            m_Timer = m_Duration;
        }

        void OnUpdate(float dt)
        {
            if (m_Timer <= 0.0f)
            {
                m_Offset = glm::vec2(0.0f);
                m_Intensity = 0.0f;
                return;
            }

            m_Timer -= dt;

            // Decay intensity over time
            float progress = m_Timer / m_Duration;
            float currentIntensity = m_Intensity * progress;

            // Generate random offset with perlin-like smoothing
            std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
            glm::vec2 target(
                dist(m_RandomEngine) * currentIntensity,
                dist(m_RandomEngine) * currentIntensity
            );

            // Smooth interpolation for less jarring motion
            m_Offset = glm::mix(m_Offset, target, 0.5f);
        }

        glm::vec2 GetOffset() const { return m_Offset; }
        bool IsShaking() const { return m_Timer > 0.0f; }

        // Preset shake types
        void ShakeSmall() { Shake(0.1f, 0.1f); }      // Kill enemy
        void ShakeMedium() { Shake(0.2f, 0.2f); }     // Player damaged
        void ShakeLarge() { Shake(0.3f, 0.4f); }      // Big explosion
        void ShakeHuge() { Shake(0.5f, 0.6f); }       // Boss death

    private:
        float m_Duration = 0.0f;
        float m_Intensity = 0.0f;
        float m_Timer = 0.0f;
        glm::vec2 m_Offset = { 0.0f, 0.0f };

        std::mt19937 m_RandomEngine{ std::random_device{}() };
    };

} // namespace Game
