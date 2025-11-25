#include "Scene.h"
#include "Entity.h"
#include "Components/Core/TagComponent.h"
#include "Components/Core/TransformComponent.h"
#include "Components/Core/UUIDComponent.h"
#include "Components/Physics/RigidbodyComponent.h"
#include "Systems/PhysicsSystem.h"

namespace Pillar {

	Scene::Scene()
	{
		// Register cleanup callback for RigidbodyComponent
		m_Registry.on_destroy<RigidbodyComponent>().connect<&Scene::OnRigidbodyDestroyed>(this);
	}

	Scene::~Scene()
	{
	}

	Entity Scene::CreateEntity(const std::string& name)
	{
		Entity entity = { m_Registry.create(), this };
		entity.AddComponent<TagComponent>(name);
		entity.AddComponent<TransformComponent>();
		entity.AddComponent<UUIDComponent>();
		return entity;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		m_Registry.destroy(entity);
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
