#include <gtest/gtest.h>
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/SceneManager.h"
#include "Pillar/ECS/SceneSerializer.h"
#include "Pillar/ECS/Components/Core/TagComponent.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Core/UUIDComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"
#include "Pillar/ECS/Components/Gameplay/XPGemComponent.h"
#include <filesystem>

using namespace Pillar;

// ========================================
// Scene Serializer Tests
// ========================================

class SceneSerializerTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Use absolute path in temp directory to avoid AssetManager resolution
        m_TestFilePath = std::filesystem::temp_directory_path() / "pillar_test_scene.json";
    }

    void TearDown() override
    {
        // Clean up test file
        if (std::filesystem::exists(m_TestFilePath))
        {
            std::filesystem::remove(m_TestFilePath);
        }
    }

    std::filesystem::path m_TestFilePath;
};

TEST_F(SceneSerializerTests, Serialize_CreatesFile)
{
    Scene scene("TestScene");
    scene.CreateEntity("Entity1");
    scene.CreateEntity("Entity2");

    SceneSerializer serializer(&scene);
    bool result = serializer.Serialize(m_TestFilePath.string());

    EXPECT_TRUE(result);
    EXPECT_TRUE(std::filesystem::exists(m_TestFilePath));
}

TEST_F(SceneSerializerTests, Deserialize_RestoresEntities)
{
    // Create and save scene
    {
        Scene scene("TestScene");
        auto entity = scene.CreateEntity("TestEntity");
        auto& transform = entity.GetComponent<TransformComponent>();
        transform.Position = glm::vec2(10.0f, 20.0f);
        transform.Rotation = 1.5f;
        transform.Scale = glm::vec2(2.0f, 3.0f);

        SceneSerializer serializer(&scene);
        serializer.Serialize(m_TestFilePath.string());
    }

    // Load scene
    Scene loadedScene;
    SceneSerializer serializer(&loadedScene);
    bool result = serializer.Deserialize(m_TestFilePath.string());

    EXPECT_TRUE(result);
    EXPECT_EQ(loadedScene.GetEntityCount(), 1);

    auto entity = loadedScene.FindEntityByName("TestEntity");
    EXPECT_TRUE(entity);

    auto& transform = entity.GetComponent<TransformComponent>();
    EXPECT_FLOAT_EQ(transform.Position.x, 10.0f);
    EXPECT_FLOAT_EQ(transform.Position.y, 20.0f);
    EXPECT_FLOAT_EQ(transform.Rotation, 1.5f);
    EXPECT_FLOAT_EQ(transform.Scale.x, 2.0f);
    EXPECT_FLOAT_EQ(transform.Scale.y, 3.0f);
}

TEST_F(SceneSerializerTests, Deserialize_PreservesUUID)
{
    uint64_t originalUUID;

    {
        Scene scene("TestScene");
        auto entity = scene.CreateEntity("TestEntity");
        originalUUID = entity.GetComponent<UUIDComponent>().UUID;

        SceneSerializer serializer(&scene);
        serializer.Serialize(m_TestFilePath.string());
    }

    Scene loadedScene;
    SceneSerializer serializer(&loadedScene);
    serializer.Deserialize(m_TestFilePath.string());

    auto entity = loadedScene.FindEntityByUUID(originalUUID);
    EXPECT_TRUE(entity);
}

TEST_F(SceneSerializerTests, SerializeToString_ReturnsValidJSON)
{
    Scene scene("TestScene");
    scene.CreateEntity("Entity1");
    scene.CreateEntity("Entity2");

    SceneSerializer serializer(&scene);
    std::string jsonString = serializer.SerializeToString();

    EXPECT_FALSE(jsonString.empty());
    EXPECT_NE(jsonString.find("TestScene"), std::string::npos);
    EXPECT_NE(jsonString.find("Entity1"), std::string::npos);
    EXPECT_NE(jsonString.find("Entity2"), std::string::npos);
}

TEST_F(SceneSerializerTests, DeserializeFromString_Works)
{
    std::string jsonData = R"({
        "scene": { "name": "StringScene", "version": "1.0" },
        "entities": [
            { "tag": "FromString", "transform": { "position": [5.0, 5.0], "rotation": 0.0, "scale": [1.0, 1.0] } }
        ]
    })";

    Scene scene;
    SceneSerializer serializer(&scene);
    bool result = serializer.DeserializeFromString(jsonData);

    EXPECT_TRUE(result);
    EXPECT_EQ(scene.GetName(), "StringScene");
    
    auto entity = scene.FindEntityByName("FromString");
    EXPECT_TRUE(entity);
}

TEST_F(SceneSerializerTests, Serialize_VelocityComponent)
{
    {
        Scene scene("VelocityTest");
        auto entity = scene.CreateEntity("MovingEntity");
        auto& vel = entity.AddComponent<VelocityComponent>();
        vel.Velocity = glm::vec2(5.0f, -3.0f);
        vel.MaxSpeed = 15.0f;

        SceneSerializer serializer(&scene);
        serializer.Serialize(m_TestFilePath.string());
    }

    Scene loadedScene;
    SceneSerializer serializer(&loadedScene);
    serializer.Deserialize(m_TestFilePath.string());

    auto entity = loadedScene.FindEntityByName("MovingEntity");
    EXPECT_TRUE(entity);
    EXPECT_TRUE(entity.HasComponent<VelocityComponent>());

    auto& vel = entity.GetComponent<VelocityComponent>();
    EXPECT_FLOAT_EQ(vel.Velocity.x, 5.0f);
    EXPECT_FLOAT_EQ(vel.Velocity.y, -3.0f);
    EXPECT_FLOAT_EQ(vel.MaxSpeed, 15.0f);
}

TEST_F(SceneSerializerTests, Serialize_XPGemComponent)
{
    {
        Scene scene("XPGemTest");
        auto entity = scene.CreateEntity("Gem");
        entity.AddComponent<VelocityComponent>();
        auto& gem = entity.AddComponent<XPGemComponent>(25);
        gem.AttractionRadius = 5.0f;
        gem.MoveSpeed = 12.0f;

        SceneSerializer serializer(&scene);
        serializer.Serialize(m_TestFilePath.string());
    }

    Scene loadedScene;
    SceneSerializer serializer(&loadedScene);
    serializer.Deserialize(m_TestFilePath.string());

    auto entity = loadedScene.FindEntityByName("Gem");
    EXPECT_TRUE(entity);
    EXPECT_TRUE(entity.HasComponent<XPGemComponent>());

    auto& gem = entity.GetComponent<XPGemComponent>();
    EXPECT_EQ(gem.XPValue, 25);
    EXPECT_FLOAT_EQ(gem.AttractionRadius, 5.0f);
    EXPECT_FLOAT_EQ(gem.MoveSpeed, 12.0f);
}

// ========================================
// Scene Manager Tests
// ========================================

class SceneManagerTests : public ::testing::Test
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

TEST_F(SceneManagerTests, CreateScene_AddsToManager)
{
    auto& manager = SceneManager::Get();
    
    auto scene = manager.CreateScene("TestScene");
    
    EXPECT_NE(scene, nullptr);
    EXPECT_TRUE(manager.HasScene("TestScene"));
    EXPECT_EQ(manager.GetSceneCount(), 1);
}

TEST_F(SceneManagerTests, CreateScene_DuplicateNameReturnsSame)
{
    auto& manager = SceneManager::Get();
    
    auto scene1 = manager.CreateScene("TestScene");
    auto scene2 = manager.CreateScene("TestScene");
    
    EXPECT_EQ(scene1, scene2);
    EXPECT_EQ(manager.GetSceneCount(), 1);
}

TEST_F(SceneManagerTests, GetScene_ReturnsCorrectScene)
{
    auto& manager = SceneManager::Get();
    
    manager.CreateScene("Scene1");
    manager.CreateScene("Scene2");
    
    auto scene1 = manager.GetScene("Scene1");
    auto scene2 = manager.GetScene("Scene2");
    auto notFound = manager.GetScene("NonExistent");
    
    EXPECT_NE(scene1, nullptr);
    EXPECT_NE(scene2, nullptr);
    EXPECT_EQ(notFound, nullptr);
    EXPECT_EQ(scene1->GetName(), "Scene1");
    EXPECT_EQ(scene2->GetName(), "Scene2");
}

TEST_F(SceneManagerTests, SetActiveScene_ChangesActiveScene)
{
    auto& manager = SceneManager::Get();
    
    manager.CreateScene("Scene1");
    manager.CreateScene("Scene2");
    
    manager.SetActiveScene("Scene1");
    EXPECT_EQ(manager.GetActiveSceneName(), "Scene1");
    
    manager.SetActiveScene("Scene2");
    EXPECT_EQ(manager.GetActiveSceneName(), "Scene2");
}

TEST_F(SceneManagerTests, SetActiveScene_NonExistentReturnsFalse)
{
    auto& manager = SceneManager::Get();
    
    bool result = manager.SetActiveScene("NonExistent");
    
    EXPECT_FALSE(result);
}

TEST_F(SceneManagerTests, RequestSceneChange_DefersUntilUpdate)
{
    auto& manager = SceneManager::Get();
    
    manager.CreateScene("Scene1");
    manager.CreateScene("Scene2");
    manager.SetActiveScene("Scene1");
    
    manager.RequestSceneChange("Scene2");
    
    // Scene should not change immediately
    EXPECT_EQ(manager.GetActiveSceneName(), "Scene1");
    EXPECT_TRUE(manager.IsTransitioning());
    
    // After update, scene should change
    manager.OnUpdate(0.016f);
    EXPECT_EQ(manager.GetActiveSceneName(), "Scene2");
    EXPECT_FALSE(manager.IsTransitioning());
}

TEST_F(SceneManagerTests, RemoveScene_CannotRemoveActiveScene)
{
    auto& manager = SceneManager::Get();
    
    manager.CreateScene("OnlyScene");
    manager.SetActiveScene("OnlyScene");
    
    bool result = manager.RemoveScene("OnlyScene");
    
    EXPECT_FALSE(result);
    EXPECT_TRUE(manager.HasScene("OnlyScene"));
}

TEST_F(SceneManagerTests, RemoveScene_CanRemoveInactiveScene)
{
    auto& manager = SceneManager::Get();
    
    manager.CreateScene("Scene1");
    manager.CreateScene("Scene2");
    manager.SetActiveScene("Scene1");
    
    bool result = manager.RemoveScene("Scene2");
    
    EXPECT_TRUE(result);
    EXPECT_FALSE(manager.HasScene("Scene2"));
    EXPECT_EQ(manager.GetSceneCount(), 1);
}

TEST_F(SceneManagerTests, GetSceneNames_ReturnsAllScenes)
{
    auto& manager = SceneManager::Get();
    
    manager.CreateScene("Alpha");
    manager.CreateScene("Beta");
    manager.CreateScene("Gamma");
    
    auto names = manager.GetSceneNames();
    
    EXPECT_EQ(names.size(), 3);
    EXPECT_TRUE(std::find(names.begin(), names.end(), "Alpha") != names.end());
    EXPECT_TRUE(std::find(names.begin(), names.end(), "Beta") != names.end());
    EXPECT_TRUE(std::find(names.begin(), names.end(), "Gamma") != names.end());
}

TEST_F(SceneManagerTests, Clear_RemovesAllScenes)
{
    auto& manager = SceneManager::Get();
    
    manager.CreateScene("Scene1");
    manager.CreateScene("Scene2");
    manager.CreateScene("Scene3");
    
    manager.Clear();
    
    EXPECT_EQ(manager.GetSceneCount(), 0);
    EXPECT_EQ(manager.GetActiveScene(), nullptr);
}

TEST_F(SceneManagerTests, OnSceneChangeCallback_Called)
{
    auto& manager = SceneManager::Get();
    
    std::string fromScene, toScene;
    manager.SetOnSceneChangeCallback([&](const std::string& from, const std::string& to) {
        fromScene = from;
        toScene = to;
    });
    
    manager.CreateScene("Scene1");
    manager.CreateScene("Scene2");
    manager.SetActiveScene("Scene1");
    
    fromScene.clear();
    toScene.clear();
    
    manager.SetActiveScene("Scene2");
    
    EXPECT_EQ(fromScene, "Scene1");
    EXPECT_EQ(toScene, "Scene2");
}

// ========================================
// Enhanced Scene Tests
// ========================================

TEST(EnhancedSceneTests, FindEntityByName_Works)
{
    Scene scene;
    scene.CreateEntity("Player");
    scene.CreateEntity("Enemy");
    scene.CreateEntity("Item");

    auto player = scene.FindEntityByName("Player");
    auto enemy = scene.FindEntityByName("Enemy");
    auto notFound = scene.FindEntityByName("NonExistent");

    EXPECT_TRUE(player);
    EXPECT_TRUE(enemy);
    EXPECT_FALSE(notFound);
}

TEST(EnhancedSceneTests, FindEntityByUUID_Works)
{
    Scene scene;
    auto entity = scene.CreateEntity("Test");
    uint64_t uuid = entity.GetUUID();

    auto found = scene.FindEntityByUUID(uuid);
    auto notFound = scene.FindEntityByUUID(12345);

    EXPECT_TRUE(found);
    EXPECT_FALSE(notFound);
    EXPECT_EQ(found.GetUUID(), uuid);
}

TEST(EnhancedSceneTests, GetAllEntities_ReturnsAll)
{
    Scene scene;
    scene.CreateEntity("E1");
    scene.CreateEntity("E2");
    scene.CreateEntity("E3");

    auto entities = scene.GetAllEntities();

    EXPECT_EQ(entities.size(), 3);
}

TEST(EnhancedSceneTests, DuplicateEntity_CreatesCopy)
{
    Scene scene;
    auto original = scene.CreateEntity("Original");
    auto& transform = original.GetComponent<TransformComponent>();
    transform.Position = glm::vec2(5.0f, 10.0f);
    transform.Rotation = 1.5f;

    auto copy = scene.DuplicateEntity(original);

    EXPECT_TRUE(copy);
    EXPECT_NE(original.GetUUID(), copy.GetUUID());
    
    auto& copyTag = copy.GetComponent<TagComponent>();
    EXPECT_EQ(copyTag.Tag, "Original (Copy)");

    auto& copyTransform = copy.GetComponent<TransformComponent>();
    EXPECT_FLOAT_EQ(copyTransform.Position.x, 5.0f);
    EXPECT_FLOAT_EQ(copyTransform.Position.y, 10.0f);
    EXPECT_FLOAT_EQ(copyTransform.Rotation, 1.5f);
}

TEST(EnhancedSceneTests, SceneCopy_CreatesIndependentCopy)
{
    auto original = std::make_shared<Scene>("Original");
    original->CreateEntity("Entity1");
    original->CreateEntity("Entity2");

    auto copy = Scene::Copy(original);

    EXPECT_EQ(copy->GetName(), "Original");
    EXPECT_EQ(copy->GetEntityCount(), 2);
    
    // Verify entities are independent
    original->CreateEntity("Entity3");
    EXPECT_EQ(original->GetEntityCount(), 3);
    EXPECT_EQ(copy->GetEntityCount(), 2);
}

TEST(EnhancedSceneTests, CreateEntityWithUUID_PreservesUUID)
{
    Scene scene;
    uint64_t specificUUID = 9876543210;

    auto entity = scene.CreateEntityWithUUID(specificUUID, "SpecificEntity");

    EXPECT_EQ(entity.GetUUID(), specificUUID);
}
