#pragma once

#include "Pillar/Core.h"
#include "Entity.h"
#include <vector>
#include <functional>

namespace Pillar {

/**
 * @brief Generic object pool for reusing entities
 * 
 * This pool manages a collection of inactive entities that can be reused
 * instead of constantly creating and destroying them. This significantly
 * reduces memory allocations and improves performance.
 * 
 * Usage:
 * @code
 * ObjectPool bulletPool;
 * bulletPool.Init(scene, 100);  // Pre-allocate 100 bullets
 * 
 * // Get entity from pool
 * Entity bullet = bulletPool.Acquire();
 * 
 * // When done, return to pool
 * bulletPool.Release(bullet);
 * @endcode
 */
class PIL_API ObjectPool
{
public:
	ObjectPool() = default;
	~ObjectPool() = default;

	/**
	 * @brief Initialize the pool with a scene and capacity
	 * @param scene The scene that will own the pooled entities
	 * @param initialCapacity Number of entities to pre-allocate
	 */
	void Init(Scene* scene, uint32_t initialCapacity = 100);

	/**
	 * @brief Get an entity from the pool
	 * @return An available entity from the pool, or a new one if pool is empty
	 */
	Entity Acquire();

	/**
	 * @brief Return an entity to the pool for reuse
	 * @param entity The entity to return to the pool
	 */
	void Release(Entity entity);

	/**
	 * @brief Check if an entity is currently in the pool (inactive)
	 * @param entity The entity to check
	 * @return True if entity is in the pool, false if it's active
	 */
	bool IsInPool(Entity entity) const;

	/**
	 * @brief Get the number of available entities in the pool
	 * @return Number of inactive entities ready for reuse
	 */
	size_t GetAvailableCount() const { return m_AvailableEntities.size(); }

	/**
	 * @brief Get the total number of entities managed by this pool
	 * @return Total entities (active + inactive)
	 */
	size_t GetTotalCount() const { return m_TotalEntities; }

	/**
	 * @brief Get the number of currently active entities
	 * @return Number of entities acquired but not yet released
	 */
	size_t GetActiveCount() const { return m_TotalEntities - m_AvailableEntities.size(); }

	/**
	 * @brief Clear all entities from the pool
	 * WARNING: This will destroy all pooled entities
	 */
	void Clear();

	/**
	 * @brief Set a callback to initialize newly created entities
	 * @param callback Function to call when a new entity is created
	 * 
	 * Example:
	 * @code
	 * pool.SetInitCallback([](Entity entity) {
	 *     entity.AddComponent<TransformComponent>();
	 *     entity.AddComponent<VelocityComponent>();
	 *     entity.AddComponent<BulletComponent>();
	 * });
	 * @endcode
	 */
	void SetInitCallback(std::function<void(Entity)> callback) { m_InitCallback = callback; }

	/**
	 * @brief Set a callback to reset entities when returned to pool
	 * @param callback Function to call when entity is released back to pool
	 * 
	 * Example:
	 * @code
	 * pool.SetResetCallback([](Entity entity) {
	 *     auto& transform = entity.GetComponent<TransformComponent>();
	 *     transform.Position = glm::vec2(0.0f);
	 *     
	 *     auto& bullet = entity.GetComponent<BulletComponent>();
	 *     bullet.TimeAlive = 0.0f;
	 *     bullet.HitsRemaining = bullet.MaxHits;
	 * });
	 * @endcode
	 */
	void SetResetCallback(std::function<void(Entity)> callback) { m_ResetCallback = callback; }

private:
	Scene* m_Scene = nullptr;
	std::vector<Entity> m_AvailableEntities;
	uint32_t m_TotalEntities = 0;

	std::function<void(Entity)> m_InitCallback;
	std::function<void(Entity)> m_ResetCallback;

	/**
	 * @brief Create a new entity and add it to the pool
	 * @return The newly created entity
	 */
	Entity CreateEntity();
};

} // namespace Pillar
