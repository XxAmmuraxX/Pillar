#include "Pillar/Audio/AudioEngine.h"
#include "Pillar/Audio/AudioBuffer.h"
#include "Pillar/Audio/AudioSource.h"
#include "Platform/OpenAL/OpenALContext.h"
#include "Pillar/Logger.h"
#include <AL/al.h>
#include <algorithm>
#include <array>
#include <optional>
#include <vector>

namespace Pillar {

    // Static member storage
    static float s_MasterVolume = 1.0f;
    static glm::vec3 s_ListenerPosition = { 0.0f, 0.0f, 0.0f };
    static glm::vec3 s_ListenerVelocity = { 0.0f, 0.0f, 0.0f };
    static glm::vec3 s_ListenerForward = { 0.0f, 0.0f, -1.0f };
    static glm::vec3 s_ListenerUp = { 0.0f, 1.0f, 0.0f };

    std::vector<AudioEngine::TrackedSource> AudioEngine::s_TrackedSources;
    std::vector<AudioEngine::BusState> AudioEngine::s_BusStates(static_cast<size_t>(AudioEngine::AudioBus::Count));

    namespace {
        inline size_t BusIndex(AudioEngine::AudioBus bus)
        {
            return static_cast<size_t>(bus);
        }

        inline float Lerp(float a, float b, float t)
        {
            return a + (b - a) * t;
        }
    }

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

        for (auto& bus : s_BusStates)
        {
            bus.Volume = 1.0f;
            bus.Muted = false;
            bus.Fading = false;
            bus.FadeStart = 1.0f;
            bus.FadeTarget = 1.0f;
            bus.FadeDuration = 0.0f;
            bus.FadeElapsed = 0.0f;
        }

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
        
        s_TrackedSources.clear();
        s_BusStates.assign(static_cast<size_t>(AudioBus::Count), AudioEngine::BusState{});
        
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

    std::shared_ptr<AudioSource> AudioEngine::PlayOneShot(const std::string& filepath, float volume, float pitch, std::optional<glm::vec3> position, AudioBus bus)
    {
        if (!OpenALContext::IsInitialized())
        {
            PIL_CORE_WARN("AudioEngine::PlayOneShot: Audio engine not initialized");
            return nullptr;
        }

        auto buffer = AudioBuffer::Create(filepath);
        if (!buffer)
        {
            PIL_CORE_WARN("AudioEngine::PlayOneShot: Failed to load buffer for '{0}'", filepath);
            return nullptr;
        }

        auto source = CreateSource();
        if (!source)
            return nullptr;

        source->SetBuffer(buffer);
        SetSourceBus(source, bus);
        SetSourceVolume(source, volume);
        source->SetPitch(pitch);
        source->SetLooping(false);
        if (position.has_value())
        {
            source->SetPosition(position.value());
        }

        source->Play();
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

        ApplyAllBusGains();
    }

    float AudioEngine::GetMasterVolume()
    {
        return s_MasterVolume;
    }

    void AudioEngine::SetBusVolume(AudioBus bus, float volume)
    {
        auto& state = GetBusState(bus);
        state.Volume = std::clamp(volume, 0.0f, 1.0f);
        state.Muted = false;
        state.Fading = false;
        ApplyAllBusGains();
    }

    float AudioEngine::GetBusVolume(AudioBus bus)
    {
        return GetEffectiveBusVolume(bus);
    }

    void AudioEngine::MuteBus(AudioBus bus)
    {
        auto& state = GetBusState(bus);
        state.Muted = true;
        ApplyAllBusGains();
    }

    void AudioEngine::UnmuteBus(AudioBus bus)
    {
        auto& state = GetBusState(bus);
        state.Muted = false;
        ApplyAllBusGains();
    }

    bool AudioEngine::IsBusMuted(AudioBus bus)
    {
        return GetBusState(bus).Muted;
    }

    void AudioEngine::FadeBusTo(AudioBus bus, float targetVolume, float durationSeconds)
    {
        auto& state = GetBusState(bus);
        targetVolume = std::clamp(targetVolume, 0.0f, 1.0f);
        if (durationSeconds <= 0.0f)
        {
            state.Volume = targetVolume;
            state.Muted = targetVolume <= 0.0f;
            state.Fading = false;
            ApplyAllBusGains();
            return;
        }

        state.FadeStart = GetEffectiveBusVolume(bus);
        state.FadeTarget = targetVolume;
        state.FadeDuration = durationSeconds;
        state.FadeElapsed = 0.0f;
        state.Fading = true;
    }

    void AudioEngine::FadeIn(AudioBus bus, float durationSeconds, float targetVolume)
    {
        SetBusVolume(bus, 0.0f);
        FadeBusTo(bus, targetVolume, durationSeconds);
    }

    void AudioEngine::FadeOut(AudioBus bus, float durationSeconds, float targetVolume)
    {
        FadeBusTo(bus, targetVolume, durationSeconds);
    }

    void AudioEngine::Update(float deltaSeconds)
    {
        if (deltaSeconds <= 0.0f)
            return;

        bool changed = false;
        for (size_t i = 0; i < s_BusStates.size(); ++i)
        {
            auto& state = s_BusStates[i];
            if (!state.Fading)
                continue;

            state.FadeElapsed += deltaSeconds;
            float t = state.FadeDuration > 0.0f ? std::clamp(state.FadeElapsed / state.FadeDuration, 0.0f, 1.0f) : 1.0f;
            state.Volume = std::clamp(Lerp(state.FadeStart, state.FadeTarget, t), 0.0f, 1.0f);

            if (t >= 1.0f)
            {
                state.Fading = false;
                state.FadeElapsed = 0.0f;
                state.Muted = state.Muted || state.FadeTarget <= 0.0f;
            }
            changed = true;
        }

        if (changed)
        {
            ApplyAllBusGains();
        }
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
        CleanupSources();
        for (auto& tracked : s_TrackedSources)
        {
            if (auto source = tracked.Source.lock())
            {
                source->Stop();
            }
        }
    }

    void AudioEngine::PauseAllSounds()
    {
        CleanupSources();
        for (auto& tracked : s_TrackedSources)
        {
            if (auto source = tracked.Source.lock())
            {
                if (source->IsPlaying())
                {
                    source->Pause();
                }
            }
        }
    }

    void AudioEngine::ResumeAllSounds()
    {
        CleanupSources();
        for (auto& tracked : s_TrackedSources)
        {
            if (auto source = tracked.Source.lock())
            {
                if (source->IsPaused())
                {
                    source->Play();
                }
            }
        }
    }

    void AudioEngine::SetSourceBus(const std::shared_ptr<AudioSource>& source, AudioBus bus)
    {
        CleanupSources();
        for (auto& tracked : s_TrackedSources)
        {
            if (tracked.Source.lock() == source)
            {
                tracked.Bus = bus;
                ApplyGainToSource(tracked);
                return;
            }
        }
    }

    AudioEngine::AudioBus AudioEngine::GetSourceBus(const std::shared_ptr<AudioSource>& source)
    {
        CleanupSources();
        for (auto& tracked : s_TrackedSources)
        {
            if (tracked.Source.lock() == source)
            {
                return tracked.Bus;
            }
        }
        return AudioBus::SFX;
    }

    void AudioEngine::SetSourceVolume(const std::shared_ptr<AudioSource>& source, float volume)
    {
        CleanupSources();
        for (auto& tracked : s_TrackedSources)
        {
            if (tracked.Source.lock() == source)
            {
                tracked.UserVolume = std::clamp(volume, 0.0f, 1.0f);
                ApplyGainToSource(tracked);
                return;
            }
        }

        // Not tracked (should not happen for CreateSource/PlayOneShot), set directly
        if (source)
        {
            source->SetVolume(std::clamp(volume, 0.0f, 1.0f));
        }
    }

    float AudioEngine::GetSourceUserVolume(const std::shared_ptr<AudioSource>& source)
    {
        CleanupSources();
        for (auto& tracked : s_TrackedSources)
        {
            if (tracked.Source.lock() == source)
            {
                return tracked.UserVolume;
            }
        }
        return 0.0f;
    }

    void AudioEngine::RegisterSource(const std::shared_ptr<AudioSource>& source)
    {
        CleanupSources();
        TrackedSource tracked;
        tracked.Source = source;
        tracked.Bus = AudioBus::SFX;
        tracked.UserVolume = 1.0f;
        s_TrackedSources.push_back(tracked);
        ApplyGainToSource(s_TrackedSources.back());
    }

    void AudioEngine::CleanupSources()
    {
        auto it = s_TrackedSources.begin();
        while (it != s_TrackedSources.end())
        {
            if (it->Source.expired())
            {
                it = s_TrackedSources.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void AudioEngine::ApplyAllBusGains()
    {
        CleanupSources();
        for (auto& tracked : s_TrackedSources)
        {
            ApplyGainToSource(tracked);
        }
    }

    void AudioEngine::ApplyGainToSource(TrackedSource& tracked)
    {
        auto source = tracked.Source.lock();
        if (!source)
            return;

        float gain = tracked.UserVolume * s_MasterVolume * GetEffectiveBusVolume(tracked.Bus);
        source->SetVolume(std::clamp(gain, 0.0f, 1.0f));
    }

    AudioEngine::BusState& AudioEngine::GetBusState(AudioBus bus)
    {
        return s_BusStates.at(BusIndex(bus));
    }

    float AudioEngine::GetEffectiveBusVolume(AudioBus bus)
    {
        const auto& state = s_BusStates.at(BusIndex(bus));
        if (state.Fading && state.FadeDuration > 0.0f)
        {
            float t = std::clamp(state.FadeElapsed / state.FadeDuration, 0.0f, 1.0f);
            float volume = std::clamp(Lerp(state.FadeStart, state.FadeTarget, t), 0.0f, 1.0f);
            return state.Muted ? 0.0f : volume;
        }

        return state.Muted ? 0.0f : state.Volume;
    }

}
