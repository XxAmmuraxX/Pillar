#pragma once

#include "Pillar.h"
#include "Pillar/Renderer/Renderer2DBackend.h"
#include "Pillar/ECS/SceneManager.h"
#include "Pillar/ECS/SceneSerializer.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/Components/Core/TagComponent.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Physics/RigidbodyComponent.h"
#include "Pillar/ECS/Components/Physics/ColliderComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"
#include "Pillar/ECS/Components/Gameplay/XPGemComponent.h"
#include "Pillar/ECS/Components/Audio/AudioSourceComponent.h"
#include "Pillar/ECS/Components/Audio/AudioListenerComponent.h"
#include "Pillar/ECS/Systems/AudioSystem.h"
#include "Pillar/Renderer/Renderer2D.h"
#include "Pillar/Audio/AudioEngine.h"
#include <imgui.h>
#include <memory>

/**
 * @brief Demo layer showcasing the Scene System
 * 
 * Features demonstrated:
 * 1. SceneManager - Creating and switching between scenes
 * 2. SceneSerializer - Saving/loading scenes to JSON
 * 3. Scene transitions with callbacks
 * 4. Entity queries (by name, UUID)
 * 5. Audio system integration with scene entities
 */
class SceneDemoLayer : public Pillar::Layer
{
public:
    SceneDemoLayer()
        : Layer("SceneDemoLayer"),
          m_CameraController(16.0f / 9.0f, true)
    {
    }

    void OnAttach() override
    {
        Layer::OnAttach();
        PIL_INFO("Scene Demo Layer attached!");

        auto& sceneManager = Pillar::SceneManager::Get();

        // Create multiple scenes programmatically
        CreateMainMenuScene();
        CreateGameScene();
        CreatePauseMenuScene();

        // Load audio demo scene from file
        if (sceneManager.LoadScene("scenes/audio_demo.scene.json", "AudioDemo"))
        {
            PIL_INFO("Audio Demo scene loaded from file");
        }
        else
        {
            PIL_WARN("Could not load Audio Demo scene - file may not exist");
        }

        // Set callback for scene changes
        sceneManager.SetOnSceneChangeCallback([this](const std::string& from, const std::string& to) {
            PIL_INFO("Scene changed from '{}' to '{}'", from, to);
            OnSceneChanged();
        });

        // Initialize audio system
        m_AudioSystem = new Pillar::AudioSystem();

        // Start with main menu
        sceneManager.SetActiveScene("MainMenu");
        OnSceneChanged();
    }

    void OnDetach() override
    {
        delete m_AudioSystem;
        Pillar::SceneManager::Get().Clear();
        Layer::OnDetach();
    }

    void OnUpdate(float dt) override
    {
        m_CameraController.OnUpdate(dt);

        auto& sceneManager = Pillar::SceneManager::Get();
        sceneManager.OnUpdate(dt);

        // Update audio system for current scene
        auto activeScene = sceneManager.GetActiveScene();
        if (activeScene && m_AudioSystem)
        {
            m_AudioSystem->OnUpdate(dt);
        }

        // Render
        Pillar::Renderer::SetClearColor({ 0.1f, 0.1f, 0.15f, 1.0f });
        Pillar::Renderer::Clear();

        Pillar::Renderer2D::BeginScene(m_CameraController.GetCamera());
        
        if (activeScene)
        {
            DrawScene(activeScene);
        }

        Pillar::Renderer2D::EndScene();
    }

    void OnEvent(Pillar::Event& event) override
    {
        m_CameraController.OnEvent(event);

        // Handle keyboard shortcuts for audio control
        if (event.GetEventType() == Pillar::EventType::KeyPressed)
        {
            auto& keyEvent = static_cast<Pillar::KeyPressedEvent&>(event);
            if (keyEvent.GetRepeatCount() == 0)
            {
                HandleAudioKeyPress(keyEvent.GetKeyCode());
            }
        }
    }

    void OnImGuiRender() override
    {
        auto& sceneManager = Pillar::SceneManager::Get();

        ImGui::Begin("Scene System Demo");

        // Current scene info
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Scene System");
        ImGui::Separator();

        ImGui::Text("Active Scene: %s", sceneManager.GetActiveSceneName().c_str());
        
        if (auto scene = sceneManager.GetActiveScene())
        {
            ImGui::Text("Entity Count: %zu", scene->GetEntityCount());
            ImGui::Text("Scene State: %s", 
                scene->IsPlaying() ? "Playing" : 
                scene->IsPaused() ? "Paused" : "Edit");
        }

        ImGui::Separator();
        ImGui::Text("Available Scenes:");

        // Scene selection buttons
        for (const auto& name : sceneManager.GetSceneNames())
        {
            bool isActive = (name == sceneManager.GetActiveSceneName());
            
            if (isActive)
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
            
            if (ImGui::Button(name.c_str(), ImVec2(150, 0)))
            {
                sceneManager.RequestSceneChange(name);
            }
            
            if (isActive)
                ImGui::PopStyleColor();
            
            ImGui::SameLine();
            ImGui::Text(isActive ? "(Active)" : "");
        }

        ImGui::Separator();

        // Audio Controls Section
        RenderAudioControls();

        ImGui::Separator();

        // Save/Load section
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Save/Load");
        
        static char filepath[256] = "assets/scenes/saved.scene.json";
        ImGui::InputText("File Path", filepath, sizeof(filepath));

        if (ImGui::Button("Save Scene"))
        {
            if (sceneManager.SaveScene(filepath))
            {
                m_StatusMessage = "Scene saved successfully!";
                m_StatusColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
            }
            else
            {
                m_StatusMessage = "Failed to save scene!";
                m_StatusColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
            }
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Load Scene"))
        {
            if (sceneManager.LoadScene(filepath, "LoadedScene"))
            {
                sceneManager.SetActiveScene("LoadedScene");
                m_StatusMessage = "Scene loaded successfully!";
                m_StatusColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
            }
            else
            {
                m_StatusMessage = "Failed to load scene!";
                m_StatusColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
            }
        }

        // Status message
        if (!m_StatusMessage.empty())
        {
            ImGui::TextColored(m_StatusColor, "%s", m_StatusMessage.c_str());
        }

        ImGui::Separator();

        // Entity Inspector
        ImGui::TextColored(ImVec4(0.8f, 0.4f, 1.0f, 1.0f), "Entity Inspector");
        
        if (auto scene = sceneManager.GetActiveScene())
        {
            auto entities = scene->GetAllEntities();
            
            static int selectedEntityIndex = -1;
            
            ImGui::BeginChild("EntityList", ImVec2(0, 150), true);
            int index = 0;
            for (auto& entity : entities)
            {
                if (!entity) continue;
                
                auto& tag = entity.GetComponent<Pillar::TagComponent>();
                bool isSelected = (index == selectedEntityIndex);
                
                if (ImGui::Selectable(tag.Tag.c_str(), isSelected))
                {
                    selectedEntityIndex = index;
                }
                index++;
            }
            ImGui::EndChild();

            // Show selected entity details
            if (selectedEntityIndex >= 0 && selectedEntityIndex < (int)entities.size())
            {
                auto& selectedEntity = entities[selectedEntityIndex];
                if (selectedEntity)
                {
                    ShowEntityDetails(selectedEntity);
                }
            }
        }

        ImGui::Separator();

        // Controls
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Controls:");
        ImGui::BulletText("WASD: Move camera");
        ImGui::BulletText("Q/E: Rotate camera");
        ImGui::BulletText("Mouse Wheel: Zoom");
        ImGui::BulletText("1-4: Play audio sources (if present in scene)");
        ImGui::BulletText("M: Mute/Unmute master volume");

        ImGui::End();
    }

private:
    void OnSceneChanged()
    {
        auto& sceneManager = Pillar::SceneManager::Get();
        auto activeScene = sceneManager.GetActiveScene();
        
        // Stop all audio sources from previous scene before switching
        for (auto& entity : m_AudioSourceEntities)
        {
            if (entity && entity.HasComponent<Pillar::AudioSourceComponent>())
            {
                auto& audioComp = entity.GetComponent<Pillar::AudioSourceComponent>();
                if (audioComp.Source && audioComp.Source->IsPlaying())
                {
                    audioComp.Source->Stop();
                    PIL_INFO("Stopped audio: {}", entity.GetComponent<Pillar::TagComponent>().Tag);
                }
            }
        }
        
        if (activeScene && m_AudioSystem)
        {
            m_AudioSystem->OnAttach(activeScene.get());
            
            // Find all audio source entities in the new scene
            m_AudioSourceEntities.clear();
            auto view = activeScene->GetRegistry().view<Pillar::AudioSourceComponent>();
            for (auto entity : view)
            {
                m_AudioSourceEntities.push_back(Pillar::Entity(entity, activeScene.get()));
            }
            
            PIL_INFO("Found {} audio sources in scene '{}'", 
                m_AudioSourceEntities.size(), 
                sceneManager.GetActiveSceneName());
        }
    }

    void HandleAudioKeyPress(int keyCode)
    {
        if (!Pillar::AudioEngine::IsInitialized())
            return;

        switch (keyCode)
        {
        case PIL_KEY_1:
            PlayAudioSource(0);
            break;
        case PIL_KEY_2:
            PlayAudioSource(1);
            break;
        case PIL_KEY_3:
            PlayAudioSource(2);
            break;
        case PIL_KEY_4:
            PlayAudioSource(3);
            break;
        case PIL_KEY_M:
            ToggleMute();
            break;
        }
    }

    void PlayAudioSource(size_t index)
    {
        if (index >= m_AudioSourceEntities.size())
            return;

        auto& entity = m_AudioSourceEntities[index];
        if (!entity.HasComponent<Pillar::AudioSourceComponent>())
            return;

        auto& audioComp = entity.GetComponent<Pillar::AudioSourceComponent>();
        if (audioComp.Source)
        {
            audioComp.Source->Stop();
            audioComp.Source->Play();
            
            auto& tag = entity.GetComponent<Pillar::TagComponent>();
            PIL_INFO("Playing audio: {}", tag.Tag);
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

    void RenderAudioControls()
    {
        if (!Pillar::AudioEngine::IsInitialized())
        {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Audio System: Not Initialized");
            return;
        }

        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Audio Controls");
        ImGui::Separator();

        // Master volume
        float masterVolume = Pillar::AudioEngine::GetMasterVolume();
        if (ImGui::SliderFloat("Master Volume", &masterVolume, 0.0f, 1.0f))
        {
            Pillar::AudioEngine::SetMasterVolume(masterVolume);
        }

        if (ImGui::Button(m_Muted ? "Unmute (M)" : "Mute (M)"))
        {
            ToggleMute();
        }

        // Show audio sources in current scene
        if (!m_AudioSourceEntities.empty())
        {
            ImGui::Spacing();
            ImGui::Text("Audio Sources in Scene:");
            
            for (size_t i = 0; i < m_AudioSourceEntities.size(); ++i)
            {
                auto& entity = m_AudioSourceEntities[i];
                if (!entity.HasComponent<Pillar::AudioSourceComponent>())
                    continue;

                ImGui::PushID(static_cast<int>(i));

                auto& tag = entity.GetComponent<Pillar::TagComponent>();
                auto& audioComp = entity.GetComponent<Pillar::AudioSourceComponent>();

                ImGui::Text("%zu. %s", i + 1, tag.Tag.c_str());
                ImGui::SameLine();

                if (ImGui::Button("Play"))
                {
                    PlayAudioSource(i);
                }

                if (audioComp.Source)
                {
                    ImGui::SameLine();
                    if (ImGui::Button("Stop"))
                    {
                        audioComp.Source->Stop();
                    }

                    // Volume control
                    float volume = audioComp.Volume;
                    if (ImGui::SliderFloat("Vol", &volume, 0.0f, 1.0f))
                    {
                        audioComp.Volume = volume;
                        if (audioComp.Source)
                        {
                            audioComp.Source->SetVolume(volume);
                        }
                    }

                    // Show playback status
                    if (audioComp.Source->IsPlaying())
                    {
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Playing");
                    }
                }
                else
                {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Not Loaded");
                }

                ImGui::PopID();
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No audio sources in current scene");
            ImGui::TextWrapped("Switch to 'AudioDemo' scene to hear sounds");
        }
    }

    void CreateMainMenuScene()
    {
        auto& sceneManager = Pillar::SceneManager::Get();
        auto scene = sceneManager.CreateScene("MainMenu");

        // Title entity
        auto title = scene->CreateEntity("Title");
        auto& titleTransform = title.GetComponent<Pillar::TransformComponent>();
        titleTransform.Position = glm::vec2(0.0f, 2.0f);
        titleTransform.Scale = glm::vec2(4.0f, 1.0f);

        // Play button
        auto playBtn = scene->CreateEntity("PlayButton");
        auto& playTransform = playBtn.GetComponent<Pillar::TransformComponent>();
        playTransform.Position = glm::vec2(0.0f, 0.0f);
        playTransform.Scale = glm::vec2(2.0f, 0.5f);

        // Quit button
        auto quitBtn = scene->CreateEntity("QuitButton");
        auto& quitTransform = quitBtn.GetComponent<Pillar::TransformComponent>();
        quitTransform.Position = glm::vec2(0.0f, -1.5f);
        quitTransform.Scale = glm::vec2(2.0f, 0.5f);
    }

    void CreateGameScene()
    {
        auto& sceneManager = Pillar::SceneManager::Get();
        auto scene = sceneManager.CreateScene("GameScene");

        // Player
        auto player = scene->CreateEntity("Player");
        auto& playerTransform = player.GetComponent<Pillar::TransformComponent>();
        playerTransform.Position = glm::vec2(0.0f, 0.0f);

        // Some enemies in a circle
        for (int i = 0; i < 5; i++)
        {
            float angle = (i / 5.0f) * 2.0f * 3.14159f;
            float radius = 5.0f;
            
            auto enemy = scene->CreateEntity("Enemy");
            auto& t = enemy.GetComponent<Pillar::TransformComponent>();
            t.Position = glm::vec2(cos(angle) * radius, sin(angle) * radius);
        }

        // XP Gems
        for (int i = 0; i < 10; i++)
        {
            auto gem = scene->CreateEntity("XPGem");
            auto& t = gem.GetComponent<Pillar::TransformComponent>();
            t.Position = glm::vec2(
                (rand() % 200 - 100) / 10.0f,
                (rand() % 140 - 70) / 10.0f
            );
            gem.AddComponent<Pillar::VelocityComponent>();
            gem.AddComponent<Pillar::XPGemComponent>(rand() % 10 + 1);
        }
    }

    void CreatePauseMenuScene()
    {
        auto& sceneManager = Pillar::SceneManager::Get();
        auto scene = sceneManager.CreateScene("PauseMenu");

        // Pause text
        auto pauseText = scene->CreateEntity("PauseText");
        auto& pauseTransform = pauseText.GetComponent<Pillar::TransformComponent>();
        pauseTransform.Position = glm::vec2(0.0f, 2.0f);
        pauseTransform.Scale = glm::vec2(3.0f, 0.8f);

        // Resume button
        auto resumeBtn = scene->CreateEntity("ResumeButton");
        auto& resumeTransform = resumeBtn.GetComponent<Pillar::TransformComponent>();
        resumeTransform.Position = glm::vec2(0.0f, 0.0f);
        resumeTransform.Scale = glm::vec2(2.0f, 0.5f);

        // Main Menu button
        auto menuBtn = scene->CreateEntity("MainMenuButton");
        auto& menuTransform = menuBtn.GetComponent<Pillar::TransformComponent>();
        menuTransform.Position = glm::vec2(0.0f, -1.5f);
        menuTransform.Scale = glm::vec2(2.0f, 0.5f);
    }

    void DrawScene(std::shared_ptr<Pillar::Scene> scene)
    {
        auto view = scene->GetRegistry().view<Pillar::TagComponent, Pillar::TransformComponent>();
        
        for (auto entity : view)
        {
            auto& tag = view.get<Pillar::TagComponent>(entity);
            auto& transform = view.get<Pillar::TransformComponent>(entity);

            glm::vec4 color = { 0.5f, 0.5f, 0.5f, 1.0f };
            glm::vec2 size = transform.Scale;

            // Color based on entity type
            if (tag.Tag == "Player")
            {
                color = { 0.2f, 0.8f, 0.3f, 1.0f };
                size = { 1.0f, 1.0f };
            }
            else if (tag.Tag == "Enemy")
            {
                color = { 0.9f, 0.2f, 0.2f, 1.0f };
                size = { 0.8f, 0.8f };
            }
            else if (tag.Tag == "XPGem")
            {
                color = { 0.9f, 0.9f, 0.2f, 1.0f };
                size = { 0.3f, 0.3f };
            }
            else if (tag.Tag == "Title" || tag.Tag == "PauseText")
            {
                color = { 0.8f, 0.8f, 0.2f, 1.0f };
            }
            else if (tag.Tag.find("Button") != std::string::npos)
            {
                color = { 0.3f, 0.5f, 0.8f, 1.0f };
            }
            else if (tag.Tag.find("Wall") != std::string::npos)
            {
                color = { 0.3f, 0.3f, 0.3f, 1.0f };
            }
            else if (tag.Tag == "Listener")
            {
                color = { 0.2f, 0.8f, 0.2f, 1.0f };
                size = { 0.3f, 0.3f };
            }
            else if (tag.Tag.find("SFX_") == 0 || tag.Tag == "BackgroundMusic")
            {
                // Audio source entities - orange/yellow
                color = { 1.0f, 0.6f, 0.2f, 1.0f };
                size = { 0.2f, 0.2f };
            }

            if (transform.Rotation != 0.0f)
                Pillar::Renderer2DBackend::DrawRotatedQuad(transform.Position, size, transform.Rotation, color);
            else
                Pillar::Renderer2DBackend::DrawQuad(transform.Position, size, color);
        }
    }

    void ShowEntityDetails(Pillar::Entity& entity)
    {
        ImGui::Separator();
        ImGui::Text("Selected Entity Details:");

        auto& tag = entity.GetComponent<Pillar::TagComponent>();
        ImGui::Text("Name: %s", tag.Tag.c_str());

        if (entity.HasComponent<Pillar::UUIDComponent>())
        {
            auto& uuid = entity.GetComponent<Pillar::UUIDComponent>();
            ImGui::Text("UUID: %llu", uuid.UUID);
        }

        if (entity.HasComponent<Pillar::TransformComponent>())
        {
            auto& transform = entity.GetComponent<Pillar::TransformComponent>();
            ImGui::Text("Position: (%.2f, %.2f)", transform.Position.x, transform.Position.y);
            ImGui::Text("Rotation: %.2f rad", transform.Rotation);
            ImGui::Text("Scale: (%.2f, %.2f)", transform.Scale.x, transform.Scale.y);
        }

        if (entity.HasComponent<Pillar::VelocityComponent>())
        {
            auto& vel = entity.GetComponent<Pillar::VelocityComponent>();
            ImGui::Text("Velocity: (%.2f, %.2f)", vel.Velocity.x, vel.Velocity.y);
        }

        if (entity.HasComponent<Pillar::XPGemComponent>())
        {
            auto& gem = entity.GetComponent<Pillar::XPGemComponent>();
            ImGui::Text("XP Value: %d", gem.XPValue);
            ImGui::Text("Attraction Radius: %.2f", gem.AttractionRadius);
        }

        if (entity.HasComponent<Pillar::AudioSourceComponent>())
        {
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "Audio Source");
            auto& audioSrc = entity.GetComponent<Pillar::AudioSourceComponent>();
            ImGui::Text("Audio File: %s", audioSrc.AudioFile.c_str());
            ImGui::Text("Volume: %.2f", audioSrc.Volume);
            ImGui::Text("Pitch: %.2f", audioSrc.Pitch);
            ImGui::Text("Loop: %s", audioSrc.Loop ? "Yes" : "No");
            ImGui::Text("Play On Awake: %s", audioSrc.PlayOnAwake ? "Yes" : "No");
            ImGui::Text("3D Audio: %s", audioSrc.Is3D ? "Yes" : "No");
            if (audioSrc.Is3D)
            {
                ImGui::Text("Min Distance: %.2f", audioSrc.MinDistance);
                ImGui::Text("Max Distance: %.2f", audioSrc.MaxDistance);
                ImGui::Text("Rolloff Factor: %.2f", audioSrc.RolloffFactor);
            }

            // Add play/stop buttons in inspector
            if (audioSrc.Source)
            {
                if (ImGui::Button("Play##inspector"))
                {
                    audioSrc.Source->Stop();
                    audioSrc.Source->Play();
                }
                ImGui::SameLine();
                if (ImGui::Button("Stop##inspector"))
                {
                    audioSrc.Source->Stop();
                }
            }
        }

        if (entity.HasComponent<Pillar::AudioListenerComponent>())
        {
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Audio Listener");
            auto& listener = entity.GetComponent<Pillar::AudioListenerComponent>();
            ImGui::Text("Active: %s", listener.IsActive ? "Yes" : "No");
            ImGui::Text("Forward: (%.2f, %.2f, %.2f)", 
                listener.Forward.x, listener.Forward.y, listener.Forward.z);
            ImGui::Text("Up: (%.2f, %.2f, %.2f)", 
                listener.Up.x, listener.Up.y, listener.Up.z);
        }
    }

private:
    Pillar::OrthographicCameraController m_CameraController;
    std::string m_StatusMessage;
    ImVec4 m_StatusColor;

    // Audio system
    Pillar::AudioSystem* m_AudioSystem = nullptr;
    std::vector<Pillar::Entity> m_AudioSourceEntities;
    
    // Audio controls
    bool m_Muted = false;
    float m_PreviousMasterVolume = 1.0f;
};
