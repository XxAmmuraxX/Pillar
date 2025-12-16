#include <gtest/gtest.h>
// EntityTests: unit tests for `Entity` API behavior including component
// management, equality, validity and handle casting.
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/Components/Core/TagComponent.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"

using namespace Pillar;

// ========================================
// Entity Tests
// ========================================

TEST(EntityTests, DefaultConstructor_InvalidEntity)
{
	Entity entity;
	EXPECT_FALSE(entity);
}

TEST(EntityTests, ValidEntity_BoolOperator)
{
	Scene scene;
	Entity entity = scene.CreateEntity();
	EXPECT_TRUE(entity);
}

TEST(EntityTests, AddComponent_Success)
{
	Scene scene;
	Entity entity = scene.CreateEntity();

	// TagComponent already exists (added by CreateEntity)
	EXPECT_TRUE(entity.HasComponent<TagComponent>());
}

TEST(EntityTests, GetComponent_ReturnsReference)
{
	Scene scene;
	Entity entity = scene.CreateEntity();

	auto& tag = entity.GetComponent<TagComponent>();
	tag.Tag = "Modified";

	EXPECT_EQ(entity.GetComponent<TagComponent>().Tag, "Modified");
}

TEST(EntityTests, HasComponent_CorrectBehavior)
{
	Scene scene;
	Entity entity = scene.CreateEntity();

	EXPECT_TRUE(entity.HasComponent<TagComponent>());
	EXPECT_TRUE(entity.HasComponent<TransformComponent>());
}

TEST(EntityTests, RemoveComponent_Success)
{
	Scene scene;
	Entity entity = scene.CreateEntity();

	EXPECT_TRUE(entity.HasComponent<TransformComponent>());
	entity.RemoveComponent<TransformComponent>();
	EXPECT_FALSE(entity.HasComponent<TransformComponent>());
}

TEST(EntityTests, Equality_SameEntity)
{
	Scene scene;
	Entity entity1 = scene.CreateEntity();
	Entity entity2 = entity1;

	EXPECT_EQ(entity1, entity2);
}

TEST(EntityTests, Equality_DifferentEntities)
{
	Scene scene;
	Entity entity1 = scene.CreateEntity();
	Entity entity2 = scene.CreateEntity();

	EXPECT_NE(entity1, entity2);
}

TEST(EntityTests, Inequality_DifferentEntities)
{
	Scene scene;
	Entity entity1 = scene.CreateEntity();
	Entity entity2 = scene.CreateEntity();

	EXPECT_TRUE(entity1 != entity2);
}

TEST(EntityTests, CastToUint32_ReturnsHandle)
{
	Scene scene;
	Entity entity = scene.CreateEntity();

	uint32_t handle = static_cast<uint32_t>(entity);
	// EnTT uses 0-based indexing, so the first entity has handle 0
	// Just verify that the cast works and returns a valid handle
	EXPECT_EQ(handle, static_cast<uint32_t>(static_cast<entt::entity>(entity)));

	// Create another entity and verify they have different handles
	Entity entity2 = scene.CreateEntity();
	uint32_t handle2 = static_cast<uint32_t>(entity2);
	EXPECT_NE(handle, handle2);
}

// Test component modification
TEST(EntityTests, ModifyTransform_Persists)
{
	Scene scene;
	Entity entity = scene.CreateEntity();

	auto& transform = entity.GetComponent<TransformComponent>();
	transform.Position = glm::vec2(100.0f, 200.0f);
	transform.Rotation = 1.5f;
	transform.Scale = glm::vec2(2.0f, 3.0f);

	// Retrieve again and verify
	auto& transform2 = entity.GetComponent<TransformComponent>();
	EXPECT_EQ(transform2.Position.x, 100.0f);
	EXPECT_EQ(transform2.Position.y, 200.0f);
	EXPECT_EQ(transform2.Rotation, 1.5f);
	EXPECT_EQ(transform2.Scale.x, 2.0f);
	EXPECT_EQ(transform2.Scale.y, 3.0f);
}
