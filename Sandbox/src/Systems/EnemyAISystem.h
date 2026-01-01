#pragma once

#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include <glm/glm.hpp>
#include <vector>

namespace Pillar {

	/**
	 * @brief System for updating enemy AI behaviors
	 * Part of Arena Protocol showcase
	 */
	class EnemyAISystem
	{
	public:
		EnemyAISystem() = default;
		~EnemyAISystem() = default;

		void OnUpdate(Scene* scene, Entity player, float deltaTime);

	private:
		void UpdateDroneAI(Entity enemy, const glm::vec2& playerPos, float deltaTime);
		void UpdateTurretAI(Scene* scene, Entity enemy, const glm::vec2& playerPos, Entity player, float deltaTime);
		void UpdateChargerAI(Entity enemy, const glm::vec2& playerPos, float deltaTime);
	};

} // namespace Pillar
