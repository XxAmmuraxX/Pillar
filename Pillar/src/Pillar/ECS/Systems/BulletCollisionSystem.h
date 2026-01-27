#pragma once

#include "Pillar/Core.h"
#include "System.h"
#include <glm/glm.hpp>
#include <vector>
#include <functional>

namespace Pillar {

	class PhysicsSystem; // Forward declaration
	class Entity;

	// Uses Box2D Raycasts to detect bullet hits against Heavy Entities
	// Also performs circle-circle collision against Light Entities (VelocityComponent only)
	class PIL_API BulletCollisionSystem : public System
	{
	public:
		// Callback signature: (bullet, hitEntity, damage, hitPosition)
		using OnBulletHitCallback = std::function<void(Entity, Entity, float, const glm::vec2&)>;

		BulletCollisionSystem(PhysicsSystem* physicsSystem);

		void OnUpdate(float deltaTime) override;

		// Set callback for when a bullet hits something
		void SetOnBulletHit(OnBulletHitCallback callback) { m_OnBulletHit = callback; }

	private:
		PhysicsSystem* m_PhysicsSystem;
		OnBulletHitCallback m_OnBulletHit;

		void ProcessBullets(float deltaTime);
		void ProcessBulletLifetime(float deltaTime);
		bool RaycastBullet(Entity bulletEntity, const glm::vec2& start, const glm::vec2& end, Entity& hitEntity, glm::vec2& hitPoint);
		bool CheckCircleCollision(Entity bulletEntity, Entity& targetEntity, glm::vec2& hitPoint);
	};

} // namespace Pillar
