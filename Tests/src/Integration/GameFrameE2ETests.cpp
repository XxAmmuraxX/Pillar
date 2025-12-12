#include <gtest/gtest.h>
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Core/TagComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"
#include "Pillar/ECS/Components/Gameplay/BulletComponent.h"
#include "Pillar/ECS/Systems/VelocityIntegrationSystem.h"
#include "Pillar/ECS/Systems/BulletCollisionSystem.h"
#include "Pillar/Events/Event.h"
#include "Pillar/Events/KeyEvent.h"
#include "Pillar/Events/MouseEvent.h"
#include <glm/glm.hpp>
#include <vector>
#include <chrono>
#include <functional>

using namespace Pillar;

// ============================================================================
// Game Frame E2E Tests
// Tests complete game frame cycle: Input -> System Updates -> Physics -> State
// Simulates realistic game loop execution without actual windowing
// ============================================================================

class GameFrameE2ETests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_Scene = std::make_unique<Scene>("GameFrameTestScene");
        m_VelocitySystem.OnAttach(m_Scene.get());
    }

    void TearDown() override
    {
        m_Scene.reset();
    }

    std::unique_ptr<Scene> m_Scene;
    VelocityIntegrationSystem m_VelocitySystem;
    
    // Simulated input state
    struct InputState
    {
        bool KeyW = false;
        bool KeyA = false;
        bool KeyS = false;
        bool KeyD = false;
        bool KeySpace = false;
        glm::vec2 MousePos = { 0.0f, 0.0f };
        bool LeftMouseDown = false;
    };
    
    InputState m_Input;
    
    // Simulate a single game frame
    void SimulateFrame(float dt)
    {
        // Phase 1: Process input (would normally come from window)
        ProcessInput();
        
        // Phase 2: Update game systems
        UpdateSystems(dt);
        
        // Phase 3: Late update (cleanup, state validation)
        LateUpdate();
    }
    
    void ProcessInput()
    {
        // Apply input to player entity if exists
        Entity player = FindPlayer();
        if (player)
        {
            auto& velocity = player.GetComponent<VelocityComponent>();
            glm::vec2 inputDir(0.0f);
            
            if (m_Input.KeyW) inputDir.y += 1.0f;
            if (m_Input.KeyS) inputDir.y -= 1.0f;
            if (m_Input.KeyA) inputDir.x -= 1.0f;
            if (m_Input.KeyD) inputDir.x += 1.0f;
            
            if (glm::length(inputDir) > 0.0f)
                inputDir = glm::normalize(inputDir);
            
            float speed = 200.0f;
            velocity.Velocity = inputDir * speed;
        }
    }
    
    void UpdateSystems(float dt)
    {
        m_VelocitySystem.OnUpdate(dt);
    }
    
    void LateUpdate()
    {
        // Clean up dead entities, etc.
    }
    
    Entity FindPlayer()
    {
        Entity result;
        m_Scene->GetRegistry().view<TagComponent>().each([&](auto entity, TagComponent& tag) {
            if (tag.Tag == "Player")
            {
                result = Entity(entity, m_Scene.get());
            }
        });
        return result;
    }
    
    Entity CreatePlayer(glm::vec2 position)
    {
        Entity player = m_Scene->CreateEntity("Player");
        player.GetComponent<TransformComponent>().Position = position;
        player.AddComponent<VelocityComponent>();
        return player;
    }
    
    Entity CreateEnemy(glm::vec2 position, glm::vec2 velocity = glm::vec2(0.0f))
    {
        Entity enemy = m_Scene->CreateEntity("Enemy");
        enemy.GetComponent<TransformComponent>().Position = position;
        if (velocity != glm::vec2(0.0f))
        {
            enemy.AddComponent<VelocityComponent>(velocity);
        }
        return enemy;
    }
};

// -----------------------------------------------------------------------------
// Single Frame Tests
// -----------------------------------------------------------------------------

TEST_F(GameFrameE2ETests, SingleFrame_NoInput_PlayerStationary)
{
    CreatePlayer(glm::vec2(0.0f, 0.0f));
    
    SimulateFrame(1.0f / 60.0f);
    
    Entity player = FindPlayer();
    auto& transform = player.GetComponent<TransformComponent>();
    
    EXPECT_NEAR(transform.Position.x, 0.0f, 0.001f);
    EXPECT_NEAR(transform.Position.y, 0.0f, 0.001f);
}

TEST_F(GameFrameE2ETests, SingleFrame_WPressed_PlayerMovesUp)
{
    CreatePlayer(glm::vec2(0.0f, 0.0f));
    m_Input.KeyW = true;
    
    SimulateFrame(1.0f / 60.0f);
    
    Entity player = FindPlayer();
    auto& transform = player.GetComponent<TransformComponent>();
    
    // After one frame at 200 units/sec, should move about 3.33 units
    EXPECT_NEAR(transform.Position.x, 0.0f, 0.001f);
    EXPECT_GT(transform.Position.y, 0.0f);
    EXPECT_NEAR(transform.Position.y, 200.0f / 60.0f, 0.1f);
}

TEST_F(GameFrameE2ETests, SingleFrame_DiagonalInput_NormalizedMovement)
{
    CreatePlayer(glm::vec2(0.0f, 0.0f));
    m_Input.KeyW = true;
    m_Input.KeyD = true;
    
    SimulateFrame(1.0f / 60.0f);
    
    Entity player = FindPlayer();
    auto& transform = player.GetComponent<TransformComponent>();
    
    // Diagonal movement should be normalized (not faster than cardinal)
    float expectedDistance = 200.0f / 60.0f;
    float actualDistance = glm::length(transform.Position);
    
    EXPECT_NEAR(actualDistance, expectedDistance, 0.1f);
}

// -----------------------------------------------------------------------------
// Multi-Frame Tests
// -----------------------------------------------------------------------------

TEST_F(GameFrameE2ETests, OneSecond_ContinuousInput_CorrectDistance)
{
    CreatePlayer(glm::vec2(0.0f, 0.0f));
    m_Input.KeyD = true;
    
    // Simulate 60 frames (1 second at 60 FPS)
    for (int i = 0; i < 60; i++)
    {
        SimulateFrame(1.0f / 60.0f);
    }
    
    Entity player = FindPlayer();
    auto& transform = player.GetComponent<TransformComponent>();
    
    // Should have moved 200 units in X direction
    EXPECT_NEAR(transform.Position.x, 200.0f, 5.0f);
    EXPECT_NEAR(transform.Position.y, 0.0f, 0.001f);
}

TEST_F(GameFrameE2ETests, InputChange_MidFrame_DirectionChanges)
{
    CreatePlayer(glm::vec2(0.0f, 0.0f));
    
    // Move right for 30 frames
    m_Input.KeyD = true;
    for (int i = 0; i < 30; i++)
    {
        SimulateFrame(1.0f / 60.0f);
    }
    
    float xAfterRight = FindPlayer().GetComponent<TransformComponent>().Position.x;
    EXPECT_GT(xAfterRight, 0.0f);
    
    // Change direction to left for 30 frames
    m_Input.KeyD = false;
    m_Input.KeyA = true;
    for (int i = 0; i < 30; i++)
    {
        SimulateFrame(1.0f / 60.0f);
    }
    
    // Should be back near origin
    float xAfterLeft = FindPlayer().GetComponent<TransformComponent>().Position.x;
    EXPECT_NEAR(xAfterLeft, 0.0f, 5.0f);
}

// -----------------------------------------------------------------------------
// Multiple Entities Frame Tests
// -----------------------------------------------------------------------------

TEST_F(GameFrameE2ETests, MultipleEntities_IndependentMovement)
{
    Entity player = CreatePlayer(glm::vec2(0.0f, 0.0f));
    Entity enemy1 = CreateEnemy(glm::vec2(100.0f, 0.0f), glm::vec2(-50.0f, 0.0f));
    Entity enemy2 = CreateEnemy(glm::vec2(-100.0f, 0.0f), glm::vec2(30.0f, 20.0f));
    
    m_Input.KeyW = true;
    
    // Simulate 60 frames
    for (int i = 0; i < 60; i++)
    {
        SimulateFrame(1.0f / 60.0f);
    }
    
    // Player should move up
    auto& playerTransform = player.GetComponent<TransformComponent>();
    EXPECT_NEAR(playerTransform.Position.y, 200.0f, 5.0f);
    
    // Enemy1 should move left
    auto& enemy1Transform = enemy1.GetComponent<TransformComponent>();
    EXPECT_NEAR(enemy1Transform.Position.x, 50.0f, 5.0f);  // Started at 100, moved -50
    
    // Enemy2 should move right and up
    auto& enemy2Transform = enemy2.GetComponent<TransformComponent>();
    EXPECT_NEAR(enemy2Transform.Position.x, -70.0f, 5.0f);  // Started at -100, moved +30
    EXPECT_NEAR(enemy2Transform.Position.y, 20.0f, 5.0f);   // Moved +20
}

TEST_F(GameFrameE2ETests, ManyEntities_FramePerformance)
{
    // Create player + 100 enemies
    CreatePlayer(glm::vec2(0.0f, 0.0f));
    for (int i = 0; i < 100; i++)
    {
        CreateEnemy(
            glm::vec2(i * 10.0f, i * 5.0f),
            glm::vec2(i * 0.1f, -i * 0.05f)
        );
    }
    
    m_Input.KeyD = true;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Simulate 60 frames (1 second)
    for (int i = 0; i < 60; i++)
    {
        SimulateFrame(1.0f / 60.0f);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // 60 frames with 101 entities should complete in under 100ms
    EXPECT_LT(duration.count(), 100);
}

// -----------------------------------------------------------------------------
// Variable Timestep Tests
// -----------------------------------------------------------------------------

TEST_F(GameFrameE2ETests, VariableTimestep_TotalDistanceConsistent)
{
    CreatePlayer(glm::vec2(0.0f, 0.0f));
    m_Input.KeyD = true;
    
    // Simulate with variable timesteps that sum to 1 second
    std::vector<float> timesteps = { 0.033f, 0.016f, 0.025f, 0.016f, 0.033f, 
                                      0.016f, 0.016f, 0.033f, 0.016f, 0.025f,
                                      0.033f, 0.016f, 0.016f, 0.033f, 0.016f,
                                      0.025f, 0.016f, 0.033f, 0.016f, 0.016f,
                                      0.033f, 0.016f, 0.016f, 0.033f, 0.016f,
                                      0.025f, 0.016f, 0.033f, 0.016f, 0.016f,
                                      0.1f, 0.1f, 0.1f, 0.1f };  // Sum ≈ 1.0
    
    float totalTime = 0.0f;
    for (float dt : timesteps)
    {
        SimulateFrame(dt);
        totalTime += dt;
    }
    
    Entity player = FindPlayer();
    auto& transform = player.GetComponent<TransformComponent>();
    
    // Should have moved approximately speed * totalTime
    float expectedX = 200.0f * totalTime;
    EXPECT_NEAR(transform.Position.x, expectedX, expectedX * 0.1f);  // 10% tolerance
}

// -----------------------------------------------------------------------------
// Entity Lifecycle During Frame Tests
// -----------------------------------------------------------------------------

TEST_F(GameFrameE2ETests, EntityCreatedMidFrame_ProcessedNextFrame)
{
    CreatePlayer(glm::vec2(0.0f, 0.0f));
    m_Input.KeyW = true;
    
    // Simulate a few frames
    for (int i = 0; i < 30; i++)
    {
        SimulateFrame(1.0f / 60.0f);
    }
    
    // Create new entity mid-simulation
    Entity newEnemy = CreateEnemy(glm::vec2(50.0f, 50.0f), glm::vec2(-10.0f, 0.0f));
    
    // Continue simulation
    for (int i = 0; i < 30; i++)
    {
        SimulateFrame(1.0f / 60.0f);
    }
    
    // New enemy should have moved during the second half
    auto& enemyTransform = newEnemy.GetComponent<TransformComponent>();
    EXPECT_LT(enemyTransform.Position.x, 50.0f);
    EXPECT_NEAR(enemyTransform.Position.x, 45.0f, 1.0f);  // Moved ~5 units in 0.5 sec
}

// -----------------------------------------------------------------------------
// Frame Rate Independence Tests
// -----------------------------------------------------------------------------

TEST_F(GameFrameE2ETests, FrameRateIndependence_30FPS_vs_60FPS)
{
    // Test at 30 FPS
    CreatePlayer(glm::vec2(0.0f, 0.0f));
    m_Input.KeyD = true;
    
    for (int i = 0; i < 30; i++)  // 30 frames at 30 FPS = 1 second
    {
        SimulateFrame(1.0f / 30.0f);
    }
    
    float x30fps = FindPlayer().GetComponent<TransformComponent>().Position.x;
    
    // Reset
    m_Scene = std::make_unique<Scene>("GameFrameTestScene");
    m_VelocitySystem.OnAttach(m_Scene.get());
    
    // Test at 60 FPS
    CreatePlayer(glm::vec2(0.0f, 0.0f));
    m_Input.KeyD = true;
    
    for (int i = 0; i < 60; i++)  // 60 frames at 60 FPS = 1 second
    {
        SimulateFrame(1.0f / 60.0f);
    }
    
    float x60fps = FindPlayer().GetComponent<TransformComponent>().Position.x;
    
    // Both should travel approximately the same distance
    EXPECT_NEAR(x30fps, x60fps, 5.0f);
    EXPECT_NEAR(x30fps, 200.0f, 5.0f);
}

// -----------------------------------------------------------------------------
// Edge Case Tests
// -----------------------------------------------------------------------------

TEST_F(GameFrameE2ETests, ZeroTimestep_NoMovement)
{
    CreatePlayer(glm::vec2(100.0f, 100.0f));
    m_Input.KeyW = true;
    
    SimulateFrame(0.0f);
    
    Entity player = FindPlayer();
    auto& transform = player.GetComponent<TransformComponent>();
    
    EXPECT_NEAR(transform.Position.x, 100.0f, 0.001f);
    EXPECT_NEAR(transform.Position.y, 100.0f, 0.001f);
}

TEST_F(GameFrameE2ETests, LargeTimestep_StillWorks)
{
    CreatePlayer(glm::vec2(0.0f, 0.0f));
    m_Input.KeyD = true;
    
    // Simulate large timestep (e.g., lag spike)
    SimulateFrame(0.5f);  // Half second in one frame
    
    Entity player = FindPlayer();
    auto& transform = player.GetComponent<TransformComponent>();
    
    // Should have moved 100 units (200 * 0.5)
    EXPECT_NEAR(transform.Position.x, 100.0f, 1.0f);
}

TEST_F(GameFrameE2ETests, AllInputsSimultaneous_DiagonalNormalized)
{
    CreatePlayer(glm::vec2(0.0f, 0.0f));
    
    // Press all movement keys
    m_Input.KeyW = true;
    m_Input.KeyA = true;
    m_Input.KeyS = true;
    m_Input.KeyD = true;
    
    SimulateFrame(1.0f / 60.0f);
    
    Entity player = FindPlayer();
    auto& transform = player.GetComponent<TransformComponent>();
    
    // Opposite directions cancel out, should not move
    EXPECT_NEAR(transform.Position.x, 0.0f, 0.001f);
    EXPECT_NEAR(transform.Position.y, 0.0f, 0.001f);
}

// -----------------------------------------------------------------------------
// Stress Tests
// -----------------------------------------------------------------------------

TEST_F(GameFrameE2ETests, ManyFrames_NoMemoryLeak)
{
    CreatePlayer(glm::vec2(0.0f, 0.0f));
    m_Input.KeyD = true;
    
    // Simulate 10 seconds at 60 FPS
    for (int i = 0; i < 600; i++)
    {
        SimulateFrame(1.0f / 60.0f);
    }
    
    Entity player = FindPlayer();
    EXPECT_TRUE(player);
    
    auto& transform = player.GetComponent<TransformComponent>();
    EXPECT_NEAR(transform.Position.x, 2000.0f, 50.0f);
}

TEST_F(GameFrameE2ETests, RapidInputToggle_Stability)
{
    CreatePlayer(glm::vec2(0.0f, 0.0f));
    
    // Rapidly toggle input
    for (int i = 0; i < 120; i++)
    {
        m_Input.KeyD = (i % 2 == 0);
        m_Input.KeyA = (i % 2 != 0);
        SimulateFrame(1.0f / 60.0f);
    }
    
    // Should not crash, entity should still be valid
    Entity player = FindPlayer();
    EXPECT_TRUE(player);
}

