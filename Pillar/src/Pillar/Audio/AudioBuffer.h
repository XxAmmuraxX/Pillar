#pragma once

#include "Pillar/Core.h"
#include <memory>
#include <string>
#include <cstdint>

namespace Pillar {

    /**
     * @brief Abstract audio buffer class for storing audio data.
     * 
     * Audio buffers hold decoded audio data that can be attached to
     * audio sources for playback. Buffers can be shared between multiple sources.
     */
    class PIL_API AudioBuffer
    {
    public:
        virtual ~AudioBuffer() = default;

        /**
         * @brief Get the internal buffer ID (platform-specific).
         * @return The buffer identifier.
         */
        virtual uint32_t GetBufferID() const = 0;

        /**
         * @brief Get the duration of the audio in seconds.
         * @return Duration in seconds.
         */
        virtual float GetDuration() const = 0;

        /**
         * @brief Get the sample rate of the audio.
         * @return Sample rate in Hz.
         */
        virtual int GetSampleRate() const = 0;

        /**
         * @brief Get the number of audio channels.
         * @return Number of channels (1 = mono, 2 = stereo).
         */
        virtual int GetChannels() const = 0;

        /**
         * @brief Get the number of bits per sample.
         * @return Bits per sample (8 or 16 typically).
         */
        virtual int GetBitsPerSample() const = 0;

        /**
         * @brief Check if the buffer is loaded and valid.
         * @return true if loaded, false otherwise.
         */
        virtual bool IsLoaded() const = 0;

        /**
         * @brief Get the file path this buffer was loaded from.
         * @return The file path string.
         */
        virtual const std::string& GetFilePath() const = 0;

        /**
         * @brief Create an audio buffer from a WAV file.
         * @param filepath Path to the WAV file.
         * @return Shared pointer to the created buffer, or nullptr on failure.
         */
        static std::shared_ptr<AudioBuffer> Create(const std::string& filepath);
    };

}
