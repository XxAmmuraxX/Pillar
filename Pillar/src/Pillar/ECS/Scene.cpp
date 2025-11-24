#include "Scene.h"
#include "Entity.h"
#include "Components/Core/TagComponent.h"
#include "Components/Core/TransformComponent.h"
#include "Components/Core/UUIDComponent.h"

namespace Pillar {

	Scene::Scene()
	{
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

} // namespace Pillar
