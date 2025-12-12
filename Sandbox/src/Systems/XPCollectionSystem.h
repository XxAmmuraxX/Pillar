#pragma once

#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/Audio/AudioClip.h"
#include <glm/glm.hpp>

namespace ArenaProtocol {

	/**
	 * @brief System for handling XP gem behavior and collection
	 * Part of Arena Protocol showcase
	 */
	class XPCollectionSystem
	{
	public:
		XPCollectionSystem() = default;
		~XPCollectionSystem() = default;

		void OnUpdate(Pillar::Scene* scene, Pillar::Entity player, float deltaTime);
		void SetPickupSFX(const std::shared_ptr<Pillar::AudioClip>& clip) { m_PickupSFX = clip; }

	private:
		void UpdateGemVisuals(Pillar::Scene* scene, float deltaTime);
		void UpdateGemAttraction(Pillar::Scene* scene, Pillar::Entity player, float deltaTime);
		void CollectGems(Pillar::Scene* scene, Pillar::Entity player);

		float m_CollectionRadius = 0.5f;
		std::shared_ptr<Pillar::AudioClip> m_PickupSFX;
	};

} // namespace ArenaProtocol
