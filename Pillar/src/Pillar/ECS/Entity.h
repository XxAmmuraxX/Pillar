#pragma once

#include "Pillar/Core.h"
#include "Scene.h"
#include "Pillar/Logger.h"
#include "Components/Core/UUIDComponent.h"
#include "Components/Core/TagComponent.h"
#include <entt/entt.hpp>
#include <utility>

namespace Pillar {

	class PIL_API Entity
	{
	public:
		Entity() = default;
		Entity(entt::entity handle, Scene* scene)
			: m_EntityHandle(handle), m_Scene(scene)
		{
		}
		Entity(const Entity& other) = default;

		// Component management
		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			PIL_CORE_ASSERT(!HasComponent<T>(), "Entity already has component!");
			return m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
		}

		template<typename T>
		T& GetComponent()
		{
			PIL_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
			return m_Scene->m_Registry.get<T>(m_EntityHandle);
		}

		template<typename T>
		const T& GetComponent() const
		{
			PIL_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
			return m_Scene->m_Registry.get<T>(m_EntityHandle);
		}

		template<typename T>
		bool HasComponent() const
		{
			if (!IsValid())
				return false;
			return m_Scene->m_Registry.all_of<T>(m_EntityHandle);
		}

		template<typename T>
		T* TryGetComponent()
		{
			if (!IsValid())
				return nullptr;
			return m_Scene->m_Registry.try_get<T>(m_EntityHandle);
		}

		template<typename T>
		const T* TryGetComponent() const
		{
			if (!IsValid())
				return nullptr;
			return m_Scene->m_Registry.try_get<T>(m_EntityHandle);
		}

		template<typename T, typename... Args>
		T& GetOrAddComponent(Args&&... args)
		{
			if (auto* existing = TryGetComponent<T>())
				return *existing;
			return AddComponent<T>(std::forward<Args>(args)...);
		}

		template<typename T, typename... Args>
		T& AddOrReplaceComponent(Args&&... args)
		{
			return m_Scene->m_Registry.emplace_or_replace<T>(m_EntityHandle, std::forward<Args>(args)...);
		}

		template<typename T>
		void RemoveComponent()
		{
			PIL_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
			m_Scene->m_Registry.remove<T>(m_EntityHandle);
		}

		// Convenience methods
		uint64_t GetUUID() const
		{
			if (HasComponent<UUIDComponent>())
				return GetComponent<UUIDComponent>().UUID;
			return 0;
		}

		uint64_t UUID() const { return GetUUID(); }

		std::string& Name() { return GetComponent<TagComponent>().Tag; }
		const std::string& Name() const { return GetComponent<TagComponent>().Tag; }
		void SetName(const std::string& name) { GetComponent<TagComponent>().Tag = name; }

		Scene* GetScene() const { return m_Scene; }

		// Validity check - checks both handle and scene, plus whether entity exists in registry
		bool IsValid() const
		{
			return m_EntityHandle != entt::null && 
			       m_Scene != nullptr && 
			       m_Scene->m_Registry.valid(m_EntityHandle);
		}

		// Validity check (operator version)
		operator bool() const { return IsValid(); }
		operator entt::entity() const { return m_EntityHandle; }
		operator uint32_t() const { return static_cast<uint32_t>(m_EntityHandle); }

		bool operator==(const Entity& other) const
		{
			return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene;
		}

		bool operator!=(const Entity& other) const
		{
			return !(*this == other);
		}

	private:
		entt::entity m_EntityHandle{ entt::null };
		Scene* m_Scene = nullptr;
	};

} // namespace Pillar

namespace Pillar {

template<typename... Components, typename Func>
void Scene::ForEach(Func&& fn)
{
	auto view = m_Registry.view<Components...>();
	view.each([&](auto entityHandle, Components&... components)
	{
		fn(Entity{ entityHandle, this }, components...);
	});
}

template<typename Func>
void Scene::EachEntity(Func&& fn)
{
	m_Registry.each([&](auto entityHandle)
	{
		fn(Entity{ entityHandle, this });
	});
}

} // namespace Pillar
