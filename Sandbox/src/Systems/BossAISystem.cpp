#include "BossAISystem.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "../Components/BossAIComponent.h"
#include "../Components/HealthComponent.h"
#include "Pillar/Logger.h"
#include <glm/gtc/constants.hpp>

namespace Pillar {

	void BossAISystem::OnUpdate(Scene* scene, Entity boss, Entity player, float deltaTime)
	{
		if (!scene || !boss.IsValid() || !player.IsValid()) return;
		if (!boss.HasComponent<BossAIComponent>()) return;

		auto& bossAI = boss.GetComponent<BossAIComponent>();
		auto& bossTransform = boss.GetComponent<TransformComponent>();
		auto& playerTransform = player.GetComponent<TransformComponent>();

		glm::vec2 bossPos = { bossTransform.Position.x, bossTransform.Position.y };
		glm::vec2 playerPos = { playerTransform.Position.x, playerTransform.Position.y };

		// Check for phase transition
		CheckPhaseTransition(boss);

		// Handle phase transition animation
		if (bossAI.IsTransitioning)
		{
			bossAI.PhaseTransitionTimer -= deltaTime;
			if (bossAI.PhaseTransitionTimer <= 0.0f)
			{
				bossAI.IsTransitioning = false;
				bossAI.AttackIndex = 0;
				PIL_INFO("Boss entered Phase 2!");
			}
			return;
		}

		// If no current attack, wait for cooldown
		if (bossAI.CurrentAttack == BossAttack::None)
		{
			bossAI.AttackTimer -= deltaTime;
			if (bossAI.AttackTimer <= 0.0f)
			{
				SelectNextAttack(boss);
			}
			return;
		}

		// Update current attack
		switch (bossAI.CurrentAttack)
		{
		case BossAttack::BulletSpray:
			UpdateBulletSpray(boss, bossPos, deltaTime);
			break;
		case BossAttack::LaserSweep:
			UpdateLaserSweep(boss, bossPos, playerPos, deltaTime);
			break;
		case BossAttack::SummonDrones:
			UpdateSummonDrones(boss, bossPos, deltaTime);
			break;
		case BossAttack::Shield:
			UpdateShield(boss, deltaTime);
			break;
		case BossAttack::ChargeSlam:
			UpdateChargeSlam(boss, playerPos, deltaTime);
			break;
		default:
			break;
		}
	}

	void BossAISystem::UpdateBulletSpray(Entity boss, const glm::vec2& bossPos, float deltaTime)
	{
		auto& bossAI = boss.GetComponent<BossAIComponent>();

		bossAI.BulletSprayTimer -= deltaTime;
		bossAI.AttackDuration -= deltaTime;

		if (bossAI.BulletSprayTimer <= 0.0f && m_FireBulletCallback)
		{
			// Fire bullets in a circular pattern
			int bulletsPerSpray = (bossAI.Phase == BossPhase::Phase2) ? 8 : 5;
			float angleStep = (2.0f * glm::pi<float>()) / bulletsPerSpray;

			for (int i = 0; i < bulletsPerSpray; i++)
			{
				float angle = bossAI.BulletSprayAngle + (i * angleStep);
				glm::vec2 direction = { cosf(angle), sinf(angle) };
				m_FireBulletCallback(bossPos, direction, boss);
			}

			// Rotate for next spray
			bossAI.BulletSprayAngle += glm::radians(15.0f);
			bossAI.BulletSprayTimer = bossAI.BulletSprayRate;
		}

		// End attack
		if (bossAI.AttackDuration <= 0.0f)
		{
			bossAI.CurrentAttack = BossAttack::None;
			bossAI.AttackTimer = bossAI.AttackCooldown;
		}
	}

	void BossAISystem::UpdateLaserSweep(Entity boss, const glm::vec2& bossPos, const glm::vec2& playerPos, float deltaTime)
	{
		auto& bossAI = boss.GetComponent<BossAIComponent>();
		auto& transform = boss.GetComponent<TransformComponent>();

		bossAI.AttackDuration -= deltaTime;

		// Rotate laser
		float sweepSpeed = (bossAI.Phase == BossPhase::Phase2) ? 
			bossAI.LaserSweepSpeed * 1.5f : bossAI.LaserSweepSpeed;
		bossAI.LaserAngle += glm::radians(sweepSpeed) * deltaTime;

		// Fire bullets along laser line
		bossAI.BulletSprayTimer -= deltaTime;
		if (bossAI.BulletSprayTimer <= 0.0f && m_FireBulletCallback)
		{
			glm::vec2 laserDir = { cosf(bossAI.LaserAngle), sinf(bossAI.LaserAngle) };
			m_FireBulletCallback(bossPos, laserDir, boss);
			bossAI.BulletSprayTimer = 0.02f; // Very fast bullets for laser effect
		}

		// End attack
		if (bossAI.AttackDuration <= 0.0f)
		{
			bossAI.CurrentAttack = BossAttack::None;
			bossAI.AttackTimer = bossAI.AttackCooldown;
		}
	}

	void BossAISystem::UpdateSummonDrones(Entity boss, const glm::vec2& bossPos, float deltaTime)
	{
		auto& bossAI = boss.GetComponent<BossAIComponent>();

		bossAI.SummonTimer -= deltaTime;

		if (bossAI.SummonsRemaining > 0 && bossAI.SummonTimer <= 0.0f && m_SpawnDroneCallback)
		{
			// Spawn drone at random position around boss
			float angle = (rand() % 360) * glm::pi<float>() / 180.0f;
			float distance = 2.0f + (rand() % 30) / 10.0f;
			glm::vec2 spawnPos = bossPos + glm::vec2(cosf(angle), sinf(angle)) * distance;
			
			m_SpawnDroneCallback(spawnPos);
			bossAI.SummonsRemaining--;
			bossAI.SummonTimer = bossAI.SummonInterval;
			
			PIL_TRACE("Boss summoned drone ({} remaining)", bossAI.SummonsRemaining);
		}

		// End attack when all drones spawned
		if (bossAI.SummonsRemaining <= 0)
		{
			bossAI.CurrentAttack = BossAttack::None;
			bossAI.AttackTimer = bossAI.AttackCooldown;
		}
	}

	void BossAISystem::UpdateShield(Entity boss, float deltaTime)
	{
		auto& bossAI = boss.GetComponent<BossAIComponent>();

		bossAI.ShieldTimer -= deltaTime;

		if (bossAI.ShieldTimer <= 0.0f)
		{
			bossAI.ShieldActive = false;
			bossAI.CurrentAttack = BossAttack::None;
			bossAI.AttackTimer = bossAI.AttackCooldown;
			PIL_TRACE("Boss shield deactivated");
		}
	}

	void BossAISystem::UpdateChargeSlam(Entity boss, const glm::vec2& playerPos, float deltaTime)
	{
		auto& bossAI = boss.GetComponent<BossAIComponent>();
		auto& transform = boss.GetComponent<TransformComponent>();

		if (!bossAI.IsCharging)
		{
			// Windup - face the player
			bossAI.ChargeTarget = playerPos;
			bossAI.AttackDuration -= deltaTime;
			
			if (bossAI.AttackDuration <= 0.0f)
			{
				bossAI.IsCharging = true;
				bossAI.AttackDuration = 1.0f; // Charge duration
			}
		}
		else
		{
			// Charge toward target
			glm::vec2 bossPos = { transform.Position.x, transform.Position.y };
			glm::vec2 toTarget = bossAI.ChargeTarget - bossPos;
			float distance = glm::length(toTarget);

			if (distance > 0.5f)
			{
				glm::vec2 direction = glm::normalize(toTarget);
				float chargeSpeed = 15.0f;
				transform.Position.x += direction.x * chargeSpeed * deltaTime;
				transform.Position.y += direction.y * chargeSpeed * deltaTime;
				transform.Dirty = true;
			}

			bossAI.AttackDuration -= deltaTime;
			if (bossAI.AttackDuration <= 0.0f || distance <= 0.5f)
			{
				bossAI.IsCharging = false;
				bossAI.CurrentAttack = BossAttack::None;
				bossAI.AttackTimer = bossAI.AttackCooldown * 1.5f; // Longer cooldown after charge
			}
		}
	}

	void BossAISystem::SelectNextAttack(Entity boss)
	{
		auto& bossAI = boss.GetComponent<BossAIComponent>();

		// Select pattern based on phase
		auto& pattern = (bossAI.Phase == BossPhase::Phase2) ? 
			bossAI.Phase2Pattern : bossAI.AttackPattern;

		if (pattern.empty()) return;

		// Get next attack from pattern
		bossAI.CurrentAttack = pattern[bossAI.AttackIndex];
		bossAI.AttackIndex = (bossAI.AttackIndex + 1) % pattern.size();

		// Initialize attack
		switch (bossAI.CurrentAttack)
		{
		case BossAttack::BulletSpray:
			bossAI.AttackDuration = (bossAI.Phase == BossPhase::Phase2) ? 4.0f : 3.0f;
			bossAI.BulletSprayTimer = 0.0f;
			PIL_INFO("Boss attack: Bullet Spray");
			break;
		case BossAttack::LaserSweep:
			bossAI.AttackDuration = 3.0f;
			bossAI.BulletSprayTimer = 0.0f;
			PIL_INFO("Boss attack: Laser Sweep");
			break;
		case BossAttack::SummonDrones:
			bossAI.SummonCount = (bossAI.Phase == BossPhase::Phase2) ? 5 : 3;
			bossAI.SummonsRemaining = bossAI.SummonCount;
			bossAI.SummonTimer = 0.0f;
			PIL_INFO("Boss attack: Summon Drones ({})", bossAI.SummonCount);
			break;
		case BossAttack::Shield:
			bossAI.ShieldActive = true;
			bossAI.ShieldTimer = bossAI.ShieldDuration;
			PIL_INFO("Boss attack: Shield");
			break;
		case BossAttack::ChargeSlam:
			bossAI.AttackDuration = 0.5f; // Windup time
			bossAI.IsCharging = false;
			PIL_INFO("Boss attack: Charge Slam");
			break;
		default:
			break;
		}
	}

	void BossAISystem::CheckPhaseTransition(Entity boss)
	{
		if (!boss.HasComponent<HealthComponent>()) return;

		auto& bossAI = boss.GetComponent<BossAIComponent>();
		auto& health = boss.GetComponent<HealthComponent>();

		if (bossAI.Phase == BossPhase::Phase1)
		{
			float healthPercent = health.GetHealthPercent();
			if (healthPercent <= bossAI.Phase2HealthThreshold)
			{
				PIL_INFO("Boss transitioning to Phase 2!");
				bossAI.Phase = BossPhase::Phase2;
				bossAI.IsTransitioning = true;
				bossAI.PhaseTransitionTimer = bossAI.TransitionDuration;
				bossAI.CurrentAttack = BossAttack::None;
				
				// Cancel shield if active
				bossAI.ShieldActive = false;
			}
		}
	}

} // namespace Pillar
