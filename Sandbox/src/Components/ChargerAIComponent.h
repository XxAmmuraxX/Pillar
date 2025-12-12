#pragma once

#include <glm/glm.hpp>

namespace Pillar {

	/**
	 * @brief Charger AI component for dash attack enemies
	 * Part of Arena Protocol showcase
	 */
	struct ChargerAIComponent
	{
		float ChargeSpeed = 15.0f;
		float WindupTime = 0.5f;
		float WindupTimer = 0.0f;
		float ChargeDuration = 1.0f;
		glm::vec2 ChargeDirection = glm::vec2(0.0f);
		bool IsCharging = false;
		bool IsWindingUp = false;

		ChargerAIComponent() = default;
		ChargerAIComponent(const ChargerAIComponent&) = default;
	};

} // namespace Pillar
