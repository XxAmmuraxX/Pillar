#pragma once

#include "ObjectPool.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"
#include "Pillar/ECS/Components/Gameplay/BulletComponent.h"

namespace Pillar {

/**
 * @brief Specialized object pool for bullet entities
 * 
 * This pool manages bullet entities with all required components pre-attached.
 * Bullets are light entities that use raycasting for collision detection.
 * 
 * Note: Rendering components will be added when rendering system is complete.
 */
class PIL_API BulletPool
{
public:
	BulletPool() = default;
	~BulletPool() = default;

	/**
	 * @brief Initialize the bullet pool
	 * @param scene The scene that will own the bullets
	 * @param initialCapacity Number of bullets to pre-allocate (default: 200)
	 */
	void Init(Scene* scene, uint32_t initialCapacity = 200);

	/**
	 * @brief Spawn a bullet from the pool
	 * @param position Starting position of the bullet
	 * @param direction Direction vector (will be normalized)
	 * @param speed Speed in units per second
	 * @param owner Entity that fired the bullet
	 * @param damage Damage dealt on hit
	 * @param lifetime How long bullet lives before auto-destroy (seconds)
	 * @return The spawned bullet entity
	 */
	Entity SpawnBullet(
		const glm::vec2& position,
		const glm::vec2& direction,
		float speed,
		Entity owner,
		float damage = 25.0f,
		float lifetime = 5.0f
	);

	/**
	 * @brief Return a bullet to the pool
	 * @param bullet The bullet entity to return
	 */
	void ReturnBullet(Entity bullet);

	/**
	 * @brief Get pool statistics
	 */
	size_t GetAvailableCount() const { return m_Pool.GetAvailableCount(); }
	size_t GetActiveCount() const { return m_Pool.GetActiveCount(); }
	size_t GetTotalCount() const { return m_Pool.GetTotalCount(); }

	/**
	 * @brief Clear all bullets
	 */
	void Clear() { m_Pool.Clear(); }

private:
	ObjectPool m_Pool;
	Scene* m_Scene = nullptr;
};

/**
 * @brief Specialized object pool for particle entities
 * 
 * Particles are purely visual light entities with simple physics.
 * 
 * Note: Rendering components will be added when rendering system is complete.
 */
class PIL_API ParticlePool
{
public:
	ParticlePool() = default;
	~ParticlePool() = default;

	/**
	 * @brief Initialize the particle pool
	 * @param scene The scene that will owner the particles
	 * @param initialCapacity Number of particles to pre-allocate (default: 1000)
	 */
	void Init(Scene* scene, uint32_t initialCapacity = 1000);

	/**
	 * @brief Spawn a particle from the pool
	 * @param position Starting position
	 * @param velocity Initial velocity
	 * @param color Particle color (currently not used, for future rendering)
	 * @param size Particle size
	 * @param lifetime How long particle lives (seconds)
	 * @return The spawned particle entity
	 */
	Entity SpawnParticle(
		const glm::vec2& position,
		const glm::vec2& velocity,
		const glm::vec4& color = glm::vec4(1.0f),
		float size = 0.1f,
		float lifetime = 1.0f
	);

	/**
	 * @brief Return a particle to the pool
	 * @param particle The particle entity to return
	 */
	void ReturnParticle(Entity particle);

	/**
	 * @brief Get pool statistics
	 */
	size_t GetAvailableCount() const { return m_Pool.GetAvailableCount(); }
	size_t GetActiveCount() const { return m_Pool.GetActiveCount(); }
	size_t GetTotalCount() const { return m_Pool.GetTotalCount(); }

	/**
	 * @brief Clear all particles
	 */
	void Clear() { m_Pool.Clear(); }

private:
	ObjectPool m_Pool;
	Scene* m_Scene = nullptr;
};

} // namespace Pillar
