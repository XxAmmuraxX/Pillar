#pragma once

#include "Pillar/Core.h"
#include "System.h"

namespace Pillar {

	// Custom physics for Light Entities (no Box2D)
	// Simple Euler integration: pos += vel * dt
	class PIL_API VelocityIntegrationSystem : public System
	{
	public:
		void OnUpdate(float deltaTime) override;

	private:
		void IntegrateVelocity(float deltaTime);
	};

} // namespace Pillar
