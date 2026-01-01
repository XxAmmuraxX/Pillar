#pragma once

#include "Pillar/Core.h"
#include "Scene.h"
#include "Entity.h"
#include <cstdint>
#include <string>
#include <vector>

namespace Pillar {

	struct PrefabOptions
	{
		bool IncludeChildren = true;
		bool PreserveUUIDs = false;
		std::string NameOverride;
	};

	class PIL_API PrefabSerializer
	{
	public:
		PrefabSerializer(Scene* scene);

		// JSON serialization
		std::string SerializeToString(Entity root, const PrefabOptions& options = {});
		bool SerializeToFile(Entity root, const std::string& filepath, const PrefabOptions& options = {});

		// Binary (msgpack) serialization
		std::vector<uint8_t> SerializeToBinary(Entity root, const PrefabOptions& options = {});
		bool SerializeBinaryToFile(Entity root, const std::string& filepath, const PrefabOptions& options = {});

		// Deserialization
		Entity DeserializeFromString(const std::string& data, const PrefabOptions& options = {});
		Entity DeserializeFromFile(const std::string& filepath, const PrefabOptions& options = {});
		Entity DeserializeFromBinary(const std::vector<uint8_t>& data, const PrefabOptions& options = {});
		Entity DeserializeBinaryFromFile(const std::string& filepath, const PrefabOptions& options = {});

	private:
		Scene* m_Scene;
	};

} // namespace Pillar
