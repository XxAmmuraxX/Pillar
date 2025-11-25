#pragma once

#include <cstdint>
#include "Pillar/ECS/Entity.h"

namespace Pillar {

	struct BulletComponent
	{
		Entity Owner;               // Who shot this bullet (for damage attribution)
		float Damage = 10.0f;
		float Lifetime = 5.0f;      // Auto-destroy after this many seconds
		float TimeAlive = 0.0f;

		bool Pierce = false;        // Penetrate through enemies
		uint32_t MaxHits = 1;       // How many targets to hit before destruction
		uint32_t HitsRemaining = 1;

		BulletComponent() = default;
		BulletComponent(const BulletComponent&) = default;
		BulletComponent(Entity owner, float damage)
			: Owner(owner), Damage(damage) {}
	};

} // namespace Pillar
