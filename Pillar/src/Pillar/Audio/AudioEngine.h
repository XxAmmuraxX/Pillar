#pragma once

#include "Pillar/Core.h"
#include <glm/glm.hpp>
#include <memory>
#include <optional>
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

        enum class AudioBus
        {
            Master = 0,
            SFX,
            Music,
            UI,
            Count
        };

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
         * @brief Simple one-shot playback helper.
         * Creates a transient source, assigns the buffer, and plays immediately.
         * @return The created source, or nullptr if the buffer fails to load or audio is not initialized.
         */
        static std::shared_ptr<AudioSource> PlayOneShot(const std::string& filepath, float volume = 1.0f, float pitch = 1.0f, std::optional<glm::vec3> position = std::nullopt, AudioBus bus = AudioBus::SFX);

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

        // ==================== Bus Controls ====================
        static void SetBusVolume(AudioBus bus, float volume);
        static float GetBusVolume(AudioBus bus);
        static void MuteBus(AudioBus bus);
        static void UnmuteBus(AudioBus bus);
        static bool IsBusMuted(AudioBus bus);

        // ==================== Bus Fades ====================
        static void FadeBusTo(AudioBus bus, float targetVolume, float durationSeconds);
        static void FadeIn(AudioBus bus, float durationSeconds, float targetVolume = 1.0f);
        static void FadeOut(AudioBus bus, float durationSeconds, float targetVolume = 0.0f);
        static void Update(float deltaSeconds);

        // ==================== Source Routing ====================
        static void SetSourceBus(const std::shared_ptr<AudioSource>& source, AudioBus bus);
        static AudioBus GetSourceBus(const std::shared_ptr<AudioSource>& source);
        static void SetSourceVolume(const std::shared_ptr<AudioSource>& source, float volume);
        static float GetSourceUserVolume(const std::shared_ptr<AudioSource>& source);

    private:
        struct BusState
        {
            float Volume = 1.0f;
            bool Muted = false;
            bool Fading = false;
            float FadeStart = 1.0f;
            float FadeTarget = 1.0f;
            float FadeDuration = 0.0f;
            float FadeElapsed = 0.0f;
        };

        struct TrackedSource
        {
            std::weak_ptr<AudioSource> Source;
            AudioBus Bus = AudioBus::SFX;
            float UserVolume = 1.0f;
        };

        static std::vector<TrackedSource> s_TrackedSources;
        static std::vector<BusState> s_BusStates;

        static void RegisterSource(const std::shared_ptr<AudioSource>& source);
        static void CleanupSources();
        static void ApplyAllBusGains();
        static void ApplyGainToSource(TrackedSource& tracked);
        static BusState& GetBusState(AudioBus bus);
        static float GetEffectiveBusVolume(AudioBus bus);
        friend class AudioSource;
    };

}
