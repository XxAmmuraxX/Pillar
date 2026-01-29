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

            // Pre-allocate audio source pool to avoid runtime allocation
            m_SourcePool.reserve(m_MaxConcurrentSounds);
            for (size_t i = 0; i < m_MaxConcurrentSounds; ++i)
            {
                auto source = Pillar::AudioEngine::CreateSource();
                if (source)
                {
                    m_SourcePool.push_back(source);
                }
            }
            PIL_INFO("AudioManager: Pre-allocated {} audio sources", m_SourcePool.size());

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
            m_SourcePool.clear();
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

            // Get an available source from the pool (no runtime allocation)
            auto source = AcquireSource();
            if (!source)
            {
                return;  // All sources busy, skip sound
            }

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

            auto source = AcquireSource();
            if (!source)
            {
                return;
            }

            float pitchVariation = pitch * RandomPitchVariation();

            source->SetBuffer(it->second);
            source->SetVolume(volume * m_SFXVolume);
            source->SetPitch(pitchVariation);
            source->Play();
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

        // Get an available source from the pre-allocated pool (LRU eviction)
        std::shared_ptr<Pillar::AudioSource> AcquireSource()
        {
            // Find a stopped source first (free slot)
            for (auto& source : m_SourcePool)
            {
                if (source && source->IsStopped())
                {
                    return source;
                }
            }

            // All sources busy - use LRU eviction (steal first source)
            // This ensures we never allocate at runtime
            if (!m_SourcePool.empty())
            {
                auto source = m_SourcePool.front();
                source->Stop();
                return source;
            }

            return nullptr;
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

        // Pre-allocated audio source pool (no runtime allocation)
        std::vector<std::shared_ptr<Pillar::AudioSource>> m_SourcePool;

        // Music
        std::shared_ptr<Pillar::AudioBuffer> m_MusicBuffer;
        std::shared_ptr<Pillar::AudioSource> m_MusicSource;
    };

} // namespace Game
