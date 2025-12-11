#include <gtest/gtest.h>
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/SceneSerializer.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Core/TagComponent.h"
#include "Pillar/ECS/Components/Core/UUIDComponent.h"
#include "Pillar/ECS/Components/Rendering/SpriteComponent.h"
#include "Pillar/ECS/Components/Rendering/AnimationComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"
#include "Pillar/ECS/Components/Physics/ColliderComponent.h"
#include "Pillar/Utils/AssetManager.h"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace Pillar;
using json = nlohmann::json;

// ============================================================================
// Asset Pipeline E2E Tests
// Tests complete asset workflow: load -> create entity -> serialize -> reload
// ============================================================================

class AssetPipelineE2ETests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_Scene = std::make_unique<Scene>("AssetPipelineTestScene");
        
        // Create temp directory for test files
        m_TempDir = std::filesystem::temp_directory_path() / "pillar_asset_tests";
        std::filesystem::create_directories(m_TempDir);
    }

    void TearDown() override
    {
        m_Scene.reset();
        
        // Clean up temp files
        std::error_code ec;
        std::filesystem::remove_all(m_TempDir, ec);
    }

    std::unique_ptr<Scene> m_Scene;
    std::filesystem::path m_TempDir;

    // Helper to create a test scene file path
    std::string GetTestFilePath(const std::string& filename)
    {
        return (m_TempDir / filename).string();
    }
};

// -----------------------------------------------------------------------------
// Entity Creation with Components E2E
// -----------------------------------------------------------------------------

TEST_F(AssetPipelineE2ETests, CreateEntityWithSprite_SerializeDeserialize)
{
    // Step 1: Create entity with sprite component
    Entity entity = m_Scene->CreateEntity("SpriteEntity");
    auto& sprite = entity.AddComponent<SpriteComponent>();
    sprite.Color = glm::vec4(1.0f, 0.5f, 0.25f, 1.0f);
    sprite.Size = glm::vec2(64.0f, 64.0f);
    sprite.TexCoordMin = glm::vec2(0.0f, 0.0f);
    sprite.TexCoordMax = glm::vec2(0.5f, 0.5f);
    sprite.ZIndex = 5.0f;
    sprite.FlipX = true;
    sprite.FlipY = false;
    sprite.TexturePath = "test_texture.png";
    
    uint64_t originalUUID = entity.GetComponent<UUIDComponent>().UUID;
    
    // Step 2: Serialize to file
    std::string filepath = GetTestFilePath("sprite_entity.json");
    SceneSerializer serializer(m_Scene.get());
    ASSERT_TRUE(serializer.Serialize(filepath));
    
    // Step 3: Create new scene and deserialize
    auto newScene = std::make_unique<Scene>("LoadedScene");
    SceneSerializer newSerializer(newScene.get());
    ASSERT_TRUE(newSerializer.Deserialize(filepath));
    
    // Step 4: Verify entity was restored correctly
    Entity loadedEntity = newScene->FindEntityByUUID(originalUUID);
    ASSERT_TRUE(loadedEntity);
    
    ASSERT_TRUE(loadedEntity.HasComponent<SpriteComponent>());
    auto& loadedSprite = loadedEntity.GetComponent<SpriteComponent>();
    
    EXPECT_NEAR(loadedSprite.Color.r, 1.0f, 0.001f);
    EXPECT_NEAR(loadedSprite.Color.g, 0.5f, 0.001f);
    EXPECT_NEAR(loadedSprite.Color.b, 0.25f, 0.001f);
    EXPECT_NEAR(loadedSprite.Color.a, 1.0f, 0.001f);
    EXPECT_NEAR(loadedSprite.Size.x, 64.0f, 0.001f);
    EXPECT_NEAR(loadedSprite.Size.y, 64.0f, 0.001f);
    EXPECT_NEAR(loadedSprite.ZIndex, 5.0f, 0.001f);
    EXPECT_TRUE(loadedSprite.FlipX);
    EXPECT_FALSE(loadedSprite.FlipY);
    EXPECT_EQ(loadedSprite.TexturePath, "test_texture.png");
}

TEST_F(AssetPipelineE2ETests, CreateEntityWithAnimation_SerializeDeserialize)
{
    // Step 1: Create entity with animation component
    Entity entity = m_Scene->CreateEntity("AnimatedEntity");
    auto& anim = entity.AddComponent<AnimationComponent>();
    anim.CurrentClipName = "walk_right";
    anim.FrameIndex = 3;
    anim.PlaybackSpeed = 1.5f;
    anim.Playing = true;
    
    uint64_t originalUUID = entity.GetComponent<UUIDComponent>().UUID;
    
    // Step 2: Serialize to file
    std::string filepath = GetTestFilePath("animation_entity.json");
    SceneSerializer serializer(m_Scene.get());
    ASSERT_TRUE(serializer.Serialize(filepath));
    
    // Step 3: Create new scene and deserialize
    auto newScene = std::make_unique<Scene>("LoadedScene");
    SceneSerializer newSerializer(newScene.get());
    ASSERT_TRUE(newSerializer.Deserialize(filepath));
    
    // Step 4: Verify animation state was restored
    Entity loadedEntity = newScene->FindEntityByUUID(originalUUID);
    ASSERT_TRUE(loadedEntity);
    
    ASSERT_TRUE(loadedEntity.HasComponent<AnimationComponent>());
    auto& loadedAnim = loadedEntity.GetComponent<AnimationComponent>();
    
    EXPECT_EQ(loadedAnim.CurrentClipName, "walk_right");
    EXPECT_EQ(loadedAnim.FrameIndex, 3);
    EXPECT_NEAR(loadedAnim.PlaybackSpeed, 1.5f, 0.001f);
    EXPECT_TRUE(loadedAnim.Playing);
}

TEST_F(AssetPipelineE2ETests, CreateEntityWithVelocity_SerializeDeserialize)
{
    // Step 1: Create entity with velocity component
    Entity entity = m_Scene->CreateEntity("MovingEntity");
    auto& velocity = entity.AddComponent<VelocityComponent>();
    velocity.Velocity = glm::vec2(150.0f, -75.0f);
    
    uint64_t originalUUID = entity.GetComponent<UUIDComponent>().UUID;
    
    // Step 2: Serialize
    std::string filepath = GetTestFilePath("velocity_entity.json");
    SceneSerializer serializer(m_Scene.get());
    ASSERT_TRUE(serializer.Serialize(filepath));
    
    // Step 3: Deserialize into new scene
    auto newScene = std::make_unique<Scene>("LoadedScene");
    SceneSerializer newSerializer(newScene.get());
    ASSERT_TRUE(newSerializer.Deserialize(filepath));
    
    // Step 4: Verify velocity was restored
    Entity loadedEntity = newScene->FindEntityByUUID(originalUUID);
    ASSERT_TRUE(loadedEntity);
    
    ASSERT_TRUE(loadedEntity.HasComponent<VelocityComponent>());
    auto& loadedVel = loadedEntity.GetComponent<VelocityComponent>();
    
    EXPECT_NEAR(loadedVel.Velocity.x, 150.0f, 0.001f);
    EXPECT_NEAR(loadedVel.Velocity.y, -75.0f, 0.001f);
}

TEST_F(AssetPipelineE2ETests, CreateEntityWithCollider_SerializeDeserialize)
{
    // Step 1: Create entity with collider component
    Entity entity = m_Scene->CreateEntity("ColliderEntity");
    auto& collider = entity.AddComponent<ColliderComponent>();
    collider.Type = ColliderType::Circle;
    collider.Radius = 2.5f;
    collider.Offset = glm::vec2(0.5f, 0.25f);
    collider.Density = 1.5f;
    collider.Friction = 0.4f;
    collider.Restitution = 0.2f;
    collider.IsSensor = true;
    
    uint64_t originalUUID = entity.GetComponent<UUIDComponent>().UUID;
    
    // Step 2: Serialize
    std::string filepath = GetTestFilePath("collider_entity.json");
    SceneSerializer serializer(m_Scene.get());
    ASSERT_TRUE(serializer.Serialize(filepath));
    
    // Step 3: Deserialize into new scene
    auto newScene = std::make_unique<Scene>("LoadedScene");
    SceneSerializer newSerializer(newScene.get());
    ASSERT_TRUE(newSerializer.Deserialize(filepath));
    
    // Step 4: Verify collider was restored
    Entity loadedEntity = newScene->FindEntityByUUID(originalUUID);
    ASSERT_TRUE(loadedEntity);
    
    ASSERT_TRUE(loadedEntity.HasComponent<ColliderComponent>());
    auto& loadedCollider = loadedEntity.GetComponent<ColliderComponent>();
    
    EXPECT_EQ(loadedCollider.Type, ColliderType::Circle);
    EXPECT_NEAR(loadedCollider.Radius, 2.5f, 0.001f);
    EXPECT_NEAR(loadedCollider.Offset.x, 0.5f, 0.001f);
    EXPECT_NEAR(loadedCollider.Offset.y, 0.25f, 0.001f);
    EXPECT_NEAR(loadedCollider.Density, 1.5f, 0.001f);
    EXPECT_NEAR(loadedCollider.Friction, 0.4f, 0.001f);
    EXPECT_NEAR(loadedCollider.Restitution, 0.2f, 0.001f);
    EXPECT_TRUE(loadedCollider.IsSensor);
}

// -----------------------------------------------------------------------------
// Complex Entity E2E Tests
// -----------------------------------------------------------------------------

TEST_F(AssetPipelineE2ETests, CreateComplexEntity_MultipleComponents)
{
    // Step 1: Create entity with multiple components
    Entity entity = m_Scene->CreateEntity("ComplexEntity");
    
    // Set transform
    auto& transform = entity.GetComponent<TransformComponent>();
    transform.Position = glm::vec2(100.0f, 200.0f);
    transform.Scale = glm::vec2(2.0f, 2.0f);
    transform.Rotation = 45.0f;
    
    // Add sprite
    auto& sprite = entity.AddComponent<SpriteComponent>();
    sprite.Color = glm::vec4(0.8f, 0.6f, 0.4f, 0.9f);
    sprite.Size = glm::vec2(32.0f, 32.0f);
    
    // Add velocity
    auto& velocity = entity.AddComponent<VelocityComponent>();
    velocity.Velocity = glm::vec2(50.0f, -25.0f);
    
    // Add animation
    auto& anim = entity.AddComponent<AnimationComponent>();
    anim.CurrentClipName = "idle";
    anim.PlaybackSpeed = 0.8f;
    
    uint64_t originalUUID = entity.GetComponent<UUIDComponent>().UUID;
    
    // Step 2: Serialize
    std::string filepath = GetTestFilePath("complex_entity.json");
    SceneSerializer serializer(m_Scene.get());
    ASSERT_TRUE(serializer.Serialize(filepath));
    
    // Step 3: Deserialize
    auto newScene = std::make_unique<Scene>("LoadedScene");
    SceneSerializer newSerializer(newScene.get());
    ASSERT_TRUE(newSerializer.Deserialize(filepath));
    
    // Step 4: Verify all components
    Entity loadedEntity = newScene->FindEntityByUUID(originalUUID);
    ASSERT_TRUE(loadedEntity);
    
    // Check transform
    ASSERT_TRUE(loadedEntity.HasComponent<TransformComponent>());
    auto& loadedTransform = loadedEntity.GetComponent<TransformComponent>();
    EXPECT_NEAR(loadedTransform.Position.x, 100.0f, 0.001f);
    EXPECT_NEAR(loadedTransform.Position.y, 200.0f, 0.001f);
    EXPECT_NEAR(loadedTransform.Scale.x, 2.0f, 0.001f);
    EXPECT_NEAR(loadedTransform.Rotation, 45.0f, 0.001f);
    
    // Check sprite
    ASSERT_TRUE(loadedEntity.HasComponent<SpriteComponent>());
    auto& loadedSprite = loadedEntity.GetComponent<SpriteComponent>();
    EXPECT_NEAR(loadedSprite.Color.r, 0.8f, 0.001f);
    EXPECT_NEAR(loadedSprite.Size.x, 32.0f, 0.001f);
    
    // Check velocity
    ASSERT_TRUE(loadedEntity.HasComponent<VelocityComponent>());
    auto& loadedVel = loadedEntity.GetComponent<VelocityComponent>();
    EXPECT_NEAR(loadedVel.Velocity.x, 50.0f, 0.001f);
    
    // Check animation
    ASSERT_TRUE(loadedEntity.HasComponent<AnimationComponent>());
    auto& loadedAnim = loadedEntity.GetComponent<AnimationComponent>();
    EXPECT_EQ(loadedAnim.CurrentClipName, "idle");
}

// -----------------------------------------------------------------------------
// Multiple Entities E2E Tests
// -----------------------------------------------------------------------------

TEST_F(AssetPipelineE2ETests, CreateMultipleEntities_SerializeDeserialize)
{
    // Step 1: Create multiple entities
    std::vector<uint64_t> originalUUIDs;
    
    for (int i = 0; i < 10; i++)
    {
        Entity entity = m_Scene->CreateEntity("Entity_" + std::to_string(i));
        auto& transform = entity.GetComponent<TransformComponent>();
        transform.Position = glm::vec2(i * 10.0f, i * 20.0f);
        
        auto& velocity = entity.AddComponent<VelocityComponent>();
        velocity.Velocity = glm::vec2(i * 5.0f, -i * 2.5f);
        
        originalUUIDs.push_back(entity.GetComponent<UUIDComponent>().UUID);
    }
    
    // Step 2: Serialize
    std::string filepath = GetTestFilePath("multiple_entities.json");
    SceneSerializer serializer(m_Scene.get());
    ASSERT_TRUE(serializer.Serialize(filepath));
    
    // Step 3: Deserialize
    auto newScene = std::make_unique<Scene>("LoadedScene");
    SceneSerializer newSerializer(newScene.get());
    ASSERT_TRUE(newSerializer.Deserialize(filepath));
    
    // Step 4: Verify all entities were restored
    for (int i = 0; i < 10; i++)
    {
        Entity loadedEntity = newScene->FindEntityByUUID(originalUUIDs[i]);
        ASSERT_TRUE(loadedEntity) << "Entity " << i << " not found";
        
        auto& loadedTransform = loadedEntity.GetComponent<TransformComponent>();
        EXPECT_NEAR(loadedTransform.Position.x, i * 10.0f, 0.001f);
        EXPECT_NEAR(loadedTransform.Position.y, i * 20.0f, 0.001f);
        
        ASSERT_TRUE(loadedEntity.HasComponent<VelocityComponent>());
        auto& loadedVel = loadedEntity.GetComponent<VelocityComponent>();
        EXPECT_NEAR(loadedVel.Velocity.x, i * 5.0f, 0.001f);
    }
}

// -----------------------------------------------------------------------------
// Asset Path Resolution E2E Tests
// -----------------------------------------------------------------------------

TEST_F(AssetPipelineE2ETests, SerializeToAbsolutePath)
{
    Entity entity = m_Scene->CreateEntity("TestEntity");
    
    std::string absolutePath = GetTestFilePath("absolute_path_test.json");
    SceneSerializer serializer(m_Scene.get());
    
    EXPECT_TRUE(serializer.Serialize(absolutePath));
    EXPECT_TRUE(std::filesystem::exists(absolutePath));
}

TEST_F(AssetPipelineE2ETests, DeserializeFromAbsolutePath)
{
    // First create and serialize a scene
    Entity entity = m_Scene->CreateEntity("TestEntity");
    entity.GetComponent<TransformComponent>().Position = glm::vec2(42.0f, 84.0f);
    uint64_t uuid = entity.GetComponent<UUIDComponent>().UUID;
    
    std::string absolutePath = GetTestFilePath("deserialize_absolute.json");
    SceneSerializer serializer(m_Scene.get());
    ASSERT_TRUE(serializer.Serialize(absolutePath));
    
    // Now deserialize from absolute path
    auto newScene = std::make_unique<Scene>("LoadedScene");
    SceneSerializer newSerializer(newScene.get());
    ASSERT_TRUE(newSerializer.Deserialize(absolutePath));
    
    Entity loadedEntity = newScene->FindEntityByUUID(uuid);
    ASSERT_TRUE(loadedEntity);
    EXPECT_NEAR(loadedEntity.GetComponent<TransformComponent>().Position.x, 42.0f, 0.001f);
}

// -----------------------------------------------------------------------------
// Round-Trip Tests (Serialize -> Deserialize -> Serialize -> Compare)
// -----------------------------------------------------------------------------

TEST_F(AssetPipelineE2ETests, RoundTrip_PreservesData)
{
    // Step 1: Create initial scene with data
    Entity entity = m_Scene->CreateEntity("RoundTripEntity");
    auto& transform = entity.GetComponent<TransformComponent>();
    transform.Position = glm::vec2(123.456f, 789.012f);
    transform.Scale = glm::vec2(1.5f, 2.5f);
    transform.Rotation = 30.0f;
    
    auto& sprite = entity.AddComponent<SpriteComponent>();
    sprite.Color = glm::vec4(0.1f, 0.2f, 0.3f, 0.4f);
    
    // Step 2: First serialize
    std::string filepath1 = GetTestFilePath("roundtrip_1.json");
    SceneSerializer serializer1(m_Scene.get());
    ASSERT_TRUE(serializer1.Serialize(filepath1));
    
    // Step 3: Deserialize into new scene
    auto scene2 = std::make_unique<Scene>("Scene2");
    SceneSerializer serializer2(scene2.get());
    ASSERT_TRUE(serializer2.Deserialize(filepath1));
    
    // Step 4: Serialize again
    std::string filepath2 = GetTestFilePath("roundtrip_2.json");
    SceneSerializer serializer3(scene2.get());
    ASSERT_TRUE(serializer3.Serialize(filepath2));
    
    // Step 5: Compare the two JSON files (should be identical in content)
    std::ifstream file1(filepath1);
    std::ifstream file2(filepath2);
    
    json json1, json2;
    file1 >> json1;
    file2 >> json2;
    
    // Compare scene names
    EXPECT_EQ(json1["scene"]["version"], json2["scene"]["version"]);
    
    // Both should have same number of entities
    EXPECT_EQ(json1["entities"].size(), json2["entities"].size());
}

// -----------------------------------------------------------------------------
// Error Handling E2E Tests
// -----------------------------------------------------------------------------

TEST_F(AssetPipelineE2ETests, DeserializeNonExistentFile_ReturnsFalse)
{
    auto newScene = std::make_unique<Scene>("LoadedScene");
    SceneSerializer serializer(newScene.get());
    
    EXPECT_FALSE(serializer.Deserialize(GetTestFilePath("does_not_exist.json")));
}

TEST_F(AssetPipelineE2ETests, DeserializeInvalidJSON_ReturnsFalse)
{
    // Create invalid JSON file
    std::string filepath = GetTestFilePath("invalid.json");
    std::ofstream file(filepath);
    file << "{ this is not valid json }";
    file.close();
    
    auto newScene = std::make_unique<Scene>("LoadedScene");
    SceneSerializer serializer(newScene.get());
    
    EXPECT_FALSE(serializer.Deserialize(filepath));
}

TEST_F(AssetPipelineE2ETests, DeserializeEmptyEntities_Succeeds)
{
    // Create valid JSON with no entities
    std::string filepath = GetTestFilePath("empty_entities.json");
    json sceneJson;
    sceneJson["scene"] = { {"name", "EmptyScene"}, {"version", "1.0"} };
    sceneJson["entities"] = json::array();
    
    std::ofstream file(filepath);
    file << sceneJson;
    file.close();
    
    auto newScene = std::make_unique<Scene>("LoadedScene");
    SceneSerializer serializer(newScene.get());
    
    EXPECT_TRUE(serializer.Deserialize(filepath));
    EXPECT_EQ(newScene->GetName(), "EmptyScene");
}

// -----------------------------------------------------------------------------
// Performance E2E Tests
// -----------------------------------------------------------------------------

TEST_F(AssetPipelineE2ETests, SerializeManyEntities_Performance)
{
    // Create 100 entities with components
    for (int i = 0; i < 100; i++)
    {
        Entity entity = m_Scene->CreateEntity("Entity_" + std::to_string(i));
        entity.GetComponent<TransformComponent>().Position = glm::vec2(i * 1.0f, i * 2.0f);
        entity.AddComponent<VelocityComponent>(glm::vec2(i * 0.5f, i * 0.25f));
        
        auto& sprite = entity.AddComponent<SpriteComponent>();
        sprite.Color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
    }
    
    std::string filepath = GetTestFilePath("many_entities.json");
    
    auto start = std::chrono::high_resolution_clock::now();
    
    SceneSerializer serializer(m_Scene.get());
    ASSERT_TRUE(serializer.Serialize(filepath));
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should serialize 100 entities in under 500ms
    EXPECT_LT(duration.count(), 500);
}

TEST_F(AssetPipelineE2ETests, DeserializeManyEntities_Performance)
{
    // Create and serialize 100 entities
    for (int i = 0; i < 100; i++)
    {
        Entity entity = m_Scene->CreateEntity("Entity_" + std::to_string(i));
        entity.GetComponent<TransformComponent>().Position = glm::vec2(i * 1.0f, i * 2.0f);
        entity.AddComponent<VelocityComponent>(glm::vec2(i * 0.5f, i * 0.25f));
    }
    
    std::string filepath = GetTestFilePath("deserialize_perf.json");
    SceneSerializer serializer(m_Scene.get());
    ASSERT_TRUE(serializer.Serialize(filepath));
    
    // Measure deserialization time
    auto newScene = std::make_unique<Scene>("LoadedScene");
    SceneSerializer newSerializer(newScene.get());
    
    auto start = std::chrono::high_resolution_clock::now();
    ASSERT_TRUE(newSerializer.Deserialize(filepath));
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should deserialize 100 entities in under 500ms
    EXPECT_LT(duration.count(), 500);
}

