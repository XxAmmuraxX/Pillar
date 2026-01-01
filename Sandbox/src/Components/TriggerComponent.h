#pragma once

#include "Pillar/ECS/Entity.h"
#include <vector>
#include <string>

namespace Pillar {

	/**
	 * @brief Trigger component for zone-based events
	 * Part of Arena Protocol showcase
	 */
	struct TriggerComponent
	{
		std::string OnEnterEvent;
		std::string OnExitEvent;
		bool TriggerOnce = false;
		bool HasTriggered = false;
		std::vector<Entity> EntitiesInside;

		TriggerComponent() = default;
		TriggerComponent(const TriggerComponent&) = default;
	};

} // namespace Pillar
