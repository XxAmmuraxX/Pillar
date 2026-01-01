#include <gtest/gtest.h>
// VelocityIntegrationTests: verifies VelocityIntegrationSystem updates entity
// transforms correctly, including acceleration, drag, max speed and dirty flags.
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"
#include "Pillar/ECS/Systems/VelocityIntegrationSystem.h"

using namespace Pillar;

// ========================================
// VelocityIntegrationSystem Tests
// ========================================

TEST(VelocityIntegrationTests, IntegrateVelocity_UpdatesPosition)
{
	Scene scene;
	VelocityIntegrationSystem system;
	system.OnAttach(&scene);

	Entity entity = scene.CreateEntity();
	// TransformComponent already added by CreateEntity
	entity.AddComponent<VelocityComponent>(glm::vec2(10, 0)); // 10 m/s right

	system.OnUpdate(1.0f); // 1 second

	auto& transform = entity.GetComponent<TransformComponent>();
	EXPECT_NEAR(transform.Position.x, 10.0f, 0.001f);
	EXPECT_NEAR(transform.Position.y, 0.0f, 0.001f);
}

TEST(VelocityIntegrationTests, Acceleration_AffectsVelocity)
{
	Scene scene;
	VelocityIntegrationSystem system;
	system.OnAttach(&scene);

	Entity entity = scene.CreateEntity();
	// TransformComponent already added by CreateEntity
	auto& velocity = entity.AddComponent<VelocityComponent>();
	velocity.Velocity = glm::vec2(0, 0);
	velocity.Acceleration = glm::vec2(0, -10); // Gravity-like acceleration

	system.OnUpdate(1.0f); // 1 second

	auto& vel = entity.GetComponent<VelocityComponent>();
	EXPECT_NEAR(vel.Velocity.x, 0.0f, 0.001f);
	EXPECT_NEAR(vel.Velocity.y, -10.0f, 0.001f);
}

TEST(VelocityIntegrationTests, Drag_ReducesVelocity)
{
	Scene scene;
	VelocityIntegrationSystem system;
	system.OnAttach(&scene);

	Entity entity = scene.CreateEntity();
	// TransformComponent already added by CreateEntity
	auto& velocity = entity.AddComponent<VelocityComponent>();
	velocity.Velocity = glm::vec2(100, 0);
	velocity.Drag = 0.5f; // 50% drag per second

	system.OnUpdate(1.0f); // 1 second

	auto& vel = entity.GetComponent<VelocityComponent>();
	// After 1 second with 50% drag: velocity = 100 * (1 - 0.5) = 50
	EXPECT_NEAR(vel.Velocity.x, 50.0f, 0.001f);
}

TEST(VelocityIntegrationTests, MaxSpeed_ClampsVelocity)
{
	Scene scene;
	VelocityIntegrationSystem system;
	system.OnAttach(&scene);

	Entity entity = scene.CreateEntity();
	// TransformComponent already added by CreateEntity
	auto& velocity = entity.AddComponent<VelocityComponent>();
	velocity.Velocity = glm::vec2(100, 100);
	velocity.MaxSpeed = 10.0f; // Max speed 10 m/s

	system.OnUpdate(0.01f); // Small timestep

	auto& vel = entity.GetComponent<VelocityComponent>();
	float speed = glm::length(vel.Velocity);
	EXPECT_NEAR(speed, 10.0f, 0.001f);
}

TEST(VelocityIntegrationTests, TransformDirty_SetAfterUpdate)
{
	Scene scene;
	VelocityIntegrationSystem system;
	system.OnAttach(&scene);

	Entity entity = scene.CreateEntity();
	// TransformComponent already added by CreateEntity
	auto& transform = entity.GetComponent<TransformComponent>();
	entity.AddComponent<VelocityComponent>(glm::vec2(10, 0));

	transform.Dirty = false; // Clear dirty flag

	system.OnUpdate(0.1f);

	EXPECT_TRUE(transform.Dirty); // Should be marked dirty after update
}

TEST(VelocityIntegrationTests, MultipleEntities_AllUpdated)
{
	Scene scene;
	VelocityIntegrationSystem system;
	system.OnAttach(&scene);

	// Create multiple entities
	// TransformComponent already added by CreateEntity for all
	Entity entity1 = scene.CreateEntity();
	entity1.AddComponent<VelocityComponent>(glm::vec2(1, 0));

	Entity entity2 = scene.CreateEntity();
	entity2.AddComponent<VelocityComponent>(glm::vec2(0, 2));

	Entity entity3 = scene.CreateEntity();
	entity3.AddComponent<VelocityComponent>(glm::vec2(3, 3));

	system.OnUpdate(1.0f);

	auto& t1 = entity1.GetComponent<TransformComponent>();
	auto& t2 = entity2.GetComponent<TransformComponent>();
	auto& t3 = entity3.GetComponent<TransformComponent>();

	EXPECT_NEAR(t1.Position.x, 1.0f, 0.001f);
	EXPECT_NEAR(t2.Position.y, 2.0f, 0.001f);
	EXPECT_NEAR(t3.Position.x, 3.0f, 0.001f);
	EXPECT_NEAR(t3.Position.y, 3.0f, 0.001f);
}
