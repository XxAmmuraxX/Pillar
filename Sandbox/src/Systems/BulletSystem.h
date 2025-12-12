#pragma once

#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/ObjectPool.h"
#include <glm/glm.hpp>

namespace Pillar {

	/**
	 * @brief System for handling bullet movement and collisions
	 * Part of Arena Protocol showcase
	 */
	class BulletSystem
	{
	public:
		BulletSystem() = default;
		~BulletSystem() = default;

		void OnUpdate(Scene* scene, ObjectPool* bulletPool, float deltaTime);

		// Returns true if bullet hit something and should be released
		bool CheckBulletEnemyCollision(Entity bullet, Scene* scene, Entity player, 
									   std::vector<Entity>& enemies);
		bool CheckBulletPlayerCollision(Entity bullet, Entity player);
		bool CheckBulletObstacleCollision(Entity bullet, Scene* scene);

	private:
		float m_BulletRadius = 0.1f;
		float m_EnemyRadius = 0.4f;
		float m_PlayerRadius = 0.5f;
	};

} // namespace Pillar
