#include <gtest/gtest.h>
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/SceneManager.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Core/TagComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"
#include "Pillar/ECS/Components/Gameplay/BulletComponent.h"
#include "Pillar/ECS/Components/Gameplay/XPGemComponent.h"
#include "Pillar/ECS/Systems/VelocityIntegrationSystem.h"
#include "Pillar/ECS/Systems/BulletCollisionSystem.h"
#include <glm/glm.hpp>
#include <vector>
#include <set>
#include <algorithm>

using namespace Pillar;

// ============================================================================
// Gameplay Scenario Tests (Acceptance Tests)
// These tests simulate real game scenarios to verify end-to-end functionality
// ============================================================================

class GameplayScenarioTests : public ::testing::Test
{
protected:
	void SetUp() override
	{
		m_Scene = std::make_unique<Scene>("GameplayTestScene");
		m_VelocitySystem.OnAttach(m_Scene.get());
	}

	void TearDown() override
	{
		m_Scene.reset();
	}

	std::unique_ptr<Scene> m_Scene;
	VelocityIntegrationSystem m_VelocitySystem;

	// Helper to simulate a game frame
	void SimulateFrame(float dt)
	{
		// Update velocity integration
		m_VelocitySystem.OnUpdate(dt);
	}

	// Helper to create a player entity
	Entity CreatePlayer(glm::vec2 position)
	{
		Entity player = m_Scene->CreateEntity("Player");
		player.GetComponent<TransformComponent>().Position = position;
		return player;
	}

	// Helper to create an enemy entity
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

	// Helper to create a bullet
	Entity CreateBullet(glm::vec2 position, glm::vec2 direction, float speed, float damage)
	{
		Entity bullet = m_Scene->CreateEntity("Bullet");
		auto& transform = bullet.GetComponent<TransformComponent>();
		transform.Position = position;

		glm::vec2 velocity = glm::normalize(direction) * speed;
		bullet.AddComponent<VelocityComponent>(velocity);

		auto& bulletComp = bullet.AddComponent<BulletComponent>();
		bulletComp.Damage = damage;
		bulletComp.Lifetime = 10.0f;

		return bullet;
	}
};

// -----------------------------------------------------------------------------
// Player Movement Scenarios
// -----------------------------------------------------------------------------

TEST_F(GameplayScenarioTests, Player_MoveRight)
{
	Entity player = CreatePlayer(glm::vec2(0.0f, 0.0f));
	player.AddComponent<VelocityComponent>(glm::vec2(100.0f, 0.0f));

	// Simulate 1 second of gameplay
	for (int i = 0; i < 60; i++)
	{
		SimulateFrame(1.0f / 60.0f);
	}

	auto& transform = player.GetComponent<TransformComponent>();
	EXPECT_NEAR(transform.Position.x, 100.0f, 1.0f);
	EXPECT_NEAR(transform.Position.y, 0.0f, 0.1f);
}

TEST_F(GameplayScenarioTests, Player_DiagonalMovement)
{
	Entity player = CreatePlayer(glm::vec2(0.0f, 0.0f));
	// Normalized diagonal movement
	float diag = 70.7f; // ~100 / sqrt(2)
	player.AddComponent<VelocityComponent>(glm::vec2(diag, diag));

	// Simulate 1 second
	for (int i = 0; i < 60; i++)
	{
		SimulateFrame(1.0f / 60.0f);
	}

	auto& transform = player.GetComponent<TransformComponent>();
	EXPECT_NEAR(transform.Position.x, diag, 1.0f);
	EXPECT_NEAR(transform.Position.y, diag, 1.0f);
}

// -----------------------------------------------------------------------------
// Combat Scenarios
// -----------------------------------------------------------------------------

TEST_F(GameplayScenarioTests, Bullet_TravelsTowardsEnemy)
{
	Entity player = CreatePlayer(glm::vec2(0.0f, 0.0f));
	Entity enemy = CreateEnemy(glm::vec2(500.0f, 0.0f));

	glm::vec2 playerPos = player.GetComponent<TransformComponent>().Position;
	glm::vec2 enemyPos = enemy.GetComponent<TransformComponent>().Position;
	glm::vec2 direction = glm::normalize(enemyPos - playerPos);

	Entity bullet = CreateBullet(playerPos, direction, 200.0f, 10.0f);

	// Simulate bullet travel
	float travelTime = 500.0f / 200.0f; // 2.5 seconds
	int frames = static_cast<int>(travelTime * 60);

	for (int i = 0; i < frames; i++)
	{
		SimulateFrame(1.0f / 60.0f);
	}

	auto& bulletTransform = bullet.GetComponent<TransformComponent>();
	EXPECT_NEAR(bulletTransform.Position.x, 500.0f, 5.0f);
}

TEST_F(GameplayScenarioTests, MultipleBullets_IndependentTrajectories)
{
	std::vector<Entity> bullets;

	// Fire bullets in 4 directions
	glm::vec2 directions[] = {
		glm::vec2(1.0f, 0.0f),
		glm::vec2(-1.0f, 0.0f),
		glm::vec2(0.0f, 1.0f),
		glm::vec2(0.0f, -1.0f)
	};

	for (int i = 0; i < 4; i++)
	{
		Entity bullet = CreateBullet(glm::vec2(0.0f, 0.0f), directions[i], 100.0f, 5.0f);
		bullets.push_back(bullet);
	}

	// Simulate 1 second
	for (int i = 0; i < 60; i++)
	{
		SimulateFrame(1.0f / 60.0f);
	}

	// Verify bullets traveled in correct directions
	EXPECT_NEAR(bullets[0].GetComponent<TransformComponent>().Position.x, 100.0f, 1.0f);
	EXPECT_NEAR(bullets[1].GetComponent<TransformComponent>().Position.x, -100.0f, 1.0f);
	EXPECT_NEAR(bullets[2].GetComponent<TransformComponent>().Position.y, 100.0f, 1.0f);
	EXPECT_NEAR(bullets[3].GetComponent<TransformComponent>().Position.y, -100.0f, 1.0f);
}

// -----------------------------------------------------------------------------
// Enemy Behavior Scenarios
// -----------------------------------------------------------------------------

TEST_F(GameplayScenarioTests, Enemy_ChasePlayer)
{
	Entity player = CreatePlayer(glm::vec2(0.0f, 0.0f));
	Entity enemy = CreateEnemy(glm::vec2(100.0f, 0.0f));

	// Simulate enemy chasing player for several frames
	float chaseSpeed = 50.0f;

	for (int frame = 0; frame < 60; frame++)
	{
		// Update enemy velocity to chase player
		glm::vec2 playerPos = player.GetComponent<TransformComponent>().Position;
		glm::vec2 enemyPos = enemy.GetComponent<TransformComponent>().Position;
		glm::vec2 direction = glm::normalize(playerPos - enemyPos);

		if (!enemy.HasComponent<VelocityComponent>())
		{
			enemy.AddComponent<VelocityComponent>(direction * chaseSpeed);
		}
		else
		{
			enemy.GetComponent<VelocityComponent>().Velocity = direction * chaseSpeed;
		}

		SimulateFrame(1.0f / 60.0f);
	}

	// Enemy should have moved closer to player
	auto& enemyTransform = enemy.GetComponent<TransformComponent>();
	float distanceToPlayer = glm::length(enemyTransform.Position);
	EXPECT_LT(distanceToPlayer, 100.0f); // Closer than initial 100 units
}

TEST_F(GameplayScenarioTests, MultipleEnemies_IndependentBehavior)
{
	Entity player = CreatePlayer(glm::vec2(0.0f, 0.0f));

	// Create enemies in different positions
	Entity enemy1 = CreateEnemy(glm::vec2(100.0f, 0.0f), glm::vec2(-10.0f, 0.0f));
	Entity enemy2 = CreateEnemy(glm::vec2(-100.0f, 0.0f), glm::vec2(10.0f, 0.0f));
	Entity enemy3 = CreateEnemy(glm::vec2(0.0f, 100.0f), glm::vec2(0.0f, -10.0f));

	// Simulate 1 second
	for (int i = 0; i < 60; i++)
	{
		SimulateFrame(1.0f / 60.0f);
	}

	// All enemies should have moved towards center
	EXPECT_LT(enemy1.GetComponent<TransformComponent>().Position.x, 100.0f);
	EXPECT_GT(enemy2.GetComponent<TransformComponent>().Position.x, -100.0f);
	EXPECT_LT(enemy3.GetComponent<TransformComponent>().Position.y, 100.0f);
}

// -----------------------------------------------------------------------------
// XP Gem Collection Scenarios
// -----------------------------------------------------------------------------

TEST_F(GameplayScenarioTests, XPGem_SpawnAfterEnemyDeath)
{
	// Simulate enemy death spawning XP gem
	glm::vec2 deathPosition(50.0f, 50.0f);

	Entity gem = m_Scene->CreateEntity("XPGem");
	gem.GetComponent<TransformComponent>().Position = deathPosition;
	auto& xpComp = gem.AddComponent<XPGemComponent>();
	xpComp.XPValue = 10;

	EXPECT_TRUE(gem.HasComponent<XPGemComponent>());
	EXPECT_EQ(gem.GetComponent<XPGemComponent>().XPValue, 10);
}

TEST_F(GameplayScenarioTests, XPGem_MagnetTowardsPlayer)
{
	Entity player = CreatePlayer(glm::vec2(0.0f, 0.0f));

	Entity gem = m_Scene->CreateEntity("XPGem");
	gem.GetComponent<TransformComponent>().Position = glm::vec2(50.0f, 0.0f);
	gem.AddComponent<XPGemComponent>().XPValue = 5;

	// Simulate magnet effect
	float magnetSpeed = 100.0f;
	gem.AddComponent<VelocityComponent>(glm::vec2(-magnetSpeed, 0.0f));

	// Simulate for 0.5 seconds
	for (int i = 0; i < 30; i++)
	{
		SimulateFrame(1.0f / 60.0f);
	}

	// Gem should be closer
	float newDistance = gem.GetComponent<TransformComponent>().Position.x;
	EXPECT_LT(newDistance, 50.0f);
}

// -----------------------------------------------------------------------------
// Wave/Round Scenarios
// -----------------------------------------------------------------------------

TEST_F(GameplayScenarioTests, Wave_SpawnMultipleEnemies)
{
	int initialCount = static_cast<int>(m_Scene->GetEntityCount());

	// Spawn wave of 10 enemies
	for (int i = 0; i < 10; i++)
	{
		float angle = (float)i * (360.0f / 10.0f) * 3.14159f / 180.0f;
		glm::vec2 spawnPos(cos(angle) * 200.0f, sin(angle) * 200.0f);
		CreateEnemy(spawnPos);
	}

	EXPECT_EQ(m_Scene->GetEntityCount(), initialCount + 10);
}

TEST_F(GameplayScenarioTests, Wave_ClearAllEnemies)
{
	// Spawn enemies
	std::vector<Entity> enemies;
	for (int i = 0; i < 5; i++)
	{
		enemies.push_back(CreateEnemy(glm::vec2((float)i * 10.0f, 0.0f)));
	}

	EXPECT_EQ(m_Scene->GetEntityCount(), 5);

	// Destroy all enemies (simulate wave clear)
	for (auto& enemy : enemies)
	{
		m_Scene->DestroyEntity(enemy);
	}

	EXPECT_EQ(m_Scene->GetEntityCount(), 0);
}

// -----------------------------------------------------------------------------
// Edge Case Scenarios
// -----------------------------------------------------------------------------

TEST_F(GameplayScenarioTests, Entity_ZeroVelocity_NoMovement)
{
	Entity entity = m_Scene->CreateEntity("Static");
	entity.GetComponent<TransformComponent>().Position = glm::vec2(100.0f, 100.0f);
	entity.AddComponent<VelocityComponent>(glm::vec2(0.0f, 0.0f));

	for (int i = 0; i < 60; i++)
	{
		SimulateFrame(1.0f / 60.0f);
	}

	auto& transform = entity.GetComponent<TransformComponent>();
	EXPECT_FLOAT_EQ(transform.Position.x, 100.0f);
	EXPECT_FLOAT_EQ(transform.Position.y, 100.0f);
}

TEST_F(GameplayScenarioTests, Entity_LargeTimestep_StillAccurate)
{
	Entity entity = m_Scene->CreateEntity("Moving");
	entity.GetComponent<TransformComponent>().Position = glm::vec2(0.0f, 0.0f);
	entity.AddComponent<VelocityComponent>(glm::vec2(100.0f, 0.0f));

	// Single large timestep
	SimulateFrame(1.0f);

	auto& transform = entity.GetComponent<TransformComponent>();
	EXPECT_FLOAT_EQ(transform.Position.x, 100.0f);
}

TEST_F(GameplayScenarioTests, ManyEntities_SimultaneousUpdate)
{
	// Spawn 100 entities moving in different directions
	for (int i = 0; i < 100; i++)
	{
		Entity entity = m_Scene->CreateEntity("Entity" + std::to_string(i));
		entity.GetComponent<TransformComponent>().Position = glm::vec2(0.0f, 0.0f);
		
		float angle = (float)i * (360.0f / 100.0f) * 3.14159f / 180.0f;
		entity.AddComponent<VelocityComponent>(glm::vec2(cos(angle) * 50.0f, sin(angle) * 50.0f));
	}

	// Simulate 1 second
	for (int i = 0; i < 60; i++)
	{
		SimulateFrame(1.0f / 60.0f);
	}

	EXPECT_EQ(m_Scene->GetEntityCount(), 100);
}

// -----------------------------------------------------------------------------
// Scene Transition Scenarios
// -----------------------------------------------------------------------------

TEST_F(GameplayScenarioTests, SceneTransition_PreservesSceneManager)
{
	SceneManager& manager = SceneManager::Get();
	manager.Clear();

	manager.CreateScene("Level1");
	manager.CreateScene("Level2");
	manager.SetActiveScene("Level1");

	// Verify we can switch scenes
	manager.SetActiveScene("Level2");
	auto activeScene = manager.GetActiveScene();
	ASSERT_NE(activeScene, nullptr);
	EXPECT_EQ(activeScene->GetName(), "Level2");

	manager.Clear();
}

TEST_F(GameplayScenarioTests, SceneTransition_NewSceneEmpty)
{
	SceneManager& manager = SceneManager::Get();
	manager.Clear();

	manager.CreateScene("GameLevel");
	manager.SetActiveScene("GameLevel");

	auto scene = manager.GetActiveScene();
	ASSERT_NE(scene, nullptr);

	// Spawn entities in current scene
	for (int i = 0; i < 10; i++)
	{
		scene->CreateEntity("Entity" + std::to_string(i));
	}
	EXPECT_EQ(scene->GetEntityCount(), 10);

	// Create and switch to new scene
	manager.CreateScene("NextLevel");
	manager.SetActiveScene("NextLevel");

	auto newScene = manager.GetActiveScene();
	EXPECT_EQ(newScene->GetEntityCount(), 0);

	manager.Clear();
}

// -----------------------------------------------------------------------------
// Collision-Triggered Event Scenarios
// Tests for gameplay events triggered by collisions
// -----------------------------------------------------------------------------

TEST_F(GameplayScenarioTests, CollisionDetection_BulletInRange)
{
	// Scenario: Bullet travels toward enemy and enters detection range
	Entity bullet = CreateBullet(glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), 200.0f, 10.0f);
	Entity enemy = CreateEnemy(glm::vec2(50.0f, 0.0f));
	
	// Simulate until bullet is near enemy
	for (int i = 0; i < 15; i++)  // ~0.25 seconds
	{
		SimulateFrame(1.0f / 60.0f);
	}
	
	// Bullet should have moved toward enemy
	auto& bulletTransform = bullet.GetComponent<TransformComponent>();
	EXPECT_GT(bulletTransform.Position.x, 40.0f);
	
	// Basic collision check (distance-based)
	auto& enemyTransform = enemy.GetComponent<TransformComponent>();
	float distance = glm::length(bulletTransform.Position - enemyTransform.Position);
	EXPECT_LT(distance, 20.0f);  // Within collision range
}

TEST_F(GameplayScenarioTests, CollisionEvent_TriggerCallback)
{
	// Scenario: Collision triggers a callback (simulating audio/effect trigger)
	bool collisionOccurred = false;
	glm::vec2 collisionPoint(0.0f);
	
	Entity bullet = CreateBullet(glm::vec2(5.0f, 5.0f), glm::vec2(1.0f, 0.0f), 100.0f, 10.0f);
	Entity enemy = CreateEnemy(glm::vec2(20.0f, 5.0f));
	
	float collisionRadius = 5.0f;
	
	// Simulate and check for collision each frame
	for (int frame = 0; frame < 30; frame++)
	{
		SimulateFrame(1.0f / 60.0f);
		
		auto& bulletPos = bullet.GetComponent<TransformComponent>().Position;
		auto& enemyPos = enemy.GetComponent<TransformComponent>().Position;
		
		float distance = glm::length(bulletPos - enemyPos);
		if (distance < collisionRadius && !collisionOccurred)
		{
			collisionOccurred = true;
			collisionPoint = bulletPos;
			// In real game: play collision sound, spawn particle effect
		}
	}
	
	EXPECT_TRUE(collisionOccurred);
	EXPECT_GT(collisionPoint.x, 10.0f);  // Collision happened after bullet moved
}

TEST_F(GameplayScenarioTests, MultipleCollisions_QueuedEvents)
{
	// Scenario: Multiple collisions happen, events are queued
	std::vector<std::pair<Entity, Entity>> collisionPairs;
	
	// Create bullet firing through multiple enemies
	Entity bullet = CreateBullet(glm::vec2(5.0f, 5.0f), glm::vec2(1.0f, 0.0f), 200.0f, 10.0f);
	
	std::vector<Entity> enemies;
	for (int i = 0; i < 5; i++)
	{
		enemies.push_back(CreateEnemy(glm::vec2(20.0f + i * 15.0f, 5.0f)));
	}
	
	float collisionRadius = 5.0f;
	std::set<size_t> hitEnemies;
	
	// Simulate and track collisions
	for (int frame = 0; frame < 60; frame++)
	{
		SimulateFrame(1.0f / 60.0f);
		
		auto& bulletPos = bullet.GetComponent<TransformComponent>().Position;
		
		for (size_t i = 0; i < enemies.size(); i++)
		{
			if (hitEnemies.find(i) != hitEnemies.end()) continue;
			
			auto& enemyPos = enemies[i].GetComponent<TransformComponent>().Position;
			float distance = glm::length(bulletPos - enemyPos);
			
			if (distance < collisionRadius)
			{
				hitEnemies.insert(i);
				collisionPairs.push_back({bullet, enemies[i]});
				// In real game: each hit would trigger a sound
			}
		}
	}
	
	// Bullet should have hit at least 2 enemies
	EXPECT_GE(collisionPairs.size(), 2u);
}

TEST_F(GameplayScenarioTests, AreaOfEffect_MultipleTargets)
{
	// Scenario: Explosion hits multiple nearby enemies (area damage)
	glm::vec2 explosionCenter(50.0f, 50.0f);
	float explosionRadius = 20.0f;
	
	std::vector<Entity> enemies;
	std::vector<bool> inBlast;
	
	// Create enemies at various distances from explosion
	std::vector<float> distances = { 5.0f, 15.0f, 25.0f, 35.0f };
	for (float dist : distances)
	{
		Entity enemy = CreateEnemy(explosionCenter + glm::vec2(dist, 0.0f));
		enemies.push_back(enemy);
	}
	
	// Check which enemies are in blast radius
	for (const auto& enemy : enemies)
	{
		auto& pos = enemy.GetComponent<TransformComponent>().Position;
		float dist = glm::length(pos - explosionCenter);
		inBlast.push_back(dist <= explosionRadius);
	}
	
	// First two should be hit (5 and 15), last two should be safe (25 and 35)
	EXPECT_TRUE(inBlast[0]);   // 5 units away
	EXPECT_TRUE(inBlast[1]);   // 15 units away
	EXPECT_FALSE(inBlast[2]);  // 25 units away (just outside)
	EXPECT_FALSE(inBlast[3]);  // 35 units away
}

TEST_F(GameplayScenarioTests, CollisionSound_Volume_BasedOnDistance)
{
	// Scenario: Sound volume calculated based on distance from listener
	glm::vec2 listenerPos(0.0f, 0.0f);
	glm::vec2 collisionPos(100.0f, 0.0f);
	
	float maxDistance = 500.0f;
	float minDistance = 10.0f;
	
	// Calculate volume based on distance (linear falloff)
	float distance = glm::length(collisionPos - listenerPos);
	float volume = 1.0f;
	
	if (distance > maxDistance)
	{
		volume = 0.0f;
	}
	else if (distance > minDistance)
	{
		volume = 1.0f - (distance - minDistance) / (maxDistance - minDistance);
	}
	
	// At 100 units, volume should be partial
	EXPECT_GT(volume, 0.0f);
	EXPECT_LT(volume, 1.0f);
	EXPECT_NEAR(volume, 0.816f, 0.01f);  // (500-100)/(500-10) ≈ 0.816
}

TEST_F(GameplayScenarioTests, CollisionPriority_ClosestFirst)
{
	// Scenario: When multiple collisions possible, process closest first
	Entity bullet = CreateBullet(glm::vec2(5.0f, 5.0f), glm::vec2(1.0f, 0.0f), 200.0f, 10.0f);
	
	// Create enemies at different distances
	Entity farEnemy = CreateEnemy(glm::vec2(100.0f, 5.0f));
	Entity nearEnemy = CreateEnemy(glm::vec2(25.0f, 5.0f));
	Entity midEnemy = CreateEnemy(glm::vec2(50.0f, 5.0f));
	
	std::vector<std::string> hitOrder;
	std::set<std::string> alreadyHit;
	float collisionRadius = 5.0f;
	
	// Simulate and track hit order
	for (int frame = 0; frame < 60; frame++)
	{
		SimulateFrame(1.0f / 60.0f);
		
		auto& bulletPos = bullet.GetComponent<TransformComponent>().Position;
		
		// Check each enemy
		std::vector<std::pair<float, std::string>> candidates;
		
		auto checkEnemy = [&](Entity enemy, const std::string& name) {
			if (alreadyHit.find(name) != alreadyHit.end()) return;
			auto& pos = enemy.GetComponent<TransformComponent>().Position;
			float dist = glm::length(bulletPos - pos);
			if (dist < collisionRadius)
			{
				candidates.push_back({dist, name});
			}
		};
		
		checkEnemy(nearEnemy, "near");
		checkEnemy(midEnemy, "mid");
		checkEnemy(farEnemy, "far");
		
		// Sort by distance and hit closest first
		std::sort(candidates.begin(), candidates.end());
		for (auto& [dist, name] : candidates)
		{
			if (alreadyHit.find(name) == alreadyHit.end())
			{
				hitOrder.push_back(name);
				alreadyHit.insert(name);
			}
		}
	}
	
	// Should hit near enemy first
	ASSERT_GE(hitOrder.size(), 1u);
	EXPECT_EQ(hitOrder[0], "near");
}
