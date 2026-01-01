#pragma once

#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/Renderer/Texture.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>

namespace Pillar {

	/**
	 * @brief Factory for creating game entities with proper textures and components
	 * Part of Arena Protocol showcase
	 */
	class EntityFactory
	{
	public:
		EntityFactory() = default;
		~EntityFactory() = default;

		void Initialize(Scene* scene, const std::string& assetsPath);
		
		// Texture management
		void LoadTextures(const std::string& basePath);
		std::shared_ptr<Texture2D> GetTexture(const std::string& name) const;

		// Entity creation
		Entity CreatePlayer(const glm::vec2& position);
		Entity CreateBullet(const glm::vec2& position, const glm::vec2& direction, Entity owner, bool isPlayerBullet);
		Entity CreateDrone(const glm::vec2& position, Entity target);
		Entity CreateTurret(const glm::vec2& position);
		Entity CreateCharger(const glm::vec2& position);
		Entity CreateBoss(const glm::vec2& position);
		Entity CreateXPGem(const glm::vec2& position, int xpValue);
		Entity CreateObstacle(const glm::vec2& position, const std::string& type, const glm::vec2& size);
		Entity CreateTriggerZone(const glm::vec2& position, const glm::vec2& size, const std::string& eventName);
		Entity CreateExplosion(const glm::vec2& position, float scale = 1.0f);

		// Texture getters for external use
		std::shared_ptr<Texture2D> GetPlayerTexture() const { return GetTexture("player"); }
		std::shared_ptr<Texture2D> GetBulletTexture() const { return GetTexture("bullet"); }
		std::shared_ptr<Texture2D> GetEnemyBulletTexture() const { return GetTexture("enemy_bullet"); }
		std::shared_ptr<Texture2D> GetDroneTexture() const { return GetTexture("drone"); }
		std::shared_ptr<Texture2D> GetTurretTexture() const { return GetTexture("turret"); }
		std::shared_ptr<Texture2D> GetChargerTexture() const { return GetTexture("charger"); }
		std::shared_ptr<Texture2D> GetBossTexture() const { return GetTexture("boss"); }

	private:
		Scene* m_Scene = nullptr;
		std::string m_AssetsPath;
		std::unordered_map<std::string, std::shared_ptr<Texture2D>> m_Textures;

		std::shared_ptr<Texture2D> LoadOrCreateTexture(const std::string& name, const std::string& filename, uint32_t fallbackColor);
	};

} // namespace Pillar
