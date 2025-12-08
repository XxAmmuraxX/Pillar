#include "Pillar/Audio/AudioClip.h"
#include "Pillar/Audio/AudioEngine.h"
#include "Pillar/Logger.h"

namespace Pillar {

    AudioClip::AudioClip(const std::string& filepath)
        : m_FilePath(filepath)
    {
        // Create buffer from file
        m_Buffer = AudioEngine::CreateBuffer(filepath);
        
        if (!m_Buffer)
        {
            PIL_CORE_ERROR("AudioClip: Failed to load audio file: {0}", filepath);
            return;
        }

        // Create source for playback
        m_Source = AudioEngine::CreateSource();
        
        if (!m_Source)
        {
            PIL_CORE_ERROR("AudioClip: Failed to create audio source");
            m_Buffer = nullptr;
            return;
        }

        // Attach buffer to source
        m_Source->SetBuffer(m_Buffer);
        
        PIL_CORE_TRACE("AudioClip: Created clip from '{0}'", filepath);
    }

    AudioClip::~AudioClip()
    {
        if (m_Source)
        {
            m_Source->Stop();
        }
    }

    void AudioClip::Play()
    {
        if (m_Source)
        {
            m_Source->Play();
        }
    }

    void AudioClip::Stop()
    {
        if (m_Source)
        {
            m_Source->Stop();
        }
    }

    void AudioClip::Pause()
    {
        if (m_Source)
        {
            m_Source->Pause();
        }
    }

    void AudioClip::Resume()
    {
        if (m_Source && m_Source->IsPaused())
        {
            m_Source->Play();
        }
    }

    void AudioClip::SetVolume(float volume)
    {
        if (m_Source)
        {
            m_Source->SetVolume(volume);
        }
    }

    float AudioClip::GetVolume() const
    {
        return m_Source ? m_Source->GetVolume() : 0.0f;
    }

    void AudioClip::SetPitch(float pitch)
    {
        if (m_Source)
        {
            m_Source->SetPitch(pitch);
        }
    }

    float AudioClip::GetPitch() const
    {
        return m_Source ? m_Source->GetPitch() : 1.0f;
    }

    void AudioClip::SetLooping(bool loop)
    {
        if (m_Source)
        {
            m_Source->SetLooping(loop);
        }
    }

    bool AudioClip::IsLooping() const
    {
        return m_Source ? m_Source->IsLooping() : false;
    }

    void AudioClip::SetPosition(const glm::vec3& position)
    {
        if (m_Source)
        {
            m_Source->SetPosition(position);
        }
    }

    glm::vec3 AudioClip::GetPosition() const
    {
        return m_Source ? m_Source->GetPosition() : glm::vec3(0.0f);
    }

    bool AudioClip::IsPlaying() const
    {
        return m_Source ? m_Source->IsPlaying() : false;
    }

    bool AudioClip::IsPaused() const
    {
        return m_Source ? m_Source->IsPaused() : false;
    }

    bool AudioClip::IsLoaded() const
    {
        return m_Buffer && m_Buffer->IsLoaded() && m_Source;
    }

    float AudioClip::GetDuration() const
    {
        return m_Buffer ? m_Buffer->GetDuration() : 0.0f;
    }

    float AudioClip::GetPlaybackPosition() const
    {
        return m_Source ? m_Source->GetPlaybackPosition() : 0.0f;
    }

    void AudioClip::SetPlaybackPosition(float seconds)
    {
        if (m_Source)
        {
            m_Source->SetPlaybackPosition(seconds);
        }
    }

    std::shared_ptr<AudioClip> AudioClip::Create(const std::string& filepath)
    {
        auto clip = std::make_shared<AudioClip>(filepath);
        
        if (!clip->IsLoaded())
        {
            return nullptr;
        }
        
        return clip;
    }

}
