#include "WaveSystem.h"
#include "Pillar/Logger.h"
#include <cstdlib>
#include <cmath>

namespace Pillar {

	void WaveSystem::OnUpdate(Scene* scene, std::vector<Entity>& enemies, float deltaTime)
	{
		// Wait for wave transition
		if (m_WaitingForNextWave)
		{
			m_WaveTransitionTimer -= deltaTime;
			if (m_WaveTransitionTimer <= 0.0f)
			{
				m_WaitingForNextWave = false;
				StartNextWave();
			}
			return;
		}

		// Check if wave is complete
		if (m_WaveInProgress)
		{
			int remaining = GetEnemiesRemaining(enemies);
			bool allSpawned = (m_DronesToSpawn + m_TurretsToSpawn + m_ChargersToSpawn) == 0;

			if (remaining == 0 && allSpawned)
			{
				PIL_INFO("Wave {} Complete!", m_CurrentWave);
				m_WaveInProgress = false;

				if (m_WaveCompleteCallback)
				{
					m_WaveCompleteCallback(m_CurrentWave);
				}

				if (m_CurrentWave < m_MaxWaves)
				{
					// Wait before next wave
					m_WaitingForNextWave = true;
					m_WaveTransitionTimer = m_WaveTransitionDelay;
					PIL_INFO("Next wave in {:.1f} seconds...", m_WaveTransitionDelay);
				}
				else
				{
					m_AllWavesComplete = true;
					PIL_INFO("All waves complete! Victory!");
				}
			}
		}

		// Spawn enemies
		if (m_WaveInProgress && m_SpawnCallback)
		{
			m_SpawnTimer -= deltaTime;
			if (m_SpawnTimer <= 0.0f)
			{
				if (m_DronesToSpawn > 0)
				{
					glm::vec2 pos = GetRandomSpawnPosition(10.0f);
					m_SpawnCallback("Drone", pos);
					m_DronesToSpawn--;
				}
				else if (m_TurretsToSpawn > 0)
				{
					glm::vec2 pos = GetRandomSpawnPosition(12.0f);
					m_SpawnCallback("Turret", pos);
					m_TurretsToSpawn--;
				}
				else if (m_ChargersToSpawn > 0)
				{
					glm::vec2 pos = GetRandomSpawnPosition(10.0f);
					m_SpawnCallback("Charger", pos);
					m_ChargersToSpawn--;
				}

				m_SpawnTimer = m_SpawnInterval;
			}
		}
	}

	void WaveSystem::StartWave(int waveNumber)
	{
		m_CurrentWave = waveNumber;
		m_WaveInProgress = true;
		m_DronesToSpawn = 0;
		m_TurretsToSpawn = 0;
		m_ChargersToSpawn = 0;

		// Calculate enemies for this wave
		float mult = 1.0f + (waveNumber - 1) * 0.2f;

		switch (waveNumber)
		{
		case 1:
			m_DronesToSpawn = (int)(5 * mult);
			PIL_INFO("=== WAVE 1: Drone Swarm ===");
			break;
		case 2:
			m_DronesToSpawn = (int)(8 * mult);
			m_TurretsToSpawn = 2;
			PIL_INFO("=== WAVE 2: Defensive Line ===");
			break;
		case 3:
			m_DronesToSpawn = (int)(6 * mult);
			m_ChargersToSpawn = (int)(3 * mult);
			PIL_INFO("=== WAVE 3: Rush Attack ===");
			break;
		case 4:
			m_DronesToSpawn = (int)(10 * mult);
			m_TurretsToSpawn = (int)(3 * mult);
			m_ChargersToSpawn = (int)(4 * mult);
			PIL_INFO("=== WAVE 4: Combined Assault ===");
			break;
		case 5:
			m_DronesToSpawn = (int)(15 * mult);
			m_TurretsToSpawn = (int)(4 * mult);
			m_ChargersToSpawn = (int)(6 * mult);
			PIL_INFO("=== WAVE 5: Final Stand ===");
			break;
		default:
			// Endless mode
			m_DronesToSpawn = (int)(10 + waveNumber * 2);
			m_TurretsToSpawn = waveNumber;
			m_ChargersToSpawn = waveNumber / 2;
			PIL_INFO("=== WAVE {}: Endless Mode ===", waveNumber);
			break;
		}

		PIL_INFO("Spawning {} drones, {} turrets, {} chargers", 
			m_DronesToSpawn, m_TurretsToSpawn, m_ChargersToSpawn);
	}

	void WaveSystem::StartNextWave()
	{
		StartWave(m_CurrentWave + 1);
	}

	bool WaveSystem::IsWaveInProgress() const
	{
		return m_WaveInProgress;
	}

	int WaveSystem::GetCurrentWave() const
	{
		return m_CurrentWave;
	}

	int WaveSystem::GetEnemiesRemaining(const std::vector<Entity>& enemies) const
	{
		int count = 0;
		for (const auto& enemy : enemies)
		{
			if (enemy.IsValid()) count++;
		}
		return count;
	}

	glm::vec2 WaveSystem::GetRandomSpawnPosition(float radius)
	{
		float angle = (rand() % 360) * 3.14159f / 180.0f;
		float dist = radius + (rand() % 30) / 10.0f;
		return { cosf(angle) * dist, sinf(angle) * dist };
	}

} // namespace Pillar
