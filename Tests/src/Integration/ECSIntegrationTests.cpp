#include <gtest/gtest.h>
// ECSIntegrationTests: integration tests covering entity lifecycle, systems
// (velocity, bullet), serialization round-trips, object pools, and SceneManager.
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/SceneManager.h"
#include "Pillar/ECS/SceneSerializer.h"
#include "Pillar/ECS/ObjectPool.h"
#include "Pillar/ECS/SpecializedPools.h"
#include "Pillar/ECS/Components/Core/TagComponent.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Core/UUIDComponent.h"
#include "Pillar/ECS/Components/Core/HierarchyComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"
#include "Pillar/ECS/Components/Gameplay/BulletComponent.h"
#include "Pillar/ECS/Systems/VelocityIntegrationSystem.h"
#include <filesystem>

using namespace Pillar;

// ============================================================================
// ECS Integration Tests
// Tests that verify multiple ECS subsystems work together correctly
// ============================================================================

class ECSIntegrationTests : public ::testing::Test
{
protected:
	void SetUp() override
	{
		m_Scene = std::make_unique<Scene>("IntegrationTestScene");
	}

	void TearDown() override
	{
		m_Scene.reset();
	}

	std::unique_ptr<Scene> m_Scene;
};

// -----------------------------------------------------------------------------
// Entity Lifecycle Integration
// -----------------------------------------------------------------------------

TEST_F(ECSIntegrationTests, EntityCreation_AllCoreComponentsAdded)
{
	// Integration test: Creating an entity should add all default components
	Entity entity = m_Scene->CreateEntity("TestEntity");

	EXPECT_TRUE(entity);
	EXPECT_TRUE(entity.HasComponent<TagComponent>());
	EXPECT_TRUE(entity.HasComponent<TransformComponent>());
	EXPECT_TRUE(entity.HasComponent<UUIDComponent>());
	EXPECT_EQ(entity.GetComponent<TagComponent>().Tag, "TestEntity");
}

TEST_F(ECSIntegrationTests, EntityDestruction_RemovesFromAllSystems)
{
	Entity entity = m_Scene->CreateEntity("ToBeDestroyed");
	entity.AddComponent<VelocityComponent>(glm::vec2(10.0f, 0.0f));
	
	uint64_t uuid = entity.GetComponent<UUIDComponent>().UUID;
	EXPECT_TRUE(m_Scene->FindEntityByUUID(uuid));

	m_Scene->DestroyEntity(entity);

	// Entity should no longer be findable
	EXPECT_FALSE(m_Scene->FindEntityByUUID(uuid));
}

TEST_F(ECSIntegrationTests, MultipleEntities_IndependentComponents)
{
	Entity player = m_Scene->CreateEntity("Player");
	Entity enemy = m_Scene->CreateEntity("Enemy");

	auto& playerTransform = player.GetComponent<TransformComponent>();
	auto& enemyTransform = enemy.GetComponent<TransformComponent>();

	playerTransform.Position = glm::vec2(100.0f, 50.0f);
	enemyTransform.Position = glm::vec2(-50.0f, 30.0f);

	// Verify components are independent
	EXPECT_FLOAT_EQ(player.GetComponent<TransformComponent>().Position.x, 100.0f);
	EXPECT_FLOAT_EQ(enemy.GetComponent<TransformComponent>().Position.x, -50.0f);
}

// -----------------------------------------------------------------------------
// System Integration
// -----------------------------------------------------------------------------

TEST_F(ECSIntegrationTests, VelocitySystem_UpdatesTransform)
{
	VelocityIntegrationSystem velocitySystem;
	velocitySystem.OnAttach(m_Scene.get());

	Entity entity = m_Scene->CreateEntity("MovingEntity");
	auto& velocity = entity.AddComponent<VelocityComponent>();
	velocity.Velocity = glm::vec2(100.0f, 50.0f);

	// Simulate 1 second
	velocitySystem.OnUpdate(1.0f);

	auto& transform = entity.GetComponent<TransformComponent>();
	EXPECT_NEAR(transform.Position.x, 100.0f, 0.1f);
	EXPECT_NEAR(transform.Position.y, 50.0f, 0.1f);
}

TEST_F(ECSIntegrationTests, MultipleSystems_WorkTogether)
{
	// Test velocity system with bullet aging (without Box2D physics)
	VelocityIntegrationSystem velocitySystem;
	velocitySystem.OnAttach(m_Scene.get());

	// Create a bullet at non-zero position
	Entity bullet = m_Scene->CreateEntity("Bullet");
	bullet.GetComponent<TransformComponent>().Position = glm::vec2(100.0f, 100.0f);
	auto& velocity = bullet.AddComponent<VelocityComponent>(glm::vec2(500.0f, 0.0f));
	auto& bulletComp = bullet.AddComponent<BulletComponent>();
	bulletComp.Lifetime = 2.0f;

	// Simulate game loop
	float dt = 0.016f; // ~60fps
	for (int frame = 0; frame < 60; frame++) // 1 second of simulation
	{
		velocitySystem.OnUpdate(dt);
		// Manually age bullet (BulletCollisionSystem would do this)
		bulletComp.TimeAlive += dt;
	}

	// Bullet should have moved and aged
	auto& transform = bullet.GetComponent<TransformComponent>();
	EXPECT_GT(transform.Position.x, 500.0f); // Should have moved significantly (100 + ~500)
	EXPECT_GT(bulletComp.TimeAlive, 0.9f); // Should have aged ~1 second
}

// -----------------------------------------------------------------------------
// Serialization Integration
// -----------------------------------------------------------------------------

TEST_F(ECSIntegrationTests, SceneRoundTrip_PreservesAllData)
{
	std::filesystem::path testFile = std::filesystem::temp_directory_path() / "ecs_integration_test.json";

	// Create scene with various components
	Entity player = m_Scene->CreateEntity("Player");
	auto& playerVel = player.AddComponent<VelocityComponent>();
	playerVel.Velocity = glm::vec2(10.0f, 5.0f);
	playerVel.MaxSpeed = 100.0f;

	Entity enemy = m_Scene->CreateEntity("Enemy");
	auto& enemyTransform = enemy.GetComponent<TransformComponent>();
	enemyTransform.Position = glm::vec2(50.0f, 50.0f);
	enemyTransform.Rotation = 1.57f;

	uint64_t playerUUID = player.GetComponent<UUIDComponent>().UUID;
	uint64_t enemyUUID = enemy.GetComponent<UUIDComponent>().UUID;

	// Serialize
	{
		SceneSerializer serializer(m_Scene.get());
		ASSERT_TRUE(serializer.Serialize(testFile.string()));
	}

	// Load into new scene
	Scene loadedScene;
	{
		SceneSerializer serializer(&loadedScene);
		ASSERT_TRUE(serializer.Deserialize(testFile.string()));
	}

	// Verify entities
	EXPECT_EQ(loadedScene.GetEntityCount(), 2);

	Entity loadedPlayer = loadedScene.FindEntityByUUID(playerUUID);
	EXPECT_TRUE(loadedPlayer);
	EXPECT_TRUE(loadedPlayer.HasComponent<VelocityComponent>());
	EXPECT_FLOAT_EQ(loadedPlayer.GetComponent<VelocityComponent>().Velocity.x, 10.0f);

	Entity loadedEnemy = loadedScene.FindEntityByUUID(enemyUUID);
	EXPECT_TRUE(loadedEnemy);
	EXPECT_FLOAT_EQ(loadedEnemy.GetComponent<TransformComponent>().Position.x, 50.0f);

	// Cleanup
	std::filesystem::remove(testFile);
}

// -----------------------------------------------------------------------------
// Object Pool Integration
// -----------------------------------------------------------------------------

TEST_F(ECSIntegrationTests, BulletPool_IntegrationWithSystems)
{
	// Test bullet pool with velocity system only (no physics/Box2D)
	VelocityIntegrationSystem velocitySystem;
	velocitySystem.OnAttach(m_Scene.get());

	BulletPool bulletPool;
	bulletPool.Init(m_Scene.get(), 50);

	Entity player = m_Scene->CreateEntity("Player");

	// Spawn bullets from pool at non-zero positions
	std::vector<Entity> bullets;
	for (int i = 0; i < 10; i++)
	{
		Entity bullet = bulletPool.SpawnBullet(
			glm::vec2(100.0f + i * 10.0f, 100.0f),  // Non-zero position
			glm::vec2(1.0f, 0.0f),
			500.0f,
			player,
			25.0f,
			0.5f  // Short lifetime for test
		);
		bullets.push_back(bullet);
	}

	EXPECT_EQ(bulletPool.GetActiveCount(), 10);

	// Simulate velocity updates
	float dt = 0.016f;
	for (int frame = 0; frame < 30; frame++)
	{
		velocitySystem.OnUpdate(dt);
	}

	// Verify bullets have moved
	auto& transform = bullets[0].GetComponent<TransformComponent>();
	EXPECT_GT(transform.Position.x, 100.0f);  // Should have moved right
}

// -----------------------------------------------------------------------------
// Scene Manager Integration
// -----------------------------------------------------------------------------

class SceneManagerIntegrationTests : public ::testing::Test
{
protected:
	void SetUp() override
	{
		SceneManager::Get().Clear();
	}

	void TearDown() override
	{
		SceneManager::Get().Clear();
	}
};

TEST_F(SceneManagerIntegrationTests, SceneTransition_PreservesGameState)
{
	auto scene1 = SceneManager::Get().CreateScene("Level1");
	auto scene2 = SceneManager::Get().CreateScene("Level2");

	// Verify scenes were created
	ASSERT_NE(scene1, nullptr);
	ASSERT_NE(scene2, nullptr);

	// Set up scene 1 - create entity before setting active
	Entity player1 = scene1->CreateEntity("Player");
	player1.GetComponent<TransformComponent>().Position = glm::vec2(100.0f, 100.0f);

	// Set up scene 2
	Entity player2 = scene2->CreateEntity("Player");
	player2.GetComponent<TransformComponent>().Position = glm::vec2(0.0f, 0.0f);

	// Set active and transition
	SceneManager::Get().SetActiveScene("Level1");
	SceneManager::Get().RequestSceneChange("Level2");
	SceneManager::Get().OnUpdate(0.016f);

	// Verify scene changed
	EXPECT_EQ(SceneManager::Get().GetActiveSceneName(), "Level2");

	// Scene 1 data should still exist
	EXPECT_EQ(scene1->GetEntityCount(), 1);
}

TEST_F(SceneManagerIntegrationTests, MultipleScenes_IndependentEntities)
{
	auto gameScene = SceneManager::Get().CreateScene("Game");
	auto uiScene = SceneManager::Get().CreateScene("UI");

	ASSERT_NE(gameScene, nullptr);
	ASSERT_NE(uiScene, nullptr);

	// Create entities in each scene
	Entity gamePlayer = gameScene->CreateEntity("Player");
	Entity uiButton = uiScene->CreateEntity("Button");

	gamePlayer.AddComponent<VelocityComponent>(glm::vec2(10.0f, 0.0f));

	// Verify entities are scene-specific
	EXPECT_EQ(gameScene->GetEntityCount(), 1);
	EXPECT_EQ(uiScene->GetEntityCount(), 1);
}

TEST_F(SceneManagerIntegrationTests, SceneLoadSave_Integration)
{
	std::filesystem::path testFile = std::filesystem::temp_directory_path() / "scene_manager_test.json";

	// Create and populate scene via manager
	auto scene = SceneManager::Get().CreateScene("SaveTest");
	ASSERT_NE(scene, nullptr);

	Entity entity = scene->CreateEntity("TestEntity");
	entity.GetComponent<TransformComponent>().Position = glm::vec2(42.0f, 24.0f);
	entity.AddComponent<VelocityComponent>(glm::vec2(5.0f, 5.0f));

	// Set as active before saving
	SceneManager::Get().SetActiveScene("SaveTest");

	// Save via manager
	ASSERT_TRUE(SceneManager::Get().SaveScene(testFile.string()));

	// Clear and reload
	SceneManager::Get().Clear();
	EXPECT_EQ(SceneManager::Get().GetSceneCount(), 0);

	ASSERT_TRUE(SceneManager::Get().LoadScene(testFile.string(), "LoadedScene"));

	// Verify
	EXPECT_EQ(SceneManager::Get().GetSceneCount(), 1);
	auto loadedScene = SceneManager::Get().GetScene("LoadedScene");
	ASSERT_NE(loadedScene, nullptr);
	EXPECT_EQ(loadedScene->GetEntityCount(), 1);

	// Cleanup
	std::filesystem::remove(testFile);
}
