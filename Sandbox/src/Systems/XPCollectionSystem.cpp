#include "XPCollectionSystem.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"
#include "Pillar/ECS/Components/Gameplay/XPGemComponent.h"
#include "../Components/PlayerStatsComponent.h"
#include "../Components/XPGemVisualComponent.h"
#include "Pillar/Logger.h"
#include <vector>
#include <cmath>

using namespace Pillar;

namespace ArenaProtocol {

	void XPCollectionSystem::OnUpdate(Scene* scene, Entity player, float deltaTime)
	{
		if (!scene || !player.IsValid()) return;

		UpdateGemVisuals(scene, deltaTime);
		UpdateGemAttraction(scene, player, deltaTime);
		CollectGems(scene, player);
	}

	void XPCollectionSystem::UpdateGemVisuals(Scene* scene, float deltaTime)
	{
		auto view = scene->GetRegistry().view<TransformComponent, XPGemVisualComponent>();

		for (auto entity : view)
		{
			auto& transform = view.get<TransformComponent>(entity);
			auto& visual = view.get<XPGemVisualComponent>(entity);

			// Update bobbing animation
			visual.BobTimer += deltaTime * visual.BobSpeed;
			float bobOffset = sinf(visual.BobTimer) * visual.BobAmplitude;
			
			// Apply bob offset to position (only Y axis)
			// Note: We need the base position stored to avoid drift
			transform.Position.y = visual.BasePosition.y + bobOffset;

			// Update rotation
			transform.Rotation += glm::radians(visual.RotationSpeed) * deltaTime;
			transform.Dirty = true;
		}
	}

	void XPCollectionSystem::UpdateGemAttraction(Scene* scene, Entity player, float deltaTime)
	{
		auto& playerTransform = player.GetComponent<TransformComponent>();
		glm::vec2 playerPos = { playerTransform.Position.x, playerTransform.Position.y };

		auto view = scene->GetRegistry().view<TransformComponent, XPGemComponent, VelocityComponent>();

		for (auto entity : view)
		{
			auto& transform = view.get<TransformComponent>(entity);
			auto& gem = view.get<XPGemComponent>(entity);
			auto& velocity = view.get<VelocityComponent>(entity);

			glm::vec2 gemPos = { transform.Position.x, transform.Position.y };
			float distance = glm::length(playerPos - gemPos);

			if (distance < gem.AttractionRadius && distance > m_CollectionRadius)
			{
				// Attract toward player
				gem.IsAttracted = true;
				glm::vec2 direction = glm::normalize(playerPos - gemPos);
				
				// Accelerate toward player
				float speedMultiplier = 1.0f + (gem.AttractionRadius - distance) / gem.AttractionRadius;
				velocity.Velocity = direction * gem.MoveSpeed * speedMultiplier;
				
				// Apply velocity
				transform.Position.x += velocity.Velocity.x * deltaTime;
				transform.Position.y += velocity.Velocity.y * deltaTime;
				
				// Update base position for visual component if present
				Entity gemEntity(entity, scene);
				if (gemEntity.HasComponent<XPGemVisualComponent>())
				{
					auto& visual = gemEntity.GetComponent<XPGemVisualComponent>();
					visual.BasePosition = { transform.Position.x, transform.Position.y };
				}
				
				transform.Dirty = true;
			}
			else if (!gem.IsAttracted)
			{
				velocity.Velocity = glm::vec2(0.0f);
			}
		}
	}

	void XPCollectionSystem::CollectGems(Scene* scene, Entity player)
	{
		auto& playerTransform = player.GetComponent<TransformComponent>();
		auto& playerStats = player.GetComponent<PlayerStatsComponent>();
		glm::vec2 playerPos = { playerTransform.Position.x, playerTransform.Position.y };

		std::vector<Entity> gemsToCollect;

		auto view = scene->GetRegistry().view<TransformComponent, XPGemComponent>();

		for (auto entity : view)
		{
			auto& transform = view.get<TransformComponent>(entity);
			auto& gem = view.get<XPGemComponent>(entity);

			glm::vec2 gemPos = { transform.Position.x, transform.Position.y };
			float distance = glm::length(playerPos - gemPos);

			if (distance < m_CollectionRadius)
			{
				gemsToCollect.push_back(Entity(entity, scene));
			}
		}

		// Collect gems and destroy entities
		for (auto& gem : gemsToCollect)
		{
			if (!gem.IsValid()) continue;

			auto& gemComp = gem.GetComponent<XPGemComponent>();
			playerStats.XP += gemComp.XPValue;

			if (m_PickupSFX)
				m_PickupSFX->Play();

			// Check for level up
			while (playerStats.XP >= playerStats.XPToNextLevel)
			{
				playerStats.XP -= playerStats.XPToNextLevel;
				playerStats.Level++;
				playerStats.XPToNextLevel = (int)(100 * powf(1.5f, (float)(playerStats.Level - 1)));
				PIL_INFO("Player leveled up to Level {}!", playerStats.Level);
				
				// Could trigger powerup selection here
			}

			PIL_TRACE("Collected {} XP (Total: {})", gemComp.XPValue, playerStats.XP);
			scene->DestroyEntity(gem);
		}
	}

} // namespace ArenaProtocol
