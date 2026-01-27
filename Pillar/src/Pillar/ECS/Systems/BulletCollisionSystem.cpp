#include "BulletCollisionSystem.h"
#include "PhysicsSystem.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"
#include "Pillar/ECS/Components/Physics/RigidbodyComponent.h"
#include "Pillar/ECS/Components/Rendering/SpriteComponent.h"
#include "Pillar/ECS/Components/Gameplay/BulletComponent.h"
#include "Pillar/ECS/Components/Gameplay/HealthComponent.h"
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
		: m_PhysicsSystem(physicsSystem), m_OnBulletHit(nullptr)
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
		std::vector<entt::entity> bulletsToDestroy;

		for (auto entity : view)
		{
			auto& transform = view.get<TransformComponent>(entity);
			auto& velocity = view.get<VelocityComponent>(entity);
			auto& bullet = view.get<BulletComponent>(entity);

			Entity bulletEntity(entity, m_Scene);

			// Calculate raycast start and end points
			glm::vec2 start = transform.Position;
			glm::vec2 end = transform.Position + velocity.Velocity * deltaTime;

			// Try raycast first (for heavy entities with Box2D bodies)
			Entity hitEntity;
			glm::vec2 hitPoint;
			bool hitDetected = false;

			if (RaycastBullet(bulletEntity, start, end, hitEntity, hitPoint))
			{
				hitDetected = true;
			}
			else
			{
				// Check circle collision against light entities (no Box2D body)
				if (CheckCircleCollision(bulletEntity, hitEntity, hitPoint))
				{
					hitDetected = true;
				}
			}

			if (hitDetected)
			{
				// Trigger callback
				if (m_OnBulletHit)
				{
					m_OnBulletHit(bulletEntity, hitEntity, bullet.Damage, hitPoint);
				}

				// Decrement hits remaining
				bullet.HitsRemaining--;

				if (bullet.HitsRemaining <= 0)
				{
					bulletsToDestroy.push_back(entity);
				}
			}
		}

		// Destroy bullets that hit and ran out of hits
		for (auto entity : bulletsToDestroy)
		{
			Entity e(entity, m_Scene);
			m_Scene->DestroyEntity(e);
		}
	}

	bool BulletCollisionSystem::RaycastBullet(Entity bulletEntity, const glm::vec2& start, const glm::vec2& end, Entity& hitEntity, glm::vec2& hitPoint)
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

			// Store hit point
			hitPoint = glm::vec2(callback.m_Point.x, callback.m_Point.y);

			return true;
		}

		return false;
	}

	bool BulletCollisionSystem::CheckCircleCollision(Entity bulletEntity, Entity& targetEntity, glm::vec2& hitPoint)
	{
		auto& bulletTransform = bulletEntity.GetComponent<TransformComponent>();
		auto& bulletComp = bulletEntity.GetComponent<BulletComponent>();

		// Get all entities with transform and health that DON'T have a rigidbody (light entities)
		auto view = m_Scene->GetRegistry().view<TransformComponent, HealthComponent>(entt::exclude<RigidbodyComponent>);

		const float bulletRadius = 0.15f;

		for (auto entity : view)
		{
			Entity target(entity, m_Scene);

			// Don't hit ourselves (bullet owner)
			if (target == bulletComp.Owner)
				continue;

			auto& targetTransform = view.get<TransformComponent>(entity);

			// Determine target radius
			float targetRadius = 0.5f; // Default
			if (auto* sprite = target.TryGetComponent<SpriteComponent>())
			{
				targetRadius = sprite->Size.x * 0.4f;
			}

			// Circle-circle collision
			float distanceSquared = glm::distance2(bulletTransform.Position, targetTransform.Position);
			float radiusSum = bulletRadius + targetRadius;

			if (distanceSquared < radiusSum * radiusSum)
			{
				// Hit detected!
				targetEntity = target;
				hitPoint = targetTransform.Position;
				return true;
			}
		}

		return false;
	}

} // namespace Pillar
