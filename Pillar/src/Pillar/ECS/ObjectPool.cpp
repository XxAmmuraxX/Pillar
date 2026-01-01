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
	for (uint32_t i = 0; i < initialCapacity; i++)
	{
		Entity entity = CreateEntity();
		m_AvailableEntities.push_back(entity);
	}

	PIL_CORE_TRACE("ObjectPool initialized with {0} entities", initialCapacity);
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

		//PIL_CORE_TRACE("ObjectPool: Reusing entity from pool (available: {0})", m_AvailableEntities.size());
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

	// Check if entity is already in pool (avoid double-release)
	if (IsInPool(entity))
	{
		PIL_CORE_WARN("ObjectPool: Attempted to release entity already in pool!");
		return;
	}

	// Call reset callback if set
	if (m_ResetCallback)
	{
		m_ResetCallback(entity);
	}

	// Return entity to pool
	m_AvailableEntities.push_back(entity);
	//PIL_CORE_TRACE("ObjectPool: Released entity back to pool (available: {0})", m_AvailableEntities.size());
}

bool ObjectPool::IsInPool(Entity entity) const
{
	// Check if entity is in the available list
	for (const auto& pooledEntity : m_AvailableEntities)
	{
		if (pooledEntity == entity)
			return true;
	}
	return false;
}

void ObjectPool::Clear()
{
	PIL_CORE_TRACE("ObjectPool: Clearing all {0} entities", m_TotalEntities);

	// Destroy all entities in the pool
	for (auto& entity : m_AvailableEntities)
	{
		if (entity)
		{
			m_Scene->GetRegistry().destroy(static_cast<entt::entity>(entity));
		}
	}

	m_AvailableEntities.clear();
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
