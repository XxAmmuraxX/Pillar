#pragma once

#include "Pillar/Audio/AudioClip.h"
#include <memory>

namespace BallGame {

    // ============================================
    // Game Audio Manager
    // Handles all sound effects for the ball game
    // ============================================
    class GameAudio
    {
    public:
        void Init()
        {
            // Load sound effects - using available audio files
            m_ShootSound = Pillar::AudioClip::Create("audio/swing.wav");
            m_BounceSound = Pillar::AudioClip::Create("audio/sfx_1.wav");
            m_BoostSound = Pillar::AudioClip::Create("audio/boost.wav");
            m_GoalSound = Pillar::AudioClip::Create("audio/sfx_3.wav");
            m_BackgroundMusic = Pillar::AudioClip::Create("audio/background_music.wav");
            
            // Set default volumes
            if (m_ShootSound) m_ShootSound->SetVolume(0.7f);
            if (m_BounceSound) m_BounceSound->SetVolume(0.45f);
            if (m_BoostSound) m_BoostSound->SetVolume(0.55f);
            if (m_GoalSound) m_GoalSound->SetVolume(0.8f);
            if (m_BackgroundMusic)
            {
                m_BackgroundMusic->SetLooping(true);
                m_BackgroundMusic->SetVolume(0.35f);
                m_BackgroundMusic->Play();
            }
        }

        void PlayShoot()
        {
            if (m_ShootSound && m_ShootSound->IsLoaded())
                m_ShootSound->Play();
        }

        void PlayBounce()
        {
            if (m_BounceSound && m_BounceSound->IsLoaded())
                m_BounceSound->Play();
        }

        void PlayBoost()
        {
            if (m_BoostSound && m_BoostSound->IsLoaded())
                m_BoostSound->Play();
        }

        void PlayGoal()
        {
            if (m_GoalSound && m_GoalSound->IsLoaded())
                m_GoalSound->Play();
        }

        void StopMusic()
        {
            if (m_BackgroundMusic && m_BackgroundMusic->IsLoaded())
                m_BackgroundMusic->Stop();
        }

        void EnsureMusicPlaying()
        {
            if (m_BackgroundMusic && m_BackgroundMusic->IsLoaded() && !m_BackgroundMusic->IsPlaying())
                m_BackgroundMusic->Play();
        }

        void SetMasterVolume(float volume)
        {
            m_MasterVolume = volume;
            if (m_ShootSound) m_ShootSound->SetVolume(0.7f * volume);
            if (m_BounceSound) m_BounceSound->SetVolume(0.45f * volume);
            if (m_BoostSound) m_BoostSound->SetVolume(0.55f * volume);
            if (m_GoalSound) m_GoalSound->SetVolume(0.8f * volume);
            if (m_BackgroundMusic) m_BackgroundMusic->SetVolume(0.35f * volume);
        }

        float GetMasterVolume() const { return m_MasterVolume; }

    private:
        std::shared_ptr<Pillar::AudioClip> m_ShootSound;
        std::shared_ptr<Pillar::AudioClip> m_BounceSound;
        std::shared_ptr<Pillar::AudioClip> m_BoostSound;
        std::shared_ptr<Pillar::AudioClip> m_GoalSound;
        std::shared_ptr<Pillar::AudioClip> m_BackgroundMusic;
        float m_MasterVolume = 1.0f;
    };

} // namespace BallGame
