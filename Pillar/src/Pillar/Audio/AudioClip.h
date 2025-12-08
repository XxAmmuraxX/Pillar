#pragma once

#include "Pillar/Core.h"
#include "Pillar/Audio/AudioBuffer.h"
#include "Pillar/Audio/AudioSource.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>

namespace Pillar {

    /**
     * @brief High-level audio clip wrapper combining a buffer and source.
     * 
     * AudioClip provides a convenient interface for simple audio playback,
     * managing both the audio data (buffer) and playback control (source)
     * as a single unit.
     */
    class PIL_API AudioClip
    {
    public:
        /**
         * @brief Create an audio clip from a WAV file.
         * @param filepath Path to the audio file (relative to assets/audio/ or absolute).
         */
        AudioClip(const std::string& filepath);
        
        /**
         * @brief Destructor.
         */
        ~AudioClip();

        // ==================== Simple Playback ====================

        /**
         * @brief Start or resume playback.
         */
        void Play();

        /**
         * @brief Stop playback and rewind to beginning.
         */
        void Stop();

        /**
         * @brief Pause playback (can be resumed with Play()).
         */
        void Pause();

        /**
         * @brief Resume playback from paused state.
         */
        void Resume();

        // ==================== Properties ====================

        /**
         * @brief Set the volume.
         * @param volume Volume level (0.0 = silent, 1.0 = full volume).
         */
        void SetVolume(float volume);

        /**
         * @brief Get the current volume.
         * @return Volume level (0.0 to 1.0).
         */
        float GetVolume() const;

        /**
         * @brief Set the pitch (playback speed multiplier).
         * @param pitch Pitch value (0.5 = half speed, 2.0 = double speed).
         */
        void SetPitch(float pitch);

        /**
         * @brief Get the current pitch.
         * @return Pitch value.
         */
        float GetPitch() const;

        /**
         * @brief Enable or disable looping.
         * @param loop true to loop, false for one-shot playback.
         */
        void SetLooping(bool loop);

        /**
         * @brief Check if looping is enabled.
         * @return true if looping, false otherwise.
         */
        bool IsLooping() const;

        // ==================== 3D Spatial Audio ====================

        /**
         * @brief Set the position of the audio source in 3D space.
         * @param position The position vector.
         */
        void SetPosition(const glm::vec3& position);

        /**
         * @brief Get the source's position.
         * @return The position vector.
         */
        glm::vec3 GetPosition() const;

        // ==================== State ====================

        /**
         * @brief Check if the clip is currently playing.
         * @return true if playing, false otherwise.
         */
        bool IsPlaying() const;

        /**
         * @brief Check if the clip is currently paused.
         * @return true if paused, false otherwise.
         */
        bool IsPaused() const;

        /**
         * @brief Check if the clip is loaded and ready to play.
         * @return true if loaded, false otherwise.
         */
        bool IsLoaded() const;

        /**
         * @brief Get the duration of the audio in seconds.
         * @return Duration in seconds.
         */
        float GetDuration() const;

        /**
         * @brief Get the current playback position in seconds.
         * @return Current time offset.
         */
        float GetPlaybackPosition() const;

        /**
         * @brief Set the playback position in seconds.
         * @param seconds Time offset from the beginning.
         */
        void SetPlaybackPosition(float seconds);

        // ==================== Access Underlying Objects ====================

        /**
         * @brief Get the underlying audio buffer.
         * @return Shared pointer to the buffer.
         */
        std::shared_ptr<AudioBuffer> GetBuffer() const { return m_Buffer; }

        /**
         * @brief Get the underlying audio source.
         * @return Shared pointer to the source.
         */
        std::shared_ptr<AudioSource> GetSource() const { return m_Source; }

        // ==================== Factory ====================

        /**
         * @brief Create an audio clip from a file.
         * @param filepath Path to the audio file.
         * @return Shared pointer to the created clip, or nullptr on failure.
         */
        static std::shared_ptr<AudioClip> Create(const std::string& filepath);

    private:
        std::shared_ptr<AudioBuffer> m_Buffer;
        std::shared_ptr<AudioSource> m_Source;
        std::string m_FilePath;
    };

}
