#pragma once

#include <Pillar/ECS/Scene.h>
#include <Pillar/ECS/Entity.h>
#include <Pillar/Logger.h>
#include <glm/glm.hpp>
#include <vector>
#include <functional>
#include <random>
#include <cmath>

#include "../Components/EnemyComponent.h"

namespace Game {

    struct WaveConfig
    {
        int WaveNumber = 1;
        int ChaserCount = 3;
        int ShooterCount = 0;
        int SwarmCount = 0;
        float SpawnDelay = 0.5f;        // Delay between spawns
        float TimeBetweenWaves = 3.0f;  // Rest time before next wave
    };

    class WaveManager
    {
    public:
        using SpawnCallback = std::function<void(const glm::vec2&, EnemyType)>;

    public:
        void Init(float arenaWidth, float arenaHeight)
        {
            m_ArenaWidth = arenaWidth;
            m_ArenaHeight = arenaHeight;
            m_CurrentWave = 0;
            m_State = WaveState::WaveComplete;
            m_Timer = 2.0f;  // Initial delay before first wave

            PIL_INFO("WaveManager initialized: Arena {}x{}", arenaWidth, arenaHeight);
        }

        void SetSpawnCallback(SpawnCallback callback)
        {
            m_SpawnCallback = callback;
        }

        void OnUpdate(float dt, int currentEnemyCount)
        {
            m_Timer -= dt;

            switch (m_State)
            {
                case WaveState::WaveComplete:
                    // Wait for rest period before starting next wave
                    if (m_Timer <= 0.0f)
                    {
                        StartNextWave();
                    }
                    break;

                case WaveState::Spawning:
                    // Spawn enemies with delay
                    if (m_Timer <= 0.0f && m_EnemiesToSpawn > 0)
                    {
                        SpawnNextEnemy();
                        m_Timer = m_CurrentConfig.SpawnDelay;
                    }

                    // Check if all enemies spawned
                    if (m_EnemiesToSpawn <= 0)
                    {
                        m_State = WaveState::InProgress;
                        PIL_INFO("Wave {} - All enemies spawned!", m_CurrentWave);
                    }
                    break;

                case WaveState::InProgress:
                    // Wait for all enemies to be killed
                    if (currentEnemyCount <= 0)
                    {
                        OnWaveComplete();
                    }
                    break;
            }
        }

        int GetCurrentWave() const { return m_CurrentWave; }
        bool IsSpawning() const { return m_State == WaveState::Spawning; }
        bool IsWaveInProgress() const { return m_State == WaveState::InProgress; }
        float GetTimeUntilNextWave() const 
        { 
            return m_State == WaveState::WaveComplete ? m_Timer : 0.0f; 
        }
        int GetEnemiesToSpawn() const { return m_EnemiesToSpawn; }

        void Reset()
        {
            m_CurrentWave = 0;
            m_State = WaveState::WaveComplete;
            m_Timer = 2.0f;  // Initial delay before first wave
            m_EnemiesToSpawn = 0;
            m_SpawnQueue.clear();
            PIL_INFO("WaveManager reset");
        }

    private:
        enum class WaveState
        {
            WaveComplete,   // Rest period between waves
            Spawning,       // Actively spawning enemies
            InProgress      // Wave active, waiting for enemies to die
        };

        void StartNextWave()
        {
            m_CurrentWave++;
            m_CurrentConfig = GenerateWaveConfig(m_CurrentWave);
            m_State = WaveState::Spawning;
            m_Timer = 0.0f;

            // Calculate total enemies to spawn
            m_EnemiesToSpawn = m_CurrentConfig.ChaserCount + 
                               m_CurrentConfig.ShooterCount + 
                               m_CurrentConfig.SwarmCount;

            // Build spawn queue
            m_SpawnQueue.clear();
            for (int i = 0; i < m_CurrentConfig.ChaserCount; i++)
                m_SpawnQueue.push_back(EnemyType::Chaser);
            for (int i = 0; i < m_CurrentConfig.ShooterCount; i++)
                m_SpawnQueue.push_back(EnemyType::Shooter);
            for (int i = 0; i < m_CurrentConfig.SwarmCount; i++)
                m_SpawnQueue.push_back(EnemyType::Swarm);

            // Shuffle spawn queue for variety
            std::shuffle(m_SpawnQueue.begin(), m_SpawnQueue.end(), m_RandomEngine);

            PIL_INFO("Wave {} starting! Chasers: {}, Shooters: {}, Swarm: {}",
                m_CurrentWave, 
                m_CurrentConfig.ChaserCount,
                m_CurrentConfig.ShooterCount,
                m_CurrentConfig.SwarmCount);
        }

        void SpawnNextEnemy()
        {
            if (m_SpawnQueue.empty() || !m_SpawnCallback) return;

            EnemyType type = m_SpawnQueue.back();
            m_SpawnQueue.pop_back();
            m_EnemiesToSpawn--;

            glm::vec2 spawnPos = GetRandomSpawnPosition();
            m_SpawnCallback(spawnPos, type);
        }

        void OnWaveComplete()
        {
            m_State = WaveState::WaveComplete;
            m_Timer = m_CurrentConfig.TimeBetweenWaves;

            PIL_INFO("Wave {} complete! Next wave in {:.1f}s", 
                m_CurrentWave, m_CurrentConfig.TimeBetweenWaves);
        }

        WaveConfig GenerateWaveConfig(int waveNumber)
        {
            WaveConfig config;
            config.WaveNumber = waveNumber;

            // Base enemy counts - scale with wave number
            config.ChaserCount = 2 + waveNumber;

            // Introduce shooters at wave 3
            if (waveNumber >= 3)
                config.ShooterCount = (waveNumber - 2);

            // Introduce swarm at wave 5
            if (waveNumber >= 5)
                config.SwarmCount = (waveNumber - 4) * 3;

            // Cap enemy counts for performance
            config.ChaserCount = std::min(config.ChaserCount, 15);
            config.ShooterCount = std::min(config.ShooterCount, 8);
            config.SwarmCount = std::min(config.SwarmCount, 20);

            // Spawn delay decreases as waves progress (faster spawning)
            config.SpawnDelay = std::max(0.1f, 0.5f - (waveNumber * 0.03f));

            // Rest time between waves
            config.TimeBetweenWaves = std::max(2.0f, 5.0f - (waveNumber * 0.2f));

            return config;
        }

        glm::vec2 GetRandomSpawnPosition()
        {
            // Spawn from arena edges
            std::uniform_real_distribution<float> sideDist(0.0f, 4.0f);
            int side = static_cast<int>(sideDist(m_RandomEngine));

            float halfWidth = m_ArenaWidth * 0.5f - 1.0f;
            float halfHeight = m_ArenaHeight * 0.5f - 1.0f;

            std::uniform_real_distribution<float> xDist(-halfWidth, halfWidth);
            std::uniform_real_distribution<float> yDist(-halfHeight, halfHeight);

            glm::vec2 pos;
            switch (side)
            {
                case 0: // Top
                    pos = { xDist(m_RandomEngine), halfHeight };
                    break;
                case 1: // Bottom
                    pos = { xDist(m_RandomEngine), -halfHeight };
                    break;
                case 2: // Left
                    pos = { -halfWidth, yDist(m_RandomEngine) };
                    break;
                case 3: // Right
                default:
                    pos = { halfWidth, yDist(m_RandomEngine) };
                    break;
            }

            return pos;
        }

    private:
        float m_ArenaWidth = 30.0f;
        float m_ArenaHeight = 16.0f;

        int m_CurrentWave = 0;
        WaveState m_State = WaveState::WaveComplete;
        float m_Timer = 0.0f;
        int m_EnemiesToSpawn = 0;

        WaveConfig m_CurrentConfig;
        std::vector<EnemyType> m_SpawnQueue;
        SpawnCallback m_SpawnCallback;

        std::mt19937 m_RandomEngine{ std::random_device{}() };
    };

} // namespace Game
