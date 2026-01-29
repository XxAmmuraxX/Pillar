#pragma once

#include <Pillar/ECS/Entity.h>
#include <glm/glm.hpp>

namespace Game {

    // SCRAPYARD SALVATION enemy types - horrors of the wastes
    enum class EnemyType
    {
        Chaser,     // CRAWLER: Feral mutants on all fours, relentless pursuit
        Wanderer,   // SHAMBLING HUSK: Fungal corpses, slow then burst charge
        Shooter,    // SCRAP SENTINEL: Corrupted drones, ranged toxic projectiles
        Swarm       // SWARM RAT: Mutated vermin, overwhelming numbers
    };

    enum class EnemyState
    {
        Idle,
        Chasing,
        Attacking,
        Stunned,
        Dying
    };

    struct EnemyComponent
    {
        EnemyType Type = EnemyType::Chaser;
        EnemyState State = EnemyState::Idle;

        float MoveSpeed = 3.0f;
        float AttackDamage = 10.0f;
        float AttackRange = 1.0f;       // Distance to trigger attack
        float AttackCooldown = 1.0f;    // Time between attacks
        float AttackTimer = 0.0f;

        float DetectionRange = 15.0f;   // Range to detect player
        float XPValue = 10.0f;          // XP dropped on death

        // AI state
        Pillar::Entity TargetEntity;
        glm::vec2 WanderDirection = { 1.0f, 0.0f };
        float WanderTimer = 0.0f;

        // Get themed display name for enemy type
        static const char* GetDisplayName(EnemyType type)
        {
            switch (type)
            {
                case EnemyType::Chaser:   return "Crawler";
                case EnemyType::Wanderer: return "Shambling Husk";
                case EnemyType::Shooter:  return "Scrap Sentinel";
                case EnemyType::Swarm:    return "Swarm Rat";
                default:                  return "Unknown Horror";
            }
        }
    };

} // namespace Game
