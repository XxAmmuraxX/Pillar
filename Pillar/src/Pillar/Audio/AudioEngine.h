#pragma once

#include "Pillar/Core.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

namespace Pillar {

    class AudioBuffer;
    class AudioSource;

    /**
     * @brief Static audio engine API for managing audio playback.
     * 
     * Provides factory methods for creating audio resources and
     * global audio controls like master volume and listener position.
     */
    class PIL_API AudioEngine
    {
    public:
        /**
         * @brief Initialize the audio engine.
         * Must be called before using any audio functionality.
         */
        static void Init();

        /**
         * @brief Shutdown the audio engine.
         * Releases all audio resources and closes the audio device.
         */
        static void Shutdown();

        /**
         * @brief Check if the audio engine is initialized.
         * @return true if initialized, false otherwise.
         */
        static bool IsInitialized();

        /**
         * @brief Create an audio buffer from a WAV file.
         * @param filepath Path to the audio file (relative to assets/audio/ or absolute).
         * @return Shared pointer to the created audio buffer, or nullptr on failure.
         */
        static std::shared_ptr<AudioBuffer> CreateBuffer(const std::string& filepath);

        /**
         * @brief Create an audio source for playback.
         * @return Shared pointer to the created audio source.
         */
        static std::shared_ptr<AudioSource> CreateSource();

        /**
         * @brief Set the master volume for all audio.
         * @param volume Volume level (0.0 = silent, 1.0 = full volume).
         */
        static void SetMasterVolume(float volume);

        /**
         * @brief Get the current master volume.
         * @return Current master volume level.
         */
        static float GetMasterVolume();

        /**
         * @brief Set the listener's position in 3D space.
         * @param position The position vector.
         */
        static void SetListenerPosition(const glm::vec3& position);

        /**
         * @brief Get the listener's current position.
         * @return The position vector.
         */
        static glm::vec3 GetListenerPosition();

        /**
         * @brief Set the listener's velocity for Doppler effect.
         * @param velocity The velocity vector.
         */
        static void SetListenerVelocity(const glm::vec3& velocity);

        /**
         * @brief Set the listener's orientation.
         * @param forward The forward direction vector (where the listener is facing).
         * @param up The up direction vector.
         */
        static void SetListenerOrientation(const glm::vec3& forward, const glm::vec3& up);

        /**
         * @brief Stop all currently playing audio sources.
         */
        static void StopAllSounds();

        /**
         * @brief Pause all currently playing audio sources.
         */
        static void PauseAllSounds();

        /**
         * @brief Resume all paused audio sources.
         */
        static void ResumeAllSounds();

    private:
        static std::vector<std::weak_ptr<AudioSource>> s_ActiveSources;
        static void RegisterSource(const std::shared_ptr<AudioSource>& source);
        friend class AudioSource;
    };

}
