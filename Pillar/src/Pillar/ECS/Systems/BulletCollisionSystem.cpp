#include "BulletCollisionSystem.h"
#include "PhysicsSystem.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"
#include "Pillar/ECS/Components/Gameplay/BulletComponent.h"
#include "Pillar/Logger.h"
#include <box2d/box2d.h>
#include <vector>

namespace Pillar {

	// Raycast callback to find the closest hit
	class BulletRaycastCallback : public b2RayCastCallback
	{
	public:
		BulletRaycastCallback() : m_Hit(false), m_Fraction(1.0f) {}

		float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction) override
		{
			// Store the closest hit
			if (fraction < m_Fraction)
			{
				m_Hit = true;
				m_Fraction = fraction;
				m_Point = point;
				m_Normal = normal;
				m_Fixture = fixture;
			}

			// Return fraction to continue raycasting (find closest hit)
			return fraction;
		}

		bool m_Hit;
		float m_Fraction;
		b2Vec2 m_Point;
		b2Vec2 m_Normal;
		b2Fixture* m_Fixture = nullptr;
	};

	BulletCollisionSystem::BulletCollisionSystem(PhysicsSystem* physicsSystem)
		: m_PhysicsSystem(physicsSystem)
	{
	}

	void BulletCollisionSystem::OnUpdate(float deltaTime)
	{
		ProcessBulletLifetime(deltaTime);
		ProcessBullets(deltaTime);
	}

	void BulletCollisionSystem::ProcessBulletLifetime(float deltaTime)
	{
		// Update bullet lifetime and destroy expired bullets
		auto view = m_Scene->GetRegistry().view<BulletComponent>();
		std::vector<entt::entity> toDestroy;

		for (auto entity : view)
		{
			auto& bullet = view.get<BulletComponent>(entity);
			bullet.TimeAlive += deltaTime;

			if (bullet.TimeAlive >= bullet.Lifetime || bullet.HitsRemaining == 0)
			{
				toDestroy.push_back(entity);
			}
		}

		// Destroy expired bullets
		for (auto entity : toDestroy)
		{
			Entity e(entity, m_Scene);
			m_Scene->DestroyEntity(e);
		}
	}

	void BulletCollisionSystem::ProcessBullets(float deltaTime)
	{
		auto view = m_Scene->GetRegistry().view<TransformComponent, VelocityComponent, BulletComponent>();

		for (auto entity : view)
		{
			auto& transform = view.get<TransformComponent>(entity);
			auto& velocity = view.get<VelocityComponent>(entity);
			auto& bullet = view.get<BulletComponent>(entity);

			// Calculate raycast start and end points
			glm::vec2 start = transform.Position;
			glm::vec2 end = transform.Position + velocity.Velocity * deltaTime;

			// Perform raycast
			Entity hitEntity;
			if (RaycastBullet(Entity(entity, m_Scene), start, end, hitEntity))
			{
				// Hit detected!
				PIL_CORE_TRACE("Bullet hit entity!");

				// Decrement hits remaining
				bullet.HitsRemaining--;

				// TODO: Apply damage to hit entity (Phase 6: Health System)
				// For now, just log the hit
			}
		}
	}

	bool BulletCollisionSystem::RaycastBullet(Entity bulletEntity, const glm::vec2& start, const glm::vec2& end, Entity& hitEntity)
	{
		BulletRaycastCallback callback;

		b2Vec2 p1(start.x, start.y);
		b2Vec2 p2(end.x, end.y);

		m_PhysicsSystem->GetWorld()->RayCast(&callback, p1, p2);

		if (callback.m_Hit)
		{
			// Get entity from fixture user data
			b2Body* body = callback.m_Fixture->GetBody();
			uintptr_t entityPtr = body->GetUserData().pointer;
			uint32_t entityId = static_cast<uint32_t>(entityPtr);

			// Create Entity from handle
			entt::entity hitEntityHandle = static_cast<entt::entity>(entityId);
			hitEntity = Entity(hitEntityHandle, m_Scene);

			// Don't hit ourselves (bullet owner)
			auto& bullet = bulletEntity.GetComponent<BulletComponent>();
			if (hitEntity == bullet.Owner)
			{
				return false;
			}

			return true;
		}

		return false;
	}

} // namespace Pillar
