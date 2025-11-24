#pragma once

#include "Pillar/Core.h"
#include <entt/entt.hpp>
#include <string>

namespace Pillar {

	// Forward declaration
	class Entity;

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

	private:
		entt::registry m_Registry;

		friend class Entity;
	};

} // namespace Pillar
