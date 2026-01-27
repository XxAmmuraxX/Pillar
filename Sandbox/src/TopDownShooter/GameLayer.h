#pragma once

#include <Pillar/Layer.h>
#include <Pillar/ECS/Scene.h>
#include <Pillar/ECS/Entity.h>
#include <Pillar/ECS/Systems/PhysicsSystem.h>
#include <Pillar/ECS/Systems/PhysicsSyncSystem.h>
#include <Pillar/ECS/Systems/VelocityIntegrationSystem.h>
#include <Pillar/ECS/Systems/BulletCollisionSystem.h>
#include <Pillar/Renderer/Renderer2D.h>
#include <Pillar/Renderer/OrthographicCameraController.h>
#include <Pillar/Application.h>
#include <imgui.h>
#include <memory>

#include "Systems/PlayerMovementSystem.h"
#include "Systems/WeaponSystem.h"
#include "Systems/BulletLifetimeSystem.h"
#include "Systems/EnemyAISystem.h"
#include "Systems/DamageSystem.h"
#include "Systems/PowerUpSystem.h"
#include "Systems/EffectSystems.h"
#include "Utilities/EntityFactory.h"
#include "Utilities/WaveManager.h"
#include "Utilities/CameraShake.h"
#include "Utilities/EffectFactory.h"
#include "Components/EnemyComponent.h"
#include "Components/PowerUpComponent.h"
#include "Components/EffectComponents.h"

namespace Game {

    class GameLayer : public Pillar::Layer
    {
    public:
        GameLayer()
            : Layer("TopDownShooter")
        {
        }

        ~GameLayer() override
        {
            OnDetach();
        }

        void OnAttach() override
        {
            PIL_INFO("GameLayer::OnAttach");

            // Get window dimensions
            auto& window = Pillar::Application::Get().GetWindow();
            m_WindowWidth = static_cast<float>(window.GetWidth());
            m_WindowHeight = static_cast<float>(window.GetHeight());
            float aspectRatio = m_WindowWidth / m_WindowHeight;

            // 1. Create Scene
            m_Scene = std::make_unique<Pillar::Scene>("TopDownShooter");

            // 2. Initialize Physics (ZERO GRAVITY for top-down!)
            m_PhysicsSystem = new Pillar::PhysicsSystem(glm::vec2(0.0f, 0.0f));
            m_PhysicsSystem->OnAttach(m_Scene.get());
            m_Scene->SetPhysicsSystem(m_PhysicsSystem);

            // 2b. Initialize Physics Sync System (syncs Box2D positions back to ECS)
            m_PhysicsSyncSystem = new Pillar::PhysicsSyncSystem();
            m_PhysicsSyncSystem->OnAttach(m_Scene.get());

            // 3. Initialize Camera
            m_CameraController = std::make_unique<Pillar::OrthographicCameraController>(
                aspectRatio,
                false  // No rotation control
            );
            m_CameraController->SetZoomLevel(8.0f);  // Zoom out to see arena
            m_CameraController->SetKeyboardControlEnabled(false);  // We'll handle player movement ourselves

            // 4. Initialize Player Movement System
            m_PlayerMovementSystem = new PlayerMovementSystem(
                &m_CameraController->GetCamera(),
                m_WindowWidth,
                m_WindowHeight
            );
            m_PlayerMovementSystem->OnAttach(m_Scene.get());

            // 5. Initialize Weapon System
            m_WeaponSystem = new WeaponSystem();
            m_WeaponSystem->OnAttach(m_Scene.get());

            // 6. Initialize Velocity Integration System (for bullets)
            m_VelocitySystem = new Pillar::VelocityIntegrationSystem();
            m_VelocitySystem->OnAttach(m_Scene.get());

            // 7. Initialize Bullet Lifetime System
            m_BulletLifetimeSystem = new BulletLifetimeSystem();
            m_BulletLifetimeSystem->OnAttach(m_Scene.get());

            // 8. Initialize Bullet Collision System (for hit detection)
            m_BulletCollisionSystem = new Pillar::BulletCollisionSystem(m_PhysicsSystem);
            m_BulletCollisionSystem->OnAttach(m_Scene.get());

            // 9. Initialize Enemy AI System
            m_EnemyAISystem = new EnemyAISystem();
            m_EnemyAISystem->OnAttach(m_Scene.get());

            // 10. Initialize Damage System (connects to BulletCollisionSystem)
            m_DamageSystem = new DamageSystem(m_BulletCollisionSystem);
            m_DamageSystem->OnAttach(m_Scene.get());

            // 11. Initialize Power-Up System
            m_PowerUpSystem = new PowerUpSystem();
            m_PowerUpSystem->OnAttach(m_Scene.get());

            // 12. Initialize Effect Systems
            m_FlashSystem = new FlashSystem();
            m_FlashSystem->OnAttach(m_Scene.get());
            m_TemporaryCleanupSystem = new TemporaryCleanupSystem();
            m_TemporaryCleanupSystem->OnAttach(m_Scene.get());

            // 13. Initialize Wave Manager
            m_WaveManager.Init(30.0f, 16.0f);
            m_WaveManager.SetSpawnCallback([this](const glm::vec2& pos, EnemyType type) {
                EntityFactory::CreateEnemy(*m_Scene, pos, type);
            });

            // 14. Set up DamageSystem callbacks for effects
            m_DamageSystem->SetOnEnemyHit([this](const glm::vec2& pos, const glm::vec2& bulletDir) {
                EffectFactory::SpawnHitParticles(*m_Scene, pos, bulletDir);
                m_CameraShake.ShakeSmall();
            });
            m_DamageSystem->SetOnEnemyKilled([this](const glm::vec2& pos, const glm::vec4& color) {
                EffectFactory::SpawnDeathParticles(*m_Scene, pos, color);
                m_CameraShake.ShakeMedium();
            });
            m_DamageSystem->SetOnPlayerHit([this]() {
                m_CameraShake.ShakeLarge();
            });
            m_DamageSystem->SetOnGameOver([this]() {
                m_IsGameOver = true;
                PIL_INFO("===== GAME OVER =====");
                PIL_INFO("Wave Reached: {}", m_WaveManager.GetCurrentWave());
            });

            // 15. Create Arena Bounds
            CreateArenaBounds(30.0f, 16.0f);

            // 16. Create Player
            m_PlayerEntity = EntityFactory::CreatePlayer(*m_Scene, glm::vec2(0.0f, 0.0f));

            // Waves will spawn enemies automatically
            // SpawnTestEnemies();  // Disabled - using wave system now

            PIL_INFO("GameLayer initialized successfully");
        }

        void OnDetach() override
        {
            PIL_INFO("GameLayer::OnDetach");

            // Clean up systems (reverse order of creation)
            if (m_TemporaryCleanupSystem)
            {
                m_TemporaryCleanupSystem->OnDetach();
                delete m_TemporaryCleanupSystem;
                m_TemporaryCleanupSystem = nullptr;
            }

            if (m_FlashSystem)
            {
                m_FlashSystem->OnDetach();
                delete m_FlashSystem;
                m_FlashSystem = nullptr;
            }

            if (m_PowerUpSystem)
            {
                m_PowerUpSystem->OnDetach();
                delete m_PowerUpSystem;
                m_PowerUpSystem = nullptr;
            }

            if (m_DamageSystem)
            {
                m_DamageSystem->OnDetach();
                delete m_DamageSystem;
                m_DamageSystem = nullptr;
            }

            if (m_EnemyAISystem)
            {
                m_EnemyAISystem->OnDetach();
                delete m_EnemyAISystem;
                m_EnemyAISystem = nullptr;
            }

            if (m_BulletCollisionSystem)
            {
                m_BulletCollisionSystem->OnDetach();
                delete m_BulletCollisionSystem;
                m_BulletCollisionSystem = nullptr;
            }

            if (m_BulletLifetimeSystem)
            {
                m_BulletLifetimeSystem->OnDetach();
                delete m_BulletLifetimeSystem;
                m_BulletLifetimeSystem = nullptr;
            }

            if (m_VelocitySystem)
            {
                m_VelocitySystem->OnDetach();
                delete m_VelocitySystem;
                m_VelocitySystem = nullptr;
            }

            if (m_WeaponSystem)
            {
                m_WeaponSystem->OnDetach();
                delete m_WeaponSystem;
                m_WeaponSystem = nullptr;
            }

            if (m_PlayerMovementSystem)
            {
                m_PlayerMovementSystem->OnDetach();
                delete m_PlayerMovementSystem;
                m_PlayerMovementSystem = nullptr;
            }

            if (m_PhysicsSyncSystem)
            {
                m_PhysicsSyncSystem->OnDetach();
                delete m_PhysicsSyncSystem;
                m_PhysicsSyncSystem = nullptr;
            }

            if (m_PhysicsSystem)
            {
                m_PhysicsSystem->OnDetach();
                delete m_PhysicsSystem;
                m_PhysicsSystem = nullptr;
            }

            // Scene will be automatically destroyed by unique_ptr
        }

        void OnUpdate(float dt) override
        {
            // Skip updates if game over
            if (m_IsGameOver)
            {
                UpdateCamera(dt);
                RenderScene();
                return;
            }

            // 1. Wave Manager (spawns enemies)
            int enemyCount = CountEnemies();
            m_WaveManager.OnUpdate(dt, enemyCount);

            // 2. Input Processing (player movement + aiming)
            m_PlayerMovementSystem->OnUpdate(dt);

            // 3. Enemy AI (chase, attack)
            m_EnemyAISystem->OnUpdate(dt);

            // 4. Weapon System (handles firing, bullet spawning)
            m_WeaponSystem->OnUpdate(dt);

            // 5. Physics Step (Box2D for heavy entities)
            m_PhysicsSystem->OnUpdate(dt);

            // 5b. Sync Box2D positions back to ECS transforms (CRITICAL for movement!)
            m_PhysicsSyncSystem->OnUpdate(dt);

            // 6. Velocity Integration (for light entities like bullets)
            m_VelocitySystem->OnUpdate(dt);

            // 7. Bullet Collision System (raycast + circle collision detection)
            m_BulletCollisionSystem->OnUpdate(dt);

            // 8. Damage System (invulnerability timers, death processing)
            m_DamageSystem->OnUpdate(dt);

            // 9. Power-Up System (collection, effects)
            m_PowerUpSystem->OnUpdate(dt);

            // 10. Flash System (damage flash effects)
            m_FlashSystem->OnUpdate(dt);

            // 11. Bullet Lifetime (cleanup expired bullets)
            m_BulletLifetimeSystem->OnUpdate(dt);

            // 12. Temporary Cleanup (effect entities)
            m_TemporaryCleanupSystem->OnUpdate(dt);

            // 13. Camera Shake
            m_CameraShake.OnUpdate(dt);

            // 14. Camera Update (follow player)
            UpdateCamera(dt);

            // 15. Rendering
            RenderScene();
        }

        void OnEvent(Pillar::Event& event) override
        {
            m_CameraController->OnEvent(event);
        }

        void OnImGuiRender() override
        {
            // Show game over screen if dead
            if (m_IsGameOver)
            {
                ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Always);
                ImGui::SetNextWindowPos(
                    ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f,
                           ImGui::GetIO().DisplaySize.y * 0.5f),
                    ImGuiCond_Always,
                    ImVec2(0.5f, 0.5f)
                );

                ImGui::Begin("Game Over", nullptr,
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoCollapse);

                ImGui::PushFont(ImGui::GetFont());
                ImGui::SetWindowFontScale(2.0f);
                ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "GAME OVER");
                ImGui::PopFont();

                ImGui::Separator();
                ImGui::Text("Wave Reached: %d", m_WaveManager.GetCurrentWave());
                ImGui::Text("");

                if (ImGui::Button("Restart", ImVec2(150, 40)))
                {
                    RestartGame();
                }
                ImGui::SameLine();
                if (ImGui::Button("Quit", ImVec2(150, 40)))
                {
                    Pillar::Application::Get().Close();
                }

                ImGui::End();
                return;
            }

            ImGui::Begin("Top-Down Shooter - Phase 4");

            ImGui::Text("=== Game Stats ===");
            if (m_PlayerEntity.IsValid())
            {
                auto& transform = m_PlayerEntity.GetComponent<Pillar::TransformComponent>();
                auto& health = m_PlayerEntity.GetComponent<Pillar::HealthComponent>();
                auto& weapon = m_PlayerEntity.GetComponent<WeaponComponent>();

                ImGui::Text("Player Position: (%.2f, %.2f)", transform.Position.x, transform.Position.y);
                ImGui::Text("Player Health: %.0f / %.0f", health.CurrentHealth, health.MaxHealth);
                ImGui::ProgressBar(health.GetHealthPercent(), ImVec2(-1, 0), "HP");

                // Show active buffs
                if (auto* buffs = m_PlayerEntity.TryGetComponent<PlayerBuffsComponent>())
                {
                    if (!buffs->ActiveEffects.empty())
                    {
                        ImGui::Text("Active Buffs:");
                        for (const auto& effect : buffs->ActiveEffects)
                        {
                            const char* name = "Unknown";
                            switch (effect.Type)
                            {
                                case PowerUpType::SpeedBoost: name = "Speed"; break;
                                case PowerUpType::FireRateUp: name = "Fire Rate"; break;
                                case PowerUpType::DamageUp: name = "Damage"; break;
                                case PowerUpType::Shield: name = "Shield"; break;
                                default: break;
                            }
                            ImGui::BulletText("%s: %.1fs", name, effect.RemainingDuration);
                        }
                    }
                }

                ImGui::Separator();
                ImGui::Text("=== Weapon ===");
                ImGui::Text("Weapon: %s", weapon.WeaponName.c_str());
                ImGui::Text("Damage: %.1f", weapon.Damage);
                ImGui::Text("Fire Rate: %.1f/s", weapon.FireRate);
            }

            ImGui::Separator();
            ImGui::Text("=== Wave System ===");
            ImGui::Text("Current Wave: %d", m_WaveManager.GetCurrentWave());
            int enemyCount = CountEnemies();
            ImGui::Text("Enemies Alive: %d", enemyCount);
            if (m_WaveManager.IsSpawning())
            {
                ImGui::Text("Spawning: %d remaining", m_WaveManager.GetEnemiesToSpawn());
            }
            else if (m_WaveManager.GetTimeUntilNextWave() > 0.0f)
            {
                ImGui::Text("Next Wave in: %.1fs", m_WaveManager.GetTimeUntilNextWave());
            }

            ImGui::Separator();
            ImGui::Text("Total Entities: %zu", m_Scene->GetEntityCount());
            ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

            ImGui::Separator();
            ImGui::Text("=== Controls ===");
            ImGui::BulletText("WASD: Move");
            ImGui::BulletText("Mouse: Aim");
            ImGui::BulletText("Left Click: Shoot");
            ImGui::BulletText("Space: Dash");
            ImGui::BulletText("Scroll: Zoom");

            ImGui::Separator();
            ImGui::Text("=== Debug Spawns ===");
            if (ImGui::Button("Spawn Enemy (Chaser)"))
            {
                glm::vec2 spawnPos(RandomFloat(-10.0f, 10.0f), RandomFloat(-5.0f, 5.0f));
                EntityFactory::CreateEnemy(*m_Scene, spawnPos, EnemyType::Chaser);
            }
            ImGui::SameLine();
            if (ImGui::Button("Spawn Health"))
            {
                glm::vec2 spawnPos(RandomFloat(-8.0f, 8.0f), RandomFloat(-4.0f, 4.0f));
                EntityFactory::CreatePowerUp(*m_Scene, spawnPos, PowerUpType::Health);
            }
            if (ImGui::Button("Spawn Speed Boost"))
            {
                glm::vec2 spawnPos(RandomFloat(-8.0f, 8.0f), RandomFloat(-4.0f, 4.0f));
                EntityFactory::CreatePowerUp(*m_Scene, spawnPos, PowerUpType::SpeedBoost);
            }
            ImGui::SameLine();
            if (ImGui::Button("Spawn Fire Rate"))
            {
                glm::vec2 spawnPos(RandomFloat(-8.0f, 8.0f), RandomFloat(-4.0f, 4.0f));
                EntityFactory::CreatePowerUp(*m_Scene, spawnPos, PowerUpType::FireRateUp);
            }
            if (ImGui::Button("Spawn Damage Up"))
            {
                glm::vec2 spawnPos(RandomFloat(-8.0f, 8.0f), RandomFloat(-4.0f, 4.0f));
                EntityFactory::CreatePowerUp(*m_Scene, spawnPos, PowerUpType::DamageUp);
            }
            ImGui::SameLine();
            if (ImGui::Button("Spawn Shield"))
            {
                glm::vec2 spawnPos(RandomFloat(-8.0f, 8.0f), RandomFloat(-4.0f, 4.0f));
                EntityFactory::CreatePowerUp(*m_Scene, spawnPos, PowerUpType::Shield);
            }

            ImGui::End();
        }

    private:
        void CreateArenaBounds(float width, float height)
        {
            float wallThickness = 1.0f;
            float halfWidth = width * 0.5f;
            float halfHeight = height * 0.5f;

            // Top
            EntityFactory::CreateWall(*m_Scene,
                glm::vec2(0.0f, halfHeight + wallThickness * 0.5f),
                glm::vec2(width + wallThickness * 2, wallThickness)
            );

            // Bottom
            EntityFactory::CreateWall(*m_Scene,
                glm::vec2(0.0f, -halfHeight - wallThickness * 0.5f),
                glm::vec2(width + wallThickness * 2, wallThickness)
            );

            // Left
            EntityFactory::CreateWall(*m_Scene,
                glm::vec2(-halfWidth - wallThickness * 0.5f, 0.0f),
                glm::vec2(wallThickness, height)
            );

            // Right
            EntityFactory::CreateWall(*m_Scene,
                glm::vec2(halfWidth + wallThickness * 0.5f, 0.0f),
                glm::vec2(wallThickness, height)
            );

            PIL_INFO("Arena bounds created: {}x{}", width, height);
        }

        void UpdateCamera(float dt)
        {
            // Follow player with camera shake
            if (m_PlayerEntity.IsValid())
            {
                auto& playerTransform = m_PlayerEntity.GetComponent<Pillar::TransformComponent>();
                auto& camera = m_CameraController->GetCamera();
                
                // Apply camera shake offset
                glm::vec2 shakeOffset = m_CameraShake.GetOffset();
                glm::vec3 targetPos = glm::vec3(
                    playerTransform.Position.x + shakeOffset.x,
                    playerTransform.Position.y + shakeOffset.y,
                    0.0f
                );
                camera.SetPosition(targetPos);
            }

            m_CameraController->OnUpdate(dt);
        }

        void RenderScene()
        {
            // Clear
            Pillar::Renderer2D::SetClearColor(glm::vec4(0.05f, 0.05f, 0.1f, 1.0f));
            Pillar::Renderer2D::Clear();

            // Begin scene
            Pillar::Renderer2D::BeginScene(m_CameraController->GetCamera());

            // Render all entities with TransformComponent and SpriteComponent
            auto& registry = m_Scene->GetRegistry();
            auto view = registry.view<Pillar::TransformComponent, Pillar::SpriteComponent>();

            for (auto entity : view)
            {
                auto& transform = view.get<Pillar::TransformComponent>(entity);
                auto& sprite = view.get<Pillar::SpriteComponent>(entity);

                if (!sprite.Visible) continue;

                // Render sprite
                Pillar::Renderer2D::DrawRotatedQuad(
                    transform.Position,
                    sprite.Size,
                    transform.Rotation,
                    sprite.Color
                );
            }

            // End scene
            Pillar::Renderer2D::EndScene();
        }

        void SpawnTestEnemies()
        {
            // Spawn a few test enemies around the player
            EntityFactory::CreateEnemy(*m_Scene, glm::vec2(5.0f, 0.0f), EnemyType::Chaser);
            EntityFactory::CreateEnemy(*m_Scene, glm::vec2(-5.0f, 0.0f), EnemyType::Chaser);
            EntityFactory::CreateEnemy(*m_Scene, glm::vec2(0.0f, 5.0f), EnemyType::Shooter);
            EntityFactory::CreateEnemy(*m_Scene, glm::vec2(8.0f, 3.0f), EnemyType::Swarm);
            EntityFactory::CreateEnemy(*m_Scene, glm::vec2(-8.0f, -3.0f), EnemyType::Swarm);

            PIL_INFO("Spawned {} test enemies", 5);
        }

        int CountEnemies() const
        {
            auto& registry = m_Scene->GetRegistry();
            auto view = registry.view<EnemyComponent, Pillar::HealthComponent>();

            int count = 0;
            for (auto entity : view)
            {
                auto& health = view.get<Pillar::HealthComponent>(entity);
                if (!health.IsDead)
                    count++;
            }
            return count;
        }

        float RandomFloat(float min, float max)
        {
            return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
        }

    private:
        std::unique_ptr<Pillar::Scene> m_Scene;
        std::unique_ptr<Pillar::OrthographicCameraController> m_CameraController;

        // Systems
        Pillar::PhysicsSystem* m_PhysicsSystem = nullptr;
        Pillar::PhysicsSyncSystem* m_PhysicsSyncSystem = nullptr;
        Pillar::VelocityIntegrationSystem* m_VelocitySystem = nullptr;
        Pillar::BulletCollisionSystem* m_BulletCollisionSystem = nullptr;
        PlayerMovementSystem* m_PlayerMovementSystem = nullptr;
        WeaponSystem* m_WeaponSystem = nullptr;
        BulletLifetimeSystem* m_BulletLifetimeSystem = nullptr;
        EnemyAISystem* m_EnemyAISystem = nullptr;
        DamageSystem* m_DamageSystem = nullptr;
        PowerUpSystem* m_PowerUpSystem = nullptr;
        FlashSystem* m_FlashSystem = nullptr;
        TemporaryCleanupSystem* m_TemporaryCleanupSystem = nullptr;

        // Wave Manager & Effects
        WaveManager m_WaveManager;
        CameraShake m_CameraShake;

        // Entities
        Pillar::Entity m_PlayerEntity;

        // Window dimensions
        float m_WindowWidth = 1280.0f;
        float m_WindowHeight = 720.0f;

        // Game state
        bool m_IsGameOver = false;

        void RestartGame()
        {
            // Destroy all entities
            auto allEntities = m_Scene->GetAllEntities();
            for (auto entity : allEntities)
            {
                m_Scene->DestroyEntity(entity);
            }

            // Recreate arena
            CreateArenaBounds(30.0f, 16.0f);

            // Recreate player
            m_PlayerEntity = EntityFactory::CreatePlayer(*m_Scene, glm::vec2(0.0f, 0.0f));

            // Reset wave manager
            m_WaveManager.Reset();

            // Reset game state
            m_IsGameOver = false;

            PIL_INFO("Game restarted");
        }
    };

} // namespace Game
