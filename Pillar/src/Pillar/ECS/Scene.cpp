#include "Scene.h"
#include "Entity.h"
#include "ComponentRegistry.h"
#include "Components/Core/TagComponent.h"
#include "Components/Core/TransformComponent.h"
#include "Components/Core/UUIDComponent.h"
#include "Components/Physics/RigidbodyComponent.h"
#include "Systems/PhysicsSystem.h"
#include "Pillar/Logger.h"

namespace Pillar {

	Scene::Scene(const std::string& name)
		: m_Name(name)
	{
		// Register cleanup callback for RigidbodyComponent
		m_Registry.on_destroy<RigidbodyComponent>().connect<&Scene::OnRigidbodyDestroyed>(this);
		PIL_CORE_TRACE("Scene '{}' created", m_Name);
	}

	Scene::~Scene()
	{
		PIL_CORE_TRACE("Scene '{}' destroyed", m_Name);
	}

	void Scene::OnRuntimeStart()
	{
		m_State = SceneState::Play;
		PIL_CORE_INFO("Scene '{}' started runtime", m_Name);
	}

	void Scene::OnRuntimeStop()
	{
		m_State = SceneState::Edit;
		PIL_CORE_INFO("Scene '{}' stopped runtime", m_Name);
	}

	void Scene::OnUpdate(float deltaTime)
	{
		if (m_State != SceneState::Play)
			return;

		// Systems would be updated here if managed by scene
	}

	void Scene::OnRender()
	{
		// Rendering logic - can be delegated to a render system
	}

	Entity Scene::CreateEntity(const std::string& name)
	{
		Entity entity = { m_Registry.create(), this };
		entity.AddComponent<TagComponent>(name);
		entity.AddComponent<TransformComponent>();
		entity.AddComponent<UUIDComponent>();
		return entity;
	}

	Entity Scene::CreateEntityWithUUID(uint64_t uuid, const std::string& name)
	{
		Entity entity = { m_Registry.create(), this };
		entity.AddComponent<TagComponent>(name);
		entity.AddComponent<TransformComponent>();
		entity.AddComponent<UUIDComponent>(uuid);
		return entity;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		m_Registry.destroy(entity);
	}

	Entity Scene::DuplicateEntity(Entity entity)
	{
		if (!entity)
			return Entity();

		// Ensure components are registered
		ComponentRegistry::Get().EnsureBuiltinsRegistered();

		std::string name = entity.GetComponent<TagComponent>().Tag;
		Entity newEntity = CreateEntity(name + " (Copy)");

		// Use ComponentRegistry to copy all registered components
		auto& registry = ComponentRegistry::Get();
		for (const auto& [key, registration] : registry.GetRegistrations())
		{
			if (registration.Copy)
			{
				registration.Copy(entity, newEntity);
			}
		}

		return newEntity;
	}

	Entity Scene::FindEntityByName(const std::string& name)
	{
		auto view = m_Registry.view<TagComponent>();
		for (auto entity : view)
		{
			const auto& tag = view.get<TagComponent>(entity);
			if (tag.Tag == name)
				return Entity(entity, this);
		}
		return Entity();
	}

	Entity Scene::FindEntityByUUID(uint64_t uuid)
	{
		auto view = m_Registry.view<UUIDComponent>();
		for (auto entity : view)
		{
			const auto& uuidComp = view.get<UUIDComponent>(entity);
			if (uuidComp.UUID == uuid)
				return Entity(entity, this);
		}
		return Entity();
	}

	std::vector<Entity> Scene::GetAllEntities()
	{
		std::vector<Entity> entities;
		m_Registry.each([&](auto entityHandle) {
			entities.emplace_back(entityHandle, this);
		});
		return entities;
	}

	size_t Scene::GetEntityCount() const
	{
		// Count alive entities by iterating
		size_t count = 0;
		auto view = m_Registry.view<entt::entity>();
		for (auto entity : view)
		{
			if (m_Registry.valid(entity))
				count++;
		}
		return count;
	}

	std::shared_ptr<Scene> Scene::Copy(const std::shared_ptr<Scene>& other)
	{
		// Ensure components are registered
		ComponentRegistry::Get().EnsureBuiltinsRegistered();

		auto newScene = std::make_shared<Scene>(other->m_Name);

		auto& srcRegistry = other->m_Registry;

		// Copy all entities with their core components
		auto view = srcRegistry.view<UUIDComponent, TagComponent>();
		for (auto entityHandle : view)
		{
			auto& uuid = view.get<UUIDComponent>(entityHandle);
			auto& tag = view.get<TagComponent>(entityHandle);
			
			Entity srcEntity(entityHandle, other.get());
			Entity newEntity = newScene->CreateEntityWithUUID(uuid.UUID, tag.Tag);

			// Use ComponentRegistry to copy all registered components
			auto& compRegistry = ComponentRegistry::Get();
			for (const auto& [key, registration] : compRegistry.GetRegistrations())
			{
				if (registration.Copy)
				{
					registration.Copy(srcEntity, newEntity);
				}
			}
		}

		return newScene;
	}

	void Scene::OnRigidbodyDestroyed(entt::registry& registry, entt::entity entity)
	{
		auto& rigidbody = registry.get<RigidbodyComponent>(entity);
		if (rigidbody.Body && m_PhysicsSystem)
		{
			m_PhysicsSystem->GetWorld()->DestroyBody(rigidbody.Body);
			rigidbody.Body = nullptr;
		}
	}

} // namespace Pillar
