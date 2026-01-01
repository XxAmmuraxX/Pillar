#pragma once

#include <glm/glm.hpp>
#include "Pillar/ECS/Entity.h"

namespace Pillar {

	/**
	 * @brief Turret AI component for stationary shooting enemies
	 * Part of Arena Protocol showcase
	 */
	struct TurretAIComponent
	{
		float FireRate = 1.0f;           // Shots per second
		float FireTimer = 0.0f;          // Internal countdown timer
		float Range = 10.0f;             // Detection/firing range
		float RotationSpeed = 180.0f;    // Degrees per second
		
		Entity BarrelEntity;             // Child entity for barrel sprite (renders on top)

		TurretAIComponent() = default;
		TurretAIComponent(const TurretAIComponent&) = default;
	};

} // namespace Pillar
