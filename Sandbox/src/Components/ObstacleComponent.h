#pragma once

#include <glm/glm.hpp>

namespace Pillar {

	/**
	 * @brief Types of obstacles
	 */
	enum class ObstacleType
	{
		Crate,
		Pillar,
		Wall,
		Destructible
	};

	/**
	 * @brief Obstacle component for environment entities
	 * Part of Arena Protocol showcase
	 */
	struct ObstacleComponent
	{
		ObstacleType Type = ObstacleType::Crate;
		bool IsDestructible = false;
		float Health = 100.0f;
		bool BlocksBullets = true;
		bool BlocksMovement = true;

		ObstacleComponent() = default;
		ObstacleComponent(const ObstacleComponent&) = default;
		ObstacleComponent(ObstacleType type, bool destructible = false)
			: Type(type), IsDestructible(destructible) 
		{
			if (destructible)
			{
				Health = 50.0f;
			}
		}
	};

} // namespace Pillar
