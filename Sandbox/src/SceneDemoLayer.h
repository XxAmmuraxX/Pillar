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
#include "Pillar/Renderer/Renderer2D.h"
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

        // Set callback for scene changes
        sceneManager.SetOnSceneChangeCallback([](const std::string& from, const std::string& to) {
            PIL_INFO("Scene changed from '{}' to '{}'", from, to);
        });

        // Start with main menu
        sceneManager.SetActiveScene("MainMenu");
    }

    void OnDetach() override
    {
        Pillar::SceneManager::Get().Clear();
        Layer::OnDetach();
    }

    void OnUpdate(float dt) override
    {
        m_CameraController.OnUpdate(dt);

        auto& sceneManager = Pillar::SceneManager::Get();
        sceneManager.OnUpdate(dt);

        // Render
        Pillar::Renderer::SetClearColor({ 0.1f, 0.1f, 0.15f, 1.0f });
        Pillar::Renderer::Clear();

        Pillar::Renderer2D::BeginScene(m_CameraController.GetCamera());
        
        auto activeScene = sceneManager.GetActiveScene();
        if (activeScene)
        {
            DrawScene(activeScene);
        }

        Pillar::Renderer2D::EndScene();
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

        ImGui::End();
    }

    void OnEvent(Pillar::Event& event) override
    {
        m_CameraController.OnEvent(event);
    }

private:
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
    }

private:
    Pillar::OrthographicCameraController m_CameraController;
    std::string m_StatusMessage;
    ImVec4 m_StatusColor;
};
