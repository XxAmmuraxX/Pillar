#pragma once

#include "Pillar/Core.h"
#include <entt/entt.hpp>
#include <string>
#include <memory>
#include <vector>

namespace Pillar {

	// Forward declarations
	class Entity;
	class PhysicsSystem;
	class AnimationSystem;
	class SceneSerializer;

	enum class SceneState
	{
		Edit = 0,
		Play,
		Paused
	};

	class PIL_API Scene
	{
	public:
		Scene(const std::string& name = "Untitled");
		~Scene();

		// Scene lifecycle
		void OnRuntimeStart();
		void OnRuntimeStop();
		void OnUpdate(float deltaTime);
		void OnRender();

		// Entity management
		Entity CreateEntity(const std::string& name = "Entity");
		Entity CreateEntityWithUUID(uint64_t uuid, const std::string& name = "Entity");
		void DestroyEntity(Entity entity);
		Entity DuplicateEntity(Entity entity);

		// Entity queries
		Entity FindEntityByName(const std::string& name);
		Entity FindEntityByUUID(uint64_t uuid);
		std::vector<Entity> GetAllEntities();
		size_t GetEntityCount() const;

		// Scene properties
		const std::string& GetName() const { return m_Name; }
		void SetName(const std::string& name) { m_Name = name; }
		const std::string& GetFilePath() const { return m_FilePath; }
		void SetFilePath(const std::string& path) { m_FilePath = path; }
		SceneState GetState() const { return m_State; }
		bool IsPlaying() const { return m_State == SceneState::Play; }
		bool IsPaused() const { return m_State == SceneState::Paused; }

		// Registry access
		entt::registry& GetRegistry() { return m_Registry; }
		const entt::registry& GetRegistry() const { return m_Registry; }

		// Physics system management
		void SetPhysicsSystem(PhysicsSystem* physicsSystem) { m_PhysicsSystem = physicsSystem; }
		PhysicsSystem* GetPhysicsSystem() const { return m_PhysicsSystem; }

		// Animation system management
		void SetAnimationSystem(AnimationSystem* animationSystem) { m_AnimationSystem = animationSystem; }
		AnimationSystem* GetAnimationSystem() const { return m_AnimationSystem; }

		// Scene copying for play/edit mode
		static std::shared_ptr<Scene> Copy(const std::shared_ptr<Scene>& other);

		// Component iteration helper
		template<typename... Components>
		auto GetEntitiesWith()
		{
			return m_Registry.view<Components...>();
		}

	private:
		void OnRigidbodyDestroyed(entt::registry& registry, entt::entity entity);

	private:
		entt::registry m_Registry;
		std::string m_Name;
		std::string m_FilePath;
		SceneState m_State = SceneState::Edit;
		PhysicsSystem* m_PhysicsSystem = nullptr;
		AnimationSystem* m_AnimationSystem = nullptr;

		friend class Entity;
		friend class SceneSerializer;
	};

} // namespace Pillar
