#pragma once

#include "Pillar.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/ObjectPool.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Rendering/SpriteComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"
#include "Pillar/ECS/Components/Physics/ColliderComponent.h"
#include "Pillar/ECS/Components/Gameplay/BulletComponent.h"
#include "Pillar/ECS/Components/Gameplay/XPGemComponent.h"
#include "Pillar/Renderer/Texture.h"
#include "Pillar/Renderer/Renderer2DBackend.h"
#include "Pillar/Renderer/RenderCommand.h"
#include "Pillar/Renderer/OrthographicCameraController.h"
#include "Pillar/Input.h"
#include "Pillar/Events/KeyEvent.h"
#include "Components/PlayerStatsComponent.h"
#include "Components/HealthComponent.h"
#include "Components/EnemyAIComponent.h"
#include "Components/TurretAIComponent.h"
#include "Components/ChargerAIComponent.h"
#include <memory>

namespace Pillar {

	/**
	 * @brief Main game layer for Arena Protocol Technical Showcase
	 * 
	 * Phase 1 Implementation:
	 * - Player entity with movement and mouse aiming
	 * - Basic bullet firing with object pooling
	 * - Single drone enemy with seek behavior
	 * 
	 * NOTE: This is the legacy implementation. Use ArenaProtocol/ArenaProtocolLayer.h for the full version.
	 */
	class ArenaProtocolLayerLegacy : public Layer
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
		// Scene management
		std::shared_ptr<Scene> m_Scene;
		
		// Entity references
		Entity m_Player;
		std::vector<Entity> m_Enemies;
		
		// Object pools
		std::unique_ptr<ObjectPool> m_BulletPool;
		
		// Camera
		OrthographicCameraController m_CameraController;
		
		// Textures
		std::shared_ptr<Texture2D> m_PlayerTexture;
		std::shared_ptr<Texture2D> m_BulletTexture;
		std::shared_ptr<Texture2D> m_DroneTexture;
		std::shared_ptr<Texture2D> m_TurretTexture;
		std::shared_ptr<Texture2D> m_ChargerTexture;
		std::shared_ptr<Texture2D> m_XPGemTexture;
		
		// Gameplay state
		float m_FireCooldown = 0.1f;  // 10 shots per second
		float m_FireTimer = 0.0f;
		
		// Stage progression
		int m_CurrentStage = 1;
		bool m_StageComplete = false;
		
		// Debug UI
		bool m_ShowDebugPanel = true;
		int m_BulletsFired = 0;
		int m_EnemiesKilled = 0;

	private:
		// Entity creation
		void CreatePlayer();
		void CreateDrone(const glm::vec2& position);
		void CreateTurret(const glm::vec2& position);
		void CreateCharger(const glm::vec2& position);
		void CreateXPGem(const glm::vec2& position, int xpValue);
		
		// Gameplay systems
		void UpdatePlayer(float deltaTime);
		void UpdateEnemies(float deltaTime);
		void UpdateBullets(float deltaTime);
		void HandleShooting(float deltaTime);
		void UpdateCamera(float deltaTime);
		
		// Helper functions
		Entity FireBullet(const glm::vec2& position, const glm::vec2& direction, Entity owner);
		void CheckBulletCollisions();
		void CleanupDeadEntities();
		void CheckXPCollection();
		
		// Stage management
		void InitStage1();
		void InitStage2();
		void CheckStageCompletion();
	};

} // namespace Pillar
