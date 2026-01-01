#include <gtest/gtest.h>
// PrefabSerializationTests: verifies prefab string/binary serialization,
// subtree capture, hierarchy remapping, and migration hooks.
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/PrefabSerializer.h"
#include "Pillar/ECS/SceneSerializer.h"
#include "Pillar/ECS/Components/Core/HierarchyComponent.h"
#include "Pillar/ECS/Components/Core/UUIDComponent.h"
#include "Pillar/ECS/Components/Core/TagComponent.h"
#include <nlohmann/json.hpp>

using namespace Pillar;

class PrefabSerializationTests : public ::testing::Test
{
protected:
	void SetUp() override
	{
		m_Scene = std::make_unique<Scene>("PrefabTestScene");
	}

	std::unique_ptr<Scene> m_Scene;
};

TEST_F(PrefabSerializationTests, SerializeSubtree_RoundTripRemapsHierarchy)
{
	Entity parent = m_Scene->CreateEntity("Parent");
	Entity child = m_Scene->CreateEntity("Child");
	child.AddComponent<HierarchyComponent>(parent.GetUUID());
	Entity grandchild = m_Scene->CreateEntity("Grandchild");
	grandchild.AddComponent<HierarchyComponent>(child.GetUUID());

	PrefabSerializer serializer(m_Scene.get());
	std::string data = serializer.SerializeToString(parent);
	ASSERT_FALSE(data.empty());

	auto loadedScene = std::make_unique<Scene>("LoadedPrefabScene");
	PrefabSerializer loader(loadedScene.get());
	Entity loadedRoot = loader.DeserializeFromString(data);

	ASSERT_TRUE(loadedRoot);
	EXPECT_EQ(loadedRoot.Name(), "Parent");

	Entity loadedChild = loadedScene->FindEntityByName("Child");
	ASSERT_TRUE(loadedChild);
	ASSERT_TRUE(loadedChild.HasComponent<HierarchyComponent>());
	EXPECT_EQ(loadedChild.GetComponent<HierarchyComponent>().ParentUUID, loadedRoot.GetUUID());

	Entity loadedGrandchild = loadedScene->FindEntityByName("Grandchild");
	ASSERT_TRUE(loadedGrandchild);
	ASSERT_TRUE(loadedGrandchild.HasComponent<HierarchyComponent>());
	EXPECT_EQ(loadedGrandchild.GetComponent<HierarchyComponent>().ParentUUID, loadedChild.GetUUID());
}

TEST_F(PrefabSerializationTests, PreserveUUIDsOptionKeepsIdentity)
{
	uint64_t parentUUID = 1111;
	uint64_t childUUID = 2222;
	Entity parent = m_Scene->CreateEntityWithUUID(parentUUID, "ParentWithUUID");
	Entity child = m_Scene->CreateEntityWithUUID(childUUID, "ChildWithUUID");
	child.AddComponent<HierarchyComponent>(parentUUID);

	PrefabOptions options;
	options.PreserveUUIDs = true;
	PrefabSerializer serializer(m_Scene.get());
	std::string data = serializer.SerializeToString(parent, options);

	auto loadedScene = std::make_unique<Scene>("LoadedScene");
	PrefabSerializer loader(loadedScene.get());
	Entity loadedRoot = loader.DeserializeFromString(data, options);

	ASSERT_TRUE(loadedRoot);
	EXPECT_EQ(loadedRoot.GetUUID(), parentUUID);

	Entity loadedChild = loadedScene->FindEntityByUUID(childUUID);
	ASSERT_TRUE(loadedChild);
	EXPECT_EQ(loadedChild.GetComponent<HierarchyComponent>().ParentUUID, parentUUID);
}

TEST(SceneSerializerMigrationTests, MigrationCallbackAppliedDuringStringLoad)
{
	Scene scene("OldScene");
	bool migrationCalled = false;

	SceneSerializer::SetMigrationCallback([
		&](nlohmann::json& root, const std::string& fromVersion, const std::string& targetVersion)
	{
		migrationCalled = true;
		root["scene"]["name"] = "MigratedScene";
		root["scene"]["version"] = targetVersion;
	});

	std::string jsonData = R"({
		"scene": { "name": "Legacy", "version": "0.9.0" },
		"entities": []
	})";

	SceneSerializer serializer(&scene);
	EXPECT_TRUE(serializer.DeserializeFromString(jsonData));
	EXPECT_TRUE(migrationCalled);
	EXPECT_EQ(scene.GetName(), "MigratedScene");

	SceneSerializer::SetMigrationCallback(nullptr);
}
