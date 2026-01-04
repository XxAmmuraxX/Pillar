#pragma once

#include <glm/glm.hpp>
#include <cstdint>

#include "Pillar/Renderer/Lighting2D.h"

namespace Pillar
{
	struct Light2DComponent
	{
		Light2DType Type = Light2DType::Point;
		glm::vec3 Color{ 1.0f, 0.85f, 0.6f };
		float Intensity = 1.0f;
		float Radius = 6.0f;

		// Spot light parameters (optional in v1)
		float InnerAngleRadians = 0.25f;
		float OuterAngleRadians = 0.5f;

		bool CastShadows = true;
		float ShadowStrength = 1.0f;

		// Used for filtering against shadow casters (and future receiver masks).
		uint32_t LayerMask = 0xFFFFFFFFu;

		Light2DComponent() = default;
		Light2DComponent(const Light2DComponent&) = default;
	};
}
