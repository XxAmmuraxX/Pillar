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
#include <iterator>
#include <vector>
#include <utility>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace {
	json BuildSceneJson(Pillar::Scene* scene)
	{
		json sceneJson;

		sceneJson["scene"] = {
			{ "name", scene->GetName() },
			{ "version", Pillar::SceneSerializer::GetCurrentVersion() },
			{ "schema", "scene" }
		};

		json entitiesJson = json::array();

		scene->GetRegistry().each([&](auto entityHandle) {
			Pillar::Entity entity(entityHandle, scene);
			json entityJson;

			if (entity.HasComponent<Pillar::UUIDComponent>())
			{
				entityJson["uuid"] = entity.GetComponent<Pillar::UUIDComponent>().UUID;
			}

			if (entity.HasComponent<Pillar::TagComponent>())
			{
				entityJson["tag"] = entity.GetComponent<Pillar::TagComponent>().Tag;
			}

			auto& registry = Pillar::ComponentRegistry::Get();
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
		return sceneJson;
	}

	std::filesystem::path ResolveSavePath(const std::string& filepath)
	{
		std::filesystem::path fullPath;
		if (std::filesystem::path(filepath).is_absolute())
		{
			fullPath = filepath;
		}
		else
		{
			std::filesystem::path assetsDir = Pillar::AssetManager::GetAssetsDirectory();
			fullPath = assetsDir / filepath;
		}
		return fullPath;
	}

	std::filesystem::path ResolveLoadPath(const std::string& filepath)
	{
		std::filesystem::path fullPath;
		if (std::filesystem::path(filepath).is_absolute())
		{
			fullPath = filepath;
		}
		else
		{
			std::string resolvedPath = Pillar::AssetManager::GetAssetPath(filepath);
			fullPath = resolvedPath;
			if (!std::filesystem::exists(fullPath))
			{
				std::filesystem::path assetsDir = Pillar::AssetManager::GetAssetsDirectory();
				fullPath = assetsDir / filepath;
			}
		}
		return fullPath;
	}

	bool PopulateSceneFromJson(Pillar::Scene* scene, const json& sceneJson)
	{
		scene->GetRegistry().clear();

		if (sceneJson.contains("scene"))
		{
			const auto& sceneMeta = sceneJson["scene"];
			if (sceneMeta.contains("name"))
				scene->SetName(sceneMeta["name"].get<std::string>());
		}

		if (!sceneJson.contains("entities"))
		{
			PIL_CORE_WARN("Scene JSON has no entities");
			return true;
		}

		auto& registry = Pillar::ComponentRegistry::Get();

		for (const auto& entityJson : sceneJson["entities"])
		{
			uint64_t uuid = entityJson.contains("uuid") ? entityJson["uuid"].get<uint64_t>() : 0;
			std::string tag = entityJson.contains("tag") ? entityJson["tag"].get<std::string>() : "Entity";

			Pillar::Entity entity = uuid != 0 ?
				scene->CreateEntityWithUUID(uuid, tag) :
				scene->CreateEntity(tag);

			for (const auto& [key, registration] : registry.GetRegistrations())
			{
				if (entityJson.contains(key))
				{
					registration.Deserialize(entity, entityJson[key]);
				}
			}
		}

		PIL_CORE_INFO("Scene deserialized from JSON ({} entities)", scene->GetEntityCount());
		return true;
	}
}

namespace Pillar {

	std::string SceneSerializer::s_CurrentVersion = "1.1.0";
	SceneSerializer::MigrationCallback SceneSerializer::s_MigrationCallback = nullptr;

	SceneSerializer::SceneSerializer(Scene* scene)
		: m_Scene(scene)
	{
		ComponentRegistry::Get().EnsureBuiltinsRegistered();
	}

	bool SceneSerializer::Serialize(const std::string& filepath)
	{
		json sceneJson = BuildSceneJson(m_Scene);
		auto fullPath = ResolveSavePath(filepath);

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
		auto fullPath = ResolveLoadPath(filepath);

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

		std::string fileVersion = s_CurrentVersion;
		if (sceneJson.contains("scene") && sceneJson["scene"].contains("version") && sceneJson["scene"]["version"].is_string())
		{
			fileVersion = sceneJson["scene"]["version"].get<std::string>();
		}

		ApplyMigrationIfNeeded(sceneJson, fileVersion);
		return PopulateSceneFromJson(m_Scene, sceneJson);
	}

	bool SceneSerializer::SerializeBinary(const std::string& filepath)
	{
		json sceneJson = BuildSceneJson(m_Scene);
		auto fullPath = ResolveSavePath(filepath);

		std::filesystem::path dir = fullPath.parent_path();
		if (!dir.empty() && !std::filesystem::exists(dir))
		{
			try
			{
				std::filesystem::create_directories(dir);
			}
			catch (const std::filesystem::filesystem_error& e)
			{
				PIL_CORE_ERROR("Failed to create directory '{}': {}", dir.string(), e.what());
				return false;
			}
		}

		std::ofstream file(fullPath, std::ios::binary);
		if (!file.is_open())
		{
			PIL_CORE_ERROR("Failed to open file for writing: {}", fullPath.string());
			return false;
		}

		std::vector<uint8_t> data = json::to_msgpack(sceneJson);
		file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
		file.close();

		PIL_CORE_INFO("Scene serialized to binary: {} ({} bytes)", fullPath.string(), data.size());
		return true;
	}

	bool SceneSerializer::DeserializeBinary(const std::string& filepath)
	{
		auto fullPath = ResolveLoadPath(filepath);
		std::ifstream file(fullPath, std::ios::binary);
		if (!file.is_open())
		{
			PIL_CORE_ERROR("Failed to open binary scene: {}", fullPath.string());
			return false;
		}

		std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		file.close();

		json sceneJson;
		try
		{
			sceneJson = json::from_msgpack(buffer);
		}
		catch (const std::exception& e)
		{
			PIL_CORE_ERROR("Failed to parse binary scene: {}", e.what());
			return false;
		}

		std::string fileVersion = s_CurrentVersion;
		if (sceneJson.contains("scene") && sceneJson["scene"].contains("version") && sceneJson["scene"]["version"].is_string())
		{
			fileVersion = sceneJson["scene"]["version"].get<std::string>();
		}

		ApplyMigrationIfNeeded(sceneJson, fileVersion);
		return PopulateSceneFromJson(m_Scene, sceneJson);
	}

	std::string SceneSerializer::SerializeToString()
	{
		json sceneJson = BuildSceneJson(m_Scene);
		return sceneJson.dump(2);
	}

	bool SceneSerializer::DeserializeFromString(const std::string& data)
	{
		try
		{
			json sceneJson = json::parse(data);
			std::string fileVersion = s_CurrentVersion;
			if (sceneJson.contains("scene") && sceneJson["scene"].contains("version") && sceneJson["scene"]["version"].is_string())
				fileVersion = sceneJson["scene"]["version"].get<std::string>();

			ApplyMigrationIfNeeded(sceneJson, fileVersion);
			return PopulateSceneFromJson(m_Scene, sceneJson);
		}
		catch (const json::parse_error& e)
		{
			PIL_CORE_ERROR("JSON parse error: {}", e.what());
			return false;
		}
	}

	const std::string& SceneSerializer::GetCurrentVersion()
	{
		return s_CurrentVersion;
	}

	void SceneSerializer::SetMigrationCallback(MigrationCallback callback)
	{
		s_MigrationCallback = std::move(callback);
	}

	void SceneSerializer::ApplyMigrationIfNeeded(json& root, const std::string& fileVersion)
	{
		if (fileVersion == s_CurrentVersion)
			return;

		if (s_MigrationCallback)
		{
			try
			{
				s_MigrationCallback(root, fileVersion, s_CurrentVersion);
			}
			catch (const std::exception& e)
			{
				PIL_CORE_ERROR("Scene migration from {} to {} failed: {}", fileVersion, s_CurrentVersion, e.what());
			}
		}
		else
		{
			PIL_CORE_WARN("Scene version {} differs from runtime {}; no migration registered", fileVersion, s_CurrentVersion);
		}
	}

} // namespace Pillar
