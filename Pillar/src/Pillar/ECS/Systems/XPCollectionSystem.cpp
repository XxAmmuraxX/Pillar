#include "XPCollectionSystem.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Core/TagComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"
#include "Pillar/ECS/Components/Gameplay/XPGemComponent.h"
#include "Pillar/Logger.h"
#include <glm/glm.hpp>

namespace Pillar {

	XPCollectionSystem::XPCollectionSystem(float cellSize)
		: m_SpatialGrid(std::make_unique<SpatialHashGrid>(cellSize))
	{
	}

	void XPCollectionSystem::OnUpdate(float deltaTime)
	{
		// Rebuild spatial grid every frame (simple approach, fast enough for 10k entities)
		UpdateSpatialGrid();

		// Process gem attraction toward player
		ProcessGemAttraction(deltaTime);
	}

	void XPCollectionSystem::UpdateSpatialGrid()
	{
		// Clear previous frame's data
		m_SpatialGrid->Clear();

		// Insert all XP gems into spatial grid
		auto view = m_Scene->GetRegistry().view<TransformComponent, XPGemComponent>();
		for (auto entity : view)
		{
			auto& transform = view.get<TransformComponent>(entity);
			uint32_t entityId = static_cast<uint32_t>(entity);
			m_SpatialGrid->Insert(entityId, transform.Position);
		}
	}

	void XPCollectionSystem::ProcessGemAttraction(float deltaTime)
	{
		// Find player entity (assuming tagged "Player")
		auto playerView = m_Scene->GetRegistry().view<TagComponent, TransformComponent>();
		
		glm::vec2 playerPos(0, 0);
		bool playerFound = false;

		for (auto entity : playerView)
		{
			auto& tag = playerView.get<TagComponent>(entity);
			if (tag.Tag == "Player")
			{
				auto& transform = playerView.get<TransformComponent>(entity);
				playerPos = transform.Position;
				playerFound = true;
				break;
			}
		}

		if (!playerFound)
			return;

		// Process each XP gem
		auto gemView = m_Scene->GetRegistry().view<TransformComponent, VelocityComponent, XPGemComponent>();
		
		for (auto entity : gemView)
		{
			auto& transform = gemView.get<TransformComponent>(entity);
			auto& velocity = gemView.get<VelocityComponent>(entity);
			auto& gem = gemView.get<XPGemComponent>(entity);

			// Calculate distance to player
			glm::vec2 toPlayer = playerPos - transform.Position;
			float distance = glm::length(toPlayer);

			// Check if within attraction radius
			if (distance < gem.AttractionRadius)
			{
				gem.IsAttracted = true;

				// Move toward player
				if (distance > 0.01f) // Avoid division by zero
				{
					glm::vec2 direction = toPlayer / distance;
					velocity.Velocity = direction * gem.MoveSpeed;
				}

				// Check if collected (very close to player)
				if (distance < 0.5f)
				{
					// TODO: Add XP to player (Phase 6: Health/Stats System)
					// For now, just log and destroy
					PIL_CORE_TRACE("XP Gem collected! Value: {}", gem.XPValue);
					
					Entity e(entity, m_Scene);
					m_Scene->DestroyEntity(e);
				}
			}
			else
			{
				gem.IsAttracted = false;
				// Gems that aren't attracted can drift slowly or be stationary
				velocity.Velocity = glm::vec2(0, 0);
			}
		}
	}

} // namespace Pillar
