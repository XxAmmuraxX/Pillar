#include "EnemyAISystem.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"
#include "../Components/EnemyAIComponent.h"
#include "../Components/TurretAIComponent.h"
#include "../Components/ChargerAIComponent.h"
#include "../Components/HealthComponent.h"
#include <glm/gtc/constants.hpp>

namespace Pillar {

	void EnemyAISystem::OnUpdate(Scene* scene, Entity player, float deltaTime)
	{
		if (!scene || !player.IsValid()) return;

		auto& playerTransform = player.GetComponent<TransformComponent>();
		glm::vec2 playerPos = { playerTransform.Position.x, playerTransform.Position.y };

		// Update drones
		auto droneView = scene->GetRegistry().view<TransformComponent, EnemyAIComponent>();
		for (auto entity : droneView)
		{
			Entity e(entity, scene);
			UpdateDroneAI(e, playerPos, deltaTime);
		}

		// Update turrets
		auto turretView = scene->GetRegistry().view<TransformComponent, TurretAIComponent>();
		for (auto entity : turretView)
		{
			Entity e(entity, scene);
			UpdateTurretAI(scene, e, playerPos, player, deltaTime);
		}

		// Update chargers
		auto chargerView = scene->GetRegistry().view<TransformComponent, ChargerAIComponent, VelocityComponent>();
		for (auto entity : chargerView)
		{
			Entity e(entity, scene);
			UpdateChargerAI(e, playerPos, deltaTime);
		}
	}

	void EnemyAISystem::UpdateDroneAI(Entity enemy, const glm::vec2& playerPos, float deltaTime)
	{
		if (!enemy.IsValid()) return;

		auto& transform = enemy.GetComponent<TransformComponent>();
		auto& ai = enemy.GetComponent<EnemyAIComponent>();

		if (ai.Behavior == AIBehavior::SeekPlayer)
		{
			glm::vec2 enemyPos = { transform.Position.x, transform.Position.y };
			glm::vec2 toPlayer = playerPos - enemyPos;
			float distance = glm::length(toPlayer);

			if (distance > 0.5f && distance < ai.DetectionRange)
			{
				glm::vec2 direction = glm::normalize(toPlayer);
				transform.Position.x += direction.x * ai.Speed * deltaTime;
				transform.Position.y += direction.y * ai.Speed * deltaTime;
				transform.Dirty = true;

				// Face the player
				transform.Rotation = atan2f(direction.y, direction.x);
			}
		}
		else if (ai.Behavior == AIBehavior::Flee)
		{
			glm::vec2 enemyPos = { transform.Position.x, transform.Position.y };
			glm::vec2 fromPlayer = enemyPos - playerPos;
			float distance = glm::length(fromPlayer);

			if (distance < ai.DetectionRange && distance > 0.1f)
			{
				glm::vec2 direction = glm::normalize(fromPlayer);
				transform.Position.x += direction.x * ai.Speed * deltaTime;
				transform.Position.y += direction.y * ai.Speed * deltaTime;
				transform.Dirty = true;
			}
		}
		else if (ai.Behavior == AIBehavior::Patrol)
		{
			// Per-entity patrol using component state
			ai.PatrolAngle += ai.Speed * 0.1f * deltaTime;
			transform.Position.x = ai.PatrolCenter.x + cosf(ai.PatrolAngle) * ai.PatrolRadius;
			transform.Position.y = ai.PatrolCenter.y + sinf(ai.PatrolAngle) * ai.PatrolRadius;
			transform.Dirty = true;
		}
	}

	void EnemyAISystem::UpdateTurretAI(Scene* scene, Entity enemy, const glm::vec2& playerPos, Entity player, float deltaTime)
	{
		if (!enemy.IsValid()) return;

		auto& transform = enemy.GetComponent<TransformComponent>();
		auto& turretAI = enemy.GetComponent<TurretAIComponent>();

		glm::vec2 enemyPos = { transform.Position.x, transform.Position.y };
		glm::vec2 toPlayer = playerPos - enemyPos;
		float distance = glm::length(toPlayer);

		if (distance > 0.0f && distance < turretAI.Range)
		{
			// Smoothly rotate toward player
			float targetAngle = atan2f(toPlayer.y, toPlayer.x);
			float angleDiff = targetAngle - transform.Rotation;
			
			// Normalize angle difference
			while (angleDiff > glm::pi<float>()) angleDiff -= 2.0f * glm::pi<float>();
			while (angleDiff < -glm::pi<float>()) angleDiff += 2.0f * glm::pi<float>();
			
			float maxRotation = glm::radians(turretAI.RotationSpeed) * deltaTime;
			if (fabsf(angleDiff) < maxRotation)
				transform.Rotation = targetAngle;
			else
				transform.Rotation += (angleDiff > 0 ? maxRotation : -maxRotation);
			
			transform.Dirty = true;

			// Also rotate the barrel (child entity)
			if (turretAI.BarrelEntity.IsValid())
			{
				auto& barrelTransform = turretAI.BarrelEntity.GetComponent<TransformComponent>();
				barrelTransform.Position = glm::vec2(transform.Position); // Keep in sync
				barrelTransform.Rotation = transform.Rotation; // Match rotation
				barrelTransform.Dirty = true;
			}

			// Fire timer (actual firing handled in main game loop with access to bullet pool)
			turretAI.FireTimer -= deltaTime;
		}
	}

	void EnemyAISystem::UpdateChargerAI(Entity enemy, const glm::vec2& playerPos, float deltaTime)
	{
		if (!enemy.IsValid()) return;

		auto& transform = enemy.GetComponent<TransformComponent>();
		auto& chargerAI = enemy.GetComponent<ChargerAIComponent>();
		auto& velocity = enemy.GetComponent<VelocityComponent>();

		glm::vec2 enemyPos = { transform.Position.x, transform.Position.y };
		glm::vec2 toPlayer = playerPos - enemyPos;
		float distance = glm::length(toPlayer);

		// State machine
		if (!chargerAI.IsCharging && !chargerAI.IsWindingUp)
		{
			// Idle state - check if should start charge
			if (distance < 10.0f && distance > 2.0f)
			{
				chargerAI.IsWindingUp = true;
				chargerAI.WindupTimer = chargerAI.WindupTime;
				chargerAI.ChargeDirection = glm::normalize(toPlayer);
				velocity.Velocity = glm::vec2(0.0f);
			}
		}
		else if (chargerAI.IsWindingUp)
		{
			// Windup state - prepare to charge
			chargerAI.WindupTimer -= deltaTime;
			velocity.Velocity = glm::vec2(0.0f);

			// Visual feedback: slight backward movement
			transform.Position.x -= chargerAI.ChargeDirection.x * 0.5f * deltaTime;
			transform.Position.y -= chargerAI.ChargeDirection.y * 0.5f * deltaTime;
			transform.Dirty = true;

			if (chargerAI.WindupTimer <= 0.0f)
			{
				chargerAI.IsWindingUp = false;
				chargerAI.IsCharging = true;
				chargerAI.WindupTimer = chargerAI.ChargeDuration;
			}
		}
		else if (chargerAI.IsCharging)
		{
			// Charging state - dash forward
			chargerAI.WindupTimer -= deltaTime;
			velocity.Velocity = chargerAI.ChargeDirection * chargerAI.ChargeSpeed;

			transform.Position.x += velocity.Velocity.x * deltaTime;
			transform.Position.y += velocity.Velocity.y * deltaTime;
			transform.Rotation = atan2f(chargerAI.ChargeDirection.y, chargerAI.ChargeDirection.x);
			transform.Dirty = true;

			if (chargerAI.WindupTimer <= 0.0f)
			{
				chargerAI.IsCharging = false;
				velocity.Velocity = glm::vec2(0.0f);
			}
		}
	}

} // namespace Pillar
