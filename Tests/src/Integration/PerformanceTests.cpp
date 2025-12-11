#include <gtest/gtest.h>
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/ObjectPool.h"
#include "Pillar/ECS/SpecializedPools.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"
#include "Pillar/ECS/Components/Gameplay/BulletComponent.h"
#include "Pillar/ECS/Components/Gameplay/ParticleComponent.h"
#include "Pillar/ECS/Systems/VelocityIntegrationSystem.h"
#include "Pillar/ECS/Systems/BulletCollisionSystem.h"
#include "Pillar/ECS/Systems/PhysicsSystem.h"
#include <chrono>
#include <vector>

using namespace Pillar;

// ============================================================================
// Performance/Stress Tests
// Tests that verify the engine can handle high entity counts and stress scenarios
// ============================================================================

class PerformanceTests : public ::testing::Test
{
protected:
	void SetUp() override
	{
		m_Scene = std::make_unique<Scene>("PerformanceTestScene");
	}

	void TearDown() override
	{
		m_Scene.reset();
	}

	std::unique_ptr<Scene> m_Scene;
};

// -----------------------------------------------------------------------------
// Entity Creation Performance
// -----------------------------------------------------------------------------

TEST_F(PerformanceTests, EntityCreation_1000Entities_UnderThreshold)
{
	auto start = std::chrono::high_resolution_clock::now();

	for (int i = 0; i < 1000; i++)
	{
		m_Scene->CreateEntity("Entity" + std::to_string(i));
	}

	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

	EXPECT_EQ(m_Scene->GetEntityCount(), 1000);
	EXPECT_LT(duration.count(), 1000); // Should complete in under 1 second

	// Log performance info
	std::cout << "Created 1000 entities in " << duration.count() << "ms" << std::endl;
}

TEST_F(PerformanceTests, EntityWithComponents_500Entities_Reasonable)
{
	auto start = std::chrono::high_resolution_clock::now();

	for (int i = 0; i < 500; i++)
	{
		Entity entity = m_Scene->CreateEntity("Entity" + std::to_string(i));
		entity.AddComponent<VelocityComponent>(glm::vec2(float(i), float(i)));
	}

	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

	EXPECT_EQ(m_Scene->GetEntityCount(), 500);
	EXPECT_LT(duration.count(), 2000); // Should complete in under 2 seconds

	std::cout << "Created 500 entities with components in " << duration.count() << "ms" << std::endl;
}

// -----------------------------------------------------------------------------
// System Update Performance
// -----------------------------------------------------------------------------

TEST_F(PerformanceTests, VelocitySystem_1000Entities_60FPS)
{
	VelocityIntegrationSystem velocitySystem;
	velocitySystem.OnAttach(m_Scene.get());

	// Create 1000 moving entities
	for (int i = 0; i < 1000; i++)
	{
		Entity entity = m_Scene->CreateEntity();
		entity.AddComponent<VelocityComponent>(glm::vec2(float(i % 100), float(i % 50)));
	}

	// Time 60 frames
	auto start = std::chrono::high_resolution_clock::now();
	
	float dt = 1.0f / 60.0f;
	for (int frame = 0; frame < 60; frame++)
	{
		velocitySystem.OnUpdate(dt);
	}

	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

	// 60 frames should complete in reasonable time (less than 1 second for non-rendering test)
	EXPECT_LT(duration.count(), 1000);

	std::cout << "Processed 60 frames with 1000 velocity entities in " << duration.count() << "ms" << std::endl;
	std::cout << "Average frame time: " << duration.count() / 60.0 << "ms" << std::endl;
}

// -----------------------------------------------------------------------------
// Object Pool Performance
// -----------------------------------------------------------------------------

TEST_F(PerformanceTests, BulletPool_HighThroughput)
{
	BulletPool bulletPool;
	bulletPool.Init(m_Scene.get(), 500);

	Entity player = m_Scene->CreateEntity("Player");

	auto start = std::chrono::high_resolution_clock::now();

	// Simulate rapid fire: spawn and return many bullets
	std::vector<Entity> activeBullets;
	activeBullets.reserve(200);

	for (int cycle = 0; cycle < 100; cycle++)
	{
		// Spawn 10 bullets
		for (int i = 0; i < 10; i++)
		{
			Entity bullet = bulletPool.SpawnBullet(
				glm::vec2(float(cycle), float(i)),
				glm::vec2(1.0f, 0.0f),
				500.0f,
				player
			);
			activeBullets.push_back(bullet);
		}

		// Return 5 oldest bullets
		for (int i = 0; i < 5 && !activeBullets.empty(); i++)
		{
			bulletPool.ReturnBullet(activeBullets.front());
			activeBullets.erase(activeBullets.begin());
		}
	}

	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

	EXPECT_LT(duration.count(), 2000); // Should complete in reasonable time

	std::cout << "1000 spawn + 500 return operations in " << duration.count() << "ms" << std::endl;
}

TEST_F(PerformanceTests, ParticlePool_MassSpawn)
{
	ParticlePool particlePool;
	particlePool.Init(m_Scene.get(), 2000);

	auto start = std::chrono::high_resolution_clock::now();

	// Spawn 1000 particles
	std::vector<Entity> particles;
	particles.reserve(1000);

	for (int i = 0; i < 1000; i++)
	{
		Entity particle = particlePool.SpawnParticle(
			glm::vec2(float(i % 100), float(i / 100)),
			glm::vec2(0.0f, -10.0f),
			glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
			0.1f,
			2.0f
		);
		particles.push_back(particle);
	}

	auto spawnEnd = std::chrono::high_resolution_clock::now();

	// Return all particles
	for (auto& particle : particles)
	{
		particlePool.ReturnParticle(particle);
	}

	auto end = std::chrono::high_resolution_clock::now();

	auto spawnDuration = std::chrono::duration_cast<std::chrono::milliseconds>(spawnEnd - start);
	auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

	EXPECT_EQ(particlePool.GetActiveCount(), 0);
	EXPECT_LT(spawnDuration.count(), 2000); // Should complete in reasonable time

	std::cout << "Spawned 1000 particles in " << spawnDuration.count() << "ms" << std::endl;
	std::cout << "Total spawn+return in " << totalDuration.count() << "ms" << std::endl;
}

// -----------------------------------------------------------------------------
// Memory Stress Tests
// -----------------------------------------------------------------------------

TEST_F(PerformanceTests, EntityChurn_CreateDestroy_Stable)
{
	// Test that repeatedly creating and destroying entities doesn't cause issues
	for (int cycle = 0; cycle < 100; cycle++)
	{
		// Create 50 entities
		std::vector<Entity> entities;
		for (int i = 0; i < 50; i++)
		{
			entities.push_back(m_Scene->CreateEntity());
		}

		// Destroy all
		for (auto& entity : entities)
		{
			m_Scene->DestroyEntity(entity);
		}
	}

	// Scene should be empty and stable
	EXPECT_EQ(m_Scene->GetEntityCount(), 0);
}

TEST_F(PerformanceTests, ComponentChurn_AddRemove_Stable)
{
	Entity entity = m_Scene->CreateEntity("ChurnTest");

	for (int cycle = 0; cycle < 1000; cycle++)
	{
		entity.AddComponent<VelocityComponent>();
		entity.RemoveComponent<VelocityComponent>();
	}

	// Entity should still be valid
	EXPECT_TRUE(entity);
	EXPECT_TRUE(entity.HasComponent<TransformComponent>());
	EXPECT_FALSE(entity.HasComponent<VelocityComponent>());
}

// -----------------------------------------------------------------------------
// Concurrent-like Access Pattern Tests
// -----------------------------------------------------------------------------

TEST_F(PerformanceTests, SystemIteration_DuringModification)
{
	VelocityIntegrationSystem velocitySystem;
	velocitySystem.OnAttach(m_Scene.get());

	// Create initial entities
	for (int i = 0; i < 100; i++)
	{
		Entity entity = m_Scene->CreateEntity();
		entity.AddComponent<VelocityComponent>(glm::vec2(1.0f, 0.0f));
	}

	// Simulate game loop with entity creation/destruction during updates
	for (int frame = 0; frame < 10; frame++)
	{
		velocitySystem.OnUpdate(0.016f);

		// Create new entity
		Entity newEntity = m_Scene->CreateEntity();
		newEntity.AddComponent<VelocityComponent>(glm::vec2(2.0f, 0.0f));
	}

	// Should have more entities now
	EXPECT_GE(m_Scene->GetEntityCount(), 100);
}
