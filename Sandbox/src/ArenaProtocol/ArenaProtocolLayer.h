#pragma once

#include "Pillar.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/ObjectPool.h"
#include "Pillar/Renderer/Texture.h"
#include "Pillar/Renderer/OrthographicCameraController.h"
#include "Pillar/Audio/AudioClip.h"

// Game Components
#include "../Components/PlayerStatsComponent.h"
#include "../Components/HealthComponent.h"
#include "../Components/EnemyAIComponent.h"
#include "../Components/TurretAIComponent.h"
#include "../Components/ChargerAIComponent.h"
#include "../Components/BossAIComponent.h"
#include "../Components/WeaponComponent.h"
#include "../Components/ObstacleComponent.h"
#include "../Components/CameraEffectsComponent.h"
#include "../Components/WaveManagerComponent.h"

// Game Systems
#include "../Systems/EnemyAISystem.h"
#include "../Systems/BossAISystem.h"
#include "../Systems/BulletSystem.h"
#include "../Systems/XPCollectionSystem.h"
#include "../Systems/WaveSystem.h"

// Factory & Chambers
#include "../Factory/EntityFactory.h"
#include "../Chambers/ChamberManager.h"

#include <memory>

namespace Pillar {

	/**
	 * @brief Game state enumeration
	 */
	enum class GameState
	{
		Menu,
		Playing,
		Paused,
		GameOver,
		Victory
	};

	/**
	 * @brief Main game layer for Arena Protocol Technical Showcase
	 * 
	 * Full implementation featuring:
	 * - Complete entity system (Player, Enemies, Boss, Items)
	 * - Multiple AI behaviors
	 * - Object pooling for bullets
	 * - Wave-based enemy spawning
	 * - Chamber/Level management
	 * - Camera effects
	 * - Debug UI
	 */
	class ArenaProtocolLayer : public Layer
	{
	public:
		ArenaProtocolLayer();
		virtual ~ArenaProtocolLayer() = default;

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate(float deltaTime) override;
		void OnImGuiRender() override;
		void OnEvent(Event& e) override;

	private:
		// Core scene management
		std::shared_ptr<Scene> m_Scene;
		OrthographicCameraController m_CameraController;
		GameState m_GameState = GameState::Playing;

		// Entity management
		Entity m_Player;
		Entity m_Boss;
		std::vector<Entity> m_Enemies;

		// Object pools
		std::unique_ptr<ObjectPool> m_BulletPool;
		std::unique_ptr<ObjectPool> m_ParticlePool;

		// Systems
		std::unique_ptr<EntityFactory> m_EntityFactory;
		std::unique_ptr<ChamberManager> m_ChamberManager;
		std::unique_ptr<EnemyAISystem> m_EnemyAISystem;
		std::unique_ptr<BossAISystem> m_BossAISystem;
		std::unique_ptr<BulletSystem> m_BulletSystem;
		std::unique_ptr<ArenaProtocol::XPCollectionSystem> m_XPCollectionSystem;
		std::unique_ptr<WaveSystem> m_WaveSystem;

		// Camera effects
		CameraEffectsComponent m_CameraEffects;
		glm::vec3 m_BaseCameraPosition = glm::vec3(0.0f);

		// Audio clips
		std::shared_ptr<AudioClip> m_ShootSFX;
		std::shared_ptr<AudioClip> m_EnemyShootSFX;
		std::shared_ptr<AudioClip> m_HitSFX;
		std::shared_ptr<AudioClip> m_ExplosionSFX;
		std::shared_ptr<AudioClip> m_PickupSFX;
		std::shared_ptr<AudioClip> m_DashSFX;

		// Gameplay state
		float m_FireTimer = 0.0f;
		float m_SecondaryFireTimer = 0.0f;  // Separate cooldown for right-click
		float m_ContactDamageCooldown = 0.0f;  // Invulnerability after contact damage
		float m_GameTime = 0.0f;
		bool m_BossMode = false;

		// Statistics
		int m_BulletsFired = 0;
		int m_EnemiesKilled = 0;
		int m_DamageTaken = 0;
		int m_XPCollected = 0;

		// Debug UI
		bool m_ShowDebugPanel = true;
		bool m_ShowSystemsPanel = false;
		bool m_ShowChamberSelect = false;
		int m_SelectedChamber = 0;

	private:
		// Initialization
		void InitializeSystems();
		void InitializePlayer();
		void InitializeBulletPool();
		void InitializeAudio();
		void SetupCallbacks();

		// Game loop
		void UpdatePlayer(float deltaTime);
		void UpdateEnemies(float deltaTime);
		void UpdateBoss(float deltaTime);
		void UpdateCamera(float deltaTime);
		void UpdateCameraEffects(float deltaTime);
		void HandleInput(float deltaTime);
		void HandleShooting(float deltaTime);
		
		// Collision handling
		void ProcessBulletCollisions();
		void ProcessPlayerCollisions(float deltaTime);

		// Entity management
		Entity FireBullet(const glm::vec2& position, const glm::vec2& direction, Entity owner, bool isPlayerBullet = true);
		void SpawnEnemy(const std::string& type, const glm::vec2& position);
		void CreateExplosion(const glm::vec2& position, float scale = 1.0f);
		void CleanupDeadEntities();

		// Game state
		void OnPlayerDeath();
		void OnBossDefeated();
		void OnWaveComplete(int waveNumber);
		void CheckVictoryCondition();
		void RestartGame();

		// UI
		void RenderGameUI();
		void RenderDebugPanel();
		void RenderSystemsPanel();
		void RenderChamberSelect();
		void RenderGameOverScreen();
		void RenderVictoryScreen();

		// Event handlers
		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
	};

} // namespace Pillar
