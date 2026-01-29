#pragma once

#include <Pillar/Audio/AudioEngine.h>
#include <Pillar/Audio/AudioBuffer.h>
#include <Pillar/Audio/AudioSource.h>
#include <Pillar/Utils/AssetManager.h>
#include <Pillar/Logger.h>

#include <memory>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <string>
#include <glm/glm.hpp>

namespace Game {

    /**
     * AudioManager - Manages audio resources for the Top-Down Shooter
     * 
     * Pre-loads and caches audio buffers for efficient playback.
     * Provides simple methods for playing sounds at positions.
     */
    class AudioManager
    {
    public:
        static AudioManager& Instance()
        {
            static AudioManager instance;
            return instance;
        }

        void Init()
        {
            if (m_Initialized) return;

            PIL_INFO("AudioManager: Initializing audio...");

            // Pre-load all game sounds
            LoadBuffer("shoot", "shoot_fireball.wav");
            LoadBuffer("enemy_shoot", "sfx_2.wav");
            LoadBuffer("hit", "fireball_hits_enemy.wav");
            LoadBuffer("death", "sfx_3.wav");
            LoadBuffer("pickup", "pickup_coin.wav");
            LoadBuffer("player_hurt", "player_hurt.wav");
            LoadBuffer("player_death", "player_dies.wav");

            // Load background music
            m_MusicBuffer = Pillar::AudioEngine::CreateBuffer(
                Pillar::AssetManager::GetAudioPath("top_down_shooter_background_loop.wav")
            );

            if (m_MusicBuffer)
            {
                m_MusicSource = Pillar::AudioEngine::CreateSource();
                if (m_MusicSource)
                {
                    m_MusicSource->SetBuffer(m_MusicBuffer);
                    m_MusicSource->SetLooping(true);
                    m_MusicSource->SetVolume(0.3f);  // Lower volume for background
                }
            }

            m_Initialized = true;
            PIL_INFO("AudioManager: Initialized with {} sound effects", m_Buffers.size());
        }

        void Shutdown()
        {
            if (m_MusicSource)
            {
                m_MusicSource->Stop();
                m_MusicSource.reset();
            }
            m_ActiveSources.clear();
            m_MusicBuffer.reset();
            m_Buffers.clear();
            m_Initialized = false;
        }

        void SetSFXVolume(float volume) { m_SFXVolume = volume; }
        float GetSFXVolume() const { return m_SFXVolume; }

        // Play sound at world position (2D -> 3D with z=0)
        // Automatically applies random pitch variation for natural feel
        void PlaySound(const std::string& name, const glm::vec2& position, float volume = 1.0f, float pitch = 1.0f)
        {
            auto it = m_Buffers.find(name);
            if (it == m_Buffers.end() || !it->second)
            {
                PIL_WARN("AudioManager: Sound '{}' not found in cache", name);
                return;
            }

            // Enforce max concurrent sound limit
            CleanupFinishedSources();
            if (m_ActiveSources.size() >= m_MaxConcurrentSounds)
            {
                return;  // Skip sound to prevent audio clutter
            }

            auto source = Pillar::AudioEngine::CreateSource();
            if (source)
            {
                // Apply random pitch variation for natural feel
                float pitchVariation = pitch * RandomPitchVariation();

                source->SetBuffer(it->second);
                source->SetVolume(volume * m_SFXVolume);
                source->SetPitch(pitchVariation);
                source->SetPosition(glm::vec3(position, 0.0f));

                // Configure 3D audio settings for better audibility
                source->SetMinDistance(5.0f);
                source->SetMaxDistance(50.0f);
                source->SetRolloffFactor(1.0f);

                source->Play();

                m_ActiveSources.push_back(source);
            }
        }

        // Play sound without position (UI sounds, global effects)
        void PlaySound(const std::string& name, float volume = 1.0f, float pitch = 1.0f)
        {
            auto it = m_Buffers.find(name);
            if (it == m_Buffers.end() || !it->second)
            {
                PIL_WARN("AudioManager: Sound '{}' not found", name);
                return;
            }

            CleanupFinishedSources();
            if (m_ActiveSources.size() >= m_MaxConcurrentSounds)
            {
                return;
            }

            auto source = Pillar::AudioEngine::CreateSource();
            if (source)
            {
                float pitchVariation = pitch * RandomPitchVariation();

                source->SetBuffer(it->second);
                source->SetVolume(volume * m_SFXVolume);
                source->SetPitch(pitchVariation);
                source->Play();

                m_ActiveSources.push_back(source);
            }
        }

        // Music controls
        void StartMusic()
        {
            if (m_MusicSource && !m_MusicSource->IsPlaying())
            {
                m_MusicSource->Play();
                PIL_INFO("AudioManager: Music started");
            }
        }

        void StopMusic()
        {
            if (m_MusicSource)
            {
                m_MusicSource->Stop();
                PIL_INFO("AudioManager: Music stopped");
            }
        }

        void PauseMusic()
        {
            if (m_MusicSource)
            {
                m_MusicSource->Pause();
            }
        }

        void ResumeMusic()
        {
            if (m_MusicSource)
            {
                m_MusicSource->Play();
            }
        }

        void SetMusicVolume(float volume)
        {
            if (m_MusicSource)
            {
                m_MusicSource->SetVolume(volume);
            }
        }

        float GetMusicVolume() const
        {
            return m_MusicSource ? m_MusicSource->GetVolume() : 0.0f;
        }

        bool IsMusicPlaying() const
        {
            return m_MusicSource && m_MusicSource->IsPlaying();
        }

    private:
        AudioManager() = default;
        ~AudioManager() = default;

        void LoadBuffer(const std::string& name, const std::string& filename)
        {
            std::string path = Pillar::AssetManager::GetAudioPath(filename);
            PIL_INFO("AudioManager: Loading '{}' from path: {}", name, path);
            auto buffer = Pillar::AudioEngine::CreateBuffer(path);
            if (buffer)
            {
                m_Buffers[name] = buffer;
                PIL_INFO("AudioManager: Successfully loaded '{}'", filename);
            }
            else
            {
                PIL_WARN("AudioManager: Failed to load '{}' from path '{}'", filename, path);
            }
        }

        // Clean up finished sources to prevent unbounded growth
        void CleanupFinishedSources()
        {
            m_ActiveSources.erase(
                std::remove_if(m_ActiveSources.begin(), m_ActiveSources.end(),
                    [](const std::shared_ptr<Pillar::AudioSource>& source) {
                        return source->IsStopped();
                    }),
                m_ActiveSources.end()
            );
        }

        // Random pitch variation (0.9 - 1.1) for natural sound
        float RandomPitchVariation()
        {
            return 0.9f + static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 0.2f;
        }

        bool m_Initialized = false;
        float m_SFXVolume = 1.0f;
        static constexpr size_t m_MaxConcurrentSounds = 16;
        std::unordered_map<std::string, std::shared_ptr<Pillar::AudioBuffer>> m_Buffers;

        // Active sound effect sources (kept alive until playback finishes)
        std::vector<std::shared_ptr<Pillar::AudioSource>> m_ActiveSources;

        // Music
        std::shared_ptr<Pillar::AudioBuffer> m_MusicBuffer;
        std::shared_ptr<Pillar::AudioSource> m_MusicSource;
    };

} // namespace Game
