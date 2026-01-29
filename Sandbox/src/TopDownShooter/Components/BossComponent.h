#pragma once

#include <glm/glm.hpp>
#include <string>

namespace Game {

    // SCRAPYARD SALVATION boss types - nightmares of the wastes
    enum class BossType
    {
        Behemoth,       // THE BEHEMOTH: Bloated horror, corrupted flesh mountain
        Swarm_Queen,    // THE BROOD MOTHER: Spawns Swarm Rats from body
        Devastator      // THE HARVESTER: Corrupted machine spider, hooks & saws
    };

    enum class BossPhase
    {
        Phase1,         // Full health - normal attacks
        Phase2,         // 50% health - enraged, faster attacks
        Phase3          // 25% health - desperate, most dangerous
    };

    struct BossComponent
    {
        BossType Type = BossType::Behemoth;
        BossPhase Phase = BossPhase::Phase1;
        std::string Name = "Behemoth";

        // Stats (much higher than regular enemies)
        float MaxHealth = 500.0f;
        float MoveSpeed = 2.0f;
        float AttackDamage = 30.0f;
        float AttackCooldown = 2.0f;
        float AttackTimer = 0.0f;

        // Special ability
        float SpecialCooldown = 5.0f;
        float SpecialTimer = 0.0f;

        // Minion spawning (for Brood Mother)
        int MinionsToSpawn = 0;
        float MinionSpawnTimer = 0.0f;
        float MinionSpawnDelay = 1.0f;

        // XP reward (much higher than regular enemies)
        int XPReward = 200;
        int ScoreReward = 1000;

        // Visual indicators
        bool ShowHealthBar = true;
        float HealthBarWidth = 3.0f;

        // Phase transition thresholds
        float Phase2Threshold = 0.5f;   // 50% health
        float Phase3Threshold = 0.25f;  // 25% health

        void UpdatePhase(float healthPercent)
        {
            if (healthPercent <= Phase3Threshold && Phase != BossPhase::Phase3)
            {
                Phase = BossPhase::Phase3;
                // Enrage - increase attack speed
                AttackCooldown *= 0.5f;
                MoveSpeed *= 1.3f;
            }
            else if (healthPercent <= Phase2Threshold && Phase == BossPhase::Phase1)
            {
                Phase = BossPhase::Phase2;
                // Increase aggression
                AttackCooldown *= 0.75f;
                MoveSpeed *= 1.15f;
            }
        }

        // Get themed display name for boss type
        static const char* GetDisplayName(BossType type)
        {
            switch (type)
            {
                case BossType::Behemoth:    return "THE BEHEMOTH";
                case BossType::Swarm_Queen: return "THE BROOD MOTHER";
                case BossType::Devastator:  return "THE HARVESTER";
                default:                    return "UNKNOWN HORROR";
            }
        }

        static BossComponent CreateBehemoth(int waveNumber)
        {
            BossComponent boss;
            boss.Type = BossType::Behemoth;
            boss.Name = "THE BEHEMOTH";
            boss.MaxHealth = 300.0f + waveNumber * 50.0f;
            boss.MoveSpeed = 2.5f;
            boss.AttackDamage = 25.0f + waveNumber * 2.0f;
            boss.AttackCooldown = 1.5f;
            boss.XPReward = 150 + waveNumber * 25;
            boss.ScoreReward = 500 + waveNumber * 100;
            return boss;
        }

        static BossComponent CreateSwarmQueen(int waveNumber)
        {
            BossComponent boss;
            boss.Type = BossType::Swarm_Queen;
            boss.Name = "THE BROOD MOTHER";
            boss.MaxHealth = 250.0f + waveNumber * 40.0f;
            boss.MoveSpeed = 2.0f;
            boss.AttackDamage = 15.0f + waveNumber * 1.5f;
            boss.AttackCooldown = 2.0f;
            boss.SpecialCooldown = 4.0f;  // Spawn minions every 4 seconds
            boss.XPReward = 200 + waveNumber * 30;
            boss.ScoreReward = 600 + waveNumber * 100;
            return boss;
        }

        static BossComponent CreateDevastator(int waveNumber)
        {
            BossComponent boss;
            boss.Type = BossType::Devastator;
            boss.Name = "THE HARVESTER";
            boss.MaxHealth = 350.0f + waveNumber * 45.0f;
            boss.MoveSpeed = 1.5f;
            boss.AttackDamage = 35.0f + waveNumber * 2.5f;
            boss.AttackCooldown = 2.5f;
            boss.SpecialCooldown = 6.0f;  // Area attack every 6 seconds
            boss.XPReward = 250 + waveNumber * 35;
            boss.ScoreReward = 800 + waveNumber * 150;
            return boss;
        }
    };

} // namespace Game
