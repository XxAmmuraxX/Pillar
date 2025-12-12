#pragma once

#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/ObjectPool.h"
#include <glm/glm.hpp>
#include <functional>

namespace Pillar {

	/**
	 * @brief Callback for enemy spawning during boss fights
	 */
	using SpawnDroneCallback = std::function<void(const glm::vec2&)>;

	/**
	 * @brief Callback for bullet firing
	 */
	using FireBulletCallback = std::function<Entity(const glm::vec2& pos, const glm::vec2& dir, Entity owner)>;

	/**
	 * @brief System for updating boss AI behaviors
	 * Part of Arena Protocol showcase
	 */
	class BossAISystem
	{
	public:
		BossAISystem() = default;
		~BossAISystem() = default;

		void SetSpawnCallback(SpawnDroneCallback callback) { m_SpawnDroneCallback = callback; }
		void SetFireBulletCallback(FireBulletCallback callback) { m_FireBulletCallback = callback; }

		void OnUpdate(Scene* scene, Entity boss, Entity player, float deltaTime);

	private:
		void UpdateBulletSpray(Entity boss, const glm::vec2& bossPos, float deltaTime);
		void UpdateLaserSweep(Entity boss, const glm::vec2& bossPos, const glm::vec2& playerPos, float deltaTime);
		void UpdateSummonDrones(Entity boss, const glm::vec2& bossPos, float deltaTime);
		void UpdateShield(Entity boss, float deltaTime);
		void UpdateChargeSlam(Entity boss, const glm::vec2& playerPos, float deltaTime);
		
		void SelectNextAttack(Entity boss);
		void CheckPhaseTransition(Entity boss);

		SpawnDroneCallback m_SpawnDroneCallback;
		FireBulletCallback m_FireBulletCallback;
	};

} // namespace Pillar
