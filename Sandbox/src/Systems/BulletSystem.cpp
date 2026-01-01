#include "BulletSystem.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Rendering/SpriteComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"
#include "Pillar/ECS/Components/Gameplay/BulletComponent.h"
#include "../Components/HealthComponent.h"
#include "../Components/PlayerStatsComponent.h"
#include "../Components/ObstacleComponent.h"
#include "Pillar/Logger.h"

namespace Pillar {

	void BulletSystem::OnUpdate(Scene* scene, ObjectPool* bulletPool, float deltaTime)
	{
		if (!scene || !bulletPool) return;

		auto view = scene->GetRegistry().view<BulletComponent, TransformComponent, VelocityComponent>();

		for (auto entity : view)
		{
			Entity bullet(entity, scene);

			// Skip bullets in pool
			if (bulletPool->IsInPool(bullet))
				continue;

			auto& bulletComp = view.get<BulletComponent>(entity);
			auto& transform = view.get<TransformComponent>(entity);
			auto& velocity = view.get<VelocityComponent>(entity);

			// Update lifetime
			bulletComp.TimeAlive += deltaTime;

			// Apply velocity
			transform.Position.x += velocity.Velocity.x * deltaTime;
			transform.Position.y += velocity.Velocity.y * deltaTime;
			transform.Dirty = true;

			// Check if bullet should despawn (lifetime)
			if (bulletComp.TimeAlive >= bulletComp.Lifetime)
			{
				if (bullet.HasComponent<SpriteComponent>())
				{
					bullet.GetComponent<SpriteComponent>().Color.a = 0.0f;
				}
				bulletPool->Release(bullet);
			}
		}
	}

	bool BulletSystem::CheckBulletEnemyCollision(Entity bullet, Scene* scene, Entity player,
												 std::vector<Entity>& enemies)
	{
		if (!bullet.IsValid() || !bullet.HasComponent<BulletComponent>()) return false;

		auto& bulletComp = bullet.GetComponent<BulletComponent>();
		auto& bulletTransform = bullet.GetComponent<TransformComponent>();

		// Only check if this is a player bullet
		if (bulletComp.Owner != player) return false;

		glm::vec2 bulletPos = { bulletTransform.Position.x, bulletTransform.Position.y };

		for (auto& enemy : enemies)
		{
			if (!enemy.IsValid()) continue;
			if (!enemy.HasComponent<HealthComponent>()) continue;

			auto& enemyTransform = enemy.GetComponent<TransformComponent>();
			auto& health = enemy.GetComponent<HealthComponent>();

			if (health.IsDead) continue;

			glm::vec2 enemyPos = { enemyTransform.Position.x, enemyTransform.Position.y };
			float distance = glm::length(bulletPos - enemyPos);

			if (distance < (m_BulletRadius + m_EnemyRadius))
			{
				// Hit!
				health.TakeDamage(bulletComp.Damage);
				bulletComp.HitsRemaining--;
				
				PIL_TRACE("Enemy hit! Health: {:.0f}", health.Health);

				// Check if bullet should be destroyed
				if (!bulletComp.Pierce || bulletComp.HitsRemaining <= 0)
				{
					return true;
				}
			}
		}

		return false;
	}

	bool BulletSystem::CheckBulletPlayerCollision(Entity bullet, Entity player)
	{
		if (!bullet.IsValid() || !player.IsValid()) return false;
		if (!bullet.HasComponent<BulletComponent>()) return false;

		auto& bulletComp = bullet.GetComponent<BulletComponent>();

		// Only check enemy bullets
		if (bulletComp.Owner == player) return false;

		auto& bulletTransform = bullet.GetComponent<TransformComponent>();
		auto& playerTransform = player.GetComponent<TransformComponent>();

		glm::vec2 bulletPos = { bulletTransform.Position.x, bulletTransform.Position.y };
		glm::vec2 playerPos = { playerTransform.Position.x, playerTransform.Position.y };
		float distance = glm::length(bulletPos - playerPos);

		if (distance < (m_BulletRadius + m_PlayerRadius))
		{
			// Hit player!
			if (player.HasComponent<PlayerStatsComponent>())
			{
				auto& stats = player.GetComponent<PlayerStatsComponent>();
				stats.Health -= bulletComp.Damage;
				
				if (stats.Health <= 0.0f)
				{
					stats.Health = 0.0f;
					PIL_WARN("Player defeated!");
				}
				else
				{
					PIL_TRACE("Player hit! Health: {:.0f}", stats.Health);
				}
			}
			return true;
		}

		return false;
	}

	bool BulletSystem::CheckBulletObstacleCollision(Entity bullet, Scene* scene)
	{
		if (!bullet.IsValid() || !scene) return false;

		auto& bulletTransform = bullet.GetComponent<TransformComponent>();
		glm::vec2 bulletPos = { bulletTransform.Position.x, bulletTransform.Position.y };

		auto view = scene->GetRegistry().view<TransformComponent, ObstacleComponent>();

		for (auto entity : view)
		{
			auto& obstacle = view.get<ObstacleComponent>(entity);
			if (!obstacle.BlocksBullets) continue;

			auto& obstacleTransform = view.get<TransformComponent>(entity);
			glm::vec2 obstaclePos = { obstacleTransform.Position.x, obstacleTransform.Position.y };
			glm::vec2 obstacleSize = obstacleTransform.Scale;

			// Simple AABB collision for obstacles
			float halfWidth = obstacleSize.x * 0.5f;
			float halfHeight = obstacleSize.y * 0.5f;

			if (bulletPos.x > obstaclePos.x - halfWidth &&
				bulletPos.x < obstaclePos.x + halfWidth &&
				bulletPos.y > obstaclePos.y - halfHeight &&
				bulletPos.y < obstaclePos.y + halfHeight)
			{
				// Hit obstacle
				if (obstacle.IsDestructible)
				{
					obstacle.Health -= 10.0f; // Bullet damage to obstacle
				}
				return true;
			}
		}

		return false;
	}

} // namespace Pillar
