#pragma once

#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

namespace Pillar::Lighting2DGeometry
{
	struct ShadowCaster2D
	{
		std::vector<glm::vec2> WorldPoints;
		bool Closed = true;
		bool TwoSided = false;
		uint32_t LayerMask = 0xFFFFFFFFu;
	};

	struct Light2D
	{
		glm::vec2 Position{ 0.0f };
		float Radius = 1.0f;
		uint32_t LayerMask = 0xFFFFFFFFu;
	};

	// Output is a flat list of vertices where every 3 vertices form one triangle.
	// Triangles are in world-space.
	void BuildShadowVolumeTriangles(const Light2D& light,
		const ShadowCaster2D& caster,
		std::vector<glm::vec2>& outTriangleVertices);

	// Conservative 2D AABB-range test for skipping casters.
	bool IsCasterInRange(const Light2D& light, const ShadowCaster2D& caster);
}
