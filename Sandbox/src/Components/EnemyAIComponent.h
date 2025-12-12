#pragma once

#include "Pillar/ECS/Entity.h"
#include <glm/glm.hpp>

namespace Pillar {

	/**
	 * @brief AI behavior types
	 */
	enum class AIBehavior 
	{ 
		Idle, 
		SeekPlayer, 
		Flee, 
		Patrol 
	};

	/**
	 * @brief Enemy AI component for basic behaviors
	 * Part of Arena Protocol showcase
	 */
	struct EnemyAIComponent
	{
		AIBehavior Behavior = AIBehavior::SeekPlayer;
		float Speed = 3.0f;
		float DetectionRange = 15.0f;
		Entity Target;  // Usually the player
		
		// Patrol state
		float PatrolAngle = 0.0f;
		glm::vec2 PatrolCenter = glm::vec2(0.0f);
		float PatrolRadius = 5.0f;

		EnemyAIComponent() = default;
		EnemyAIComponent(const EnemyAIComponent&) = default;
	};

} // namespace Pillar
