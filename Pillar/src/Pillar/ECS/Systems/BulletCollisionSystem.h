#pragma once

#include "Pillar/Core.h"
#include "System.h"
#include <glm/glm.hpp>
#include <vector>

namespace Pillar {

	class PhysicsSystem; // Forward declaration
	class Entity;

	// Uses Box2D Raycasts to detect bullet hits against Heavy Entities
	// Does NOT use b2Bodies for bullets (they're Light Entities)
	class PIL_API BulletCollisionSystem : public System
	{
	public:
		BulletCollisionSystem(PhysicsSystem* physicsSystem);

		void OnUpdate(float deltaTime) override;

	private:
		PhysicsSystem* m_PhysicsSystem;

		void ProcessBullets(float deltaTime);
		void ProcessBulletLifetime(float deltaTime);
		bool RaycastBullet(Entity bulletEntity, const glm::vec2& start, const glm::vec2& end, Entity& hitEntity);
	};

} // namespace Pillar
