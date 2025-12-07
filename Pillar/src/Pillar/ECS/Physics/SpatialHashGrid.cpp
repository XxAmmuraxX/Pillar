#include "SpatialHashGrid.h"
#include <cmath>
#include <algorithm>

namespace Pillar {

	SpatialHashGrid::SpatialHashGrid(float cellSize)
		: m_CellSize(cellSize), m_EntityCount(0)
	{
	}

	void SpatialHashGrid::Insert(uint32_t entityId, const glm::vec2& position)
	{
		auto cellCoords = GetCellCoords(position);
		m_Grid[cellCoords].push_back(entityId);
		m_EntityCount++;
	}

	void SpatialHashGrid::Remove(uint32_t entityId, const glm::vec2& position)
	{
		auto cellCoords = GetCellCoords(position);
		auto it = m_Grid.find(cellCoords);
		if (it != m_Grid.end())
		{
			auto& entities = it->second;
			auto entityIt = std::find(entities.begin(), entities.end(), entityId);
			if (entityIt != entities.end())
			{
				entities.erase(entityIt);
				m_EntityCount--;

				// Remove empty buckets to save memory
				if (entities.empty())
				{
					m_Grid.erase(it);
				}
			}
		}
	}

	std::vector<uint32_t> SpatialHashGrid::Query(const glm::vec2& position, float radius) const
	{
		std::vector<uint32_t> results;
		std::vector<std::pair<int32_t, int32_t>> cells;

		GetCellsInRadius(position, radius, cells);

		// Collect all entities in cells
		for (const auto& cell : cells)
		{
			auto it = m_Grid.find(cell);
			if (it != m_Grid.end())
			{
				const auto& entities = it->second;
				results.insert(results.end(), entities.begin(), entities.end());
			}
		}

		return results;
	}

	std::vector<uint32_t> SpatialHashGrid::QueryAABB(const glm::vec2& min, const glm::vec2& max) const
	{
		std::vector<uint32_t> results;
		std::vector<std::pair<int32_t, int32_t>> cells;

		GetCellsInAABB(min, max, cells);

		// Collect all entities in cells
		for (const auto& cell : cells)
		{
			auto it = m_Grid.find(cell);
			if (it != m_Grid.end())
			{
				const auto& entities = it->second;
				results.insert(results.end(), entities.begin(), entities.end());
			}
		}

		return results;
	}

	void SpatialHashGrid::Clear()
	{
		m_Grid.clear();
		m_EntityCount = 0;
	}

	std::pair<int32_t, int32_t> SpatialHashGrid::GetCellCoords(const glm::vec2& position) const
	{
		int32_t x = static_cast<int32_t>(std::floor(position.x / m_CellSize));
		int32_t y = static_cast<int32_t>(std::floor(position.y / m_CellSize));
		return { x, y };
	}

	void SpatialHashGrid::GetCellsInRadius(const glm::vec2& position, float radius, std::vector<std::pair<int32_t, int32_t>>& cells) const
	{
		// Calculate AABB around the circle
		glm::vec2 min = position - glm::vec2(radius);
		glm::vec2 max = position + glm::vec2(radius);

		GetCellsInAABB(min, max, cells);
	}

	void SpatialHashGrid::GetCellsInAABB(const glm::vec2& min, const glm::vec2& max, std::vector<std::pair<int32_t, int32_t>>& cells) const
	{
		auto minCell = GetCellCoords(min);
		auto maxCell = GetCellCoords(max);

		// Iterate through all cells in the AABB
		for (int32_t x = minCell.first; x <= maxCell.first; ++x)
		{
			for (int32_t y = minCell.second; y <= maxCell.second; ++y)
			{
				cells.push_back({ x, y });
			}
		}
	}

} // namespace Pillar
