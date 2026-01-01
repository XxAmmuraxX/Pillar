#pragma once

#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include <glm/glm.hpp>
#include <vector>
#include <functional>

namespace Pillar {

	using WaveCompleteCallback = std::function<void(int waveNumber)>;
	using SpawnEnemyCallback = std::function<void(const std::string& type, const glm::vec2& pos)>;

	/**
	 * @brief System for managing enemy waves
	 * Part of Arena Protocol showcase
	 */
	class WaveSystem
	{
	public:
		WaveSystem() = default;
		~WaveSystem() = default;

		void SetWaveCompleteCallback(WaveCompleteCallback callback) { m_WaveCompleteCallback = callback; }
		void SetSpawnCallback(SpawnEnemyCallback callback) { m_SpawnCallback = callback; }

		void OnUpdate(Scene* scene, std::vector<Entity>& enemies, float deltaTime);
		
		void StartWave(int waveNumber);
		void StartNextWave();
		
		bool IsWaveInProgress() const;
		int GetCurrentWave() const;
		int GetEnemiesRemaining(const std::vector<Entity>& enemies) const;

	private:
		void SpawnEnemy(const std::string& type, const glm::vec2& position);
		glm::vec2 GetRandomSpawnPosition(float radius);

		int m_CurrentWave = 0;
		int m_MaxWaves = 5;
		bool m_WaveInProgress = false;
		bool m_AllWavesComplete = false;

		// Spawn tracking
		int m_DronesToSpawn = 0;
		int m_TurretsToSpawn = 0;
		int m_ChargersToSpawn = 0;
		float m_SpawnTimer = 0.0f;
		float m_SpawnInterval = 0.3f;

		// Wave transition
		float m_WaveTransitionDelay = 3.0f;
		float m_WaveTransitionTimer = 0.0f;
		bool m_WaitingForNextWave = false;

		WaveCompleteCallback m_WaveCompleteCallback;
		SpawnEnemyCallback m_SpawnCallback;
	};

} // namespace Pillar
