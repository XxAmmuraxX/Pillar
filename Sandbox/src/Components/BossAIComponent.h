#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace Pillar {

	/**
	 * @brief Boss attack phases
	 */
	enum class BossPhase
	{
		Phase1,
		Phase2,
		Defeated
	};

	/**
	 * @brief Boss attack types
	 */
	enum class BossAttack
	{
		BulletSpray,
		LaserSweep,
		SummonDrones,
		Shield,
		ChargeSlam,
		None
	};

	/**
	 * @brief Boss AI component for complex boss battles
	 * Part of Arena Protocol showcase
	 */
	struct BossAIComponent
	{
		BossPhase Phase = BossPhase::Phase1;
		BossAttack CurrentAttack = BossAttack::None;
		
		// Timing
		float AttackTimer = 0.0f;
		float AttackCooldown = 3.0f;
		float AttackDuration = 2.0f;
		float PhaseTransitionTimer = 0.0f;
		
		// Attack pattern
		int AttackIndex = 0;
		std::vector<BossAttack> AttackPattern = { 
			BossAttack::BulletSpray, 
			BossAttack::SummonDrones, 
			BossAttack::BulletSpray,
			BossAttack::LaserSweep 
		};
		
		// Phase 2 pattern (more aggressive)
		std::vector<BossAttack> Phase2Pattern = {
			BossAttack::BulletSpray,
			BossAttack::ChargeSlam,
			BossAttack::LaserSweep,
			BossAttack::SummonDrones,
			BossAttack::BulletSpray,
			BossAttack::Shield
		};

		// Attack parameters
		float BulletSprayAngle = 0.0f;       // Current angle for circular spray
		float BulletSprayRate = 0.05f;       // Time between bullets
		float BulletSprayTimer = 0.0f;
		
		float LaserAngle = 0.0f;
		float LaserSweepSpeed = 90.0f;       // Degrees per second
		
		int SummonCount = 3;
		int SummonsRemaining = 0;
		float SummonInterval = 0.5f;
		float SummonTimer = 0.0f;
		
		bool ShieldActive = false;
		float ShieldDuration = 3.0f;
		float ShieldTimer = 0.0f;
		
		glm::vec2 ChargeTarget = glm::vec2(0.0f);
		bool IsCharging = false;

		// Phase transition
		float Phase2HealthThreshold = 0.5f;  // Switch to phase 2 at 50% HP
		bool IsTransitioning = false;
		float TransitionDuration = 2.0f;

		BossAIComponent() = default;
		BossAIComponent(const BossAIComponent&) = default;
	};

} // namespace Pillar
