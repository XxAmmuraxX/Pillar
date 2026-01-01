#pragma once

#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "../Factory/EntityFactory.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <functional>

namespace Pillar {

	/**
	 * @brief Types of test chambers
	 */
	enum class ChamberType
	{
		Hub,              // Main hub with portals
		MovementPhysics,  // Chamber 1: Movement & Physics
		ShootingRange,    // Chamber 2: Shooting & Pooling
		EnemyGauntlet,    // Chamber 3: Enemy AI
		ParticleTest,     // Chamber 4: Particle effects
		AnimationTest,    // Chamber 5: Animations
		AudioTest,        // Chamber 6: Audio
		BossArena,        // Chamber 7: Boss fight
		StressTest        // Chamber 8: Performance
	};

	/**
	 * @brief Callbacks for chamber events
	 */
	using ChamberEventCallback = std::function<void(const std::string& eventName)>;

	/**
	 * @brief Manages game chambers/levels
	 * Part of Arena Protocol showcase
	 */
	class ChamberManager
	{
	public:
		ChamberManager() = default;
		~ChamberManager() = default;

		void Initialize(Scene* scene, EntityFactory* factory, Entity player);
		
		void SetEventCallback(ChamberEventCallback callback) { m_EventCallback = callback; }

		// Chamber management
		void LoadChamber(ChamberType chamber);
		void UnloadCurrentChamber();
		ChamberType GetCurrentChamber() const { return m_CurrentChamber; }

		// Get chamber entities
		const std::vector<Entity>& GetEnemies() const { return m_Enemies; }
		const std::vector<Entity>& GetObstacles() const { return m_Obstacles; }

		// Chamber-specific updates
		void OnUpdate(float deltaTime);

	private:
		void LoadHub();
		void LoadMovementPhysicsChamber();
		void LoadShootingRange();
		void LoadEnemyGauntlet();
		void LoadParticleTest();
		void LoadAnimationTest();
		void LoadAudioTest();
		void LoadBossArena();
		void LoadStressTest();

		void CreateArenaWalls(float size);
		void SpawnDrones(int count, float radius);
		void SpawnTurrets(const std::vector<glm::vec2>& positions);
		void SpawnChargers(int count, float radius);

		Scene* m_Scene = nullptr;
		EntityFactory* m_Factory = nullptr;
		Entity m_Player;
		ChamberType m_CurrentChamber = ChamberType::Hub;

		std::vector<Entity> m_Enemies;
		std::vector<Entity> m_Obstacles;
		std::vector<Entity> m_Triggers;
		std::vector<Entity> m_Effects;

		ChamberEventCallback m_EventCallback;
	};

} // namespace Pillar
