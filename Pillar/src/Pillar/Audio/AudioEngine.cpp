#include "Pillar/Audio/AudioEngine.h"
#include "Pillar/Audio/AudioBuffer.h"
#include "Pillar/Audio/AudioSource.h"
#include "Platform/OpenAL/OpenALContext.h"
#include "Pillar/Logger.h"
#include <AL/al.h>
#include <algorithm> 
#include <vector>

namespace Pillar {

    // Static member storage
    static float s_MasterVolume = 1.0f;
    static glm::vec3 s_ListenerPosition = { 0.0f, 0.0f, 0.0f };
    static glm::vec3 s_ListenerVelocity = { 0.0f, 0.0f, 0.0f };
    static glm::vec3 s_ListenerForward = { 0.0f, 0.0f, -1.0f };
    static glm::vec3 s_ListenerUp = { 0.0f, 1.0f, 0.0f };
    std::vector<std::weak_ptr<AudioSource>> AudioEngine::s_ActiveSources;

    void AudioEngine::Init()
    {
        PIL_CORE_INFO("AudioEngine: Initializing...");

        if (!OpenALContext::Init())
        {
            PIL_CORE_ERROR("AudioEngine: Failed to initialize OpenAL context");
            return;
        }

        // Reset static state to defaults
        s_MasterVolume = 1.0f;
        s_ListenerPosition = { 0.0f, 0.0f, 0.0f };
        s_ListenerVelocity = { 0.0f, 0.0f, 0.0f };
        s_ListenerForward = { 0.0f, 0.0f, -1.0f };
        s_ListenerUp = { 0.0f, 1.0f, 0.0f };

        // Set initial listener properties
        alListenerf(AL_GAIN, s_MasterVolume);
        alListener3f(AL_POSITION, s_ListenerPosition.x, s_ListenerPosition.y, s_ListenerPosition.z);
        alListener3f(AL_VELOCITY, s_ListenerVelocity.x, s_ListenerVelocity.y, s_ListenerVelocity.z);
        ALfloat orientation[] = {
            s_ListenerForward.x, s_ListenerForward.y, s_ListenerForward.z,
            s_ListenerUp.x, s_ListenerUp.y, s_ListenerUp.z
        };
        alListenerfv(AL_ORIENTATION, orientation);

        PIL_CORE_INFO("AudioEngine: Initialized successfully");
    }

    void AudioEngine::Shutdown()
    {
        PIL_CORE_INFO("AudioEngine: Shutting down...");
        
        // Clear all tracked sources
        s_ActiveSources.clear();
        
        OpenALContext::Shutdown();
        PIL_CORE_INFO("AudioEngine: Shutdown complete");
    }

    bool AudioEngine::IsInitialized()
    {
        return OpenALContext::IsInitialized();
    }

    std::shared_ptr<AudioBuffer> AudioEngine::CreateBuffer(const std::string& filepath)
    {
        return AudioBuffer::Create(filepath);
    }

    std::shared_ptr<AudioSource> AudioEngine::CreateSource()
    {
        auto source = AudioSource::Create();
        if (source)
        {
            RegisterSource(source);
        }
        return source;
    }

    void AudioEngine::SetMasterVolume(float volume)
    {
        s_MasterVolume = std::clamp(volume, 0.0f, 1.0f);
        
        if (OpenALContext::IsInitialized())
        {
            alListenerf(AL_GAIN, s_MasterVolume);
            OpenALContext::CheckError("SetMasterVolume");
        }
    }

    float AudioEngine::GetMasterVolume()
    {
        return s_MasterVolume;
    }

    void AudioEngine::SetListenerPosition(const glm::vec3& position)
    {
        s_ListenerPosition = position;
        
        if (OpenALContext::IsInitialized())
        {
            alListener3f(AL_POSITION, position.x, position.y, position.z);
            OpenALContext::CheckError("SetListenerPosition");
        }
    }

    glm::vec3 AudioEngine::GetListenerPosition()
    {
        return s_ListenerPosition;
    }

    void AudioEngine::SetListenerVelocity(const glm::vec3& velocity)
    {
        s_ListenerVelocity = velocity;
        
        if (OpenALContext::IsInitialized())
        {
            alListener3f(AL_VELOCITY, velocity.x, velocity.y, velocity.z);
            OpenALContext::CheckError("SetListenerVelocity");
        }
    }

    void AudioEngine::SetListenerOrientation(const glm::vec3& forward, const glm::vec3& up)
    {
        s_ListenerForward = forward;
        s_ListenerUp = up;
        
        if (OpenALContext::IsInitialized())
        {
            ALfloat orientation[] = { 
                forward.x, forward.y, forward.z,
                up.x, up.y, up.z 
            };
            alListenerfv(AL_ORIENTATION, orientation);
            OpenALContext::CheckError("SetListenerOrientation");
        }
    }

    void AudioEngine::StopAllSounds()
    {
        // Clean up expired weak pointers and stop all valid sources
        auto it = s_ActiveSources.begin();
        while (it != s_ActiveSources.end())
        {
            if (auto source = it->lock())
            {
                source->Stop();
                ++it;
            }
            else
            {
                // Remove expired weak pointer
                it = s_ActiveSources.erase(it);
            }
        }
    }

    void AudioEngine::PauseAllSounds()
    {
        // Clean up expired weak pointers and pause all valid sources
        auto it = s_ActiveSources.begin();
        while (it != s_ActiveSources.end())
        {
            if (auto source = it->lock())
            {
                if (source->IsPlaying())
                {
                    source->Pause();
                }
                ++it;
            }
            else
            {
                // Remove expired weak pointer
                it = s_ActiveSources.erase(it);
            }
        }
    }

    void AudioEngine::ResumeAllSounds()
    {
        // Clean up expired weak pointers and resume all valid sources
        auto it = s_ActiveSources.begin();
        while (it != s_ActiveSources.end())
        {
            if (auto source = it->lock())
            {
                if (source->IsPaused())
                {
                    source->Play();
                }
                ++it;
            }
            else
            {
                // Remove expired weak pointer
                it = s_ActiveSources.erase(it);
            }
        }
    }

    void AudioEngine::RegisterSource(const std::shared_ptr<AudioSource>& source)
    {
        // Clean up expired weak pointers before adding new one
        auto it = s_ActiveSources.begin();
        while (it != s_ActiveSources.end())
        {
            if (it->expired())
            {
                it = s_ActiveSources.erase(it);
            }
            else
            {
                ++it;
            }
        }
        
        s_ActiveSources.push_back(source);
    }

}
