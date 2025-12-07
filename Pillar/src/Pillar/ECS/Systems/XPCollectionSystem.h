#pragma once

#include "Pillar/Core.h"
#include "System.h"
#include "Pillar/ECS/Physics/SpatialHashGrid.h"
#include <memory>

namespace Pillar {

	// Uses Spatial Hash Grid for fast AABB checks
	// Finds XP gems near player, applies attraction
	class PIL_API XPCollectionSystem : public System
	{
	public:
		XPCollectionSystem(float cellSize = 2.0f);

		void OnUpdate(float deltaTime) override;

		// Get spatial grid statistics
		size_t GetEntityCount() const { return m_SpatialGrid->GetEntityCount(); }
		size_t GetBucketCount() const { return m_SpatialGrid->GetBucketCount(); }

	private:
		std::unique_ptr<SpatialHashGrid> m_SpatialGrid;

		void UpdateSpatialGrid();
		void ProcessGemAttraction(float deltaTime);
	};

} // namespace Pillar
