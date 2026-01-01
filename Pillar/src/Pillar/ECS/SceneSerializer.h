#pragma once

#include "Pillar/Core.h"
#include "Scene.h"
#include <string>
#include <functional>
#include <nlohmann/json_fwd.hpp>

namespace Pillar {

	class PIL_API SceneSerializer
	{
	public:
		using MigrationCallback = std::function<void(nlohmann::json& /*root*/, const std::string& /*fileVersion*/, const std::string& /*targetVersion*/)>;

		SceneSerializer(Scene* scene);

		// JSON serialization (human-readable)
		bool Serialize(const std::string& filepath);
		bool Deserialize(const std::string& filepath);

		// Binary serialization (faster, smaller) - for runtime
		bool SerializeBinary(const std::string& filepath);
		bool DeserializeBinary(const std::string& filepath);

		// Serialize to/from string (for network, clipboard, etc.)
		std::string SerializeToString();
		bool DeserializeFromString(const std::string& data);

		// Versioning and migrations
		static const std::string& GetCurrentVersion();
		static void SetMigrationCallback(MigrationCallback callback);
		static void ApplyMigrationIfNeeded(nlohmann::json& root, const std::string& fileVersion);

	private:
		Scene* m_Scene;
		static std::string s_CurrentVersion;
		static MigrationCallback s_MigrationCallback;
	};

} // namespace Pillar
