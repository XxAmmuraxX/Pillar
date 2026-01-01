#pragma once

#include <glm/glm.hpp>

namespace Pillar {

	/**
	 * @brief Wave spawn data
	 */
	struct EnemyWaveData
	{
		int DroneCount = 0;
		int TurretCount = 0;
		int ChargerCount = 0;
		float SpawnRadius = 8.0f;
		float DelayBetweenSpawns = 0.2f;
	};

	/**
	 * @brief Wave manager component for tracking enemy waves
	 * Part of Arena Protocol showcase
	 */
	struct WaveManagerComponent
	{
		int CurrentWave = 0;
		int MaxWaves = 5;
		bool WaveInProgress = false;
		bool AllWavesComplete = false;
		
		// Spawn tracking
		int EnemiesToSpawn = 0;
		float SpawnTimer = 0.0f;
		float SpawnInterval = 0.3f;
		
		// Current wave data
		EnemyWaveData CurrentWaveData;
		int DronesSpawned = 0;
		int TurretsSpawned = 0;
		int ChargersSpawned = 0;
		
		// Wave transition
		float WaveTransitionDelay = 3.0f;
		float WaveTransitionTimer = 0.0f;
		
		// Difficulty scaling
		float DifficultyMultiplier = 1.0f;
		float DifficultyIncrement = 0.2f;  // Increase per wave

		WaveManagerComponent() = default;
		WaveManagerComponent(const WaveManagerComponent&) = default;

		EnemyWaveData GetWaveData(int wave)
		{
			EnemyWaveData data;
			float mult = DifficultyMultiplier + (wave * DifficultyIncrement);
			
			switch (wave)
			{
			case 1:
				data.DroneCount = (int)(5 * mult);
				break;
			case 2:
				data.DroneCount = (int)(8 * mult);
				data.TurretCount = (int)(2 * mult);
				break;
			case 3:
				data.DroneCount = (int)(6 * mult);
				data.ChargerCount = (int)(3 * mult);
				break;
			case 4:
				data.DroneCount = (int)(10 * mult);
				data.TurretCount = (int)(3 * mult);
				data.ChargerCount = (int)(4 * mult);
				break;
			case 5:
				data.DroneCount = (int)(15 * mult);
				data.TurretCount = (int)(4 * mult);
				data.ChargerCount = (int)(6 * mult);
				break;
			default:
				// Endless mode scaling
				data.DroneCount = (int)(10 + wave * 2 * mult);
				data.TurretCount = (int)(wave * mult);
				data.ChargerCount = (int)(wave * 0.5f * mult);
				break;
			}
			
			return data;
		}
	};

} // namespace Pillar
