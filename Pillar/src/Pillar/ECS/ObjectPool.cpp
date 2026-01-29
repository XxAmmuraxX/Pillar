#include "ObjectPool.h"
#include "Scene.h"
#include "Pillar/Logger.h"

namespace Pillar {

void ObjectPool::Init(Scene* scene, uint32_t initialCapacity)
{
	PIL_CORE_ASSERT(scene, "Scene cannot be null!");
	m_Scene = scene;

	// Pre-allocate entities
	m_AvailableEntities.reserve(initialCapacity);
	m_InPoolSet.reserve(initialCapacity);
	for (uint32_t i = 0; i < initialCapacity; i++)
	{
		Entity entity = CreateEntity();
		m_AvailableEntities.push_back(entity);
		m_InPoolSet.insert(static_cast<uint32_t>(static_cast<entt::entity>(entity)));
	}

#ifdef PIL_DEBUG
	PIL_CORE_TRACE("ObjectPool initialized with {0} entities", initialCapacity);
#endif
}

Entity ObjectPool::Acquire()
{
	PIL_CORE_ASSERT(m_Scene, "ObjectPool not initialized! Call Init() first.");

	Entity entity;

	// If pool has available entities, reuse one
	if (!m_AvailableEntities.empty())
	{
		entity = m_AvailableEntities.back();
		m_AvailableEntities.pop_back();
		m_InPoolSet.erase(static_cast<uint32_t>(static_cast<entt::entity>(entity)));

		// Validate entity is still valid (could have been destroyed externally)
		if (!entity.IsValid())
		{
			PIL_CORE_WARN("ObjectPool: Pooled entity was invalid/destroyed, creating new one");
			entity = CreateEntity();
		}

#ifdef PIL_DEBUG
		PIL_CORE_TRACE("ObjectPool: Reusing entity from pool (available: {0})", m_AvailableEntities.size());
#endif
	}
	else
	{
		// Pool is empty, create new entity
		entity = CreateEntity();
		PIL_CORE_WARN("ObjectPool: Pool exhausted, creating new entity (total: {0})", m_TotalEntities);
	}

	return entity;
}

void ObjectPool::Release(Entity entity)
{
	PIL_CORE_ASSERT(m_Scene, "ObjectPool not initialized!");
	PIL_CORE_ASSERT(entity, "Cannot release invalid entity!");

	uint32_t entityId = static_cast<uint32_t>(static_cast<entt::entity>(entity));

	// O(1) check if entity is already in pool (avoid double-release)
	if (m_InPoolSet.count(entityId) > 0)
	{
#ifdef PIL_DEBUG
		PIL_CORE_WARN("ObjectPool: Attempted to release entity {0} already in pool!", entityId);
#endif
		return;
	}

	// Call reset callback if set
	if (m_ResetCallback)
	{
		m_ResetCallback(entity);
	}

	// Return entity to pool
	m_AvailableEntities.push_back(entity);
	m_InPoolSet.insert(entityId);

#ifdef PIL_DEBUG
	PIL_CORE_TRACE("ObjectPool: Released entity back to pool (available: {0})", m_AvailableEntities.size());
#endif
}

bool ObjectPool::IsInPool(Entity entity) const
{
	// O(1) lookup using hash set
	uint32_t entityId = static_cast<uint32_t>(static_cast<entt::entity>(entity));
	return m_InPoolSet.count(entityId) > 0;
}

void ObjectPool::Clear()
{
#ifdef PIL_DEBUG
	PIL_CORE_TRACE("ObjectPool: Clearing all {0} entities", m_TotalEntities);
#endif

	// Destroy all entities in the pool
	for (auto& entity : m_AvailableEntities)
	{
		if (entity)
		{
			m_Scene->GetRegistry().destroy(static_cast<entt::entity>(entity));
		}
	}

	m_AvailableEntities.clear();
	m_InPoolSet.clear();
	m_TotalEntities = 0;
}

Entity ObjectPool::CreateEntity()
{
	PIL_CORE_ASSERT(m_Scene, "ObjectPool not initialized!");

	// Create a bare entity directly via registry to avoid default components
	entt::entity handle = m_Scene->GetRegistry().create();
	Entity entity{ handle, m_Scene };
	m_TotalEntities++;

	// Initialize components via callback
	if (m_InitCallback)
	{
		m_InitCallback(entity);
	}

	return entity;
}

} // namespace Pillar
