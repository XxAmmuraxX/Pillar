#include "Platform/OpenAL/OpenALBuffer.h"
#include "Platform/OpenAL/OpenALContext.h"
#include "Pillar/Audio/WavLoader.h"
#include "Pillar/Utils/AssetManager.h"
#include "Pillar/Logger.h"

namespace Pillar {

    OpenALBuffer::OpenALBuffer(const std::string& filepath)
        : m_FilePath(filepath)
    {
        // Generate OpenAL buffer
        alGenBuffers(1, &m_BufferID);
        if (!OpenALContext::CheckError("alGenBuffers"))
        {
            PIL_CORE_ERROR("OpenALBuffer: Failed to generate buffer");
            return;
        }

        // Load the WAV file
        if (!LoadWAV(filepath))
        {
            alDeleteBuffers(1, &m_BufferID);
            m_BufferID = 0;
        }
    }

    OpenALBuffer::~OpenALBuffer()
    {
        if (m_BufferID != 0)
        {
            alDeleteBuffers(1, &m_BufferID);
            OpenALContext::CheckError("alDeleteBuffers");
        }
    }

    bool OpenALBuffer::LoadWAV(const std::string& filepath)
    {
        // Resolve the audio path
        std::string resolvedPath = AssetManager::GetAudioPath(filepath);

        // Load WAV data
        WavData wavData;
        if (!WavLoader::Load(resolvedPath, wavData))
        {
            PIL_CORE_ERROR("OpenALBuffer: Failed to load WAV file: {0}", resolvedPath);
            return false;
        }

        // Store format info
        m_SampleRate = wavData.SampleRate;
        m_Channels = wavData.Channels;
        m_BitsPerSample = wavData.BitsPerSample;
        m_Duration = wavData.Duration;

        // Get OpenAL format
        ALenum format = GetALFormat();
        if (format == 0)
        {
            PIL_CORE_ERROR("OpenALBuffer: Unsupported audio format");
            return false;
        }

        // Upload data to OpenAL buffer
        alBufferData(m_BufferID, format, wavData.Data.data(), 
                     static_cast<ALsizei>(wavData.Data.size()), 
                     static_cast<ALsizei>(m_SampleRate));
        
        if (!OpenALContext::CheckError("alBufferData"))
        {
            PIL_CORE_ERROR("OpenALBuffer: Failed to upload audio data");
            return false;
        }

        m_Loaded = true;
        PIL_CORE_INFO("OpenALBuffer: Loaded '{0}' ({1}Hz, {2}ch, {3}-bit, {4:.2f}s)",
            filepath, m_SampleRate, m_Channels, m_BitsPerSample, m_Duration);
        
        return true;
    }

    ALenum OpenALBuffer::GetALFormat() const
    {
        if (m_Channels == 1)
        {
            return (m_BitsPerSample == 8) ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
        }
        else if (m_Channels == 2)
        {
            return (m_BitsPerSample == 8) ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;
        }
        return 0;
    }

}
