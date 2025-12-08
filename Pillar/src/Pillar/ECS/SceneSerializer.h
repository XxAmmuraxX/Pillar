#pragma once

#include "Pillar/Core.h"
#include "Scene.h"
#include <string>

namespace Pillar {

	class PIL_API SceneSerializer
	{
	public:
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

	private:
		Scene* m_Scene;
	};

} // namespace Pillar
