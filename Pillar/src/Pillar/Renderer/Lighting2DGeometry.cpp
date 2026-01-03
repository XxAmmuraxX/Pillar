#include "Pillar/Renderer/Lighting2DGeometry.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Pillar::Lighting2DGeometry
{
	static float SignedAreaClosedPolygon(const std::vector<glm::vec2>& pts)
	{
		if (pts.size() < 3)
			return 0.0f;
		float a = 0.0f;
		for (size_t i = 0; i < pts.size(); ++i)
		{
			const glm::vec2& p0 = pts[i];
			const glm::vec2& p1 = pts[(i + 1) % pts.size()];
			a += (p0.x * p1.y) - (p1.x * p0.y);
		}
		return 0.5f * a;
	}

	static float LengthSquared(const glm::vec2& v)
	{
		return v.x * v.x + v.y * v.y;
	}

	static glm::vec2 SafeNormalize(const glm::vec2& v)
	{
		float lenSq = LengthSquared(v);
		if (lenSq <= std::numeric_limits<float>::epsilon())
			return glm::vec2(0.0f);
		return v / std::sqrt(lenSq);
	}

	static void ComputeAABB(const std::vector<glm::vec2>& pts, glm::vec2& outMin, glm::vec2& outMax)
	{
		if (pts.empty())
		{
			outMin = glm::vec2(0.0f);
			outMax = glm::vec2(0.0f);
			return;
		}

		outMin = pts[0];
		outMax = pts[0];
		for (size_t i = 1; i < pts.size(); ++i)
		{
			outMin = glm::min(outMin, pts[i]);
			outMax = glm::max(outMax, pts[i]);
		}
	}

	bool IsCasterInRange(const Light2D& light, const ShadowCaster2D& caster)
	{
		if (caster.WorldPoints.size() < 2)
			return false;

		if ((light.LayerMask & caster.LayerMask) == 0)
			return false;

		glm::vec2 aabbMin, aabbMax;
		ComputeAABB(caster.WorldPoints, aabbMin, aabbMax);

		// Compute squared distance from point to AABB (conservative)
		glm::vec2 p = light.Position;
		float dx = 0.0f;
		if (p.x < aabbMin.x) dx = aabbMin.x - p.x;
		else if (p.x > aabbMax.x) dx = p.x - aabbMax.x;

		float dy = 0.0f;
		if (p.y < aabbMin.y) dy = aabbMin.y - p.y;
		else if (p.y > aabbMax.y) dy = p.y - aabbMax.y;

		float distSq = dx * dx + dy * dy;
		return distSq <= (light.Radius * light.Radius);
	}

	// Helper: compute the 2D cross product (z-component of 3D cross)
	static float Cross2D(const glm::vec2& a, const glm::vec2& b)
	{
		return a.x * b.y - a.y * b.x;
	}

	// Helper: compute perpendicular vector (90 degrees CCW rotation)
	static glm::vec2 Perpendicular(const glm::vec2& v)
	{
		return glm::vec2(-v.y, v.x);
	}

	void BuildShadowVolumeTriangles(const Light2D& light,
		const ShadowCaster2D& caster,
		std::vector<glm::vec2>& outTriangleVertices)
	{
		const auto& pts = caster.WorldPoints;
		if (pts.size() < 2)
			return;

		if ((light.LayerMask & caster.LayerMask) == 0)
			return;

		const size_t edgeCount = caster.Closed ? pts.size() : (pts.size() - 1);
		if (edgeCount == 0)
			return;

		// If the polygon is closed and one-sided, fix mixed winding by flipping outward normals for CW shapes.
		bool flipOutwardNormals = false;
		if (caster.Closed && !caster.TwoSided)
		{
			const float area = SignedAreaClosedPolygon(pts);
			flipOutwardNormals = (area < 0.0f);
		}

		constexpr float kShadowExtrudeScale = 1.1f;
		const float extrudeDist = light.Radius * kShadowExtrudeScale;

		// Pre-compute which edges face the light
		std::vector<bool> edgeFacesLight(edgeCount, false);
		for (size_t i = 0; i < edgeCount; ++i)
		{
			glm::vec2 p0 = pts[i];
			glm::vec2 p1 = pts[(i + 1) % pts.size()];

			glm::vec2 edge = p1 - p0;
			glm::vec2 edgeDir = SafeNormalize(edge);
			if (edgeDir == glm::vec2(0.0f))
				continue;

			// CCW polygon outward normal is the right-hand normal.
			glm::vec2 outwardNormal = glm::vec2(edgeDir.y, -edgeDir.x);
			if (flipOutwardNormals)
				outwardNormal = -outwardNormal;

			if (caster.TwoSided)
			{
				edgeFacesLight[i] = true;
			}
			else
			{
				glm::vec2 toLight = light.Position - p0;
				edgeFacesLight[i] = (glm::dot(outwardNormal, toLight) > 0.0f);
			}
		}

		// For each edge that faces light, generate shadow geometry.
		// The key insight is that when the light is close to a long edge, the simple quad
		// approach creates a "bowtie" shape that doesn't properly shadow.
		// Instead, we generate a proper shadow wedge using a triangle fan from the light.
		for (size_t i = 0; i < edgeCount; ++i)
		{
			if (!edgeFacesLight[i])
				continue;

			glm::vec2 p0 = pts[i];
			glm::vec2 p1 = pts[(i + 1) % pts.size()];

			glm::vec2 toP0 = p0 - light.Position;
			glm::vec2 toP1 = p1 - light.Position;
			
			float lenP0 = std::sqrt(LengthSquared(toP0));
			float lenP1 = std::sqrt(LengthSquared(toP1));
			
			if (lenP0 < 1e-6f || lenP1 < 1e-6f)
				continue;

			glm::vec2 dir0 = toP0 / lenP0;
			glm::vec2 dir1 = toP1 / lenP1;

			// Check if this is a "divergent" case where the quad would self-intersect.
			// This happens when the dot product of the two directions is negative
			// (they point in roughly opposite directions from the light).
			float dirDot = glm::dot(dir0, dir1);
			
			glm::vec2 p0e = p0 + dir0 * extrudeDist;
			glm::vec2 p1e = p1 + dir1 * extrudeDist;

			if (dirDot >= 0.0f)
			{
				// Normal case: directions are not diverging too much.
				// Generate a simple quad (2 triangles).
				// But we still need to check for self-intersection of the quad.
				
				// Check if the quad is valid (not self-intersecting).
				// A quad p0->p1->p1e->p0e is valid if the edges don't cross.
				// We check if p0e is on the correct side of the line p0-p1.
				glm::vec2 edge = p1 - p0;
				glm::vec2 toP0e = p0e - p0;
				glm::vec2 toP1e = p1e - p0;
				
				float cross0 = Cross2D(edge, toP0e);
				float cross1 = Cross2D(edge, toP1e);
				
				// Both extruded points should be on the same side of the edge (negative cross = behind edge)
				// If they're on opposite sides, the quad is invalid.
				if (cross0 * cross1 >= 0.0f)
				{
					// Valid quad - use simple triangulation
					// Tri 1: p0, p1, p1e
					outTriangleVertices.push_back(p0);
					outTriangleVertices.push_back(p1);
					outTriangleVertices.push_back(p1e);

					// Tri 2: p0, p1e, p0e
					outTriangleVertices.push_back(p0);
					outTriangleVertices.push_back(p1e);
					outTriangleVertices.push_back(p0e);
				}
				else
				{
					// Invalid quad - use triangle fan from light position
					// This covers the entire wedge of shadow behind the edge.
					
					// Triangle 1: light -> p0 -> p0e (covers shadow behind p0)
					outTriangleVertices.push_back(light.Position);
					outTriangleVertices.push_back(p0);
					outTriangleVertices.push_back(p0e);
					
					// Triangle 2: light -> p1 -> p1e (covers shadow behind p1)
					outTriangleVertices.push_back(light.Position);
					outTriangleVertices.push_back(p1);
					outTriangleVertices.push_back(p1e);
					
					// Triangle 3: light -> p0 -> p1 (covers the edge itself)
					outTriangleVertices.push_back(light.Position);
					outTriangleVertices.push_back(p0);
					outTriangleVertices.push_back(p1);
					
					// We also need triangles to fill the entire shadow region.
					// Add triangles from the edge endpoints to the extruded endpoints.
					outTriangleVertices.push_back(p0);
					outTriangleVertices.push_back(p1);
					outTriangleVertices.push_back(p0e);
					
					outTriangleVertices.push_back(p1);
					outTriangleVertices.push_back(p1e);
					outTriangleVertices.push_back(p0e);
				}
			}
			else
			{
				// Divergent case: light is close and the edge spans a wide angle from light's POV.
				// The simple quad approach fails here. Instead, we need to:
				// 1. Generate a proper shadow "wedge" that covers the entire angular span
				// 2. Use a triangle fan approach from the edge
				
				// For a divergent edge, the shadow should cover everything "behind" the edge
				// from the light's perspective. We generate this as multiple triangles.
				
				// Compute the perpendicular direction from edge pointing away from light
				glm::vec2 edgeDir = SafeNormalize(p1 - p0);
				glm::vec2 edgeNormal = glm::vec2(edgeDir.y, -edgeDir.x);
				
				// Make sure normal points away from light
				glm::vec2 edgeMidpoint = (p0 + p1) * 0.5f;
				glm::vec2 lightToMid = edgeMidpoint - light.Position;
				if (glm::dot(edgeNormal, lightToMid) < 0.0f)
					edgeNormal = -edgeNormal;
				
				// Extend the edge endpoints in the perpendicular direction
				glm::vec2 p0n = p0 + edgeNormal * extrudeDist;
				glm::vec2 p1n = p1 + edgeNormal * extrudeDist;
				
				// Also extend along the radial directions for complete coverage
				// Triangle fan from edge:
				
				// Quad behind the edge (perpendicular extrusion)
				outTriangleVertices.push_back(p0);
				outTriangleVertices.push_back(p1);
				outTriangleVertices.push_back(p1n);
				
				outTriangleVertices.push_back(p0);
				outTriangleVertices.push_back(p1n);
				outTriangleVertices.push_back(p0n);
				
				// Triangles extending radially from each endpoint
				// These cover the "wings" of the shadow beyond the perpendicular quad
				
				// For p0: triangle from p0 to p0n to p0e
				outTriangleVertices.push_back(p0);
				outTriangleVertices.push_back(p0n);
				outTriangleVertices.push_back(p0e);
				
				// For p1: triangle from p1 to p1e to p1n
				outTriangleVertices.push_back(p1);
				outTriangleVertices.push_back(p1e);
				outTriangleVertices.push_back(p1n);
				
				// Additional coverage: connect the perpendicular and radial extrusions
				outTriangleVertices.push_back(p0n);
				outTriangleVertices.push_back(p1n);
				outTriangleVertices.push_back(p0e);
				
				outTriangleVertices.push_back(p1n);
				outTriangleVertices.push_back(p1e);
				outTriangleVertices.push_back(p0e);
			}
		}

		// Generate shadow fins at silhouette boundary vertices.
		// A silhouette vertex is where a front-facing edge meets a back-facing edge.
		for (size_t i = 0; i < edgeCount; ++i)
		{
			size_t prevEdge = (i == 0) ? (edgeCount - 1) : (i - 1);
			
			if (!caster.Closed && i == 0)
				continue;

			bool prevFaces = edgeFacesLight[prevEdge];
			bool currFaces = edgeFacesLight[i];

			// Only add fins at silhouette boundaries (one edge faces light, other doesn't)
			if (prevFaces == currFaces)
				continue;

			glm::vec2 vertex = pts[i];
			glm::vec2 dirToVertex = SafeNormalize(vertex - light.Position);
			if (dirToVertex == glm::vec2(0.0f))
				continue;

			glm::vec2 vertexExtruded = vertex + dirToVertex * extrudeDist;

			// Find the adjacent vertex on the front-facing edge
			glm::vec2 adjacentVertex;
			if (prevFaces)
			{
				adjacentVertex = pts[prevEdge];
			}
			else
			{
				adjacentVertex = pts[(i + 1) % pts.size()];
			}

			glm::vec2 dirToAdjacent = SafeNormalize(adjacentVertex - light.Position);
			if (dirToAdjacent == glm::vec2(0.0f))
				continue;

			glm::vec2 adjacentExtruded = adjacentVertex + dirToAdjacent * extrudeDist;

			// Create fin triangle
			float cross = Cross2D(vertexExtruded - vertex, adjacentExtruded - vertex);
			if (cross > 0.0f)
			{
				outTriangleVertices.push_back(vertex);
				outTriangleVertices.push_back(vertexExtruded);
				outTriangleVertices.push_back(adjacentExtruded);
			}
			else
			{
				outTriangleVertices.push_back(vertex);
				outTriangleVertices.push_back(adjacentExtruded);
				outTriangleVertices.push_back(vertexExtruded);
			}
		}
	}
}
