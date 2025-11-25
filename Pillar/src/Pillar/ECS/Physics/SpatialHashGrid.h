#pragma once

#include "Pillar/Core.h"
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace Pillar {

	// Spatial hash grid for fast AABB broad-phase collision detection
	// Used for Light Entities (XP gems, particles, etc.)
	// O(1) insert, O(k) query where k = nearby entities
	class PIL_API SpatialHashGrid
	{
	public:
		SpatialHashGrid(float cellSize = 2.0f);

		// Insert an entity at a position
		void Insert(uint32_t entityId, const glm::vec2& position);

		// Remove an entity from the grid
		void Remove(uint32_t entityId, const glm::vec2& position);

		// Query entities within a radius around a point
		std::vector<uint32_t> Query(const glm::vec2& position, float radius) const;

		// Query entities within an AABB
		std::vector<uint32_t> QueryAABB(const glm::vec2& min, const glm::vec2& max) const;

		// Clear all entities from the grid
		void Clear();

		// Get statistics
		size_t GetEntityCount() const { return m_EntityCount; }
		size_t GetBucketCount() const { return m_Grid.size(); }

	private:
		float m_CellSize;
		size_t m_EntityCount;

		// Hash function for grid cells
		struct CellHash
		{
			size_t operator()(const std::pair<int32_t, int32_t>& cell) const
			{
				// Simple hash combining x and y coordinates
				constexpr uint64_t prime = 0x9e3779b97f4a7c15; // Large prime for better distribution
				uint64_t hash = cell.first * prime;
				hash ^= cell.second * prime + 0x517cc1b727220a95;
				return static_cast<size_t>(hash);
			}
		};

		// Grid storage: cell coordinate -> list of entity IDs
		std::unordered_map<std::pair<int32_t, int32_t>, std::vector<uint32_t>, CellHash> m_Grid;

		// Helper functions
		std::pair<int32_t, int32_t> GetCellCoords(const glm::vec2& position) const;
		void GetCellsInRadius(const glm::vec2& position, float radius, std::vector<std::pair<int32_t, int32_t>>& cells) const;
		void GetCellsInAABB(const glm::vec2& min, const glm::vec2& max, std::vector<std::pair<int32_t, int32_t>>& cells) const;
	};

} // namespace Pillar
