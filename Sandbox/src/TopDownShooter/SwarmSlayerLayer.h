#pragma once

#include <Pillar/Layer.h>
#include <Pillar/ECS/Scene.h>
#include <Pillar/ECS/Entity.h>
#include <Pillar/ECS/Systems/PhysicsSystem.h>
#include <Pillar/ECS/Systems/PhysicsSyncSystem.h>
#include <Pillar/ECS/Systems/VelocityIntegrationSystem.h>
#include <Pillar/ECS/Systems/BulletCollisionSystem.h>
#include <Pillar/ECS/Systems/AnimationSystem.h>
#include <Pillar/Renderer/Renderer2D.h>
#include <Pillar/Renderer/OrthographicCameraController.h>
#include <Pillar/Application.h>
#include <Pillar/Events/ApplicationEvent.h>
#include <Pillar/Events/Event.h>
#include <Pillar/Input.h>
#include <Pillar/KeyCodes.h>
#include <imgui.h>
#include <algorithm>
#include <memory>
#include <vector>

// Game Systems
#include "Systems/PlayerMovementSystem.h"
#include "Systems/WeaponSystem.h"
#include "Systems/BulletLifetimeSystem.h"
#include "Systems/EnemyAISystem.h"
#include "Systems/DamageSystem.h"
#include "Systems/PowerUpSystem.h"
#include "Systems/EffectSystems.h"
#include "Systems/XPSystem.h"
#include "Systems/BossSystem.h"
#include "Systems/BulletTrailSystem.h"
#include "Systems/HazardSystem.h"

// Utilities
#include "Utilities/EntityFactory.h"
#include "Utilities/WaveManager.h"
#include "Utilities/CameraShake.h"
#include "Utilities/EffectFactory.h"
#include "Utilities/AudioManager.h"
#include "Utilities/ParticleManager.h"

// Components
#include "Components/EnemyComponent.h"
#include "Components/PowerUpComponent.h"
#include "Components/EffectComponents.h"
#include "Components/XPOrbComponent.h"
#include "Components/BossComponent.h"
#include "Components/HazardComponent.h"
#include "Components/BulletTrailComponent.h"

// Core game state and UI
#include "Core/GameState.h"
#include "UI/MenuRenderer.h"

namespace Game {

    class SwarmSlayerLayer : public Pillar::Layer
    {
    public:
        SwarmSlayerLayer()
            : Layer("SwarmSlayer")
        {
        }

        ~SwarmSlayerLayer() override
        {
            OnDetach();
        }

        void OnAttach() override
        {
            PIL_INFO("SwarmSlayerLayer::OnAttach - SWARM SLAYER");

            // Get window dimensions
            auto& window = Pillar::Application::Get().GetWindow();
            m_WindowWidth = static_cast<float>(window.GetWidth());
            m_WindowHeight = static_cast<float>(window.GetHeight());

            // Initialize game state
            GameState::Instance().Init();
            GameState::Instance().SetState(GameStateType::MainMenu);

            // Set up menu callbacks
            m_MenuRenderer.SetCallbacks(
                [this]() { StartGame(); },      // Start
                [this]() { ResumeGame(); },     // Resume
                [this]() { RestartGame(); },    // Restart
                [this]() { QuitGame(); },       // Quit
                [this](PerkType perk) { OnPerkSelected(perk); },   // Perk
                [this](WeaponType weapon) { OnWeaponUnlocked(weapon); }  // Weapon
            );

            // Initialize audio
            AudioManager::Instance().Init();

            // Initialize camera (used for menu background)
            float aspectRatio = m_WindowWidth / m_WindowHeight;
            m_CameraController = std::make_unique<Pillar::OrthographicCameraController>(aspectRatio, false);
            m_CameraController->SetZoomLevel(8.0f);
            m_CameraController->SetKeyboardControlEnabled(false);

            PIL_INFO("SwarmSlayerLayer initialized - showing main menu");
        }

        void OnDetach() override
        {
            PIL_INFO("SwarmSlayerLayer::OnDetach");

            // Shutdown systems
            ShutdownGameSystems();

            // Shutdown audio
            AudioManager::Instance().Shutdown();
        }

        void OnUpdate(float dt) override
        {
            SyncWindowSizeFromApplication();

            auto& gameState = GameState::Instance();
            auto currentState = gameState.GetState();

            // Handle ESC key for pause
            if (currentState == GameStateType::Playing)
            {
                if (Pillar::Input::IsKeyJustPressed(PIL_KEY_ESCAPE))
                {
                    gameState.SetState(GameStateType::Paused);
                    AudioManager::Instance().PauseMusic();
                    return;
                }

                // Update play time
                gameState.GetStats().PlayTime += dt;

                // Update game systems
                UpdateGame(dt);
            }
            else if (currentState == GameStateType::Paused)
            {
                // Just render, don't update
                if (Pillar::Input::IsKeyJustPressed(PIL_KEY_ESCAPE))
                {
                    ResumeGame();
                    return;
                }
            }

            // Render
            RenderGame();
        }

        void OnEvent(Pillar::Event& event) override
        {
            Pillar::EventDispatcher dispatcher(event);
            dispatcher.Dispatch<Pillar::WindowResizeEvent>([this](Pillar::WindowResizeEvent& e) {
                OnWindowResized(static_cast<float>(e.GetWidth()), static_cast<float>(e.GetHeight()));
                return false;
            });

            if (m_CameraController)
                m_CameraController->OnEvent(event);
        }

        void OnImGuiRender() override
        {
            auto& gameState = GameState::Instance();

            // Update HUD with current player health and dash state
            if (gameState.GetState() == GameStateType::Playing && m_PlayerEntity.IsValid())
            {
                auto& health = m_PlayerEntity.GetComponent<Pillar::HealthComponent>();
                m_MenuRenderer.SetPlayerHealth(health.CurrentHealth, health.MaxHealth);
                
                // Update dash cooldown
                auto& player = m_PlayerEntity.GetComponent<PlayerTagComponent>();
                bool dashReady = player.DashCooldownTimer <= 0.0f && !player.IsDashing;
                m_MenuRenderer.SetDashCooldown(player.DashCooldownTimer, dashReady);
            }

            // Render menus and HUD
            m_MenuRenderer.Render();
            m_MenuRenderer.RenderHUD();

            // Debug panel (only in playing state)
            if (gameState.GetState() == GameStateType::Playing)
            {
                RenderDebugPanel();
            }
        }

    private:
        // ===========================================
        // GAME LIFECYCLE
        // ===========================================

        void CleanupGameWorld()
        {
            // Ensure we don't keep stale entity handles across runs
            m_PlayerEntity = Pillar::Entity();

            // Shutdown systems first (they may reference the scene)
            ShutdownGameSystems();

            // IMPORTANT: Scene has an on_destroy<RigidbodyComponent> hook that can call
            // m_PhysicsSystem via Scene::m_PhysicsSystem. Clear these pointers before
            // destroying entities / dropping the scene.
            if (m_Scene)
            {
                m_Scene->SetPhysicsSystem(nullptr);
                m_Scene->SetAnimationSystem(nullptr);

                // Clear entities to avoid leaving registry state around between runs
                auto allEntities = m_Scene->GetAllEntities();
                for (auto entity : allEntities)
                    m_Scene->DestroyEntity(entity);

                m_Scene.reset();
            }

            // Reset per-run state
            m_KillStreakCount = 0;
            m_KillStreakTimer = 0.0f;
        }

        void SyncWindowSizeFromApplication()
        {
            auto& window = Pillar::Application::Get().GetWindow();
            const float width = static_cast<float>(window.GetWidth());
            const float height = static_cast<float>(window.GetHeight());

            if (width <= 0.0f || height <= 0.0f)
                return;

            if (width != m_WindowWidth || height != m_WindowHeight)
                OnWindowResized(width, height);
        }

        void OnWindowResized(float width, float height)
        {
            m_WindowWidth = width;
            m_WindowHeight = height;

            if (m_PlayerMovementSystem)
                m_PlayerMovementSystem->SetWindowSize(width, height);
            if (m_WeaponSystem)
                m_WeaponSystem->SetWindowSize(width, height);
        }

        void StartGame()
        {
            PIL_INFO("Starting new game!");

            // If a previous run exists (e.g. Play Again from GameOver), tear it down first.
            if (m_Scene || m_PhysicsSystem || m_PlayerMovementSystem || m_WeaponSystem)
                CleanupGameWorld();

            // Reset game state
            GameState::Instance().Reset();
            GameState::Instance().GetStats().Reset();
            GameState::Instance().GetPlayerStats().Reset();

            // Initialize game world
            InitializeGameWorld();

            // Start music
            AudioManager::Instance().StartMusic();

            // Set state to playing
            GameState::Instance().SetState(GameStateType::Playing);
        }

        void ResumeGame()
        {
            GameState::Instance().SetState(GameStateType::Playing);
            AudioManager::Instance().ResumeMusic();
        }

        void RestartGame()
        {
            PIL_INFO("Restarting game!");

            CleanupGameWorld();
            StartGame();
        }

        void QuitGame()
        {
            Pillar::Application::Get().Close();
        }

        void OnGameOver()
        {
            PIL_INFO("===== GAME OVER =====");
            PIL_INFO("Final Score: {}", GameState::Instance().GetStats().Score);
            PIL_INFO("Wave Reached: {}", GameState::Instance().GetStats().WaveReached);

            GameState::Instance().SetState(GameStateType::GameOver);
            AudioManager::Instance().StopMusic();

            // Play death sound
            AudioManager::Instance().PlaySound("player_death", 1.0f);
        }

        void OnPerkSelected(PerkType perk)
        {
            GameState::Instance().ApplyPerk(perk);
            GameState::Instance().SetState(GameStateType::Playing);
            
            // Apply perk effects to player immediately
            ApplyPlayerPerks();
        }

        void OnWeaponUnlocked(WeaponType weapon)
        {
            GameState::Instance().UnlockWeapon(weapon);
            GameState::Instance().SetCurrentWeapon(weapon);
            
            // Update player's weapon
            if (m_PlayerEntity.IsValid())
            {
                auto& weaponComp = m_PlayerEntity.GetComponent<WeaponComponent>();
                auto& weaponStats = GameState::Instance().GetWeapon(weapon);
                weaponComp.WeaponName = weaponStats.Name;
                weaponComp.Damage = weaponStats.Damage;
                weaponComp.FireRate = weaponStats.FireRate;
                weaponComp.BulletSpeed = weaponStats.BulletSpeed;
                weaponComp.Spread = weaponStats.Spread;
                weaponComp.BulletsPerShot = weaponStats.BulletsPerShot;
            }
        }

        void OnWaveComplete(int waveNumber, bool wasBossWave)
        {
            // Update stats
            GameState::Instance().GetStats().WaveReached = waveNumber;

            // Trigger perk selection every 2 waves or after boss
            if (waveNumber % 2 == 0 || wasBossWave)
            {
                GameState::Instance().SetState(GameStateType::PerkSelection);
            }
        }

        // ===========================================
        // GAME INITIALIZATION
        // ===========================================

        void InitializeGameWorld()
        {
            // Create Scene
            m_Scene = std::make_unique<Pillar::Scene>("SwarmSlayer");

            // Initialize Physics (ZERO GRAVITY for top-down!)
            m_PhysicsSystem = new Pillar::PhysicsSystem(glm::vec2(0.0f, 0.0f));
            m_PhysicsSystem->OnAttach(m_Scene.get());
            m_Scene->SetPhysicsSystem(m_PhysicsSystem);

            // Physics Sync System
            m_PhysicsSyncSystem = new Pillar::PhysicsSyncSystem();
            m_PhysicsSyncSystem->OnAttach(m_Scene.get());

            // Player Movement System
            m_PlayerMovementSystem = new PlayerMovementSystem(
                &m_CameraController->GetCamera(),
                m_WindowWidth, m_WindowHeight
            );
            m_PlayerMovementSystem->OnAttach(m_Scene.get());

            // Weapon System
            m_WeaponSystem = new WeaponSystem(
                &m_CameraController->GetCamera(),
                m_WindowWidth, m_WindowHeight
            );
            m_WeaponSystem->OnAttach(m_Scene.get());

            // Velocity Integration System (for bullets)
            m_VelocitySystem = new Pillar::VelocityIntegrationSystem();
            m_VelocitySystem->OnAttach(m_Scene.get());

            // Bullet Lifetime System
            m_BulletLifetimeSystem = new BulletLifetimeSystem();
            m_BulletLifetimeSystem->OnAttach(m_Scene.get());

            // Bullet Collision System
            m_BulletCollisionSystem = new Pillar::BulletCollisionSystem(m_PhysicsSystem);
            m_BulletCollisionSystem->OnAttach(m_Scene.get());

            // Enemy AI System
            m_EnemyAISystem = new EnemyAISystem();
            m_EnemyAISystem->OnAttach(m_Scene.get());

            // Boss System
            m_BossSystem = new BossSystem();
            m_BossSystem->OnAttach(m_Scene.get());
            m_BossSystem->SetOnBossDefeated([this](const glm::vec2& pos, int xp, int score) {
                OnBossDefeated(pos, xp, score);
            });
            m_BossSystem->SetOnSpawnMinion([this](const glm::vec2& pos, EnemyType type) {
                EntityFactory::CreateEnemy(*m_Scene, pos, type);
            });

            // Damage System
            m_DamageSystem = new DamageSystem(m_BulletCollisionSystem);
            m_DamageSystem->OnAttach(m_Scene.get());
            SetupDamageSystemCallbacks();

            // Power-Up System
            m_PowerUpSystem = new PowerUpSystem();
            m_PowerUpSystem->OnAttach(m_Scene.get());

            // XP System
            m_XPSystem = new XPSystem();
            m_XPSystem->OnAttach(m_Scene.get());
            m_XPSystem->SetOnLevelUp([this](int newLevel) {
                m_MenuRenderer.TriggerLevelUp();
                m_CameraShake.ShakeMedium();
                // Level up sound (using pickup at higher pitch as placeholder)
                AudioManager::Instance().PlaySound("pickup", 1.0f, 1.5f);
                PIL_INFO("LEVEL UP! Now level {}", newLevel);
            });

            // Effect Systems
            m_FlashSystem = new FlashSystem();
            m_FlashSystem->OnAttach(m_Scene.get());
            m_TemporaryCleanupSystem = new TemporaryCleanupSystem();
            m_TemporaryCleanupSystem->OnAttach(m_Scene.get());

            // Native Particle System (replaces entity-based particles)
            ParticleManager::Instance().Init(m_Scene.get(), 2000);

            // Bullet Trail System
            m_BulletTrailSystem = new BulletTrailSystem();
            m_BulletTrailSystem->OnAttach(m_Scene.get());

            // Hazard System
            m_HazardSystem = new HazardSystem();
            m_HazardSystem->OnAttach(m_Scene.get());
            m_HazardSystem->SetExplosionCallback([this](const glm::vec2& pos, float radius, float damage) {
                m_CameraShake.ShakeLarge();
                // Could add more explosion effects here
            });

            // Animation System
            m_AnimationSystem = new Pillar::AnimationSystem();
            m_AnimationSystem->OnAttach(m_Scene.get());
            m_Scene->SetAnimationSystem(m_AnimationSystem);
            LoadAnimations();

            // Wave Manager
            m_WaveManager.Init(30.0f, 16.0f);
            m_WaveManager.SetSpawnCallback([this](const glm::vec2& pos, EnemyType type) {
                EntityFactory::CreateEnemy(*m_Scene, pos, type);
            });
            m_WaveManager.SetBossSpawnCallback([this](const glm::vec2& pos, BossType type, int wave) {
                EntityFactory::CreateBoss(*m_Scene, pos, type, wave);
            });
            m_WaveManager.SetWaveCompleteCallback([this](int wave, bool wasBoss) {
                OnWaveComplete(wave, wasBoss);
            });

            // Create Arena
            CreateArenaBounds(30.0f, 16.0f);

            // Spawn environmental hazards for variety
            EntityFactory::SpawnRandomHazards(*m_Scene, 30.0f, 16.0f, 8);

            // Create Player
            m_PlayerEntity = EntityFactory::CreatePlayer(*m_Scene, glm::vec2(0.0f, 0.0f));
            ApplyPlayerPerks();

            PIL_INFO("Game world initialized successfully");
        }

        void SetupDamageSystemCallbacks()
        {
            m_DamageSystem->SetOnEnemyHit([this](const glm::vec2& pos, const glm::vec2& bulletDir) {
                EffectFactory::SpawnHitParticles(*m_Scene, pos, bulletDir);
                m_CameraShake.ShakeSmall();
                AudioManager::Instance().PlaySound("hit", pos, 0.5f);
            });

            m_DamageSystem->SetOnEnemyKilled([this](const glm::vec2& pos, const glm::vec4& color) {
                EffectFactory::SpawnDeathParticles(*m_Scene, pos, color);
                m_CameraShake.ShakeMedium();
                AudioManager::Instance().PlaySound("death", pos, 0.8f);

                // Spawn XP orb
                SpawnXPOrb(pos, 10);

                // Update stats
                GameState::Instance().GetStats().TotalKills++;
                GameState::Instance().GetStats().Score += 100;
                
                // Track kill streak
                m_KillStreakCount++;
                m_KillStreakTimer = 3.0f;  // Reset streak timer
                m_MenuRenderer.TriggerKillStreak(m_KillStreakCount);
            });

            m_DamageSystem->SetOnPlayerHit([this]() {
                m_CameraShake.ShakeLarge();
                AudioManager::Instance().PlaySound("player_hurt", 1.0f);
                
                // Spawn damage particles around player
                if (m_PlayerEntity.IsValid())
                {
                    auto& transform = m_PlayerEntity.GetComponent<Pillar::TransformComponent>();
                    EffectFactory::SpawnPlayerDamageEffect(*m_Scene, transform.Position);
                }
                
                // Reset kill streak when hit
                m_KillStreakCount = 0;
            });

            m_DamageSystem->SetOnGameOver([this]() {
                OnGameOver();
            });
        }

        void LoadAnimations()
        {
            m_AnimationSystem->LoadAnimationClip("animations/hoodzy_chaser_enemy_animation.anim.json");
            m_AnimationSystem->LoadAnimationClip("animations/floaty_enemy_animation.anim.json");
            m_AnimationSystem->LoadAnimationClip("animations/swarmer_run_animation.anim.json");
            m_AnimationSystem->LoadAnimationClip("animations/evil_archer_run_south.anim.json");
            m_AnimationSystem->LoadAnimationClip("animations/goblin_with_sword_run_south.anim.json");
            // Player + bosses (used by EntityFactory in SwarmSlayer)
            m_AnimationSystem->LoadAnimationClip("animations/red_mage_run_south.anim.json");
            m_AnimationSystem->LoadAnimationClip("animations/red_mage_idle_south.anim.json");
            m_AnimationSystem->LoadAnimationClip("animations/red_mage_fireball_south.anim.json");
            m_AnimationSystem->LoadAnimationClip("animations/goblin_queen_walk_south.anim.json");
            m_AnimationSystem->LoadAnimationClip("animations/large_behemoth_walk_south.anim.json");
            m_AnimationSystem->LoadAnimationClip("animations/monster_with_bow_run_south.anim.json");
        }

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
        }

        void ShutdownGameSystems()
        {
            // Shutdown particle manager first
            ParticleManager::Instance().Shutdown();
            
            // Delete all systems in reverse order
            if (m_HazardSystem) { m_HazardSystem->OnDetach(); delete m_HazardSystem; m_HazardSystem = nullptr; }
            if (m_BulletTrailSystem) { m_BulletTrailSystem->OnDetach(); delete m_BulletTrailSystem; m_BulletTrailSystem = nullptr; }
            if (m_AnimationSystem) { m_AnimationSystem->OnDetach(); delete m_AnimationSystem; m_AnimationSystem = nullptr; }
            if (m_TemporaryCleanupSystem) { m_TemporaryCleanupSystem->OnDetach(); delete m_TemporaryCleanupSystem; m_TemporaryCleanupSystem = nullptr; }
            if (m_FlashSystem) { m_FlashSystem->OnDetach(); delete m_FlashSystem; m_FlashSystem = nullptr; }
            if (m_XPSystem) { m_XPSystem->OnDetach(); delete m_XPSystem; m_XPSystem = nullptr; }
            if (m_PowerUpSystem) { m_PowerUpSystem->OnDetach(); delete m_PowerUpSystem; m_PowerUpSystem = nullptr; }
            if (m_DamageSystem) { m_DamageSystem->OnDetach(); delete m_DamageSystem; m_DamageSystem = nullptr; }
            if (m_BossSystem) { m_BossSystem->OnDetach(); delete m_BossSystem; m_BossSystem = nullptr; }
            if (m_EnemyAISystem) { m_EnemyAISystem->OnDetach(); delete m_EnemyAISystem; m_EnemyAISystem = nullptr; }
            if (m_BulletCollisionSystem) { m_BulletCollisionSystem->OnDetach(); delete m_BulletCollisionSystem; m_BulletCollisionSystem = nullptr; }
            if (m_BulletLifetimeSystem) { m_BulletLifetimeSystem->OnDetach(); delete m_BulletLifetimeSystem; m_BulletLifetimeSystem = nullptr; }
            if (m_VelocitySystem) { m_VelocitySystem->OnDetach(); delete m_VelocitySystem; m_VelocitySystem = nullptr; }
            if (m_WeaponSystem) { m_WeaponSystem->OnDetach(); delete m_WeaponSystem; m_WeaponSystem = nullptr; }
            if (m_PlayerMovementSystem) { m_PlayerMovementSystem->OnDetach(); delete m_PlayerMovementSystem; m_PlayerMovementSystem = nullptr; }
            if (m_PhysicsSyncSystem) { m_PhysicsSyncSystem->OnDetach(); delete m_PhysicsSyncSystem; m_PhysicsSyncSystem = nullptr; }
            if (m_PhysicsSystem) { m_PhysicsSystem->OnDetach(); delete m_PhysicsSystem; m_PhysicsSystem = nullptr; }
        }

        // ===========================================
        // GAME UPDATE
        // ===========================================

        void UpdateGame(float dt)
        {
            if (!m_Scene) return;

            // Handle weapon switching (1-5 keys)
            HandleWeaponSwitch();

            // Handle regeneration perk
            auto& playerStats = GameState::Instance().GetPlayerStats();
            if (playerStats.RegenPerSecond > 0.0f && m_PlayerEntity.IsValid())
            {
                auto& health = m_PlayerEntity.GetComponent<Pillar::HealthComponent>();
                health.Heal(playerStats.RegenPerSecond * dt);
            }

            // Wave Manager
            int enemyCount = CountEnemies();
            m_WaveManager.OnUpdate(dt, enemyCount);

            // Player Movement
            m_PlayerMovementSystem->OnUpdate(dt);

            // Enemy AI
            m_EnemyAISystem->OnUpdate(dt);

            // Boss System
            m_BossSystem->OnUpdate(dt);

            // Weapon System
            m_WeaponSystem->OnUpdate(dt);

            // Physics
            m_PhysicsSystem->OnUpdate(dt);
            m_PhysicsSyncSystem->OnUpdate(dt);

            // Velocity (bullets)
            m_VelocitySystem->OnUpdate(dt);

            // Bullet Collision
            m_BulletCollisionSystem->OnUpdate(dt);

            // Damage
            m_DamageSystem->OnUpdate(dt);

            // Power-Ups
            m_PowerUpSystem->OnUpdate(dt);

            // XP System
            m_XPSystem->OnUpdate(dt);

            // Effects
            m_FlashSystem->OnUpdate(dt);
            m_AnimationSystem->OnUpdate(dt);
            m_BulletLifetimeSystem->OnUpdate(dt);
            m_TemporaryCleanupSystem->OnUpdate(dt);

            // Bullet Trails
            m_BulletTrailSystem->OnUpdate(dt);

            // Environmental Hazards
            m_HazardSystem->OnUpdate(dt);

            // Native Particle System
            ParticleManager::Instance().OnUpdate(dt);
            
            // Update HUD timers
            m_MenuRenderer.UpdateTimers(dt);
            
            // Update kill streak timer
            if (m_KillStreakTimer > 0.0f)
            {
                m_KillStreakTimer -= dt;
                if (m_KillStreakTimer <= 0.0f)
                {
                    m_KillStreakCount = 0;  // Reset streak when timer expires
                }
            }

            // Camera
            m_CameraShake.OnUpdate(dt);
            UpdateCamera(dt);

            // Audio listener
            if (m_PlayerEntity.IsValid())
            {
                auto& transform = m_PlayerEntity.GetComponent<Pillar::TransformComponent>();
                Pillar::AudioEngine::SetListenerPosition(glm::vec3(transform.Position, 0.0f));
            }
        }

        void HandleWeaponSwitch()
        {
            auto unlockedWeapons = GameState::Instance().GetUnlockedWeapons();

            for (int i = 0; i < 5; i++)
            {
                if (Pillar::Input::IsKeyJustPressed(PIL_KEY_1 + i))
                {
                    WeaponType targetWeapon = static_cast<WeaponType>(i);
                    
                    // Check if weapon is unlocked
                    bool isUnlocked = false;
                    for (auto w : unlockedWeapons)
                    {
                        if (w == targetWeapon)
                        {
                            isUnlocked = true;
                            break;
                        }
                    }

                    if (isUnlocked)
                    {
                        GameState::Instance().SetCurrentWeapon(targetWeapon);
                        
                        // Update player weapon stats
                        if (m_PlayerEntity.IsValid())
                        {
                            auto& weaponComp = m_PlayerEntity.GetComponent<WeaponComponent>();
                            auto& weaponStats = GameState::Instance().GetWeapon(targetWeapon);
                            weaponComp.WeaponName = weaponStats.Name;
                            weaponComp.Damage = weaponStats.Damage;
                            weaponComp.FireRate = weaponStats.FireRate;
                            weaponComp.BulletSpeed = weaponStats.BulletSpeed;
                            weaponComp.Spread = weaponStats.Spread;
                            weaponComp.BulletsPerShot = weaponStats.BulletsPerShot;

                            PIL_INFO("Switched to {}", weaponStats.Name);
                        }
                    }
                }
            }
        }

        void UpdateCamera(float dt)
        {
            if (m_PlayerEntity.IsValid())
            {
                auto& playerTransform = m_PlayerEntity.GetComponent<Pillar::TransformComponent>();
                auto& camera = m_CameraController->GetCamera();

                glm::vec2 shakeOffset = m_CameraShake.GetOffset();
                glm::vec3 targetPos(
                    playerTransform.Position.x + shakeOffset.x,
                    playerTransform.Position.y + shakeOffset.y,
                    0.0f
                );
                camera.SetPosition(targetPos);
            }

            m_CameraController->OnUpdate(dt);
        }

        void ApplyPlayerPerks()
        {
            if (!m_PlayerEntity.IsValid()) return;

            auto& playerStats = GameState::Instance().GetPlayerStats();
            auto& health = m_PlayerEntity.GetComponent<Pillar::HealthComponent>();

            // Apply max health bonus
            float newMaxHealth = 100.0f + playerStats.BonusMaxHealth;
            if (health.MaxHealth != newMaxHealth)
            {
                float healthPercent = health.CurrentHealth / health.MaxHealth;
                health.MaxHealth = newMaxHealth;
                health.CurrentHealth = healthPercent * newMaxHealth;
            }

            // Apply move speed bonus
            auto& player = m_PlayerEntity.GetComponent<PlayerTagComponent>();
            player.MoveSpeed = 7.0f * playerStats.BaseMoveSpeedMultiplier;
        }

        // ===========================================
        // RENDERING
        // ===========================================

        void RenderGame()
        {
            // Clear with dark background
            Pillar::Renderer2D::SetClearColor(glm::vec4(0.03f, 0.03f, 0.06f, 1.0f));
            Pillar::Renderer2D::Clear();

            if (!m_Scene || !m_CameraController) return;

            Pillar::Renderer2D::BeginScene(m_CameraController->GetCamera());

            // Render bullet trails first (behind everything)
            if (m_BulletTrailSystem)
                m_BulletTrailSystem->RenderTrails();

            // Render all sprites (sorted: Layer, then OrderInLayer)
            auto& registry = m_Scene->GetRegistry();
            auto view = registry.view<Pillar::TransformComponent, Pillar::SpriteComponent>();

            std::vector<entt::entity> renderables;
            renderables.reserve(static_cast<size_t>(view.size_hint()));

            for (auto entity : view)
            {
                const auto& sprite = view.get<Pillar::SpriteComponent>(entity);
                if (!sprite.Visible)
                    continue;

                renderables.push_back(entity);
            }

            std::sort(renderables.begin(), renderables.end(), [&](entt::entity a, entt::entity b) {
                const auto& spriteA = view.get<Pillar::SpriteComponent>(a);
                const auto& spriteB = view.get<Pillar::SpriteComponent>(b);

                if (spriteA.Layer != spriteB.Layer)
                    return spriteA.Layer < spriteB.Layer;

                return spriteA.OrderInLayer < spriteB.OrderInLayer;
            });

            for (auto entity : renderables)
            {
                auto& transform = view.get<Pillar::TransformComponent>(entity);
                auto& sprite = view.get<Pillar::SpriteComponent>(entity);

                // Apply hazard pulsing effect if this entity has a hazard component
                Pillar::Entity ent(entity, m_Scene.get());
                if (auto* hazard = ent.TryGetComponent<HazardComponent>())
                {
                    // Create a copy of the sprite with pulsing color
                    Pillar::SpriteComponent pulsedSprite = sprite;
                    float pulse = hazard->GetPulseFactor();
                    pulsedSprite.Color = glm::mix(sprite.Color, glm::vec4(1.0f, 1.0f, 1.0f, sprite.Color.a), pulse * 0.3f);
                    Pillar::Renderer2D::DrawSprite(transform, pulsedSprite);
                }
                else
                {
                    Pillar::Renderer2D::DrawSprite(transform, sprite);
                }
            }

            // Render boss health bars
            if (m_BossSystem)
                m_BossSystem->RenderBossHealthBars();

            Pillar::Renderer2D::EndScene();
        }

        void RenderDebugPanel()
        {
            ImGui::Begin("Swarm Slayer - Debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

            auto& stats = GameState::Instance().GetStats();

            ImGui::Text("Wave: %d", stats.WaveReached);
            ImGui::Text("Enemies: %d", CountEnemies());
            ImGui::Text("Total Kills: %d", stats.TotalKills);
            ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

            if (m_WaveManager.IsBossWave())
            {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "BOSS WAVE!");
            }

            ImGui::Separator();

            // Quick spawn buttons for testing
            if (ImGui::CollapsingHeader("Debug Spawns"))
            {
                if (ImGui::Button("Spawn Chaser"))
                {
                    glm::vec2 pos(RandomFloat(-10.0f, 10.0f), RandomFloat(-5.0f, 5.0f));
                    EntityFactory::CreateEnemy(*m_Scene, pos, EnemyType::Chaser);
                }
                ImGui::SameLine();
                if (ImGui::Button("Spawn Boss"))
                {
                    glm::vec2 pos(0.0f, 5.0f);
                    EntityFactory::CreateBoss(*m_Scene, pos, EntityFactory::GetRandomBossType(), m_WaveManager.GetCurrentWave());
                }
                if (ImGui::Button("Add 100 XP"))
                {
                    GameState::Instance().GetStats().AddXP(100);
                }
                ImGui::SameLine();
                if (ImGui::Button("Trigger Perk Selection"))
                {
                    GameState::Instance().SetState(GameStateType::PerkSelection);
                }
            }

            ImGui::End();
        }

        // ===========================================
        // HELPERS
        // ===========================================

        void SpawnXPOrb(const glm::vec2& position, int xpValue)
        {
            // Apply lucky drops bonus
            float bonus = GameState::Instance().GetPlayerStats().DropRateBonus;
            int finalXP = static_cast<int>(xpValue * (1.0f + bonus));
            EntityFactory::CreateXPOrb(*m_Scene, position, finalXP);
        }

        void OnBossDefeated(const glm::vec2& position, int xpReward, int scoreReward)
        {
            // Spawn lots of XP orbs
            for (int i = 0; i < 5; i++)
            {
                glm::vec2 offset(RandomFloat(-1.0f, 1.0f), RandomFloat(-1.0f, 1.0f));
                SpawnXPOrb(position + offset, xpReward / 5);
            }

            // Add score
            GameState::Instance().GetStats().Score += scoreReward;
            GameState::Instance().GetStats().BossesKilled++;

            // Big camera shake
            m_CameraShake.ShakeLarge();

            // Spawn massive boss explosion particles
            EffectFactory::SpawnBossDeathParticles(*m_Scene, position, 50);
        }

        int CountEnemies() const
        {
            if (!m_Scene) return 0;

            auto& registry = m_Scene->GetRegistry();
            auto enemyView = registry.view<EnemyComponent, Pillar::HealthComponent>();
            auto bossView = registry.view<BossComponent, Pillar::HealthComponent>();

            int count = 0;
            for (auto entity : enemyView)
            {
                auto& health = enemyView.get<Pillar::HealthComponent>(entity);
                if (!health.IsDead) count++;
            }
            for (auto entity : bossView)
            {
                auto& health = bossView.get<Pillar::HealthComponent>(entity);
                if (!health.IsDead) count++;
            }
            return count;
        }

        float RandomFloat(float min, float max)
        {
            return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
        }

    private:
        // Scene and Camera
        std::unique_ptr<Pillar::Scene> m_Scene;
        std::unique_ptr<Pillar::OrthographicCameraController> m_CameraController;

        // Systems
        Pillar::PhysicsSystem* m_PhysicsSystem = nullptr;
        Pillar::PhysicsSyncSystem* m_PhysicsSyncSystem = nullptr;
        Pillar::VelocityIntegrationSystem* m_VelocitySystem = nullptr;
        Pillar::BulletCollisionSystem* m_BulletCollisionSystem = nullptr;
        Pillar::AnimationSystem* m_AnimationSystem = nullptr;
        PlayerMovementSystem* m_PlayerMovementSystem = nullptr;
        WeaponSystem* m_WeaponSystem = nullptr;
        BulletLifetimeSystem* m_BulletLifetimeSystem = nullptr;
        EnemyAISystem* m_EnemyAISystem = nullptr;
        BossSystem* m_BossSystem = nullptr;
        DamageSystem* m_DamageSystem = nullptr;
        PowerUpSystem* m_PowerUpSystem = nullptr;
        XPSystem* m_XPSystem = nullptr;
        FlashSystem* m_FlashSystem = nullptr;
        TemporaryCleanupSystem* m_TemporaryCleanupSystem = nullptr;
        BulletTrailSystem* m_BulletTrailSystem = nullptr;
        HazardSystem* m_HazardSystem = nullptr;

        // Game Managers
        WaveManager m_WaveManager;
        CameraShake m_CameraShake;
        MenuRenderer m_MenuRenderer;

        // Kill Streak Tracking
        int m_KillStreakCount = 0;
        float m_KillStreakTimer = 0.0f;

        // Player
        Pillar::Entity m_PlayerEntity;

        // Window
        float m_WindowWidth = 1280.0f;
        float m_WindowHeight = 720.0f;
    };

} // namespace Game
