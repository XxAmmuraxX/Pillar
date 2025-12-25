#include "PrefabSerializer.h"
#include "SceneSerializer.h"
#include "ComponentRegistry.h"
#include "Components/Core/TagComponent.h"
#include "Components/Core/UUIDComponent.h"
#include "Components/Core/HierarchyComponent.h"
#include "Pillar/Logger.h"
#include "Pillar/Utils/AssetManager.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <queue>
#include <unordered_map>
#include <utility>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace {
	std::filesystem::path ResolveSavePath(const std::string& filepath)
	{
		auto inputPath = std::filesystem::path(filepath);
		if (inputPath.is_absolute())
			return inputPath;
		auto assetsDir = std::filesystem::path(Pillar::AssetManager::GetAssetsDirectory());
		return assetsDir / inputPath;
	}

	std::filesystem::path ResolveLoadPath(const std::string& filepath)
	{
		auto inputPath = std::filesystem::path(filepath);
		if (inputPath.is_absolute())
			return inputPath;

		std::filesystem::path resolved = Pillar::AssetManager::GetAssetPath(filepath);
		if (!std::filesystem::exists(resolved))
		{
			auto assetsDir = std::filesystem::path(Pillar::AssetManager::GetAssetsDirectory());
			resolved = assetsDir / inputPath;
		}
		return resolved;
	}

	std::vector<Pillar::Entity> CollectSubtree(Pillar::Scene* scene, Pillar::Entity root, bool includeChildren)
	{
		std::vector<Pillar::Entity> entities;
		if (!scene || !root)
			return entities;

		entities.push_back(root);
		if (!includeChildren)
			return entities;

		auto& registry = scene->GetRegistry();
		std::unordered_multimap<uint64_t, entt::entity> childrenByParent;
		auto view = registry.view<Pillar::HierarchyComponent, Pillar::UUIDComponent>();
		for (auto entityHandle : view)
		{
			const auto& hierarchy = view.get<Pillar::HierarchyComponent>(entityHandle);
			childrenByParent.emplace(hierarchy.ParentUUID, entityHandle);
		}

		std::queue<uint64_t> queue;
		queue.push(root.GetUUID());
		while (!queue.empty())
		{
			uint64_t parentUUID = queue.front();
			queue.pop();

			auto range = childrenByParent.equal_range(parentUUID);
			for (auto it = range.first; it != range.second; ++it)
			{
				Pillar::Entity child(it->second, scene);
				if (!child)
					continue;
				entities.push_back(child);
				queue.push(child.GetUUID());
			}
		}

		return entities;
	}

	json BuildPrefabJson(Pillar::Scene* scene, Pillar::Entity root, const Pillar::PrefabOptions& options)
	{
		if (!scene || !root)
			return json();

		json prefabJson;
		std::vector<Pillar::Entity> entities = CollectSubtree(scene, root, options.IncludeChildren);

		prefabJson["prefab"] = {
			{ "name", options.NameOverride.empty() ? root.Name() : options.NameOverride },
			{ "root", root.GetUUID() },
			{ "version", Pillar::SceneSerializer::GetCurrentVersion() },
			{ "schema", "prefab" }
		};

		json entitiesJson = json::array();
		auto& registry = Pillar::ComponentRegistry::Get();
		for (auto entity : entities)
		{
			json entityJson;
			if (entity.HasComponent<Pillar::UUIDComponent>())
				entityJson["uuid"] = entity.GetComponent<Pillar::UUIDComponent>().UUID;
			if (entity.HasComponent<Pillar::TagComponent>())
				entityJson["tag"] = entity.GetComponent<Pillar::TagComponent>().Tag;

			for (const auto& [key, registration] : registry.GetRegistrations())
			{
				json componentJson = registration.Serialize(entity);
				if (!componentJson.is_null())
					entityJson[key] = componentJson;
			}

			entitiesJson.push_back(entityJson);
		}

		prefabJson["entities"] = entitiesJson;
		return prefabJson;
	}

	Pillar::Entity InstantiatePrefabFromJson(Pillar::Scene* scene, json prefabJson, const Pillar::PrefabOptions& options)
	{
		if (!scene || prefabJson.is_null() || !prefabJson.contains("entities"))
			return Pillar::Entity();

		std::vector<Pillar::Entity> created;
		created.reserve(prefabJson["entities"].size());
		std::unordered_map<uint64_t, uint64_t> uuidRemap;

		for (const auto& entityJson : prefabJson["entities"])
		{
			uint64_t sourceUUID = entityJson.contains("uuid") ? entityJson["uuid"].get<uint64_t>() : 0;
			std::string tag = entityJson.contains("tag") ? entityJson["tag"].get<std::string>() : "Entity";
			Pillar::Entity entity = (options.PreserveUUIDs && sourceUUID != 0) ?
				scene->CreateEntityWithUUID(sourceUUID, tag) :
				scene->CreateEntity(tag);

			created.push_back(entity);
			if (sourceUUID != 0)
			{
				uuidRemap[sourceUUID] = entity.GetComponent<Pillar::UUIDComponent>().UUID;
			}
		}

		if (!options.PreserveUUIDs)
		{
			for (auto& entityJson : prefabJson["entities"])
			{
				auto it = entityJson.find("hierarchy");
				if (it != entityJson.end() && it->is_object())
				{
					uint64_t oldParent = it->value("parentUUID", 0ull);
					if (oldParent != 0)
					{
						auto mapIt = uuidRemap.find(oldParent);
						(*it)["parentUUID"] = (mapIt != uuidRemap.end()) ? mapIt->second : 0ull;
					}
				}
			}
		}

		auto& registry = Pillar::ComponentRegistry::Get();
		for (size_t i = 0; i < prefabJson["entities"].size(); ++i)
		{
			const auto& entityJson = prefabJson["entities"][i];
			Pillar::Entity entity = created[i];

			for (const auto& [key, registration] : registry.GetRegistrations())
			{
				if (entityJson.contains(key))
				{
					registration.Deserialize(entity, entityJson[key]);
				}
			}
		}

		uint64_t rootUUID = 0;
		if (prefabJson.contains("prefab") && prefabJson["prefab"].contains("root"))
			rootUUID = prefabJson["prefab"]["root"].get<uint64_t>();

		if (!options.PreserveUUIDs && rootUUID != 0)
		{
			auto it = uuidRemap.find(rootUUID);
			if (it != uuidRemap.end())
				rootUUID = it->second;
		}

		Pillar::Entity root = rootUUID != 0 ? scene->FindEntityByUUID(rootUUID) : Pillar::Entity();
		if (!root && !created.empty())
			root = created.front();

		return root;
	}
}

namespace Pillar {

	PrefabSerializer::PrefabSerializer(Scene* scene)
		: m_Scene(scene)
	{
		ComponentRegistry::Get().EnsureBuiltinsRegistered();
	}

	std::string PrefabSerializer::SerializeToString(Entity root, const PrefabOptions& options)
	{
		json prefabJson = BuildPrefabJson(m_Scene, root, options);
		if (prefabJson.is_null())
			return {};
		return prefabJson.dump(2);
	}

	bool PrefabSerializer::SerializeToFile(Entity root, const std::string& filepath, const PrefabOptions& options)
	{
		json prefabJson = BuildPrefabJson(m_Scene, root, options);
		if (prefabJson.is_null())
			return false;

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
				PIL_CORE_ERROR("Failed to create prefab directory '{}': {}", dir.string(), e.what());
				return false;
			}
		}

		std::ofstream file(fullPath);
		if (!file.is_open())
		{
			PIL_CORE_ERROR("Failed to open prefab file for writing: {}", fullPath.string());
			return false;
		}

		file << std::setw(2) << prefabJson << std::endl;
		return true;
	}

	std::vector<uint8_t> PrefabSerializer::SerializeToBinary(Entity root, const PrefabOptions& options)
	{
		json prefabJson = BuildPrefabJson(m_Scene, root, options);
		if (prefabJson.is_null())
			return {};
		return json::to_msgpack(prefabJson);
	}

	bool PrefabSerializer::SerializeBinaryToFile(Entity root, const std::string& filepath, const PrefabOptions& options)
	{
		std::vector<uint8_t> data = SerializeToBinary(root, options);
		if (data.empty())
			return false;

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
				PIL_CORE_ERROR("Failed to create prefab directory '{}': {}", dir.string(), e.what());
				return false;
			}
		}

		std::ofstream file(fullPath, std::ios::binary);
		if (!file.is_open())
		{
			PIL_CORE_ERROR("Failed to open prefab binary for writing: {}", fullPath.string());
			return false;
		}

		file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
		return true;
	}

	Entity PrefabSerializer::DeserializeFromString(const std::string& data, const PrefabOptions& options)
	{
		try
		{
			json prefabJson = json::parse(data);
			std::string fileVersion = SceneSerializer::GetCurrentVersion();
			if (prefabJson.contains("prefab") && prefabJson["prefab"].contains("version") && prefabJson["prefab"]["version"].is_string())
				fileVersion = prefabJson["prefab"]["version"].get<std::string>();

			SceneSerializer::ApplyMigrationIfNeeded(prefabJson, fileVersion);
			return InstantiatePrefabFromJson(m_Scene, prefabJson, options);
		}
		catch (const json::parse_error& e)
		{
			PIL_CORE_ERROR("Failed to parse prefab JSON: {}", e.what());
			return {};
		}
	}

	Entity PrefabSerializer::DeserializeFromFile(const std::string& filepath, const PrefabOptions& options)
	{
		auto fullPath = ResolveLoadPath(filepath);
		std::ifstream file(fullPath);
		if (!file.is_open())
		{
			PIL_CORE_ERROR("Failed to open prefab file: {}", fullPath.string());
			return {};
		}

		json prefabJson;
		try
		{
			file >> prefabJson;
		}
		catch (const json::parse_error& e)
		{
			PIL_CORE_ERROR("Failed to parse prefab JSON: {}", e.what());
			return {};
		}

		std::string fileVersion = SceneSerializer::GetCurrentVersion();
		if (prefabJson.contains("prefab") && prefabJson["prefab"].contains("version") && prefabJson["prefab"]["version"].is_string())
			fileVersion = prefabJson["prefab"]["version"].get<std::string>();

		SceneSerializer::ApplyMigrationIfNeeded(prefabJson, fileVersion);
		return InstantiatePrefabFromJson(m_Scene, prefabJson, options);
	}

	Entity PrefabSerializer::DeserializeFromBinary(const std::vector<uint8_t>& data, const PrefabOptions& options)
	{
		if (data.empty())
			return {};

		json prefabJson;
		try
		{
			prefabJson = json::from_msgpack(data);
		}
		catch (const std::exception& e)
		{
			PIL_CORE_ERROR("Failed to parse prefab binary: {}", e.what());
			return {};
		}

		std::string fileVersion = SceneSerializer::GetCurrentVersion();
		if (prefabJson.contains("prefab") && prefabJson["prefab"].contains("version") && prefabJson["prefab"]["version"].is_string())
			fileVersion = prefabJson["prefab"]["version"].get<std::string>();

		SceneSerializer::ApplyMigrationIfNeeded(prefabJson, fileVersion);
		return InstantiatePrefabFromJson(m_Scene, prefabJson, options);
	}

	Entity PrefabSerializer::DeserializeBinaryFromFile(const std::string& filepath, const PrefabOptions& options)
	{
		auto fullPath = ResolveLoadPath(filepath);
		std::ifstream file(fullPath, std::ios::binary);
		if (!file.is_open())
		{
			PIL_CORE_ERROR("Failed to open prefab binary: {}", fullPath.string());
			return {};
		}

		std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		return DeserializeFromBinary(buffer, options);
	}

} // namespace Pillar
