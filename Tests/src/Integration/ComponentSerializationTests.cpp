#include <gtest/gtest.h>
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/SceneSerializer.h"
#include "Pillar/ECS/ComponentRegistry.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Core/TagComponent.h"
#include "Pillar/ECS/Components/Core/UUIDComponent.h"
#include "Pillar/ECS/Components/Core/HierarchyComponent.h"
#include "Pillar/ECS/Components/Rendering/SpriteComponent.h"
#include "Pillar/ECS/Components/Rendering/AnimationComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"
#include "Pillar/ECS/Components/Physics/ColliderComponent.h"
#include "Pillar/ECS/Components/Gameplay/BulletComponent.h"
#include "Pillar/ECS/Components/Gameplay/XPGemComponent.h"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace Pillar;
using json = nlohmann::json;

// ============================================================================
// Component Serialization Tests
// Tests component-level serialization correctness and edge cases
// ============================================================================

class ComponentSerializationTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_Scene = std::make_unique<Scene>("SerializationTestScene");
        
        m_TempDir = std::filesystem::temp_directory_path() / "pillar_serialization_tests";
        std::filesystem::create_directories(m_TempDir);
    }

    void TearDown() override
    {
        m_Scene.reset();
        std::error_code ec;
        std::filesystem::remove_all(m_TempDir, ec);
    }

    std::unique_ptr<Scene> m_Scene;
    std::filesystem::path m_TempDir;

    std::string GetTestFilePath(const std::string& filename)
    {
        return (m_TempDir / filename).string();
    }

    // Helper to serialize and deserialize, returning loaded entity
    Entity RoundTripEntity(Entity original)
    {
        uint64_t uuid = original.GetComponent<UUIDComponent>().UUID;
        std::string filepath = GetTestFilePath("roundtrip_" + std::to_string(uuid) + ".json");
        
        SceneSerializer serializer(m_Scene.get());
        EXPECT_TRUE(serializer.Serialize(filepath));
        
        auto newScene = std::make_unique<Scene>("LoadedScene");
        SceneSerializer newSerializer(newScene.get());
        EXPECT_TRUE(newSerializer.Deserialize(filepath));
        
        Entity loaded = newScene->FindEntityByUUID(uuid);
        
        // Keep newScene alive by moving to member for verification
        m_LoadedScene = std::move(newScene);
        return loaded;
    }
    
    std::unique_ptr<Scene> m_LoadedScene;
};

// -----------------------------------------------------------------------------
// TransformComponent Serialization Tests
// -----------------------------------------------------------------------------

TEST_F(ComponentSerializationTests, TransformComponent_ZeroValues)
{
    Entity entity = m_Scene->CreateEntity("ZeroTransform");
    auto& transform = entity.GetComponent<TransformComponent>();
    transform.Position = glm::vec2(0.0f, 0.0f);
    transform.Scale = glm::vec2(0.0f, 0.0f);
    transform.Rotation = 0.0f;
    
    Entity loaded = RoundTripEntity(entity);
    ASSERT_TRUE(loaded);
    
    auto& loadedTransform = loaded.GetComponent<TransformComponent>();
    EXPECT_NEAR(loadedTransform.Position.x, 0.0f, 0.001f);
    EXPECT_NEAR(loadedTransform.Scale.x, 0.0f, 0.001f);
    EXPECT_NEAR(loadedTransform.Rotation, 0.0f, 0.001f);
}

TEST_F(ComponentSerializationTests, TransformComponent_NegativeValues)
{
    Entity entity = m_Scene->CreateEntity("NegativeTransform");
    auto& transform = entity.GetComponent<TransformComponent>();
    transform.Position = glm::vec2(-100.0f, -200.0f);
    transform.Scale = glm::vec2(-1.0f, -2.0f);
    transform.Rotation = -90.0f;
    
    Entity loaded = RoundTripEntity(entity);
    ASSERT_TRUE(loaded);
    
    auto& loadedTransform = loaded.GetComponent<TransformComponent>();
    EXPECT_NEAR(loadedTransform.Position.x, -100.0f, 0.001f);
    EXPECT_NEAR(loadedTransform.Position.y, -200.0f, 0.001f);
    EXPECT_NEAR(loadedTransform.Scale.x, -1.0f, 0.001f);
    EXPECT_NEAR(loadedTransform.Rotation, -90.0f, 0.001f);
}

TEST_F(ComponentSerializationTests, TransformComponent_LargeValues)
{
    Entity entity = m_Scene->CreateEntity("LargeTransform");
    auto& transform = entity.GetComponent<TransformComponent>();
    transform.Position = glm::vec2(1000000.0f, 2000000.0f);
    transform.Scale = glm::vec2(1000.0f, 2000.0f);
    transform.Rotation = 360.0f * 100;  // Many rotations
    
    Entity loaded = RoundTripEntity(entity);
    ASSERT_TRUE(loaded);
    
    auto& loadedTransform = loaded.GetComponent<TransformComponent>();
    EXPECT_NEAR(loadedTransform.Position.x, 1000000.0f, 1.0f);
    EXPECT_NEAR(loadedTransform.Scale.x, 1000.0f, 0.001f);
}

TEST_F(ComponentSerializationTests, TransformComponent_SmallValues)
{
    Entity entity = m_Scene->CreateEntity("SmallTransform");
    auto& transform = entity.GetComponent<TransformComponent>();
    transform.Position = glm::vec2(0.0001f, 0.0002f);
    transform.Scale = glm::vec2(0.01f, 0.02f);
    transform.Rotation = 0.001f;
    
    Entity loaded = RoundTripEntity(entity);
    ASSERT_TRUE(loaded);
    
    auto& loadedTransform = loaded.GetComponent<TransformComponent>();
    EXPECT_NEAR(loadedTransform.Position.x, 0.0001f, 0.00001f);
    EXPECT_NEAR(loadedTransform.Scale.x, 0.01f, 0.0001f);
}

// -----------------------------------------------------------------------------
// SpriteComponent Serialization Tests
// -----------------------------------------------------------------------------

TEST_F(ComponentSerializationTests, SpriteComponent_DefaultValues)
{
    Entity entity = m_Scene->CreateEntity("DefaultSprite");
    entity.AddComponent<SpriteComponent>();
    
    Entity loaded = RoundTripEntity(entity);
    ASSERT_TRUE(loaded);
    ASSERT_TRUE(loaded.HasComponent<SpriteComponent>());
    
    auto& sprite = loaded.GetComponent<SpriteComponent>();
    EXPECT_NEAR(sprite.Color.r, 1.0f, 0.001f);
    EXPECT_NEAR(sprite.Color.g, 1.0f, 0.001f);
    EXPECT_NEAR(sprite.Color.b, 1.0f, 0.001f);
    EXPECT_NEAR(sprite.Color.a, 1.0f, 0.001f);
}

TEST_F(ComponentSerializationTests, SpriteComponent_AllFields)
{
    Entity entity = m_Scene->CreateEntity("FullSprite");
    auto& sprite = entity.AddComponent<SpriteComponent>();
    sprite.Color = glm::vec4(0.1f, 0.2f, 0.3f, 0.4f);
    sprite.Size = glm::vec2(128.0f, 64.0f);
    sprite.TexCoordMin = glm::vec2(0.25f, 0.5f);
    sprite.TexCoordMax = glm::vec2(0.75f, 1.0f);
    sprite.ZIndex = 10.0f;
    sprite.FlipX = true;
    sprite.FlipY = true;
    // Empty path avoids AssetManager lookup during deserialization
    sprite.TexturePath = "";
    
    Entity loaded = RoundTripEntity(entity);
    ASSERT_TRUE(loaded);
    ASSERT_TRUE(loaded.HasComponent<SpriteComponent>());
    
    auto& loadedSprite = loaded.GetComponent<SpriteComponent>();
    EXPECT_NEAR(loadedSprite.Color.r, 0.1f, 0.001f);
    EXPECT_NEAR(loadedSprite.Color.g, 0.2f, 0.001f);
    EXPECT_NEAR(loadedSprite.Color.b, 0.3f, 0.001f);
    EXPECT_NEAR(loadedSprite.Color.a, 0.4f, 0.001f);
    EXPECT_NEAR(loadedSprite.Size.x, 128.0f, 0.001f);
    EXPECT_NEAR(loadedSprite.Size.y, 64.0f, 0.001f);
    EXPECT_NEAR(loadedSprite.TexCoordMin.x, 0.25f, 0.001f);
    EXPECT_NEAR(loadedSprite.TexCoordMax.y, 1.0f, 0.001f);
    EXPECT_NEAR(loadedSprite.ZIndex, 10.0f, 0.001f);
    EXPECT_TRUE(loadedSprite.FlipX);
    EXPECT_TRUE(loadedSprite.FlipY);
    EXPECT_TRUE(loadedSprite.TexturePath.empty());
}

TEST_F(ComponentSerializationTests, SpriteComponent_ZeroAlpha)
{
    Entity entity = m_Scene->CreateEntity("TransparentSprite");
    auto& sprite = entity.AddComponent<SpriteComponent>();
    sprite.Color = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
    
    Entity loaded = RoundTripEntity(entity);
    ASSERT_TRUE(loaded);
    
    auto& loadedSprite = loaded.GetComponent<SpriteComponent>();
    EXPECT_NEAR(loadedSprite.Color.a, 0.0f, 0.001f);
}

TEST_F(ComponentSerializationTests, SpriteComponent_EmptyTexturePath)
{
    Entity entity = m_Scene->CreateEntity("NoTextureSprite");
    auto& sprite = entity.AddComponent<SpriteComponent>();
    sprite.TexturePath = "";
    
    Entity loaded = RoundTripEntity(entity);
    ASSERT_TRUE(loaded);
    
    auto& loadedSprite = loaded.GetComponent<SpriteComponent>();
    EXPECT_TRUE(loadedSprite.TexturePath.empty());
}

// -----------------------------------------------------------------------------
// AnimationComponent Serialization Tests
// -----------------------------------------------------------------------------

TEST_F(ComponentSerializationTests, AnimationComponent_DefaultState)
{
    Entity entity = m_Scene->CreateEntity("DefaultAnimation");
    entity.AddComponent<AnimationComponent>();
    
    Entity loaded = RoundTripEntity(entity);
    ASSERT_TRUE(loaded);
    ASSERT_TRUE(loaded.HasComponent<AnimationComponent>());
    
    auto& anim = loaded.GetComponent<AnimationComponent>();
    EXPECT_TRUE(anim.CurrentClipName.empty());
    EXPECT_EQ(anim.FrameIndex, 0);
    EXPECT_NEAR(anim.PlaybackSpeed, 1.0f, 0.001f);
    EXPECT_TRUE(anim.Playing);
}

TEST_F(ComponentSerializationTests, AnimationComponent_PlayingState)
{
    Entity entity = m_Scene->CreateEntity("PlayingAnimation");
    auto& anim = entity.AddComponent<AnimationComponent>();
    anim.CurrentClipName = "character_run";
    anim.FrameIndex = 5;
    anim.PlaybackTime = 0.25f;
    anim.PlaybackSpeed = 2.0f;
    anim.Playing = true;
    
    Entity loaded = RoundTripEntity(entity);
    ASSERT_TRUE(loaded);
    
    auto& loadedAnim = loaded.GetComponent<AnimationComponent>();
    EXPECT_EQ(loadedAnim.CurrentClipName, "character_run");
    EXPECT_EQ(loadedAnim.FrameIndex, 5);
    EXPECT_NEAR(loadedAnim.PlaybackSpeed, 2.0f, 0.001f);
    EXPECT_TRUE(loadedAnim.Playing);
}

TEST_F(ComponentSerializationTests, AnimationComponent_PausedState)
{
    Entity entity = m_Scene->CreateEntity("PausedAnimation");
    auto& anim = entity.AddComponent<AnimationComponent>();
    anim.CurrentClipName = "character_idle";
    anim.Playing = false;
    
    Entity loaded = RoundTripEntity(entity);
    ASSERT_TRUE(loaded);
    
    auto& loadedAnim = loaded.GetComponent<AnimationComponent>();
    EXPECT_FALSE(loadedAnim.Playing);
}

TEST_F(ComponentSerializationTests, AnimationComponent_ZeroSpeed)
{
    Entity entity = m_Scene->CreateEntity("FrozenAnimation");
    auto& anim = entity.AddComponent<AnimationComponent>();
    anim.PlaybackSpeed = 0.0f;
    
    Entity loaded = RoundTripEntity(entity);
    ASSERT_TRUE(loaded);
    
    auto& loadedAnim = loaded.GetComponent<AnimationComponent>();
    EXPECT_NEAR(loadedAnim.PlaybackSpeed, 0.0f, 0.001f);
}

// -----------------------------------------------------------------------------
// VelocityComponent Serialization Tests
// -----------------------------------------------------------------------------

TEST_F(ComponentSerializationTests, VelocityComponent_ZeroVelocity)
{
    Entity entity = m_Scene->CreateEntity("StationaryEntity");
    auto& vel = entity.AddComponent<VelocityComponent>();
    vel.Velocity = glm::vec2(0.0f, 0.0f);
    
    Entity loaded = RoundTripEntity(entity);
    ASSERT_TRUE(loaded);
    ASSERT_TRUE(loaded.HasComponent<VelocityComponent>());
    
    auto& loadedVel = loaded.GetComponent<VelocityComponent>();
    EXPECT_NEAR(loadedVel.Velocity.x, 0.0f, 0.001f);
    EXPECT_NEAR(loadedVel.Velocity.y, 0.0f, 0.001f);
}

TEST_F(ComponentSerializationTests, VelocityComponent_HighSpeed)
{
    Entity entity = m_Scene->CreateEntity("FastEntity");
    auto& vel = entity.AddComponent<VelocityComponent>();
    vel.Velocity = glm::vec2(10000.0f, -5000.0f);
    
    Entity loaded = RoundTripEntity(entity);
    ASSERT_TRUE(loaded);
    
    auto& loadedVel = loaded.GetComponent<VelocityComponent>();
    EXPECT_NEAR(loadedVel.Velocity.x, 10000.0f, 1.0f);
    EXPECT_NEAR(loadedVel.Velocity.y, -5000.0f, 1.0f);
}

TEST_F(ComponentSerializationTests, VelocityComponent_DiagonalMovement)
{
    Entity entity = m_Scene->CreateEntity("DiagonalEntity");
    auto& vel = entity.AddComponent<VelocityComponent>();
    vel.Velocity = glm::vec2(141.42f, 141.42f);  // ~200 magnitude diagonal
    
    Entity loaded = RoundTripEntity(entity);
    ASSERT_TRUE(loaded);
    
    auto& loadedVel = loaded.GetComponent<VelocityComponent>();
    EXPECT_NEAR(loadedVel.Velocity.x, 141.42f, 0.01f);
    EXPECT_NEAR(loadedVel.Velocity.y, 141.42f, 0.01f);
}

// -----------------------------------------------------------------------------
// ColliderComponent Serialization Tests
// -----------------------------------------------------------------------------

TEST_F(ComponentSerializationTests, ColliderComponent_CircleCollider)
{
    Entity entity = m_Scene->CreateEntity("CircleEntity");
    auto collider = ColliderComponent::Circle(2.5f);
    collider.Offset = glm::vec2(0.5f, 0.25f);
    collider.Density = 1.5f;
    collider.Friction = 0.4f;
    collider.Restitution = 0.2f;
    collider.IsSensor = false;
    entity.AddComponent<ColliderComponent>(collider);
    
    Entity loaded = RoundTripEntity(entity);
    ASSERT_TRUE(loaded);
    ASSERT_TRUE(loaded.HasComponent<ColliderComponent>());
    
    auto& loadedCollider = loaded.GetComponent<ColliderComponent>();
    EXPECT_EQ(loadedCollider.Type, ColliderType::Circle);
    EXPECT_NEAR(loadedCollider.Radius, 2.5f, 0.001f);
    EXPECT_NEAR(loadedCollider.Offset.x, 0.5f, 0.001f);
    EXPECT_NEAR(loadedCollider.Density, 1.5f, 0.001f);
    EXPECT_NEAR(loadedCollider.Friction, 0.4f, 0.001f);
    EXPECT_NEAR(loadedCollider.Restitution, 0.2f, 0.001f);
    EXPECT_FALSE(loadedCollider.IsSensor);
}

TEST_F(ComponentSerializationTests, ColliderComponent_BoxCollider)
{
    Entity entity = m_Scene->CreateEntity("BoxEntity");
    auto collider = ColliderComponent::Box(glm::vec2(1.0f, 2.0f));
    collider.IsSensor = true;
    entity.AddComponent<ColliderComponent>(collider);
    
    Entity loaded = RoundTripEntity(entity);
    ASSERT_TRUE(loaded);
    ASSERT_TRUE(loaded.HasComponent<ColliderComponent>());
    
    auto& loadedCollider = loaded.GetComponent<ColliderComponent>();
    EXPECT_EQ(loadedCollider.Type, ColliderType::Box);
    EXPECT_NEAR(loadedCollider.HalfExtents.x, 1.0f, 0.001f);
    EXPECT_NEAR(loadedCollider.HalfExtents.y, 2.0f, 0.001f);
    EXPECT_TRUE(loadedCollider.IsSensor);
}

TEST_F(ComponentSerializationTests, ColliderComponent_SensorCollider)
{
    Entity entity = m_Scene->CreateEntity("TriggerEntity");
    auto collider = ColliderComponent::Circle(5.0f);
    collider.IsSensor = true;
    collider.Density = 0.0f;  // Sensors often have no density
    entity.AddComponent<ColliderComponent>(collider);
    
    Entity loaded = RoundTripEntity(entity);
    ASSERT_TRUE(loaded);
    
    auto& loadedCollider = loaded.GetComponent<ColliderComponent>();
    EXPECT_TRUE(loadedCollider.IsSensor);
    EXPECT_NEAR(loadedCollider.Density, 0.0f, 0.001f);
}

// -----------------------------------------------------------------------------
// BulletComponent Serialization Tests
// -----------------------------------------------------------------------------

TEST_F(ComponentSerializationTests, BulletComponent_AllFields)
{
    Entity entity = m_Scene->CreateEntity("BulletEntity");
    auto& bullet = entity.AddComponent<BulletComponent>();
    bullet.Damage = 25.0f;
    bullet.Lifetime = 5.0f;
    bullet.TimeAlive = 1.5f;
    bullet.Pierce = true;
    bullet.MaxHits = 3;
    bullet.HitsRemaining = 2;
    
    Entity loaded = RoundTripEntity(entity);
    ASSERT_TRUE(loaded);
    ASSERT_TRUE(loaded.HasComponent<BulletComponent>());
    
    auto& loadedBullet = loaded.GetComponent<BulletComponent>();
    EXPECT_NEAR(loadedBullet.Damage, 25.0f, 0.001f);
    EXPECT_NEAR(loadedBullet.Lifetime, 5.0f, 0.001f);
}

// -----------------------------------------------------------------------------
// XPGemComponent Serialization Tests
// -----------------------------------------------------------------------------

TEST_F(ComponentSerializationTests, XPGemComponent_AllFields)
{
    Entity entity = m_Scene->CreateEntity("XPGemEntity");
    auto& gem = entity.AddComponent<XPGemComponent>();
    gem.XPValue = 100;
    gem.AttractionRadius = 5.0f;
    gem.MoveSpeed = 15.0f;
    gem.IsAttracted = false;
    
    Entity loaded = RoundTripEntity(entity);
    ASSERT_TRUE(loaded);
    ASSERT_TRUE(loaded.HasComponent<XPGemComponent>());
    
    auto& loadedGem = loaded.GetComponent<XPGemComponent>();
    EXPECT_EQ(loadedGem.XPValue, 100);
    EXPECT_FALSE(loadedGem.IsAttracted);
}

// Note: HierarchyComponent serialization depends on ComponentRegistry registration
// The component may not be registered for serialization in all configurations

// -----------------------------------------------------------------------------
// Multiple Components Serialization Tests
// -----------------------------------------------------------------------------

TEST_F(ComponentSerializationTests, MultipleComponents_AllPreserved)
{
    Entity entity = m_Scene->CreateEntity("FullEntity");
    
    auto& transform = entity.GetComponent<TransformComponent>();
    transform.Position = glm::vec2(10.0f, 20.0f);
    
    auto& sprite = entity.AddComponent<SpriteComponent>();
    sprite.Color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
    
    auto& anim = entity.AddComponent<AnimationComponent>();
    anim.CurrentClipName = "test_anim";
    
    auto& vel = entity.AddComponent<VelocityComponent>();
    vel.Velocity = glm::vec2(100.0f, 50.0f);
    
    entity.AddComponent<ColliderComponent>(ColliderComponent::Circle(1.0f));
    
    Entity loaded = RoundTripEntity(entity);
    ASSERT_TRUE(loaded);
    
    // All components should be present
    EXPECT_TRUE(loaded.HasComponent<TransformComponent>());
    EXPECT_TRUE(loaded.HasComponent<SpriteComponent>());
    EXPECT_TRUE(loaded.HasComponent<AnimationComponent>());
    EXPECT_TRUE(loaded.HasComponent<VelocityComponent>());
    EXPECT_TRUE(loaded.HasComponent<ColliderComponent>());
    
    // Spot check values
    EXPECT_NEAR(loaded.GetComponent<TransformComponent>().Position.x, 10.0f, 0.001f);
    EXPECT_NEAR(loaded.GetComponent<SpriteComponent>().Color.r, 0.5f, 0.001f);
    EXPECT_EQ(loaded.GetComponent<AnimationComponent>().CurrentClipName, "test_anim");
    EXPECT_NEAR(loaded.GetComponent<VelocityComponent>().Velocity.x, 100.0f, 0.001f);
    EXPECT_EQ(loaded.GetComponent<ColliderComponent>().Type, ColliderType::Circle);
}

// -----------------------------------------------------------------------------
// Edge Cases
// -----------------------------------------------------------------------------

TEST_F(ComponentSerializationTests, SpecialCharactersInTag)
{
    Entity entity = m_Scene->CreateEntity("Entity with spaces & 'special' \"chars\"");
    
    Entity loaded = RoundTripEntity(entity);
    ASSERT_TRUE(loaded);
    EXPECT_EQ(loaded.GetComponent<TagComponent>().Tag, "Entity with spaces & 'special' \"chars\"");
}

TEST_F(ComponentSerializationTests, UnicodeInTag)
{
    Entity entity = m_Scene->CreateEntity("Entity_日本語_émoji_🎮");
    
    Entity loaded = RoundTripEntity(entity);
    ASSERT_TRUE(loaded);
    EXPECT_EQ(loaded.GetComponent<TagComponent>().Tag, "Entity_日本語_émoji_🎮");
}

TEST_F(ComponentSerializationTests, EmptyTag)
{
    Entity entity = m_Scene->CreateEntity("");
    
    Entity loaded = RoundTripEntity(entity);
    ASSERT_TRUE(loaded);
    EXPECT_TRUE(loaded.GetComponent<TagComponent>().Tag.empty());
}

TEST_F(ComponentSerializationTests, VeryLongTag)
{
    std::string longTag(1000, 'a');
    Entity entity = m_Scene->CreateEntity(longTag);
    
    Entity loaded = RoundTripEntity(entity);
    ASSERT_TRUE(loaded);
    EXPECT_EQ(loaded.GetComponent<TagComponent>().Tag.length(), 1000u);
}

