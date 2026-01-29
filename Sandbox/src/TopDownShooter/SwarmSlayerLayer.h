#pragma once

#include <Pillar/Layer.h>
#include <Pillar/ECS/Scene.h>
#include <Pillar/ECS/Entity.h>
#include <Pillar/ECS/Systems/PhysicsSystem.h>
#include <Pillar/ECS/Systems/PhysicsSyncSystem.h>
#include <Pillar/ECS/Systems/VelocityIntegrationSystem.h>
#include <Pillar/ECS/Systems/BulletCollisionSystem.h>
#include <Pillar/ECS/Systems/AnimationSystem.h>
#include <Pillar/ECS/SpecializedPools.h>
#include <Pillar/Renderer/Renderer2D.h>
#include <Pillar/Renderer/Lighting2D.h>
#include <Pillar/ECS/Systems/Lighting2DSystem.h>
#include <Pillar/ECS/Components/Rendering/Light2DComponent.h>
#include <Pillar/ECS/Components/Rendering/ShadowCaster2DComponent.h>
#include <Pillar/Renderer/OrthographicCameraController.h>
#include <Pillar/Application.h>
#include <Pillar/Events/ApplicationEvent.h>
#include <Pillar/Events/Event.h>
#include <Pillar/Input.h>
#include <Pillar/KeyCodes.h>
#include <imgui.h>
#include <algorithm>
#include <cmath>
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
#include "Utilities/GameUtils.h"

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

            // Initialize Lighting2D
            Pillar::Lighting2D::Init();
            m_LightingSettings.AmbientColor = glm::vec3(0.15f, 0.12f, 0.18f);
            m_LightingSettings.AmbientIntensity = 0.2f;
            m_LightingSettings.EnableShadows = true;

            PIL_INFO("SwarmSlayerLayer initialized - showing main menu");
        }

        void OnDetach() override
        {
            PIL_INFO("SwarmSlayerLayer::OnDetach");

            // Shutdown systems
            ShutdownGameSystems();

            // Shutdown Lighting2D
            Pillar::Lighting2D::Shutdown();

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
            auto& stats = GameState::Instance().GetStats();
            stats.WaveReached = waveNumber;

            // Wave completion bonus
            int waveBonus = stats.GetWaveCompletionBonus(waveNumber);
            stats.Score += waveBonus;
            m_MenuRenderer.TriggerWaveComplete(waveNumber, waveBonus, stats.NoDamageThisWave);

            // Reset no-damage tracking for next wave
            stats.NoDamageThisWave = true;

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

            // Initialize BulletPool
            m_BulletPool.Init(m_Scene.get(), 300);

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
            m_WeaponSystem->SetBulletPool(&m_BulletPool);

            // Velocity Integration System (for bullets)
            m_VelocitySystem = new Pillar::VelocityIntegrationSystem();
            m_VelocitySystem->OnAttach(m_Scene.get());

            // Bullet Lifetime System
            m_BulletLifetimeSystem = new BulletLifetimeSystem();
            m_BulletLifetimeSystem->OnAttach(m_Scene.get());
            m_BulletLifetimeSystem->SetBulletPool(&m_BulletPool);

            // Bullet Collision System
            m_BulletCollisionSystem = new Pillar::BulletCollisionSystem(m_PhysicsSystem);
            m_BulletCollisionSystem->OnAttach(m_Scene.get());

            // Enemy AI System
            m_EnemyAISystem = new EnemyAISystem();
            m_EnemyAISystem->OnAttach(m_Scene.get());
            m_EnemyAISystem->SetBulletPool(&m_BulletPool);

            // Boss System
            m_BossSystem = new BossSystem();
            m_BossSystem->OnAttach(m_Scene.get());
            m_BossSystem->SetBulletPool(&m_BulletPool);
            m_BossSystem->SetOnBossDefeated([this](const glm::vec2& pos, int xp, int score) {
                OnBossDefeated(pos, xp, score);
            });
            m_BossSystem->SetOnSpawnMinion([this](const glm::vec2& pos, EnemyType type) {
                EntityFactory::CreateEnemy(*m_Scene, pos, type, m_WaveManager.GetCurrentWave());
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
                // Big camera shake for barrel explosions
                m_CameraShake.ShakeHuge();

                // Screen flash effect for dramatic impact
                m_ScreenFlashTimer = 0.15f;

                // Spawn explosion light
                EffectFactory::SpawnExplosionLight(*m_Scene, pos, radius);
            });

            // Animation System
            m_AnimationSystem = new Pillar::AnimationSystem();
            m_AnimationSystem->OnAttach(m_Scene.get());
            m_Scene->SetAnimationSystem(m_AnimationSystem);
            LoadAnimations();

            // Lighting2D System
            m_Lighting2DSystem = new Pillar::Lighting2DSystem();
            m_Lighting2DSystem->OnAttach(m_Scene.get());

            // Wave Manager
            m_WaveManager.Init(30.0f, 16.0f);
            m_WaveManager.SetSpawnCallback([this](const glm::vec2& pos, EnemyType type) {
                EntityFactory::CreateEnemy(*m_Scene, pos, type, m_WaveManager.GetCurrentWave());
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
            m_DamageSystem->SetOnEnemyHit([this](const glm::vec2& pos, const glm::vec2& bulletDir, float damageDealt) {
                EffectFactory::SpawnHitParticles(*m_Scene, pos, bulletDir);
                m_CameraShake.ShakeSmall();

                // Spawn floating damage number
                m_MenuRenderer.AddDamageNumber(pos, static_cast<int>(damageDealt));
            });

            m_DamageSystem->SetOnEnemyKilled([this](const glm::vec2& pos, const glm::vec4& color, EnemyType enemyType) {
                EffectFactory::SpawnDeathParticles(*m_Scene, pos, color);
                m_CameraShake.ShakeMedium();

                // Spawn XP orb
                SpawnXPOrb(pos, 10);

                // Enhanced scoring with enemy type, wave multiplier, and combo
                auto& stats = GameState::Instance().GetStats();
                int scoreGained = stats.AddKillScore(enemyType, stats.WaveReached);

                // Track kill streak
                m_KillStreakCount++;
                m_KillStreakTimer = 3.0f;
                m_MenuRenderer.TriggerKillStreak(m_KillStreakCount);

                // Show combo notification
                if (stats.ComboCount > 1)
                {
                    m_MenuRenderer.TriggerCombo(stats.ComboCount, scoreGained);
                }
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

                // Screen red flash on player damage
                m_ScreenFlashTimer = 0.3f;

                // Mark wave as damaged (no perfect wave bonus)
                GameState::Instance().GetStats().NoDamageThisWave = false;

                // Reset kill streak when hit
                m_KillStreakCount = 0;
            });

            m_DamageSystem->SetOnGameOver([this]() {
                OnGameOver();
            });

            // Wire up barrel hit detection to hazard system
            m_DamageSystem->SetOnBarrelHit([this](entt::entity barrelEntity, float damage) {
                if (m_HazardSystem)
                {
                    m_HazardSystem->OnBarrelHit(barrelEntity, damage);
                }
            });
        }

        void LoadAnimations()
        {
            m_AnimationSystem->LoadAnimationClip("animations/hoodzy_chaser_enemy_animation.anim.json");
            m_AnimationSystem->LoadAnimationClip("animations/floaty_enemy_animation.anim.json");
            m_AnimationSystem->LoadAnimationClip("animations/swarmer_run_animation.anim.json");
            
            // Evil Archer - all directions
            m_AnimationSystem->LoadAnimationClip("animations/evil_archer_run_south.anim.json");
            m_AnimationSystem->LoadAnimationClip("animations/evil_archer_run_north.anim.json");
            m_AnimationSystem->LoadAnimationClip("animations/evil_archer_run_east.anim.json");
            m_AnimationSystem->LoadAnimationClip("animations/evil_archer_run_west.anim.json");
            
            // Goblin with Sword - all directions
            m_AnimationSystem->LoadAnimationClip("animations/goblin_with_sword_run_south.anim.json");
            m_AnimationSystem->LoadAnimationClip("animations/goblin_with_sword_run_north.anim.json");
            m_AnimationSystem->LoadAnimationClip("animations/goblin_with_sword_run_east.anim.json");
            m_AnimationSystem->LoadAnimationClip("animations/goblin_with_sword_run_west.anim.json");
            
            // Player (Red Mage) - all directions for run and idle
            m_AnimationSystem->LoadAnimationClip("animations/red_mage_run_south.anim.json");
            m_AnimationSystem->LoadAnimationClip("animations/red_mage_run_north.anim.json");
            m_AnimationSystem->LoadAnimationClip("animations/red_mage_run_east.anim.json");
            m_AnimationSystem->LoadAnimationClip("animations/red_mage_run_west.anim.json");
            m_AnimationSystem->LoadAnimationClip("animations/red_mage_idle_south.anim.json");
            m_AnimationSystem->LoadAnimationClip("animations/red_mage_idle_north.anim.json");
            m_AnimationSystem->LoadAnimationClip("animations/red_mage_idle_east.anim.json");
            m_AnimationSystem->LoadAnimationClip("animations/red_mage_idle_west.anim.json");
            m_AnimationSystem->LoadAnimationClip("animations/red_mage_fireball_south.anim.json");
            
            // Goblin Queen Boss - all directions
            m_AnimationSystem->LoadAnimationClip("animations/goblin_queen_walk_south.anim.json");
            m_AnimationSystem->LoadAnimationClip("animations/goblin_queen_walk_north.anim.json");
            m_AnimationSystem->LoadAnimationClip("animations/goblin_queen_walk_east.anim.json");
            m_AnimationSystem->LoadAnimationClip("animations/goblin_queen_walk_west.anim.json");
            
            // Large Behemoth Boss - all directions
            m_AnimationSystem->LoadAnimationClip("animations/large_behemoth_walk_south.anim.json");
            m_AnimationSystem->LoadAnimationClip("animations/large_behemoth_walk_north.anim.json");
            m_AnimationSystem->LoadAnimationClip("animations/large_behemoth_walk_east.anim.json");
            m_AnimationSystem->LoadAnimationClip("animations/large_behemoth_walk_west.anim.json");
            
            // Monster with Bow Boss - all directions
            m_AnimationSystem->LoadAnimationClip("animations/monster_with_bow_run_south.anim.json");
            m_AnimationSystem->LoadAnimationClip("animations/monster_with_bow_run_north.anim.json");
            m_AnimationSystem->LoadAnimationClip("animations/monster_with_bow_run_east.anim.json");
            m_AnimationSystem->LoadAnimationClip("animations/monster_with_bow_run_west.anim.json");
        }

        void CreateArenaBounds(float width, float height)
        {
            float wallThickness = 1.0f;
            float halfWidth = width * 0.5f;
            float halfHeight = height * 0.5f;

            // === SCRAPYARD SALVATION: Arena Floor Background ===
            // Tile the floor texture across the arena
            auto floorTexture = Pillar::Texture2D::Create(Pillar::AssetManager::GetTexturePath("levels/level_background.png"));
            float tileSize = 8.0f;  // Size of each tile in world units
            
            // Calculate number of tiles needed
            int tilesX = static_cast<int>(std::ceil(width / tileSize));
            int tilesY = static_cast<int>(std::ceil(height / tileSize));
            
            // Create grid of floor tiles
            for (int y = 0; y < tilesY; ++y)
            {
                for (int x = 0; x < tilesX; ++x)
                {
                    auto floor = m_Scene->CreateEntity("ArenaFloor");
                    auto& floorTransform = floor.GetComponent<Pillar::TransformComponent>();
                    
                    // Position each tile, centered on arena
                    float posX = -halfWidth + tileSize * 0.5f + x * tileSize;
                    float posY = -halfHeight + tileSize * 0.5f + y * tileSize;
                    floorTransform.SetPosition(glm::vec2(posX, posY));
                    
                    auto& floorSprite = floor.AddComponent<Pillar::SpriteComponent>();
                    floorSprite.Size = glm::vec2(tileSize, tileSize);
                    floorSprite.Texture = floorTexture;
                    floorSprite.Color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);  // No tint
                    floorSprite.Layer = "Background";  // Sorts first alphabetically
                    floorSprite.OrderInLayer = -100;   // Rendered first within layer
                }
            }

            // Top wall - Industrial gray with rust tint
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

            PIL_INFO("Arena created with SCRAPYARD SALVATION floor texture");
        }

        void ShutdownGameSystems()
        {
            // Clear bullet pool first
            m_BulletPool.Clear();

            // Shutdown particle manager
            ParticleManager::Instance().Shutdown();

            // Delete all systems in reverse order
            if (m_Lighting2DSystem) { m_Lighting2DSystem->OnDetach(); delete m_Lighting2DSystem; m_Lighting2DSystem = nullptr; }
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

            // Update combo timer
            GameState::Instance().GetStats().UpdateCombo(dt);

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

            // Feed enemies remaining and wave countdown to HUD
            m_MenuRenderer.SetEnemiesRemaining(enemyCount);
            m_MenuRenderer.SetWaveCountdown(m_WaveManager.GetRestTimer());

            // Notify HUD of wave start
            if (m_WaveManager.IsWaveJustStarted())
            {
                m_MenuRenderer.TriggerWaveStart(m_WaveManager.GetCurrentWave());
            }

            // Feed active buffs to HUD
            if (m_PlayerEntity.IsValid())
            {
                if (auto* buffs = m_PlayerEntity.TryGetComponent<PlayerBuffsComponent>())
                {
                    m_MenuRenderer.SetActiveBuffs(buffs->ActiveEffects);
                }
                else
                {
                    m_MenuRenderer.ClearActiveBuffs();
                }
            }

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

            // Update screen flash
            if (m_ScreenFlashTimer > 0.0f)
                m_ScreenFlashTimer -= dt;

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

                // Camera lookahead - bias toward mouse cursor for better visibility
                auto [mouseX, mouseY] = Pillar::Input::GetMousePosition();
                glm::vec2 mouseWorld = ScreenToWorld(
                    mouseX, mouseY,
                    m_WindowWidth, m_WindowHeight,
                    camera
                );
                glm::vec2 toMouse = mouseWorld - playerTransform.Position;
                float mouseDistance = glm::length(toMouse);
                glm::vec2 lookahead(0.0f);
                if (mouseDistance > 0.5f)
                {
                    // Lookahead up to 2 units toward cursor
                    lookahead = glm::normalize(toMouse) * std::min(mouseDistance * 0.2f, 2.0f);
                }

                // Smooth camera movement
                glm::vec2 shakeOffset = m_CameraShake.GetOffset();
                glm::vec3 targetPos(
                    playerTransform.Position.x + lookahead.x + shakeOffset.x,
                    playerTransform.Position.y + lookahead.y + shakeOffset.y,
                    0.0f
                );

                // Smooth interpolation for camera
                glm::vec3 currentPos = camera.GetPosition();
                glm::vec3 smoothed = glm::mix(currentPos, targetPos, std::min(1.0f, dt * 8.0f));
                camera.SetPosition(smoothed);
            }
            // Note: Do NOT call m_CameraController->OnUpdate(dt) here as it would overwrite
            // the camera position we just set with lookahead. Keyboard controls are disabled anyway.
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
            // Clear with gritty charred black background
            Pillar::Renderer2D::SetClearColor(glm::vec4(0.04f, 0.04f, 0.05f, 1.0f));
            Pillar::Renderer2D::Clear();

            if (!m_Scene || !m_CameraController) return;

            auto& window = Pillar::Application::Get().GetWindow();
            uint32_t vpWidth = static_cast<uint32_t>(window.GetWidth());
            uint32_t vpHeight = static_cast<uint32_t>(window.GetHeight());

            // Collect and sort all renderable sprites
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

            // === LIT PASS ===
            Pillar::Lighting2D::BeginScene(
                m_CameraController->GetCamera(),
                vpWidth, vpHeight,
                m_LightingSettings
            );
            // Note: Lighting2D::BeginScene already called Renderer2D::BeginScene

            // Bullet trails first
            if (m_BulletTrailSystem)
                m_BulletTrailSystem->RenderTrails();

            // Flush after trails before sprites
            Pillar::Renderer2D::EndScene();
            Pillar::Renderer2D::BeginScene(m_CameraController->GetCamera());

            // Render sorted sprites with mid-scene flushes on layer changes
            std::string currentLayer = "";
            for (size_t i = 0; i < renderables.size(); ++i)
            {
                auto entity = renderables[i];
                auto& transform = view.get<Pillar::TransformComponent>(entity);
                auto& sprite = view.get<Pillar::SpriteComponent>(entity);

                if (sprite.Layer != currentLayer && !currentLayer.empty())
                {
                    // Mid-scene flush: preserves draw order while staying in lit FBO
                    Pillar::Renderer2D::EndScene();
                    Pillar::Renderer2D::BeginScene(m_CameraController->GetCamera());
                }
                currentLayer = sprite.Layer;

                Pillar::Entity ent(entity, m_Scene.get());
                if (auto* hazard = ent.TryGetComponent<HazardComponent>())
                {
                    Pillar::SpriteComponent pulsedSprite = sprite;
                    float pulse = hazard->GetPulseFactor();
                    pulsedSprite.Color = glm::mix(sprite.Color,
                        glm::vec4(1.0f, 1.0f, 1.0f, sprite.Color.a), pulse * 0.3f);
                    Pillar::Renderer2D::DrawSprite(transform, pulsedSprite);
                }
                else
                {
                    Pillar::Renderer2D::DrawSprite(transform, sprite);
                }
            }

            // Flush final sprite batch
            Pillar::Renderer2D::EndScene();

            // Submit lights and shadow casters
            if (m_Lighting2DSystem)
                m_Lighting2DSystem->OnUpdate(0.0f);

            // Begin a new scene for Lighting2D to composite correctly
            Pillar::Renderer2D::BeginScene(m_CameraController->GetCamera());

            // End lit pass (composites scene * lighting to screen)
            Pillar::Lighting2D::EndScene();

            // === UNLIT PASS (overlays) ===
            Pillar::Renderer2D::BeginScene(m_CameraController->GetCamera());
            if (m_BossSystem)
                m_BossSystem->RenderBossHealthBars();

            // Screen flash overlay (red tint on player damage)
            if (m_ScreenFlashTimer > 0.0f)
            {
                float flashAlpha = m_ScreenFlashTimer / 0.3f * 0.3f;  // Fade from 0.3 to 0
                auto& camera = m_CameraController->GetCamera();
                glm::vec3 camPos = camera.GetPosition();
                float zoom = m_CameraController->GetZoomLevel();

                Pillar::TransformComponent flashTransform;
                flashTransform.SetPosition(glm::vec2(camPos.x, camPos.y));
                Pillar::SpriteComponent flashSprite;
                flashSprite.Size = glm::vec2(zoom * 4.0f, zoom * 4.0f);
                flashSprite.Color = glm::vec4(1.0f, 0.0f, 0.0f, flashAlpha);
                flashSprite.Layer = "Overlay";
                flashSprite.OrderInLayer = 100;
                Pillar::Renderer2D::DrawSprite(flashTransform, flashSprite);
            }

            // Low health pulsing red vignette
            if (m_PlayerEntity.IsValid())
            {
                auto& health = m_PlayerEntity.GetComponent<Pillar::HealthComponent>();
                float healthPercent = health.CurrentHealth / health.MaxHealth;
                if (healthPercent < 0.3f && healthPercent > 0.0f)
                {
                    float pulse = 0.1f + 0.1f * std::sin(GameState::Instance().GetStats().PlayTime * 6.0f);
                    float intensity = (1.0f - healthPercent / 0.3f) * pulse;

                    auto& camera = m_CameraController->GetCamera();
                    glm::vec3 camPos = camera.GetPosition();
                    float zoom = m_CameraController->GetZoomLevel();

                    Pillar::TransformComponent vignetteTransform;
                    vignetteTransform.SetPosition(glm::vec2(camPos.x, camPos.y));
                    Pillar::SpriteComponent vignetteSprite;
                    vignetteSprite.Size = glm::vec2(zoom * 4.0f, zoom * 4.0f);
                    vignetteSprite.Color = glm::vec4(0.8f, 0.0f, 0.0f, intensity);
                    vignetteSprite.Layer = "Overlay";
                    vignetteSprite.OrderInLayer = 99;
                    Pillar::Renderer2D::DrawSprite(vignetteTransform, vignetteSprite);
                }
            }

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
            
            // Render stats for debugging texture/batch issues
            auto renderStats = Pillar::Renderer2D::GetStats();
            ImGui::Separator();
            ImGui::Text("Draw Calls: %u", renderStats.DrawCalls);
            ImGui::Text("Quads: %u", renderStats.QuadCount);
            ImGui::Text("Flush Count: %u", renderStats.FlushCount);
            ImGui::Text("Buffer Uploads: %u", renderStats.BufferUploads);

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
        Pillar::Lighting2DSystem* m_Lighting2DSystem = nullptr;

        // Pools
        Pillar::BulletPool m_BulletPool;

        // Lighting
        Pillar::Lighting2DSettings m_LightingSettings;

        // Game Managers
        WaveManager m_WaveManager;
        CameraShake m_CameraShake;
        MenuRenderer m_MenuRenderer;

        // Kill Streak Tracking
        int m_KillStreakCount = 0;
        float m_KillStreakTimer = 0.0f;

        // Game feel
        float m_ScreenFlashTimer = 0.0f;

        // Player
        Pillar::Entity m_PlayerEntity;

        // Window
        float m_WindowWidth = 1280.0f;
        float m_WindowHeight = 720.0f;
    };

} // namespace Game
