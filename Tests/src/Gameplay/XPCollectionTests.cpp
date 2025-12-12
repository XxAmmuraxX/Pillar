#include <gtest/gtest.h>
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Core/TagComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"
#include "Pillar/ECS/Components/Gameplay/XPGemComponent.h"
#include "Pillar/ECS/Systems/XPCollectionSystem.h"

using namespace Pillar;

// ========================================
// XPGemComponent Tests
// ========================================

TEST(XPGemTests, DefaultConstructor_InitializesValues)
{
	XPGemComponent gem;
	EXPECT_EQ(gem.XPValue, 1);
	EXPECT_EQ(gem.AttractionRadius, 3.0f);
	EXPECT_EQ(gem.MoveSpeed, 10.0f);
	EXPECT_FALSE(gem.IsAttracted);
}

TEST(XPGemTests, ParameterizedConstructor_SetsValue)
{
	XPGemComponent gem(5);
	EXPECT_EQ(gem.XPValue, 5);
}

// ========================================
// XPCollectionSystem Tests
// ========================================

TEST(XPCollectionTests, Constructor_InitializesGrid)
{
	XPCollectionSystem system;
	EXPECT_EQ(system.GetEntityCount(), 0);
	EXPECT_EQ(system.GetBucketCount(), 0);
}

TEST(XPCollectionTests, UpdateSpatialGrid_InsertsGems)
{
	Scene scene;
	XPCollectionSystem system;
	system.OnAttach(&scene);

	// Create some XP gems
	for (int i = 0; i < 10; ++i)
	{
		Entity gem = scene.CreateEntity("Gem");
		auto& transform = gem.GetComponent<TransformComponent>();
		transform.Position = glm::vec2(i * 1.0f, 0);
		gem.AddComponent<XPGemComponent>(1);
	}

	// Update should populate spatial grid
	system.OnUpdate(0.016f);

	EXPECT_EQ(system.GetEntityCount(), 10);
}

TEST(XPCollectionTests, GemAttraction_MovesTowardPlayer)
{
	Scene scene;
	XPCollectionSystem system;
	system.OnAttach(&scene);

	// Create player
	Entity player = scene.CreateEntity("Player");
	auto& playerTransform = player.GetComponent<TransformComponent>();
	playerTransform.Position = glm::vec2(10, 0);

	// Create gem near player
	Entity gem = scene.CreateEntity("Gem");
	auto& gemTransform = gem.GetComponent<TransformComponent>();
	gemTransform.Position = glm::vec2(8, 0); // 2 units away
	gem.AddComponent<VelocityComponent>();
	auto& gemComp = gem.AddComponent<XPGemComponent>(1);
	gemComp.AttractionRadius = 5.0f; // Within attraction range

	// Initial position
	glm::vec2 initialPos = gemTransform.Position;

	// Update system
	system.OnUpdate(0.016f);

	// Gem should be attracted
	EXPECT_TRUE(gemComp.IsAttracted);

	// Velocity should point toward player
	auto& velocity = gem.GetComponent<VelocityComponent>();
	EXPECT_GT(velocity.Velocity.x, 0); // Moving right toward player
}

TEST(XPCollectionTests, GemNotAttracted_WhenFarFromPlayer)
{
	Scene scene;
	XPCollectionSystem system;
	system.OnAttach(&scene);

	// Create player
	Entity player = scene.CreateEntity("Player");
	auto& playerTransform = player.GetComponent<TransformComponent>();
	playerTransform.Position = glm::vec2(0, 0);

	// Create gem far from player
	Entity gem = scene.CreateEntity("Gem");
	auto& gemTransform = gem.GetComponent<TransformComponent>();
	gemTransform.Position = glm::vec2(100, 100); // Very far away
	gem.AddComponent<VelocityComponent>();
	auto& gemComp = gem.AddComponent<XPGemComponent>(1);
	gemComp.AttractionRadius = 3.0f; // Small attraction radius

	// Update system
	system.OnUpdate(0.016f);

	// Gem should NOT be attracted
	EXPECT_FALSE(gemComp.IsAttracted);

	// Velocity should be zero (not moving)
	auto& velocity = gem.GetComponent<VelocityComponent>();
	EXPECT_EQ(velocity.Velocity.x, 0);
	EXPECT_EQ(velocity.Velocity.y, 0);
}

TEST(XPCollectionTests, GemCollection_DestroysWhenClose)
{
	Scene scene;
	XPCollectionSystem system;
	system.OnAttach(&scene);

	// Create player
	Entity player = scene.CreateEntity("Player");
	auto& playerTransform = player.GetComponent<TransformComponent>();
	playerTransform.Position = glm::vec2(0, 0);

	// Create gem very close to player (should be collected)
	Entity gem = scene.CreateEntity("Gem");
	auto& gemTransform = gem.GetComponent<TransformComponent>();
	gemTransform.Position = glm::vec2(0.1f, 0.1f); // Very close
	gem.AddComponent<VelocityComponent>();
	gem.AddComponent<XPGemComponent>(5);

	EXPECT_EQ(scene.GetRegistry().alive(), 2); // Player + Gem

	// Update system
	system.OnUpdate(0.016f);

	// Gem should be collected (destroyed)
	EXPECT_EQ(scene.GetRegistry().alive(), 1); // Only player remains
}

TEST(XPCollectionTests, MultipleGems_ProcessedIndependently)
{
	Scene scene;
	XPCollectionSystem system;
	system.OnAttach(&scene);

	// Create player
	Entity player = scene.CreateEntity("Player");
	auto& playerTransform = player.GetComponent<TransformComponent>();
	playerTransform.Position = glm::vec2(0, 0);

	// Create gem close to player (will be attracted)
	Entity gem1 = scene.CreateEntity("Gem1");
	auto& gem1Transform = gem1.GetComponent<TransformComponent>();
	gem1Transform.Position = glm::vec2(2, 0);
	gem1.AddComponent<VelocityComponent>();
	auto& gem1Comp = gem1.AddComponent<XPGemComponent>(1);

	// Create gem far from player (will not be attracted)
	Entity gem2 = scene.CreateEntity("Gem2");
	auto& gem2Transform = gem2.GetComponent<TransformComponent>();
	gem2Transform.Position = glm::vec2(20, 20);
	gem2.AddComponent<VelocityComponent>();
	auto& gem2Comp = gem2.AddComponent<XPGemComponent>(1);

	// Update system
	system.OnUpdate(0.016f);

	// Gem1 should be attracted
	EXPECT_TRUE(gem1Comp.IsAttracted);

	// Gem2 should NOT be attracted
	EXPECT_FALSE(gem2Comp.IsAttracted);
}
