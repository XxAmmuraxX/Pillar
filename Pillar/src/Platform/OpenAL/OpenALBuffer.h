#pragma once

#include "Pillar/Audio/AudioBuffer.h"
#include <AL/al.h>

namespace Pillar {

    /**
     * @brief OpenAL implementation of AudioBuffer.
     */
    class OpenALBuffer : public AudioBuffer
    {
    public:
        /**
         * @brief Create an OpenAL buffer from a WAV file.
         * @param filepath Path to the WAV file.
         */
        OpenALBuffer(const std::string& filepath);
        
        /**
         * @brief Destructor - releases OpenAL buffer.
         */
        ~OpenALBuffer();

        // AudioBuffer interface
        uint32_t GetBufferID() const override { return m_BufferID; }
        float GetDuration() const override { return m_Duration; }
        int GetSampleRate() const override { return m_SampleRate; }
        int GetChannels() const override { return m_Channels; }
        int GetBitsPerSample() const override { return m_BitsPerSample; }
        bool IsLoaded() const override { return m_Loaded; }
        const std::string& GetFilePath() const override { return m_FilePath; }

    private:
        /**
         * @brief Load a WAV file into the OpenAL buffer.
         * @param filepath Path to the WAV file.
         * @return true if loading succeeded, false otherwise.
         */
        bool LoadWAV(const std::string& filepath);

        /**
         * @brief Get the OpenAL format enum for the current audio format.
         * @return The ALenum format value.
         */
        ALenum GetALFormat() const;

        ALuint m_BufferID = 0;
        std::string m_FilePath;
        float m_Duration = 0.0f;
        int m_SampleRate = 0;
        int m_Channels = 0;
        int m_BitsPerSample = 0;
        bool m_Loaded = false;
    };

}
