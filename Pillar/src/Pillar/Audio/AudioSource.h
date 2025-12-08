#pragma once

#include "Pillar/Core.h"
#include "Pillar/Audio/AudioBuffer.h"
#include <glm/glm.hpp>
#include <memory>
#include <cstdint>

namespace Pillar {

    /**
     * @brief Enumeration of audio playback states.
     */
    enum class AudioState
    {
        Stopped,
        Playing,
        Paused
    };

    /**
     * @brief Abstract audio source class for playing audio buffers.
     * 
     * Audio sources control playback of audio buffers, including
     * volume, pitch, looping, and 3D spatial positioning.
     */
    class PIL_API AudioSource
    {
    public:
        virtual ~AudioSource() = default;

        // ==================== Buffer Management ====================

        /**
         * @brief Set the audio buffer to play.
         * @param buffer The audio buffer to attach to this source.
         */
        virtual void SetBuffer(const std::shared_ptr<AudioBuffer>& buffer) = 0;

        /**
         * @brief Get the currently attached audio buffer.
         * @return The attached buffer, or nullptr if none.
         */
        virtual std::shared_ptr<AudioBuffer> GetBuffer() const = 0;

        // ==================== Playback Control ====================

        /**
         * @brief Start or resume playback.
         */
        virtual void Play() = 0;

        /**
         * @brief Pause playback (can be resumed).
         */
        virtual void Pause() = 0;

        /**
         * @brief Stop playback and rewind to beginning.
         */
        virtual void Stop() = 0;

        /**
         * @brief Rewind to the beginning without stopping.
         */
        virtual void Rewind() = 0;

        // ==================== State Queries ====================

        /**
         * @brief Get the current playback state.
         * @return The current AudioState.
         */
        virtual AudioState GetState() const = 0;

        /**
         * @brief Check if the source is currently playing.
         */
        virtual bool IsPlaying() const = 0;

        /**
         * @brief Check if the source is currently paused.
         */
        virtual bool IsPaused() const = 0;

        /**
         * @brief Check if the source is stopped.
         */
        virtual bool IsStopped() const = 0;

        // ==================== Audio Properties ====================

        /**
         * @brief Set the volume (gain) of the source.
         * @param volume Volume level (0.0 = silent, 1.0 = full volume).
         */
        virtual void SetVolume(float volume) = 0;

        /**
         * @brief Get the current volume.
         * @return Volume level (0.0 to 1.0).
         */
        virtual float GetVolume() const = 0;

        /**
         * @brief Set the pitch (playback speed multiplier).
         * @param pitch Pitch value (0.5 = half speed, 2.0 = double speed).
         */
        virtual void SetPitch(float pitch) = 0;

        /**
         * @brief Get the current pitch.
         * @return Pitch value.
         */
        virtual float GetPitch() const = 0;

        /**
         * @brief Enable or disable looping.
         * @param loop true to loop, false for one-shot playback.
         */
        virtual void SetLooping(bool loop) = 0;

        /**
         * @brief Check if looping is enabled.
         * @return true if looping, false otherwise.
         */
        virtual bool IsLooping() const = 0;

        // ==================== 3D Spatial Audio ====================

        /**
         * @brief Set the position of the source in 3D space.
         * @param position The position vector.
         */
        virtual void SetPosition(const glm::vec3& position) = 0;

        /**
         * @brief Get the source's position.
         * @return The position vector.
         */
        virtual glm::vec3 GetPosition() const = 0;

        /**
         * @brief Set the velocity for Doppler effect calculations.
         * @param velocity The velocity vector.
         */
        virtual void SetVelocity(const glm::vec3& velocity) = 0;

        /**
         * @brief Set the direction the source is facing (for cone effects).
         * @param direction The direction vector.
         */
        virtual void SetDirection(const glm::vec3& direction) = 0;

        // ==================== Attenuation ====================

        /**
         * @brief Set the reference distance for attenuation.
         * At this distance, volume is at its maximum.
         * @param distance The reference distance.
         */
        virtual void SetMinDistance(float distance) = 0;

        /**
         * @brief Set the maximum distance for attenuation.
         * Beyond this distance, volume does not decrease further.
         * @param distance The maximum distance.
         */
        virtual void SetMaxDistance(float distance) = 0;

        /**
         * @brief Set the rolloff factor for distance attenuation.
         * Higher values = faster falloff.
         * @param factor The rolloff factor.
         */
        virtual void SetRolloffFactor(float factor) = 0;

        // ==================== Playback Position ====================

        /**
         * @brief Set the playback position in seconds.
         * @param seconds Time offset from the beginning.
         */
        virtual void SetPlaybackPosition(float seconds) = 0;

        /**
         * @brief Get the current playback position in seconds.
         * @return Current time offset.
         */
        virtual float GetPlaybackPosition() const = 0;

        // ==================== Internal ====================

        /**
         * @brief Get the internal source ID (platform-specific).
         * @return The source identifier.
         */
        virtual uint32_t GetSourceID() const = 0;

        // ==================== Factory ====================

        /**
         * @brief Create a new audio source.
         * @return Shared pointer to the created source.
         */
        static std::shared_ptr<AudioSource> Create();
    };

}
