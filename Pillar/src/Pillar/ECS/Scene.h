#pragma once

#include "Pillar/Core.h"
#include <entt/entt.hpp>
#include <string>

namespace Pillar {

	// Forward declarations
	class Entity;
	class PhysicsSystem;

	class PIL_API Scene
	{
	public:
		Scene();
		~Scene();

		// Entity management
		Entity CreateEntity(const std::string& name = "Entity");
		void DestroyEntity(Entity entity);

		// Registry access
		entt::registry& GetRegistry() { return m_Registry; }
		const entt::registry& GetRegistry() const { return m_Registry; }

		// Physics system management
		void SetPhysicsSystem(PhysicsSystem* physicsSystem) { m_PhysicsSystem = physicsSystem; }

	private:
		entt::registry m_Registry;
		PhysicsSystem* m_PhysicsSystem = nullptr;

		// Cleanup callback for RigidbodyComponent
		void OnRigidbodyDestroyed(entt::registry& registry, entt::entity entity);

		friend class Entity;
	};

} // namespace Pillar
