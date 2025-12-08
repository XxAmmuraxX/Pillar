#pragma once

#include "Pillar.h"
#include "Pillar/Renderer/Renderer2D.h"
#include <imgui.h>
#include <memory>
#include <vector>

/**
 * @brief Demo layer showcasing the Pillar Audio System.
 * 
 * Features demonstrated:
 * - Loading and playing WAV audio files
 * - Volume, pitch, and looping controls
 * - 3D spatial audio positioning
 * - Multiple simultaneous audio sources
 * - Master volume control
 * 
 * Controls:
 * - 1-4: Play sound effects
 * - SPACE: Play/Pause background music
 * - M: Mute/Unmute master volume
 * - ImGui panel for detailed audio controls
 */
class AudioDemoLayer : public Pillar::Layer
{
public:
    AudioDemoLayer()
        : Layer("AudioDemoLayer"),
          m_CameraController(16.0f / 9.0f, false)
    {
    }

    void OnAttach() override
    {
        Layer::OnAttach();
        PIL_INFO("Audio Demo Layer attached!");

        // Audio engine is automatically initialized by Application
        if (!Pillar::AudioEngine::IsInitialized())
        {
            PIL_ERROR("Audio engine is not initialized!");
            return;
        }

        PIL_INFO("Audio engine is ready!");

        // Try to load audio files
        // NOTE: Users need to place WAV files in Sandbox/assets/audio/
        // Example files: background_music.wav, sfx_click.wav, sfx_explosion.wav, sfx_pickup.wav

        LoadAudioFiles();

        // Set up listener at origin
        Pillar::AudioEngine::SetListenerPosition(glm::vec3(0.0f));
        Pillar::AudioEngine::SetListenerOrientation(
            glm::vec3(0.0f, 0.0f, -1.0f),  // Forward (into screen)
            glm::vec3(0.0f, 1.0f, 0.0f)    // Up
        );

        PIL_INFO("Audio Demo initialized! Press 1-4 for sounds, SPACE for music, M to mute.");
    }

    void OnDetach() override
    {
        // Stop all sounds
        if (m_BackgroundMusic && m_BackgroundMusic->IsPlaying())
            m_BackgroundMusic->Stop();

        for (auto& sfx : m_SoundEffects)
        {
            if (sfx && sfx->IsPlaying())
                sfx->Stop();
        }

        // Audio engine shutdown is handled by Application
        Layer::OnDetach();
        PIL_INFO("Audio Demo Layer detached.");
    }

    void OnUpdate(float dt) override
    {
        m_Time += dt;

        // Update moving sound position (for 3D audio demo)
        if (m_MovingSoundEnabled && m_SoundEffects.size() > 0 && m_SoundEffects[0])
        {
            float angle = m_Time * m_MovingSoundSpeed;
            glm::vec3 newPos(
                cos(angle) * m_MovingSoundRadius,
                0.0f,
                sin(angle) * m_MovingSoundRadius
            );
            
            // Calculate velocity for Doppler effect
            if (m_EnableDopplerEffect)
            {
                glm::vec3 velocity = (newPos - m_MovingSoundPosition) / dt;
                m_SoundEffects[0]->GetSource()->SetVelocity(velocity);
                m_MovingSoundVelocity = velocity;
            }
            else
            {
                m_SoundEffects[0]->GetSource()->SetVelocity(glm::vec3(0.0f));
                m_MovingSoundVelocity = glm::vec3(0.0f);
            }
            
            m_SoundEffects[0]->SetPosition(newPos);
            m_MovingSoundPosition = newPos;
        }

        // Update listener orientation if rotating
        if (m_RotateListener)
        {
            m_ListenerRotation += m_ListenerRotationSpeed * dt;
            m_ListenerForward = glm::vec3(
                sin(m_ListenerRotation),
                0.0f,
                -cos(m_ListenerRotation)
            );
            Pillar::AudioEngine::SetListenerOrientation(
                m_ListenerForward,
                glm::vec3(0.0f, 1.0f, 0.0f)
            );
        }

        // Clear screen
        Pillar::Renderer::SetClearColor({ 0.05f, 0.05f, 0.1f, 1.0f });
        Pillar::Renderer::Clear();

        // Render visualization
        Pillar::Renderer2D::BeginScene(m_CameraController.GetCamera());

        // Draw listener position (center)
        Pillar::Renderer2D::DrawQuad({ 0.0f, 0.0f }, { 0.3f, 0.3f }, { 0.2f, 0.8f, 0.2f, 1.0f });

        // Draw sound source positions
        if (m_MovingSoundEnabled)
        {
            // Moving sound indicator
            Pillar::Renderer2D::DrawQuad(
                { m_MovingSoundPosition.x, m_MovingSoundPosition.z },
                { 0.2f, 0.2f },
                { 1.0f, 0.5f, 0.0f, 1.0f }
            );

            // Draw orbit path
            const int segments = 32;
            for (int i = 0; i < segments; ++i)
            {
                float angle1 = (i / (float)segments) * 2.0f * 3.14159f;
                float angle2 = ((i + 1) / (float)segments) * 2.0f * 3.14159f;
                glm::vec2 p1(cos(angle1) * m_MovingSoundRadius, sin(angle1) * m_MovingSoundRadius);
                glm::vec2 p2(cos(angle2) * m_MovingSoundRadius, sin(angle2) * m_MovingSoundRadius);
                glm::vec2 mid = (p1 + p2) * 0.5f;
                Pillar::Renderer2D::DrawQuad(mid, { 0.05f, 0.05f }, { 0.3f, 0.3f, 0.3f, 0.5f });
            }
        }

        // Draw static sound positions
        for (size_t i = 1; i < m_SoundEffects.size() && i < m_SoundPositions.size(); ++i)
        {
            glm::vec4 color = { 0.8f, 0.2f, 0.2f + i * 0.2f, 1.0f };
            Pillar::Renderer2D::DrawQuad(
                { m_SoundPositions[i].x, m_SoundPositions[i].z },
                { 0.15f, 0.15f },
                color
            );
        }

        Pillar::Renderer2D::EndScene();
    }

    void OnEvent(Pillar::Event& event) override
    {
        m_CameraController.OnEvent(event);

        if (event.GetEventType() == Pillar::EventType::KeyPressed)
        {
            auto& keyEvent = static_cast<Pillar::KeyPressedEvent&>(event);
            if (keyEvent.GetRepeatCount() == 0)
            {
                HandleKeyPress(keyEvent.GetKeyCode());
            }
        }
    }

    void OnImGuiRender() override
    {
        ImGui::Begin("Audio Demo Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        // Audio Engine Status
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Audio Engine Status");
        ImGui::Separator();

        bool initialized = Pillar::AudioEngine::IsInitialized();
        ImGui::Text("Initialized: %s", initialized ? "Yes" : "No");

        if (!initialized)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Audio engine failed to initialize!");
            ImGui::End();
            return;
        }

        // Master Volume
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.4f, 1.0f), "Master Volume");
        ImGui::Separator();

        float masterVolume = Pillar::AudioEngine::GetMasterVolume();
        if (ImGui::SliderFloat("Master##vol", &masterVolume, 0.0f, 1.0f))
        {
            Pillar::AudioEngine::SetMasterVolume(masterVolume);
        }

        if (ImGui::Button(m_Muted ? "Unmute (M)" : "Mute (M)"))
        {
            ToggleMute();
        }

        // Background Music
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Background Music");
        ImGui::Separator();

        if (m_BackgroundMusic && m_BackgroundMusic->IsLoaded())
        {
            bool isPlaying = m_BackgroundMusic->IsPlaying();
            bool isPaused = m_BackgroundMusic->IsPaused();

            ImGui::Text("Status: %s", isPlaying ? "Playing" : (isPaused ? "Paused" : "Stopped"));
            ImGui::Text("Duration: %.2f sec", m_BackgroundMusic->GetDuration());
            ImGui::Text("Position: %.2f sec", m_BackgroundMusic->GetPlaybackPosition());

            // Progress bar
            float progress = m_BackgroundMusic->GetDuration() > 0 ?
                m_BackgroundMusic->GetPlaybackPosition() / m_BackgroundMusic->GetDuration() : 0.0f;
            ImGui::ProgressBar(progress, ImVec2(-1, 0));

            if (ImGui::Button(isPlaying ? "Pause##music" : "Play##music"))
            {
                if (isPlaying)
                    m_BackgroundMusic->Pause();
                else
                    m_BackgroundMusic->Play();
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop##music"))
            {
                m_BackgroundMusic->Stop();
            }

            float musicVolume = m_BackgroundMusic->GetVolume();
            if (ImGui::SliderFloat("Volume##music", &musicVolume, 0.0f, 1.0f))
            {
                m_BackgroundMusic->SetVolume(musicVolume);
            }

            float musicPitch = m_BackgroundMusic->GetPitch();
            if (ImGui::SliderFloat("Pitch##music", &musicPitch, 0.5f, 2.0f))
            {
                m_BackgroundMusic->SetPitch(musicPitch);
            }

            bool looping = m_BackgroundMusic->IsLooping();
            if (ImGui::Checkbox("Loop##music", &looping))
            {
                m_BackgroundMusic->SetLooping(looping);
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "No music loaded");
            ImGui::TextWrapped("Place 'background_music.wav' in Sandbox/assets/audio/");
        }

        // Sound Effects
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.4f, 1.0f), "Sound Effects");
        ImGui::Separator();

        for (size_t i = 0; i < m_SoundEffects.size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));

            if (m_SoundEffects[i] && m_SoundEffects[i]->IsLoaded())
            {
                ImGui::Text("SFX %zu:", i + 1);
                ImGui::SameLine();
                if (ImGui::Button("Play"))
                {
                    m_SoundEffects[i]->Stop();
                    m_SoundEffects[i]->Play();
                }

                float vol = m_SoundEffects[i]->GetVolume();
                if (ImGui::SliderFloat("Vol", &vol, 0.0f, 1.0f))
                {
                    m_SoundEffects[i]->SetVolume(vol);
                }
            }
            else
            {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "SFX %zu: Not loaded", i + 1);
            }

            ImGui::PopID();
        }

        // 3D Audio
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.6f, 0.4f, 1.0f, 1.0f), "3D Spatial Audio");
        ImGui::Separator();

        ImGui::Checkbox("Enable Moving Sound", &m_MovingSoundEnabled);
        if (m_MovingSoundEnabled)
        {
            ImGui::SliderFloat("Orbit Radius", &m_MovingSoundRadius, 1.0f, 10.0f);
            ImGui::SliderFloat("Orbit Speed", &m_MovingSoundSpeed, 0.1f, 3.0f);
            ImGui::Text("Position: (%.2f, %.2f, %.2f)",
                m_MovingSoundPosition.x, m_MovingSoundPosition.y, m_MovingSoundPosition.z);

            // Doppler Effect
            ImGui::Spacing();
            ImGui::Checkbox("Enable Doppler Effect", &m_EnableDopplerEffect);
            if (m_EnableDopplerEffect)
            {
                ImGui::Text("Velocity: (%.2f, %.2f, %.2f)",
                    m_MovingSoundVelocity.x, m_MovingSoundVelocity.y, m_MovingSoundVelocity.z);
                ImGui::TextWrapped("Notice the pitch change as the sound moves toward/away from listener");
            }

            if (m_SoundEffects.size() > 0 && m_SoundEffects[0] && m_SoundEffects[0]->IsLoaded())
            {
                if (ImGui::Button("Play Moving Sound"))
                {
                    m_SoundEffects[0]->SetLooping(true);
                    m_SoundEffects[0]->Play();
                }
                ImGui::SameLine();
                if (ImGui::Button("Stop Moving Sound"))
                {
                    m_SoundEffects[0]->Stop();
                }
                
                // Attenuation controls
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "Attenuation Settings");
                if (ImGui::SliderFloat("Min Distance", &m_MinDistance, 0.1f, 10.0f))
                {
                    m_SoundEffects[0]->GetSource()->SetMinDistance(m_MinDistance);
                }
                if (ImGui::SliderFloat("Max Distance", &m_MaxDistance, 5.0f, 50.0f))
                {
                    m_SoundEffects[0]->GetSource()->SetMaxDistance(m_MaxDistance);
                }
                if (ImGui::SliderFloat("Rolloff Factor", &m_RolloffFactor, 0.1f, 5.0f))
                {
                    m_SoundEffects[0]->GetSource()->SetRolloffFactor(m_RolloffFactor);
                }
                ImGui::TextWrapped("Min Distance: sound at full volume. Max Distance: sound silent. Rolloff: how quickly sound fades.");
            }
        }

        // Listener Orientation
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Listener Orientation");
        ImGui::Separator();
        
        ImGui::Checkbox("Rotate Listener", &m_RotateListener);
        if (m_RotateListener)
        {
            ImGui::SliderFloat("Rotation Speed", &m_ListenerRotationSpeed, 0.1f, 3.0f);
            ImGui::Text("Rotation: %.2f degrees", m_ListenerRotation * 57.2958f);
            ImGui::Text("Forward: (%.2f, %.2f, %.2f)",
                m_ListenerForward.x, m_ListenerForward.y, m_ListenerForward.z);
            ImGui::TextWrapped("Listener rotation affects perceived sound direction in 3D audio");
        }

        // Keyboard shortcuts
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Keyboard Shortcuts");
        ImGui::Separator();
        ImGui::BulletText("1-4: Play sound effects");
        ImGui::BulletText("SPACE: Play/Pause music");
        ImGui::BulletText("M: Mute/Unmute");
        ImGui::BulletText("R: Toggle listener rotation");

        // File instructions
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.5f, 1.0f), "Audio Files");
        ImGui::Separator();
        ImGui::TextWrapped(
            "Place WAV files in: Sandbox/assets/audio/\n"
            "- background_music.wav\n"
            "- sfx_1.wav, sfx_2.wav, sfx_3.wav, sfx_4.wav"
        );

        ImGui::End();
    }

private:
    void LoadAudioFiles()
    {
        // Try to load background music
        m_BackgroundMusic = Pillar::AudioClip::Create("background_music.wav");
        if (m_BackgroundMusic && m_BackgroundMusic->IsLoaded())
        {
            m_BackgroundMusic->SetLooping(true);
            m_BackgroundMusic->SetVolume(0.5f);
            PIL_INFO("Background music loaded!");
        }
        else
        {
            PIL_WARN("Could not load background_music.wav - place it in Sandbox/assets/audio/");
        }

        // Try to load sound effects
        const char* sfxFiles[] = { "sfx_1.wav", "sfx_2.wav", "sfx_3.wav", "sfx_4.wav" };
        m_SoundPositions = {
            glm::vec3(0.0f),          // Moving sound (position updated in OnUpdate)
            glm::vec3(-3.0f, 0.0f, 0.0f),
            glm::vec3(3.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 3.0f)
        };

        for (int i = 0; i < 4; ++i)
        {
            auto sfx = Pillar::AudioClip::Create(sfxFiles[i]);
            if (sfx && sfx->IsLoaded())
            {
                sfx->SetPosition(m_SoundPositions[i]);
                PIL_INFO("Loaded {}", sfxFiles[i]);
            }
            else
            {
                PIL_WARN("Could not load {} - place it in Sandbox/assets/audio/", sfxFiles[i]);
            }
            m_SoundEffects.push_back(sfx);
        }
    }

    void HandleKeyPress(int keyCode)
    {
        switch (keyCode)
        {
        case PIL_KEY_1:
            if (m_SoundEffects.size() > 0 && m_SoundEffects[0] && m_SoundEffects[0]->IsLoaded())
            {
                m_SoundEffects[0]->Stop();
                m_SoundEffects[0]->Play();
                PIL_INFO("Playing SFX 1");
            }
            break;

        case PIL_KEY_2:
            if (m_SoundEffects.size() > 1 && m_SoundEffects[1] && m_SoundEffects[1]->IsLoaded())
            {
                m_SoundEffects[1]->Stop();
                m_SoundEffects[1]->Play();
                PIL_INFO("Playing SFX 2");
            }
            break;

        case PIL_KEY_3:
            if (m_SoundEffects.size() > 2 && m_SoundEffects[2] && m_SoundEffects[2]->IsLoaded())
            {
                m_SoundEffects[2]->Stop();
                m_SoundEffects[2]->Play();
                PIL_INFO("Playing SFX 3");
            }
            break;

        case PIL_KEY_4:
            if (m_SoundEffects.size() > 3 && m_SoundEffects[3] && m_SoundEffects[3]->IsLoaded())
            {
                m_SoundEffects[3]->Stop();
                m_SoundEffects[3]->Play();
                PIL_INFO("Playing SFX 4");
            }
            break;

        case PIL_KEY_SPACE:
            if (m_BackgroundMusic && m_BackgroundMusic->IsLoaded())
            {
                if (m_BackgroundMusic->IsPlaying())
                {
                    m_BackgroundMusic->Pause();
                    PIL_INFO("Music paused");
                }
                else
                {
                    m_BackgroundMusic->Play();
                    PIL_INFO("Music playing");
                }
            }
            break;

        case PIL_KEY_M:
            ToggleMute();
            break;

        case PIL_KEY_R:
            m_RotateListener = !m_RotateListener;
            if (!m_RotateListener)
            {
                // Reset to forward-facing
                Pillar::AudioEngine::SetListenerOrientation(
                    glm::vec3(0.0f, 0.0f, -1.0f),
                    glm::vec3(0.0f, 1.0f, 0.0f)
                );
                m_ListenerForward = glm::vec3(0.0f, 0.0f, -1.0f);
                m_ListenerRotation = 0.0f;
            }
            PIL_INFO("Listener rotation: {}", m_RotateListener ? "ON" : "OFF");
            break;
        }
    }

    void ToggleMute()
    {
        m_Muted = !m_Muted;
        if (m_Muted)
        {
            m_PreviousMasterVolume = Pillar::AudioEngine::GetMasterVolume();
            Pillar::AudioEngine::SetMasterVolume(0.0f);
            PIL_INFO("Audio muted");
        }
        else
        {
            Pillar::AudioEngine::SetMasterVolume(m_PreviousMasterVolume);
            PIL_INFO("Audio unmuted");
        }
    }

private:
    Pillar::OrthographicCameraController m_CameraController;
    float m_Time = 0.0f;

    // Audio
    std::shared_ptr<Pillar::AudioClip> m_BackgroundMusic;
    std::vector<std::shared_ptr<Pillar::AudioClip>> m_SoundEffects;
    std::vector<glm::vec3> m_SoundPositions;

    // 3D Audio demo
    bool m_MovingSoundEnabled = false;
    float m_MovingSoundRadius = 5.0f;
    float m_MovingSoundSpeed = 1.0f;
    glm::vec3 m_MovingSoundPosition = glm::vec3(0.0f);
    glm::vec3 m_MovingSoundVelocity = glm::vec3(0.0f);
    bool m_EnableDopplerEffect = true;

    // Attenuation settings
    float m_MinDistance = 1.0f;
    float m_MaxDistance = 20.0f;
    float m_RolloffFactor = 1.0f;

    // Listener orientation
    bool m_RotateListener = false;
    float m_ListenerRotation = 0.0f;
    float m_ListenerRotationSpeed = 1.0f;
    glm::vec3 m_ListenerForward = glm::vec3(0.0f, 0.0f, -1.0f);

    // Volume control
    bool m_Muted = false;
    float m_PreviousMasterVolume = 1.0f;
};
