#pragma once

#include "Pillar/Core.h"
#include "System.h"
#include "Pillar/ECS/Physics/Box2DWorld.h"
#include "Pillar/ECS/Physics/Box2DContactListener.h"
#include <glm/glm.hpp>
#include <memory>

namespace Pillar {

	// Responsible for:
	// 1. Creating/destroying b2Bodies for entities with RigidbodyComponent
	// 2. Stepping the Box2D world
	// 3. Applying forces/impulses from ECS to Box2D
	class PIL_API PhysicsSystem : public System
	{
	public:
		PhysicsSystem(const glm::vec2& gravity = { 0.0f, -9.81f });
		~PhysicsSystem() override;

		void OnAttach(Scene* scene) override;
		void OnDetach() override;
		void OnUpdate(float deltaTime) override;

		// Access to Box2D world (for raycasts, queries)
		b2World* GetWorld() { return m_World->GetWorld(); }
		Box2DWorld* GetBox2DWorld() { return m_World.get(); }

	private:
		std::unique_ptr<Box2DWorld> m_World;
		std::unique_ptr<Box2DContactListener> m_ContactListener;
		glm::vec2 m_Gravity;
		float m_Accumulator = 0.0f;
		const float m_FixedTimeStep = 1.0f / 60.0f; // 60 Hz physics

		void FixedUpdate(float fixedDeltaTime);
		void CreatePhysicsBodies();  // For new entities with Rigidbody
		void SyncTransformsToBox2D(); // Write ECS transforms to Box2D (for kinematic bodies)
	};

} // namespace Pillar
