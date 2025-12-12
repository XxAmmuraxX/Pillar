#include "ChamberManager.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Core/TagComponent.h"
#include "../Components/HealthComponent.h"
#include "../Components/EnemyAIComponent.h"
#include "Pillar/Logger.h"
#include <glm/gtc/constants.hpp>
#include <cstdlib>

namespace Pillar {

	void ChamberManager::Initialize(Scene* scene, EntityFactory* factory, Entity player)
	{
		m_Scene = scene;
		m_Factory = factory;
		m_Player = player;
	}

	void ChamberManager::LoadChamber(ChamberType chamber)
	{
		PIL_INFO("Loading chamber: {}", static_cast<int>(chamber));
		
		UnloadCurrentChamber();
		m_CurrentChamber = chamber;

		switch (chamber)
		{
		case ChamberType::Hub:
			LoadHub();
			break;
		case ChamberType::MovementPhysics:
			LoadMovementPhysicsChamber();
			break;
		case ChamberType::ShootingRange:
			LoadShootingRange();
			break;
		case ChamberType::EnemyGauntlet:
			LoadEnemyGauntlet();
			break;
		case ChamberType::ParticleTest:
			LoadParticleTest();
			break;
		case ChamberType::AnimationTest:
			LoadAnimationTest();
			break;
		case ChamberType::AudioTest:
			LoadAudioTest();
			break;
		case ChamberType::BossArena:
			LoadBossArena();
			break;
		case ChamberType::StressTest:
			LoadStressTest();
			break;
		}
	}

	void ChamberManager::UnloadCurrentChamber()
	{
		// Destroy all chamber-specific entities
		for (auto& enemy : m_Enemies)
		{
			if (enemy.IsValid())
				m_Scene->DestroyEntity(enemy);
		}
		m_Enemies.clear();

		for (auto& obstacle : m_Obstacles)
		{
			if (obstacle.IsValid())
				m_Scene->DestroyEntity(obstacle);
		}
		m_Obstacles.clear();

		for (auto& trigger : m_Triggers)
		{
			if (trigger.IsValid())
				m_Scene->DestroyEntity(trigger);
		}
		m_Triggers.clear();

		for (auto& effect : m_Effects)
		{
			if (effect.IsValid())
				m_Scene->DestroyEntity(effect);
		}
		m_Effects.clear();
	}

	void ChamberManager::OnUpdate(float deltaTime)
	{
		// Clean up dead enemies
		auto it = m_Enemies.begin();
		while (it != m_Enemies.end())
		{
			if (!it->IsValid())
			{
				it = m_Enemies.erase(it);
				continue;
			}

			if (it->HasComponent<HealthComponent>())
			{
				auto& health = it->GetComponent<HealthComponent>();
				if (health.IsDead)
				{
					// Create XP gem at enemy position
					auto& transform = it->GetComponent<TransformComponent>();
					int xpValue = 1;
					
					// Different enemies drop different XP
					auto& tag = it->GetComponent<TagComponent>();
					if (tag.Tag.find("Turret") != std::string::npos)
						xpValue = 5;
					else if (tag.Tag.find("Charger") != std::string::npos)
						xpValue = 3;
					else if (tag.Tag.find("Boss") != std::string::npos)
						xpValue = 50;

					m_Factory->CreateXPGem({ transform.Position.x, transform.Position.y }, xpValue);
					
					m_Scene->DestroyEntity(*it);
					it = m_Enemies.erase(it);
					continue;
				}
			}
			++it;
		}
	}

	void ChamberManager::CreateArenaWalls(float size)
	{
		float wallThickness = 1.0f;
		float halfSize = size / 2.0f;

		// Top wall
		m_Obstacles.push_back(m_Factory->CreateObstacle(
			{ 0.0f, halfSize + wallThickness / 2.0f },
			"wall",
			{ size + wallThickness * 2, wallThickness }
		));

		// Bottom wall
		m_Obstacles.push_back(m_Factory->CreateObstacle(
			{ 0.0f, -halfSize - wallThickness / 2.0f },
			"wall",
			{ size + wallThickness * 2, wallThickness }
		));

		// Left wall
		m_Obstacles.push_back(m_Factory->CreateObstacle(
			{ -halfSize - wallThickness / 2.0f, 0.0f },
			"wall",
			{ wallThickness, size }
		));

		// Right wall
		m_Obstacles.push_back(m_Factory->CreateObstacle(
			{ halfSize + wallThickness / 2.0f, 0.0f },
			"wall",
			{ wallThickness, size }
		));
	}

	void ChamberManager::SpawnDrones(int count, float radius)
	{
		for (int i = 0; i < count; i++)
		{
			float angle = (i / (float)count) * 2.0f * glm::pi<float>();
			float dist = radius + (rand() % 30) / 10.0f;
			glm::vec2 pos = { cosf(angle) * dist, sinf(angle) * dist };
			
			Entity drone = m_Factory->CreateDrone(pos, m_Player);
			m_Enemies.push_back(drone);
		}
	}

	void ChamberManager::SpawnTurrets(const std::vector<glm::vec2>& positions)
	{
		for (const auto& pos : positions)
		{
			Entity turret = m_Factory->CreateTurret(pos);
			m_Enemies.push_back(turret);
		}
	}

	void ChamberManager::SpawnChargers(int count, float radius)
	{
		for (int i = 0; i < count; i++)
		{
			float angle = ((i + 0.5f) / (float)count) * 2.0f * glm::pi<float>();
			float dist = radius + (rand() % 20) / 10.0f;
			glm::vec2 pos = { cosf(angle) * dist, sinf(angle) * dist };
			
			Entity charger = m_Factory->CreateCharger(pos);
			m_Enemies.push_back(charger);
		}
	}

	void ChamberManager::LoadHub()
	{
		PIL_INFO("=== HUB: Main Menu ===");
		CreateArenaWalls(20.0f);

		// Create portal triggers to other chambers
		m_Triggers.push_back(m_Factory->CreateTriggerZone({ 5.0f, 0.0f }, { 2.0f, 2.0f }, "goto_shooting_range"));
		m_Triggers.push_back(m_Factory->CreateTriggerZone({ -5.0f, 0.0f }, { 2.0f, 2.0f }, "goto_enemy_gauntlet"));
		m_Triggers.push_back(m_Factory->CreateTriggerZone({ 0.0f, 5.0f }, { 2.0f, 2.0f }, "goto_boss_arena"));

		// Visual markers for portals (obstacles as placeholders)
		m_Obstacles.push_back(m_Factory->CreateObstacle({ 5.0f, 0.0f }, "pillar", { 0.5f, 0.5f }));
		m_Obstacles.push_back(m_Factory->CreateObstacle({ -5.0f, 0.0f }, "pillar", { 0.5f, 0.5f }));
		m_Obstacles.push_back(m_Factory->CreateObstacle({ 0.0f, 5.0f }, "pillar", { 0.5f, 0.5f }));
	}

	void ChamberManager::LoadMovementPhysicsChamber()
	{
		PIL_INFO("=== CHAMBER 1: Movement & Physics ===");
		CreateArenaWalls(30.0f);

		// Create obstacle course
		// Scattered crates
		for (int i = 0; i < 10; i++)
		{
			float x = (rand() % 200 - 100) / 10.0f;
			float y = (rand() % 200 - 100) / 10.0f;
			m_Obstacles.push_back(m_Factory->CreateObstacle({ x, y }, "crate", { 1.5f, 1.5f }));
		}

		// Pillars in corners
		m_Obstacles.push_back(m_Factory->CreateObstacle({ -8.0f, -8.0f }, "pillar", { 2.0f, 2.0f }));
		m_Obstacles.push_back(m_Factory->CreateObstacle({ 8.0f, -8.0f }, "pillar", { 2.0f, 2.0f }));
		m_Obstacles.push_back(m_Factory->CreateObstacle({ -8.0f, 8.0f }, "pillar", { 2.0f, 2.0f }));
		m_Obstacles.push_back(m_Factory->CreateObstacle({ 8.0f, 8.0f }, "pillar", { 2.0f, 2.0f }));
	}

	void ChamberManager::LoadShootingRange()
	{
		PIL_INFO("=== CHAMBER 2: Shooting Range ===");
		CreateArenaWalls(25.0f);

		// Create targets in a line
		for (int i = 0; i < 5; i++)
		{
			float x = 8.0f;
			float y = -4.0f + i * 2.0f;
			
			// Use drones as targets (stationary)
			Entity target = m_Factory->CreateDrone({ x, y }, Entity());
			if (target.HasComponent<EnemyAIComponent>())
			{
				auto& ai = target.GetComponent<EnemyAIComponent>();
				ai.Behavior = AIBehavior::Idle;
			}
			m_Enemies.push_back(target);
		}

		// Breakable crates
		for (int i = 0; i < 5; i++)
		{
			m_Obstacles.push_back(m_Factory->CreateObstacle(
				{ -6.0f + i * 3.0f, -8.0f },
				"crate",
				{ 1.5f, 1.5f }
			));
		}

		// Cover walls
		m_Obstacles.push_back(m_Factory->CreateObstacle({ -4.0f, 0.0f }, "wall", { 4.0f, 0.5f }));
		m_Obstacles.push_back(m_Factory->CreateObstacle({ 4.0f, 0.0f }, "wall", { 4.0f, 0.5f }));
	}

	void ChamberManager::LoadEnemyGauntlet()
	{
		PIL_INFO("=== CHAMBER 3: Enemy Gauntlet ===");
		CreateArenaWalls(30.0f);

		// Initial wave - starts with drones only
		// More enemies will be spawned by wave system
		SpawnDrones(5, 8.0f);
	}

	void ChamberManager::LoadParticleTest()
	{
		PIL_INFO("=== CHAMBER 4: Particle Test ===");
		CreateArenaWalls(20.0f);

		// Create explosion effect entities at fixed positions
		m_Effects.push_back(m_Factory->CreateExplosion({ -5.0f, 0.0f }, 0.5f));
		m_Effects.push_back(m_Factory->CreateExplosion({ 5.0f, 0.0f }, 1.0f));
		m_Effects.push_back(m_Factory->CreateExplosion({ 0.0f, 5.0f }, 1.5f));
	}

	void ChamberManager::LoadAnimationTest()
	{
		PIL_INFO("=== CHAMBER 5: Animation Test ===");
		CreateArenaWalls(20.0f);

		// Create some enemies with different behaviors for animation showcase
		Entity drone = m_Factory->CreateDrone({ 0.0f, 5.0f }, m_Player);
		m_Enemies.push_back(drone);

		Entity charger = m_Factory->CreateCharger({ 5.0f, 5.0f });
		m_Enemies.push_back(charger);

		// Scattered gems for collection animation
		for (int i = 0; i < 10; i++)
		{
			float angle = (i / 10.0f) * 2.0f * glm::pi<float>();
			m_Factory->CreateXPGem({ cosf(angle) * 4.0f, sinf(angle) * 4.0f }, (i % 3) + 1);
		}
	}

	void ChamberManager::LoadAudioTest()
	{
		PIL_INFO("=== CHAMBER 6: Audio Test ===");
		CreateArenaWalls(20.0f);

		// Place markers for audio sources
		m_Obstacles.push_back(m_Factory->CreateObstacle({ -7.0f, -7.0f }, "pillar", { 1.0f, 1.0f }));
		m_Obstacles.push_back(m_Factory->CreateObstacle({ 7.0f, -7.0f }, "pillar", { 1.0f, 1.0f }));
		m_Obstacles.push_back(m_Factory->CreateObstacle({ -7.0f, 7.0f }, "pillar", { 1.0f, 1.0f }));
		m_Obstacles.push_back(m_Factory->CreateObstacle({ 7.0f, 7.0f }, "pillar", { 1.0f, 1.0f }));
	}

	void ChamberManager::LoadBossArena()
	{
		PIL_INFO("=== CHAMBER 7: Boss Arena ===");
		CreateArenaWalls(40.0f);

		// Create boss
		Entity boss = m_Factory->CreateBoss({ 0.0f, 10.0f });
		m_Enemies.push_back(boss);

		// Add cover pillars around arena
		m_Obstacles.push_back(m_Factory->CreateObstacle({ -10.0f, 0.0f }, "pillar", { 2.0f, 2.0f }));
		m_Obstacles.push_back(m_Factory->CreateObstacle({ 10.0f, 0.0f }, "pillar", { 2.0f, 2.0f }));
		m_Obstacles.push_back(m_Factory->CreateObstacle({ -10.0f, -10.0f }, "pillar", { 2.0f, 2.0f }));
		m_Obstacles.push_back(m_Factory->CreateObstacle({ 10.0f, -10.0f }, "pillar", { 2.0f, 2.0f }));
	}

	void ChamberManager::LoadStressTest()
	{
		PIL_INFO("=== CHAMBER 8: Stress Test ===");
		CreateArenaWalls(50.0f);

		// Spawn many enemies
		SpawnDrones(50, 15.0f);
		SpawnChargers(20, 12.0f);
		SpawnTurrets({
			{ -15.0f, -15.0f }, { 15.0f, -15.0f }, { -15.0f, 15.0f }, { 15.0f, 15.0f },
			{ 0.0f, -15.0f }, { 0.0f, 15.0f }, { -15.0f, 0.0f }, { 15.0f, 0.0f }
		});

		// Scatter XP gems
		for (int i = 0; i < 50; i++)
		{
			float x = (rand() % 300 - 150) / 10.0f;
			float y = (rand() % 300 - 150) / 10.0f;
			m_Factory->CreateXPGem({ x, y }, (rand() % 3) + 1);
		}
	}

} // namespace Pillar
