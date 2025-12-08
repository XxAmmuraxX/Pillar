#include "SceneSerializer.h"
#include "Entity.h"
#include "ComponentRegistry.h"
#include "Components/Core/TagComponent.h"
#include "Components/Core/TransformComponent.h"
#include "Components/Core/UUIDComponent.h"
#include "Pillar/Logger.h"
#include "Pillar/Utils/AssetManager.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Pillar {

	SceneSerializer::SceneSerializer(Scene* scene)
		: m_Scene(scene)
	{
		// Ensure builtin components are registered
		ComponentRegistry::Get().EnsureBuiltinsRegistered();
	}

	bool SceneSerializer::Serialize(const std::string& filepath)
	{
		json sceneJson;
		
		// Scene metadata
		sceneJson["scene"] = {
			{ "name", m_Scene->GetName() },
			{ "version", "1.0" }
		};

		// Entities array
		json entitiesJson = json::array();

		m_Scene->GetRegistry().each([&](auto entityHandle) {
			Entity entity(entityHandle, m_Scene);
			json entityJson;

			// UUID (required) - always serialized in header
			if (entity.HasComponent<UUIDComponent>())
			{
				entityJson["uuid"] = entity.GetComponent<UUIDComponent>().UUID;
			}

			// Tag (required) - always serialized in header
			if (entity.HasComponent<TagComponent>())
			{
				entityJson["tag"] = entity.GetComponent<TagComponent>().Tag;
			}

			// Serialize all registered components using the registry
			auto& registry = ComponentRegistry::Get();
			for (const auto& [key, registration] : registry.GetRegistrations())
			{
				json componentJson = registration.Serialize(entity);
				if (!componentJson.is_null())
				{
					entityJson[key] = componentJson;
				}
			}

			entitiesJson.push_back(entityJson);
		});

		sceneJson["entities"] = entitiesJson;

		// Resolve full path for saving
		std::filesystem::path fullPath;
		
		// Check if path is absolute
		if (std::filesystem::path(filepath).is_absolute())
		{
			fullPath = filepath;
		}
		else
		{
			// Relative path - resolve using AssetManager
			std::filesystem::path assetsDir = AssetManager::GetAssetsDirectory();
			fullPath = assetsDir / filepath;
		}

		// Create directories if they don't exist
		std::filesystem::path dir = fullPath.parent_path();
		if (!dir.empty() && !std::filesystem::exists(dir))
		{
			try
			{
				std::filesystem::create_directories(dir);
				PIL_CORE_INFO("Created directory: {}", dir.string());
			}
			catch (const std::filesystem::filesystem_error& e)
			{
				PIL_CORE_ERROR("Failed to create directory '{}': {}", dir.string(), e.what());
				return false;
			}
		}

		// Write to file
		std::ofstream file(fullPath);
		if (!file.is_open())
		{
			PIL_CORE_ERROR("Failed to open file for writing: {}", fullPath.string());
			return false;
		}

		file << std::setw(2) << sceneJson << std::endl;
		file.close();

		PIL_CORE_INFO("Scene serialized to: {}", fullPath.string());
		return true;
	}

	bool SceneSerializer::Deserialize(const std::string& filepath)
	{
		// Resolve full path for loading
		std::filesystem::path fullPath;
		
		// Check if path is absolute
		if (std::filesystem::path(filepath).is_absolute())
		{
			fullPath = filepath;
		}
		else
		{
			// Try AssetManager resolution first
			std::string resolvedPath = AssetManager::GetAssetPath(filepath);
			fullPath = resolvedPath;
			
			// If AssetManager couldn't find it, try relative to assets directory
			if (!std::filesystem::exists(fullPath))
			{
				std::filesystem::path assetsDir = AssetManager::GetAssetsDirectory();
				fullPath = assetsDir / filepath;
			}
		}

		std::ifstream file(fullPath);
		if (!file.is_open())
		{
			PIL_CORE_ERROR("Failed to open file for reading: {}", fullPath.string());
			return false;
		}

		json sceneJson;
		try
		{
			file >> sceneJson;
		}
		catch (const json::parse_error& e)
		{
			PIL_CORE_ERROR("JSON parse error: {}", e.what());
			return false;
		}
		file.close();

		// Clear existing entities
		m_Scene->GetRegistry().clear();

		// Scene metadata
		if (sceneJson.contains("scene"))
		{
			auto& sceneMeta = sceneJson["scene"];
			if (sceneMeta.contains("name"))
			{
				m_Scene->SetName(sceneMeta["name"].get<std::string>());
			}
		}

		// Entities
		if (!sceneJson.contains("entities"))
		{
			PIL_CORE_WARN("Scene file has no entities");
			return true;
		}

		auto& registry = ComponentRegistry::Get();

		for (const auto& entityJson : sceneJson["entities"])
		{
			// Create entity with UUID and Tag (required components)
			uint64_t uuid = entityJson.contains("uuid") ? 
				entityJson["uuid"].get<uint64_t>() : 0;
			std::string tag = entityJson.contains("tag") ? 
				entityJson["tag"].get<std::string>() : "Entity";

			Entity entity = uuid != 0 ? 
				m_Scene->CreateEntityWithUUID(uuid, tag) : 
				m_Scene->CreateEntity(tag);

			// Deserialize all registered components using the registry
			for (const auto& [key, registration] : registry.GetRegistrations())
			{
				if (entityJson.contains(key))
				{
					registration.Deserialize(entity, entityJson[key]);
				}
			}
		}

		PIL_CORE_INFO("Scene deserialized from: {} ({} entities)", 
			fullPath.string(), m_Scene->GetEntityCount());
		return true;
	}

	bool SceneSerializer::SerializeBinary(const std::string& filepath)
	{
		// Binary serialization for faster loading in release builds
		// Implementation left as extension point
		PIL_CORE_WARN("Binary serialization not yet implemented - falling back to JSON");
		return Serialize(filepath);
	}

	bool SceneSerializer::DeserializeBinary(const std::string& filepath)
	{
		PIL_CORE_WARN("Binary deserialization not yet implemented - falling back to JSON");
		return Deserialize(filepath);
	}

	std::string SceneSerializer::SerializeToString()
	{
		json sceneJson;
		
		sceneJson["scene"] = {
			{ "name", m_Scene->GetName() },
			{ "version", "1.0" }
		};

		json entitiesJson = json::array();
		m_Scene->GetRegistry().each([&](auto entityHandle) {
			Entity entity(entityHandle, m_Scene);
			json entityJson;

			if (entity.HasComponent<UUIDComponent>())
				entityJson["uuid"] = entity.GetComponent<UUIDComponent>().UUID;
			if (entity.HasComponent<TagComponent>())
				entityJson["tag"] = entity.GetComponent<TagComponent>().Tag;

			// Serialize all registered components
			auto& registry = ComponentRegistry::Get();
			for (const auto& [key, registration] : registry.GetRegistrations())
			{
				json componentJson = registration.Serialize(entity);
				if (!componentJson.is_null())
				{
					entityJson[key] = componentJson;
				}
			}

			entitiesJson.push_back(entityJson);
		});

		sceneJson["entities"] = entitiesJson;
		return sceneJson.dump(2);
	}

	bool SceneSerializer::DeserializeFromString(const std::string& data)
	{
		try
		{
			json sceneJson = json::parse(data);
			
			// Clear existing entities
			m_Scene->GetRegistry().clear();

			// Scene metadata
			if (sceneJson.contains("scene") && sceneJson["scene"].contains("name"))
			{
				m_Scene->SetName(sceneJson["scene"]["name"].get<std::string>());
			}

			// Entities
			if (sceneJson.contains("entities"))
			{
				auto& registry = ComponentRegistry::Get();

				for (const auto& entityJson : sceneJson["entities"])
				{
					uint64_t uuid = entityJson.contains("uuid") ? 
						entityJson["uuid"].get<uint64_t>() : 0;
					std::string tag = entityJson.contains("tag") ? 
						entityJson["tag"].get<std::string>() : "Entity";

					Entity entity = uuid != 0 ? 
						m_Scene->CreateEntityWithUUID(uuid, tag) : 
						m_Scene->CreateEntity(tag);

					// Deserialize all registered components
					for (const auto& [key, registration] : registry.GetRegistrations())
					{
						if (entityJson.contains(key))
						{
							registration.Deserialize(entity, entityJson[key]);
						}
					}
				}
			}
			
			return true;
		}
		catch (const json::parse_error& e)
		{
			PIL_CORE_ERROR("JSON parse error: {}", e.what());
			return false;
		}
	}

} // namespace Pillar
