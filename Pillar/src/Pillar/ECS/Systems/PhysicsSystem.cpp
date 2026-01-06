#include "PhysicsSystem.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Physics/RigidbodyComponent.h"
#include "Pillar/ECS/Components/Physics/ColliderComponent.h"
#include "Pillar/ECS/Physics/Box2DBodyFactory.h"
#include "Pillar/Logger.h"

namespace Pillar {

	PhysicsSystem::PhysicsSystem(const glm::vec2& gravity)
		: m_World(std::make_unique<Box2DWorld>(gravity))
		, m_ContactListener(std::make_unique<Box2DContactListener>())
		, m_Gravity(gravity)
	{
	}

	PhysicsSystem::~PhysicsSystem()
	{
	}

	void PhysicsSystem::OnAttach(Scene* scene)
	{
		System::OnAttach(scene);

		// Recreate the Box2D world with a fresh state
		m_World = std::make_unique<Box2DWorld>(m_Gravity);

		// Set contact listener
		m_World->GetWorld()->SetContactListener(m_ContactListener.get());

		// Reset accumulator
		m_Accumulator = 0.0f;

		// Create physics bodies for existing entities
		CreatePhysicsBodies();
	}

	void PhysicsSystem::OnDetach()
	{
		System::OnDetach();

		// Clean up all physics bodies
		if (m_Scene)
		{
			auto view = m_Scene->GetRegistry().view<RigidbodyComponent>();
			for (auto entity : view)
			{
				auto& rigidbody = view.get<RigidbodyComponent>(entity);
				if (rigidbody.Body != nullptr)
				{
					m_World->GetWorld()->DestroyBody(rigidbody.Body);
					rigidbody.Body = nullptr;
				}
			}
		}

		// Reset contact listener
		if (m_World && m_World->GetWorld())
		{
			m_World->GetWorld()->SetContactListener(nullptr);
		}
	}

	void PhysicsSystem::OnUpdate(float deltaTime)
	{
		// Fixed timestep accumulator
		m_Accumulator += deltaTime;

		// Clamp accumulator to prevent spiral of death
		if (m_Accumulator > 0.25f)
		{
			m_Accumulator = 0.25f;
		}

		// Step physics at fixed rate
		while (m_Accumulator >= m_FixedTimeStep)
		{
			FixedUpdate(m_FixedTimeStep);
			m_Accumulator -= m_FixedTimeStep;
		}
	}

	void PhysicsSystem::FixedUpdate(float fixedDeltaTime)
	{
		// Create bodies for new entities
		CreatePhysicsBodies();

		// Sync kinematic body transforms from ECS
		SyncTransformsToBox2D();

		// Step the Box2D world
		m_World->Step(fixedDeltaTime, 8, 3);
	}

	void PhysicsSystem::CreatePhysicsBodies()
	{
		// Find entities with Rigidbody but no Body yet
		auto view = m_Scene->GetRegistry().view<TransformComponent, RigidbodyComponent>();

		for (auto entity : view)
		{
			auto& transform = view.get<TransformComponent>(entity);
			auto& rigidbody = view.get<RigidbodyComponent>(entity);

			// Skip if body already exists
			if (rigidbody.Body != nullptr)
				continue;

			// Create Box2D body
			rigidbody.Body = Box2DBodyFactory::CreateBody(
				m_World->GetWorld(),
				transform.Position,
				transform.Rotation,
				rigidbody.BodyType,
				rigidbody.FixedRotation,
				rigidbody.GravityScale,
				rigidbody.LinearDamping,
				rigidbody.AngularDamping,
				rigidbody.IsBullet,
				rigidbody.IsEnabled
			);

			// Store entity handle in user data for collision callbacks
			// Cast entt::entity to uint32_t first, then to uintptr_t
			uint32_t entityId = static_cast<uint32_t>(entity);
			rigidbody.Body->GetUserData().pointer = static_cast<uintptr_t>(entityId);

			// Create fixture if entity has ColliderComponent
			Entity e(entity, m_Scene);
			if (e.HasComponent<ColliderComponent>())
			{
				auto& collider = e.GetComponent<ColliderComponent>();
				Box2DBodyFactory::CreateFixture(rigidbody.Body, collider);
			}

			PIL_CORE_TRACE("Created Box2D body for entity");
		}
	}

	void PhysicsSystem::SyncTransformsToBox2D()
	{
		// For kinematic bodies, we need to sync ECS transforms to Box2D
		auto view = m_Scene->GetRegistry().view<TransformComponent, RigidbodyComponent>();

		for (auto entity : view)
		{
			auto& transform = view.get<TransformComponent>(entity);
			auto& rigidbody = view.get<RigidbodyComponent>(entity);

			if (rigidbody.Body && rigidbody.BodyType == b2_kinematicBody)
			{
				// Write ECS transform to Box2D
				rigidbody.Body->SetTransform(
					b2Vec2(transform.Position.x, transform.Position.y),
					transform.Rotation
				);
			}
		}
	}

} // namespace Pillar
