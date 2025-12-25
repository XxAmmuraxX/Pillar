#include <gtest/gtest.h>
#include <algorithm>
#include <vector>
// SceneTests: basic Scene API tests covering entity creation/destruction,
// UUID uniqueness and default component values.
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/Components/Core/TagComponent.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Core/UUIDComponent.h"

using namespace Pillar;

namespace {
	struct DummyComponent
	{
		int Value = 0;
	};
}

// ========================================
// Scene Tests
// ========================================

TEST(SceneTests, Constructor_CreatesEmptyScene)
{
	Scene scene;
	EXPECT_EQ(scene.GetRegistry().size(), 0);
}

TEST(SceneTests, CreateEntity_ReturnsValidEntity)
{
	Scene scene;
	Entity entity = scene.CreateEntity("TestEntity");

	EXPECT_TRUE(entity);
	EXPECT_TRUE(entity.HasComponent<TagComponent>());
	EXPECT_TRUE(entity.HasComponent<TransformComponent>());
	EXPECT_TRUE(entity.HasComponent<UUIDComponent>());
	EXPECT_EQ(entity.GetComponent<TagComponent>().Tag, "TestEntity");
}

TEST(SceneTests, CreateEntity_DefaultName)
{
	Scene scene;
	Entity entity = scene.CreateEntity();

	EXPECT_TRUE(entity);
	EXPECT_EQ(entity.GetComponent<TagComponent>().Tag, "Entity");
}

TEST(SceneTests, CreateMultipleEntities_AllValid)
{
	Scene scene;
	Entity entity1 = scene.CreateEntity("Entity1");
	Entity entity2 = scene.CreateEntity("Entity2");
	Entity entity3 = scene.CreateEntity("Entity3");

	EXPECT_TRUE(entity1);
	EXPECT_TRUE(entity2);
	EXPECT_TRUE(entity3);
	EXPECT_NE(entity1, entity2);
	EXPECT_NE(entity2, entity3);
	EXPECT_EQ(scene.GetRegistry().size(), 3);
}

TEST(SceneTests, DestroyEntity_RemovesEntity)
{
	Scene scene;
	Entity entity = scene.CreateEntity("TestEntity");
	entt::entity handle = entity;
	EXPECT_TRUE(entity);
	EXPECT_EQ(scene.GetRegistry().size(), 1);

	scene.DestroyEntity(entity);
	// After destroying, the registry should not have the entity anymore
	EXPECT_FALSE(scene.GetRegistry().valid(handle));
	// Registry might still have size > 0 due to internal storage, 
	// so we check entity count using alive() instead
	EXPECT_EQ(scene.GetRegistry().alive(), 0);
}

TEST(SceneTests, DestroyEntity_InvalidatesHandle)
{
	Scene scene;
	Entity entity = scene.CreateEntity("TestEntity");
	entt::entity handle = entity;

	scene.DestroyEntity(entity);

	// EnTT doesn't invalidate the handle, but the registry won't have the entity
	EXPECT_FALSE(scene.GetRegistry().valid(handle));
}

TEST(SceneTests, UUID_Unique)
{
	Scene scene;
	Entity entity1 = scene.CreateEntity();
	Entity entity2 = scene.CreateEntity();

	uint64_t uuid1 = entity1.GetComponent<UUIDComponent>().UUID;
	uint64_t uuid2 = entity2.GetComponent<UUIDComponent>().UUID;

	EXPECT_NE(uuid1, uuid2);
}

TEST(SceneTests, TransformComponent_DefaultValues)
{
	Scene scene;
	Entity entity = scene.CreateEntity();

	auto& transform = entity.GetComponent<TransformComponent>();
	EXPECT_EQ(transform.Position.x, 0.0f);
	EXPECT_EQ(transform.Position.y, 0.0f);
	EXPECT_EQ(transform.Rotation, 0.0f);
	EXPECT_EQ(transform.Scale.x, 1.0f);
	EXPECT_EQ(transform.Scale.y, 1.0f);
}

TEST(SceneTests, TransformComponent_GetTransform)
{
	Scene scene;
	Entity entity = scene.CreateEntity();

	auto& transform = entity.GetComponent<TransformComponent>();
	transform.Position = glm::vec2(10.0f, 20.0f);
	transform.Rotation = glm::radians(45.0f);
	transform.Scale = glm::vec2(2.0f, 2.0f);

	glm::mat4 mat = transform.GetTransform();

	// Basic check that matrix is not identity
	EXPECT_NE(mat, glm::mat4(1.0f));
}

TEST(SceneTests, EachEntity_VisitsAllEntities)
{
	Scene scene;
	Entity a = scene.CreateEntity("A");
	Entity b = scene.CreateEntity("B");
	Entity c = scene.CreateEntity("C");

	std::vector<std::string> names;
	scene.EachEntity([&](Entity entity)
	{
		names.push_back(entity.Name());
	});

	EXPECT_EQ(names.size(), 3u);
	EXPECT_NE(std::find(names.begin(), names.end(), "A"), names.end());
	EXPECT_NE(std::find(names.begin(), names.end(), "B"), names.end());
	EXPECT_NE(std::find(names.begin(), names.end(), "C"), names.end());
}

TEST(SceneTests, ForEach_ProvidesComponentsAndEntity)
{
	Scene scene;
	Entity entity = scene.CreateEntity("Target");
	entity.AddComponent<DummyComponent>().Value = 1;

	scene.ForEach<TagComponent, DummyComponent>([](Entity e, TagComponent& tag, DummyComponent& dummy)
	{
		tag.Tag = "Renamed";
		dummy.Value = 99;
	});

	EXPECT_EQ(entity.Name(), "Renamed");
	EXPECT_EQ(entity.GetComponent<DummyComponent>().Value, 99);
}
