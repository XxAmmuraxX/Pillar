#pragma once

#include <Pillar/ECS/Entity.h>
#include <glm/glm.hpp>

namespace Game {

    enum class EnemyType
    {
        Chaser,     // Moves directly toward player
        Wanderer,   // Random movement, occasional charge
        Shooter,    // Maintains distance, fires projectiles
        Swarm       // Lightweight, spawns in groups
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
    };

} // namespace Game
