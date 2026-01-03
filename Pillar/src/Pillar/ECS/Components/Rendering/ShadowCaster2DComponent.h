#pragma once

#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

namespace Pillar
{
	struct ShadowCaster2DComponent
	{
		// Local-space points, recommended CCW for consistent one-sided casting.
		std::vector<glm::vec2> Points;
		bool Closed = true;
		bool TwoSided = false;
		uint32_t LayerMask = 0xFFFFFFFFu;

		ShadowCaster2DComponent() = default;
		ShadowCaster2DComponent(const ShadowCaster2DComponent&) = default;
	};
}
