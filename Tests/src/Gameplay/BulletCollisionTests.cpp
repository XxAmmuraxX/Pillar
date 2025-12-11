#include <gtest/gtest.h>
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

TEST(BulletCollisionTests, BulletLifetime_ExpiresAfterTime)
{
	Scene scene;
	PhysicsSystem physicsSystem;
	BulletCollisionSystem bulletSystem(&physicsSystem);

	physicsSystem.OnAttach(&scene);
	bulletSystem.OnAttach(&scene);

	// Create bullet with short lifetime
	// TransformComponent already added by CreateEntity
	Entity bullet = scene.CreateEntity("Bullet");
	bullet.AddComponent<VelocityComponent>(glm::vec2(10, 0));
	auto& bulletComp = bullet.AddComponent<BulletComponent>();
	bulletComp.Lifetime = 1.0f; // 1 second lifetime

	EXPECT_EQ(scene.GetRegistry().alive(), 1);

	// Update for 0.5 seconds (bullet should still exist)
	bulletSystem.OnUpdate(0.5f);
	EXPECT_EQ(scene.GetRegistry().alive(), 1);

	// Update for another 0.6 seconds (total 1.1 seconds, bullet should be destroyed)
	bulletSystem.OnUpdate(0.6f);
	EXPECT_EQ(scene.GetRegistry().alive(), 0);
}

TEST(BulletCollisionTests, BulletHitsRemaining_DestroysAtZero)
{
	Scene scene;
	PhysicsSystem physicsSystem;
	BulletCollisionSystem bulletSystem(&physicsSystem);

	physicsSystem.OnAttach(&scene);
	bulletSystem.OnAttach(&scene);

	// Create bullet
	// TransformComponent already added by CreateEntity
	Entity bullet = scene.CreateEntity("Bullet");
	bullet.AddComponent<VelocityComponent>(glm::vec2(10, 0));
	auto& bulletComp = bullet.AddComponent<BulletComponent>();
	bulletComp.HitsRemaining = 0; // Already hit max targets

	EXPECT_EQ(scene.GetRegistry().alive(), 1);

	// Update (bullet should be destroyed)
	bulletSystem.OnUpdate(0.016f);
	EXPECT_EQ(scene.GetRegistry().alive(), 0);
}

TEST(BulletCollisionTests, MultipleBullets_AllProcessed)
{
	Scene scene;
	PhysicsSystem physicsSystem;
	BulletCollisionSystem bulletSystem(&physicsSystem);

	physicsSystem.OnAttach(&scene);
	bulletSystem.OnAttach(&scene);

	// Create multiple bullets with different lifetimes
	// TransformComponent already added by CreateEntity for all
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

	// Update for 0.6 seconds (bullet1 should be destroyed)
	bulletSystem.OnUpdate(0.6f);
	EXPECT_EQ(scene.GetRegistry().alive(), 2);

	// Update for another 0.5 seconds (bullet2 should be destroyed)
	bulletSystem.OnUpdate(0.5f);
	EXPECT_EQ(scene.GetRegistry().alive(), 1);

	// Update for another 0.5 seconds (bullet3 should be destroyed)
	bulletSystem.OnUpdate(0.5f);
	EXPECT_EQ(scene.GetRegistry().alive(), 0);
}
