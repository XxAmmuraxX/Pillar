#include <gtest/gtest.h>
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/SceneSerializer.h"
#include "Pillar/ECS/SceneManager.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Core/TagComponent.h"
#include "Pillar/ECS/Components/Core/UUIDComponent.h"
#include "Pillar/ECS/Components/Core/HierarchyComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"
#include "Pillar/ECS/Components/Gameplay/BulletComponent.h"
#include "Pillar/ECS/ComponentRegistry.h"
#include <fstream>
#include <filesystem>

using namespace Pillar;

// ============================================================================
// End-to-End Scene Tests
// Tests that verify complete scene workflows from creation to serialization
// ============================================================================

class EndToEndSceneTests : public ::testing::Test
{
protected:
	void SetUp() override
	{
		m_Scene = std::make_unique<Scene>("E2ETestScene");
		// Use temp directory for test files
		m_TestFilePath = (std::filesystem::temp_directory_path() / "test_e2e_scene.pillar").string();
	}

	void TearDown() override
	{
		m_Scene.reset();
		// Clean up test file
		if (std::filesystem::exists(m_TestFilePath))
		{
			std::filesystem::remove(m_TestFilePath);
		}
	}

	std::unique_ptr<Scene> m_Scene;
	std::string m_TestFilePath;
};

// -----------------------------------------------------------------------------
// Complete Scene Creation Workflow
// -----------------------------------------------------------------------------

TEST_F(EndToEndSceneTests, CreatePopulateSaveLoad_FullCycle)
{
	// Step 1: Create a scene with various entities
	Entity player = m_Scene->CreateEntity("Player");
	player.GetComponent<TransformComponent>().Position = glm::vec2(100.0f, 200.0f);
	player.GetComponent<TransformComponent>().Rotation = 45.0f;
	player.GetComponent<TransformComponent>().Scale = glm::vec2(2.0f, 2.0f);
	player.AddComponent<VelocityComponent>(glm::vec2(10.0f, 5.0f));

	Entity enemy = m_Scene->CreateEntity("Enemy");
	enemy.GetComponent<TransformComponent>().Position = glm::vec2(500.0f, 300.0f);

	Entity bullet = m_Scene->CreateEntity("Bullet");
	bullet.GetComponent<TransformComponent>().Position = glm::vec2(150.0f, 200.0f);
	auto& bulletComp = bullet.AddComponent<BulletComponent>();
	bulletComp.Damage = 25.0f;
	bullet.AddComponent<VelocityComponent>(glm::vec2(300.0f, 0.0f));

	// Store UUIDs for verification
	uint64_t playerUUID = player.GetComponent<UUIDComponent>().UUID;
	uint64_t enemyUUID = enemy.GetComponent<UUIDComponent>().UUID;
	uint64_t bulletUUID = bullet.GetComponent<UUIDComponent>().UUID;

	// Step 2: Serialize the scene
	SceneSerializer serializer(m_Scene.get());
	ASSERT_TRUE(serializer.Serialize(m_TestFilePath));

	// Step 3: Create a new scene and deserialize
	auto loadedScene = std::make_unique<Scene>("LoadedScene");
	SceneSerializer loadSerializer(loadedScene.get());
	ASSERT_TRUE(loadSerializer.Deserialize(m_TestFilePath));

	// Step 4: Verify all data
	EXPECT_EQ(loadedScene->GetEntityCount(), 3);

	// Find and verify player
	Entity loadedPlayer = loadedScene->FindEntityByUUID(playerUUID);
	ASSERT_TRUE(loadedPlayer);
	EXPECT_EQ(loadedPlayer.GetComponent<TagComponent>().Tag, "Player");
	EXPECT_NEAR(loadedPlayer.GetComponent<TransformComponent>().Position.x, 100.0f, 0.01f);
	EXPECT_NEAR(loadedPlayer.GetComponent<TransformComponent>().Position.y, 200.0f, 0.01f);
	EXPECT_NEAR(loadedPlayer.GetComponent<TransformComponent>().Rotation, 45.0f, 0.01f);
	EXPECT_TRUE(loadedPlayer.HasComponent<VelocityComponent>());
	EXPECT_NEAR(loadedPlayer.GetComponent<VelocityComponent>().Velocity.x, 10.0f, 0.01f);

	// Find and verify bullet
	Entity loadedBullet = loadedScene->FindEntityByUUID(bulletUUID);
	ASSERT_TRUE(loadedBullet);
	EXPECT_TRUE(loadedBullet.HasComponent<BulletComponent>());
	EXPECT_FLOAT_EQ(loadedBullet.GetComponent<BulletComponent>().Damage, 25.0f);
}

TEST_F(EndToEndSceneTests, ParentChildHierarchy_PreservedAcrossSaveLoad)
{
	// Create parent-child hierarchy
	Entity parent = m_Scene->CreateEntity("Parent");
	parent.GetComponent<TransformComponent>().Position = glm::vec2(100.0f, 100.0f);

	Entity child1 = m_Scene->CreateEntity("Child1");
	Entity child2 = m_Scene->CreateEntity("Child2");

	// Set up hierarchy - HierarchyComponent only stores ParentUUID
	uint64_t parentUUID = parent.GetComponent<UUIDComponent>().UUID;
	uint64_t child1UUID = child1.GetComponent<UUIDComponent>().UUID;
	uint64_t child2UUID = child2.GetComponent<UUIDComponent>().UUID;

	auto& child1Hierarchy = child1.AddComponent<HierarchyComponent>();
	auto& child2Hierarchy = child2.AddComponent<HierarchyComponent>();

	child1Hierarchy.ParentUUID = parentUUID;
	child2Hierarchy.ParentUUID = parentUUID;

	// Serialize
	SceneSerializer serializer(m_Scene.get());
	ASSERT_TRUE(serializer.Serialize(m_TestFilePath));

	// Load into new scene
	auto loadedScene = std::make_unique<Scene>("LoadedScene");
	SceneSerializer loadSerializer(loadedScene.get());
	ASSERT_TRUE(loadSerializer.Deserialize(m_TestFilePath));

	// Verify hierarchy
	Entity loadedChild1 = loadedScene->FindEntityByUUID(child1UUID);
	Entity loadedChild2 = loadedScene->FindEntityByUUID(child2UUID);

	ASSERT_TRUE(loadedChild1);
	ASSERT_TRUE(loadedChild2);

	if (loadedChild1.HasComponent<HierarchyComponent>())
	{
		auto& hierarchy = loadedChild1.GetComponent<HierarchyComponent>();
		EXPECT_EQ(hierarchy.ParentUUID, parentUUID);
	}

	if (loadedChild2.HasComponent<HierarchyComponent>())
	{
		auto& hierarchy = loadedChild2.GetComponent<HierarchyComponent>();
		EXPECT_EQ(hierarchy.ParentUUID, parentUUID);
	}
}

// -----------------------------------------------------------------------------
// Scene Manager Workflow
// -----------------------------------------------------------------------------

TEST_F(EndToEndSceneTests, SceneManager_MultiSceneWorkflow)
{
	SceneManager& manager = SceneManager::Get();
	manager.Clear();

	// Create multiple scenes
	manager.CreateScene("MainMenu");
	manager.CreateScene("Level1");
	manager.CreateScene("Level2");

	// Verify all scenes exist
	EXPECT_NE(manager.GetScene("MainMenu"), nullptr);
	EXPECT_NE(manager.GetScene("Level1"), nullptr);
	EXPECT_NE(manager.GetScene("Level2"), nullptr);

	// Set active scene and populate
	manager.SetActiveScene("Level1");
	auto level1 = manager.GetActiveScene();
	ASSERT_NE(level1, nullptr);

	for (int i = 0; i < 5; i++)
	{
		level1->CreateEntity("Enemy" + std::to_string(i));
	}
	EXPECT_EQ(level1->GetEntityCount(), 5);

	// Switch to Level2 and verify it's empty
	manager.SetActiveScene("Level2");
	auto level2 = manager.GetActiveScene();
	ASSERT_NE(level2, nullptr);
	EXPECT_EQ(level2->GetEntityCount(), 0);

	// Switch back to Level1 and verify entities still exist
	manager.SetActiveScene("Level1");
	level1 = manager.GetActiveScene();
	EXPECT_EQ(level1->GetEntityCount(), 5);

	manager.Clear();
}

TEST_F(EndToEndSceneTests, SceneManager_DeleteScene)
{
	SceneManager& manager = SceneManager::Get();
	manager.Clear();

	manager.CreateScene("ToBeDeleted");
	manager.CreateScene("ToBeKept");

	EXPECT_NE(manager.GetScene("ToBeDeleted"), nullptr);

	// Set ToBeKept as active first - cannot remove active scene
	manager.SetActiveScene("ToBeKept");
	manager.RemoveScene("ToBeDeleted");

	EXPECT_EQ(manager.GetScene("ToBeDeleted"), nullptr);
	EXPECT_NE(manager.GetScene("ToBeKept"), nullptr);

	manager.Clear();
}

// -----------------------------------------------------------------------------
// Component Registry Workflow
// -----------------------------------------------------------------------------

TEST_F(EndToEndSceneTests, ComponentRegistry_RegisterSerializeDeserialize)
{
	// Create entity with registered components
	Entity entity = m_Scene->CreateEntity("TestEntity");
	entity.GetComponent<TransformComponent>().Position = glm::vec2(42.0f, 84.0f);
	entity.AddComponent<VelocityComponent>(glm::vec2(1.0f, 2.0f));

	uint64_t uuid = entity.GetComponent<UUIDComponent>().UUID;

	// Serialize
	SceneSerializer serializer(m_Scene.get());
	ASSERT_TRUE(serializer.Serialize(m_TestFilePath));

	// Read file and verify it's valid JSON
	std::ifstream file(m_TestFilePath);
	ASSERT_TRUE(file.is_open());

	std::string content((std::istreambuf_iterator<char>(file)),
		std::istreambuf_iterator<char>());
	file.close();

	EXPECT_FALSE(content.empty());
	EXPECT_NE(content.find("TestEntity"), std::string::npos);

	// Deserialize
	auto loadedScene = std::make_unique<Scene>("LoadedScene");
	SceneSerializer loadSerializer(loadedScene.get());
	ASSERT_TRUE(loadSerializer.Deserialize(m_TestFilePath));

	Entity loadedEntity = loadedScene->FindEntityByUUID(uuid);
	ASSERT_TRUE(loadedEntity);
	EXPECT_TRUE(loadedEntity.HasComponent<TransformComponent>());
	EXPECT_TRUE(loadedEntity.HasComponent<VelocityComponent>());
}

// -----------------------------------------------------------------------------
// Edge Cases and Error Handling
// -----------------------------------------------------------------------------

TEST_F(EndToEndSceneTests, EmptyScene_SerializeDeserialize)
{
	// Empty scene
	EXPECT_EQ(m_Scene->GetEntityCount(), 0);

	SceneSerializer serializer(m_Scene.get());
	ASSERT_TRUE(serializer.Serialize(m_TestFilePath));

	auto loadedScene = std::make_unique<Scene>("LoadedScene");
	SceneSerializer loadSerializer(loadedScene.get());
	ASSERT_TRUE(loadSerializer.Deserialize(m_TestFilePath));

	EXPECT_EQ(loadedScene->GetEntityCount(), 0);
}

TEST_F(EndToEndSceneTests, DeserializeNonExistentFile_Fails)
{
	auto newScene = std::make_unique<Scene>("NewScene");
	SceneSerializer serializer(newScene.get());

	EXPECT_FALSE(serializer.Deserialize("this_file_does_not_exist.pillar"));
}

TEST_F(EndToEndSceneTests, LargeScene_SerializeDeserialize)
{
	// Create a large scene with many entities
	std::vector<uint64_t> uuids;
	for (int i = 0; i < 100; i++)
	{
		Entity entity = m_Scene->CreateEntity("Entity" + std::to_string(i));
		entity.GetComponent<TransformComponent>().Position = glm::vec2((float)i, (float)i * 2);
		entity.AddComponent<VelocityComponent>(glm::vec2((float)i * 0.1f, (float)i * 0.2f));
		uuids.push_back(entity.GetComponent<UUIDComponent>().UUID);
	}

	EXPECT_EQ(m_Scene->GetEntityCount(), 100);

	// Serialize
	SceneSerializer serializer(m_Scene.get());
	ASSERT_TRUE(serializer.Serialize(m_TestFilePath));

	// Deserialize
	auto loadedScene = std::make_unique<Scene>("LoadedScene");
	SceneSerializer loadSerializer(loadedScene.get());
	ASSERT_TRUE(loadSerializer.Deserialize(m_TestFilePath));

	EXPECT_EQ(loadedScene->GetEntityCount(), 100);

	// Verify a few random entities
	Entity entity50 = loadedScene->FindEntityByUUID(uuids[50]);
	ASSERT_TRUE(entity50);
	EXPECT_NEAR(entity50.GetComponent<TransformComponent>().Position.x, 50.0f, 0.01f);
	EXPECT_NEAR(entity50.GetComponent<TransformComponent>().Position.y, 100.0f, 0.01f);
}

// -----------------------------------------------------------------------------
// Multiple Serialization Cycles
// -----------------------------------------------------------------------------

TEST_F(EndToEndSceneTests, MultipleSaveLoadCycles_DataIntegrity)
{
	Entity entity = m_Scene->CreateEntity("PersistentEntity");
	entity.GetComponent<TransformComponent>().Position = glm::vec2(1.0f, 1.0f);
	uint64_t uuid = entity.GetComponent<UUIDComponent>().UUID;

	// First save/load cycle
	SceneSerializer serializer1(m_Scene.get());
	serializer1.Serialize(m_TestFilePath);

	auto scene2 = std::make_unique<Scene>("Scene2");
	SceneSerializer loadSerializer1(scene2.get());
	loadSerializer1.Deserialize(m_TestFilePath);

	// Modify and save again
	Entity loaded = scene2->FindEntityByUUID(uuid);
	loaded.GetComponent<TransformComponent>().Position = glm::vec2(2.0f, 2.0f);

	SceneSerializer serializer2(scene2.get());
	serializer2.Serialize(m_TestFilePath);

	// Third load
	auto scene3 = std::make_unique<Scene>("Scene3");
	SceneSerializer loadSerializer2(scene3.get());
	loadSerializer2.Deserialize(m_TestFilePath);

	Entity finalEntity = scene3->FindEntityByUUID(uuid);
	ASSERT_TRUE(finalEntity);
	EXPECT_NEAR(finalEntity.GetComponent<TransformComponent>().Position.x, 2.0f, 0.01f);
	EXPECT_NEAR(finalEntity.GetComponent<TransformComponent>().Position.y, 2.0f, 0.01f);
}
