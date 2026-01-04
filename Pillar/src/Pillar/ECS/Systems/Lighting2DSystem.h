#pragma once

#include "Pillar/ECS/Systems/System.h"

namespace Pillar
{
	// Collects Light2DComponent + ShadowCaster2DComponent from the ECS and submits
	// them to the Lighting2D renderer for the current frame.
	//
	// Usage: call this after your sprite rendering submissions and before
	// Lighting2D::EndScene().
	class Lighting2DSystem : public System
	{
	public:
		Lighting2DSystem() = default;
		virtual ~Lighting2DSystem() = default;

		void OnUpdate(float dt) override;
	};
}
