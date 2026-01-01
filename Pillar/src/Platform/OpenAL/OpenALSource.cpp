#include "Platform/OpenAL/OpenALSource.h"
#include "Platform/OpenAL/OpenALContext.h"
#include "Pillar/Logger.h"
#include <algorithm>

namespace Pillar {

    OpenALSource::OpenALSource()
    {
        // Generate OpenAL source
        alGenSources(1, &m_SourceID);
        if (!OpenALContext::CheckError("alGenSources"))
        {
            PIL_CORE_ERROR("OpenALSource: Failed to generate source");
            m_SourceID = 0;
            return;
        }

        // Set default properties
        alSourcef(m_SourceID, AL_GAIN, m_Volume);
        alSourcef(m_SourceID, AL_PITCH, m_Pitch);
        alSourcei(m_SourceID, AL_LOOPING, m_Looping ? AL_TRUE : AL_FALSE);
        alSource3f(m_SourceID, AL_POSITION, m_Position.x, m_Position.y, m_Position.z);
        alSource3f(m_SourceID, AL_VELOCITY, m_Velocity.x, m_Velocity.y, m_Velocity.z);
        alSourcef(m_SourceID, AL_REFERENCE_DISTANCE, m_MinDistance);
        alSourcef(m_SourceID, AL_MAX_DISTANCE, m_MaxDistance);
        alSourcef(m_SourceID, AL_ROLLOFF_FACTOR, m_RolloffFactor);

        PIL_CORE_TRACE("OpenALSource: Created source ID {0}", m_SourceID);
    }

    OpenALSource::~OpenALSource()
    {
        if (m_SourceID != 0)
        {
            // Stop playback before deleting
            alSourceStop(m_SourceID);
            alSourcei(m_SourceID, AL_BUFFER, 0); // Detach buffer
            alDeleteSources(1, &m_SourceID);
            OpenALContext::CheckError("alDeleteSources");
        }
    }

    void OpenALSource::SetBuffer(const std::shared_ptr<AudioBuffer>& buffer)
    {
        m_Buffer = buffer;
        
        if (m_SourceID == 0)
            return;

        if (buffer)
        {
            alSourcei(m_SourceID, AL_BUFFER, static_cast<ALint>(buffer->GetBufferID()));
            OpenALContext::CheckError("SetBuffer");
        }
        else
        {
            // Detach buffer
            alSourcei(m_SourceID, AL_BUFFER, 0);
        }
    }

    void OpenALSource::Play()
    {
        if (m_SourceID == 0)
            return;

        alSourcePlay(m_SourceID);
        OpenALContext::CheckError("alSourcePlay");
    }

    void OpenALSource::Pause()
    {
        if (m_SourceID == 0)
            return;

        alSourcePause(m_SourceID);
        OpenALContext::CheckError("alSourcePause");
    }

    void OpenALSource::Stop()
    {
        if (m_SourceID == 0)
            return;

        alSourceStop(m_SourceID);
        OpenALContext::CheckError("alSourceStop");
    }

    void OpenALSource::Rewind()
    {
        if (m_SourceID == 0)
            return;

        alSourceRewind(m_SourceID);
        OpenALContext::CheckError("alSourceRewind");
    }

    AudioState OpenALSource::GetState() const
    {
        if (m_SourceID == 0)
            return AudioState::Stopped;

        ALint state;
        alGetSourcei(m_SourceID, AL_SOURCE_STATE, &state);

        switch (state)
        {
            case AL_PLAYING: return AudioState::Playing;
            case AL_PAUSED:  return AudioState::Paused;
            default:         return AudioState::Stopped;
        }
    }

    bool OpenALSource::IsPlaying() const
    {
        return GetState() == AudioState::Playing;
    }

    bool OpenALSource::IsPaused() const
    {
        return GetState() == AudioState::Paused;
    }

    bool OpenALSource::IsStopped() const
    {
        return GetState() == AudioState::Stopped;
    }

    void OpenALSource::SetVolume(float volume)
    {
        m_Volume = std::clamp(volume, 0.0f, 1.0f);
        
        if (m_SourceID == 0)
            return;

        alSourcef(m_SourceID, AL_GAIN, m_Volume);
        OpenALContext::CheckError("SetVolume");
    }

    void OpenALSource::SetPitch(float pitch)
    {
        m_Pitch = std::clamp(pitch, 0.5f, 2.0f);
        
        if (m_SourceID == 0)
            return;

        alSourcef(m_SourceID, AL_PITCH, m_Pitch);
        OpenALContext::CheckError("SetPitch");
    }

    void OpenALSource::SetLooping(bool loop)
    {
        m_Looping = loop;
        
        if (m_SourceID == 0)
            return;

        alSourcei(m_SourceID, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
        OpenALContext::CheckError("SetLooping");
    }

    void OpenALSource::SetPosition(const glm::vec3& position)
    {
        m_Position = position;
        
        if (m_SourceID == 0)
            return;

        alSource3f(m_SourceID, AL_POSITION, position.x, position.y, position.z);
        OpenALContext::CheckError("SetPosition");
    }

    void OpenALSource::SetVelocity(const glm::vec3& velocity)
    {
        m_Velocity = velocity;
        
        if (m_SourceID == 0)
            return;

        alSource3f(m_SourceID, AL_VELOCITY, velocity.x, velocity.y, velocity.z);
        OpenALContext::CheckError("SetVelocity");
    }

    void OpenALSource::SetDirection(const glm::vec3& direction)
    {
        m_Direction = direction;
        
        if (m_SourceID == 0)
            return;

        alSource3f(m_SourceID, AL_DIRECTION, direction.x, direction.y, direction.z);
        OpenALContext::CheckError("SetDirection");
    }

    void OpenALSource::SetMinDistance(float distance)
    {
        m_MinDistance = std::max(0.0f, distance);
        
        if (m_SourceID == 0)
            return;

        alSourcef(m_SourceID, AL_REFERENCE_DISTANCE, m_MinDistance);
        OpenALContext::CheckError("SetMinDistance");
    }

    void OpenALSource::SetMaxDistance(float distance)
    {
        m_MaxDistance = std::max(0.0f, distance);
        
        if (m_SourceID == 0)
            return;

        alSourcef(m_SourceID, AL_MAX_DISTANCE, m_MaxDistance);
        OpenALContext::CheckError("SetMaxDistance");
    }

    void OpenALSource::SetRolloffFactor(float factor)
    {
        m_RolloffFactor = std::max(0.0f, factor);
        
        if (m_SourceID == 0)
            return;

        alSourcef(m_SourceID, AL_ROLLOFF_FACTOR, m_RolloffFactor);
        OpenALContext::CheckError("SetRolloffFactor");
    }

    void OpenALSource::SetPlaybackPosition(float seconds)
    {
        if (m_SourceID == 0)
            return;

        alSourcef(m_SourceID, AL_SEC_OFFSET, std::max(0.0f, seconds));
        OpenALContext::CheckError("SetPlaybackPosition");
    }

    float OpenALSource::GetPlaybackPosition() const
    {
        if (m_SourceID == 0)
            return 0.0f;

        ALfloat seconds = 0.0f;
        alGetSourcef(m_SourceID, AL_SEC_OFFSET, &seconds);
        return seconds;
    }

}
