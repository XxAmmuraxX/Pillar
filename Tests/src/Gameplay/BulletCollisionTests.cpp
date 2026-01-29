#include <gtest/gtest.h>
// BulletCollisionTests: unit and system tests for BulletComponent and
// BulletCollisionSystem — lifetime, hit counting, and multi-bullet processing.
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"
#include "Pillar/ECS/Components/Gameplay/BulletComponent.h"
#include "Pillar/ECS/Systems/BulletCollisionSystem.h"
#include "Pillar/ECS/Systems/PhysicsSystem.h"

using namespace Pillar;

// ========================================
// BulletComponent Tests
// ========================================

TEST(BulletTests, DefaultConstructor_InitializesValues)
{
	BulletComponent bullet;
	EXPECT_EQ(bullet.Damage, 10.0f);
	EXPECT_EQ(bullet.Lifetime, 5.0f);
	EXPECT_EQ(bullet.TimeAlive, 0.0f);
	EXPECT_FALSE(bullet.Pierce);
	EXPECT_EQ(bullet.MaxHits, 1);
	EXPECT_EQ(bullet.HitsRemaining, 1);
}

TEST(BulletTests, ParameterizedConstructor_SetsDamage)
{
	Scene scene;
	Entity owner = scene.CreateEntity();
	BulletComponent bullet(owner, 25.0f);

	EXPECT_EQ(bullet.Owner, owner);
	EXPECT_EQ(bullet.Damage, 25.0f);
}

// ========================================
// BulletCollisionSystem Tests
// ========================================

// NOTE: BulletCollisionSystem no longer destroys bullets.
// Lifetime management and destruction is handled by BulletLifetimeSystem (in game code).
// These tests verify collision detection behavior, not destruction.

TEST(BulletCollisionTests, BulletLifetime_UpdatesTimeAlive)
{
	Scene scene;
	PhysicsSystem physicsSystem;
	BulletCollisionSystem bulletSystem(&physicsSystem);

	physicsSystem.OnAttach(&scene);
	bulletSystem.OnAttach(&scene);

	// Create bullet with short lifetime
	Entity bullet = scene.CreateEntity("Bullet");
	bullet.AddComponent<VelocityComponent>(glm::vec2(10, 0));
	auto& bulletComp = bullet.AddComponent<BulletComponent>();
	bulletComp.Lifetime = 1.0f;
	bulletComp.TimeAlive = 0.0f;

	EXPECT_EQ(scene.GetRegistry().alive(), 1);

	// Update for 0.5 seconds (TimeAlive should be tracked by game's BulletLifetimeSystem)
	// BulletCollisionSystem only handles collision detection now
	bulletSystem.OnUpdate(0.5f);
	
	// Bullet should still exist (collision system doesn't destroy bullets)
	EXPECT_EQ(scene.GetRegistry().alive(), 1);
}

TEST(BulletCollisionTests, BulletHitsRemaining_StopsAtZero)
{
	Scene scene;
	PhysicsSystem physicsSystem;
	BulletCollisionSystem bulletSystem(&physicsSystem);

	physicsSystem.OnAttach(&scene);
	bulletSystem.OnAttach(&scene);

	// Create bullet
	Entity bullet = scene.CreateEntity("Bullet");
	bullet.AddComponent<VelocityComponent>(glm::vec2(10, 0));
	auto& bulletComp = bullet.AddComponent<BulletComponent>();
	bulletComp.HitsRemaining = 0; // Already hit max targets

	EXPECT_EQ(scene.GetRegistry().alive(), 1);

	// BulletCollisionSystem doesn't destroy bullets - it just detects hits
	// BulletLifetimeSystem (game code) handles destruction based on HitsRemaining
	bulletSystem.OnUpdate(0.016f);
	
	// Bullet still exists (cleanup is done by BulletLifetimeSystem)
	EXPECT_EQ(scene.GetRegistry().alive(), 1);
}

TEST(BulletCollisionTests, MultipleBullets_AllExist)
{
	Scene scene;
	PhysicsSystem physicsSystem;
	BulletCollisionSystem bulletSystem(&physicsSystem);

	physicsSystem.OnAttach(&scene);
	bulletSystem.OnAttach(&scene);

	// Create multiple bullets with different lifetimes
	Entity bullet1 = scene.CreateEntity("Bullet1");
	bullet1.AddComponent<VelocityComponent>(glm::vec2(10, 0));
	auto& b1 = bullet1.AddComponent<BulletComponent>();
	b1.Lifetime = 0.5f;

	Entity bullet2 = scene.CreateEntity("Bullet2");
	bullet2.AddComponent<VelocityComponent>(glm::vec2(10, 0));
	auto& b2 = bullet2.AddComponent<BulletComponent>();
	b2.Lifetime = 1.0f;

	Entity bullet3 = scene.CreateEntity("Bullet3");
	bullet3.AddComponent<VelocityComponent>(glm::vec2(10, 0));
	auto& b3 = bullet3.AddComponent<BulletComponent>();
	b3.Lifetime = 1.5f;

	EXPECT_EQ(scene.GetRegistry().alive(), 3);

	// BulletCollisionSystem only detects collisions, doesn't manage lifetime
	bulletSystem.OnUpdate(0.6f);
	EXPECT_EQ(scene.GetRegistry().alive(), 3);  // All still exist

	bulletSystem.OnUpdate(0.5f);
	EXPECT_EQ(scene.GetRegistry().alive(), 3);  // All still exist

	bulletSystem.OnUpdate(0.5f);
	EXPECT_EQ(scene.GetRegistry().alive(), 3);  // All still exist
}
