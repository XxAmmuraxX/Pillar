#pragma once

#include "Pillar/Audio/AudioSource.h"
#include <AL/al.h>

namespace Pillar {

    /**
     * @brief OpenAL implementation of AudioSource.
     */
    class OpenALSource : public AudioSource
    {
    public:
        /**
         * @brief Create a new OpenAL source.
         */
        OpenALSource();
        
        /**
         * @brief Destructor - releases OpenAL source.
         */
        ~OpenALSource();

        // Buffer management
        void SetBuffer(const std::shared_ptr<AudioBuffer>& buffer) override;
        std::shared_ptr<AudioBuffer> GetBuffer() const override { return m_Buffer; }

        // Playback control
        void Play() override;
        void Pause() override;
        void Stop() override;
        void Rewind() override;

        // State queries
        AudioState GetState() const override;
        bool IsPlaying() const override;
        bool IsPaused() const override;
        bool IsStopped() const override;

        // Audio properties
        void SetVolume(float volume) override;
        float GetVolume() const override { return m_Volume; }
        void SetPitch(float pitch) override;
        float GetPitch() const override { return m_Pitch; }
        void SetLooping(bool loop) override;
        bool IsLooping() const override { return m_Looping; }

        // 3D spatial audio
        void SetPosition(const glm::vec3& position) override;
        glm::vec3 GetPosition() const override { return m_Position; }
        void SetVelocity(const glm::vec3& velocity) override;
        void SetDirection(const glm::vec3& direction) override;

        // Attenuation
        void SetMinDistance(float distance) override;
        void SetMaxDistance(float distance) override;
        void SetRolloffFactor(float factor) override;

        // Playback position
        void SetPlaybackPosition(float seconds) override;
        float GetPlaybackPosition() const override;

        // Internal
        uint32_t GetSourceID() const override { return m_SourceID; }

    private:
        ALuint m_SourceID = 0;
        std::shared_ptr<AudioBuffer> m_Buffer;
        
        // Cached properties
        float m_Volume = 1.0f;
        float m_Pitch = 1.0f;
        bool m_Looping = false;
        glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };
        glm::vec3 m_Velocity = { 0.0f, 0.0f, 0.0f };
        glm::vec3 m_Direction = { 0.0f, 0.0f, 0.0f };
        float m_MinDistance = 1.0f;
        float m_MaxDistance = 1000.0f;
        float m_RolloffFactor = 1.0f;
    };

}
