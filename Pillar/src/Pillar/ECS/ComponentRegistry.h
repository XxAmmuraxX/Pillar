#pragma once

#include "Pillar/Core.h"
#include "Entity.h"
#include <nlohmann/json.hpp>
#include <functional>
#include <unordered_map>
#include <string>
#include <typeindex>

namespace Pillar {

	using json = nlohmann::json;

	/**
	 * @brief Component serialization/deserialization function types
	 * 
	 * SerializeFunc: Takes an entity, returns JSON for that component (or null if not present)
	 * DeserializeFunc: Takes an entity and JSON, adds/updates the component
	 * CopyFunc: Copies component from source entity to destination entity
	 */
	using ComponentSerializeFunc = std::function<json(Entity)>;
	using ComponentDeserializeFunc = std::function<void(Entity, const json&)>;
	using ComponentCopyFunc = std::function<void(Entity src, Entity dst)>;

	/**
	 * @brief Registration info for a serializable component
	 */
	struct ComponentRegistration
	{
		std::string Name;                      // JSON key name (e.g., "transform", "velocity")
		ComponentSerializeFunc Serialize;       // Serialize to JSON
		ComponentDeserializeFunc Deserialize;   // Deserialize from JSON
		ComponentCopyFunc Copy;                 // Copy between entities
	};

	/**
	 * @brief Registry for component serialization
	 * 
	 * Components register themselves with serialize/deserialize/copy functions.
	 * The SceneSerializer uses this registry to handle all components generically.
	 * 
	 * ## How to Register a Custom Component
	 * 
	 * You can register custom components at any time before serialization is used.
	 * A good place is in your game's initialization code:
	 * 
	 * ```cpp
	 * // In your game initialization:
	 * void MyGame::Init()
	 * {
	 *     auto& registry = Pillar::ComponentRegistry::Get();
	 *     
	 *     registry.Register<MyCustomComponent>("myCustom",
	 *         // Serialize: Return JSON if component exists, otherwise nullptr
	 *         [](Pillar::Entity e) -> nlohmann::json {
	 *             if (!e.HasComponent<MyCustomComponent>()) return nullptr;
	 *             auto& comp = e.GetComponent<MyCustomComponent>();
	 *             return nlohmann::json{
	 *                 { "health", comp.Health },
	 *                 { "name", comp.Name }
	 *             };
	 *         },
	 *         // Deserialize: Add component and populate from JSON
	 *         [](Pillar::Entity e, const nlohmann::json& j) {
	 *             auto& comp = e.AddComponent<MyCustomComponent>();
	 *             comp.Health = j.value("health", 100);
	 *             comp.Name = j.value("name", "Unknown");
	 *         },
	 *         // Copy: Copy component from src to dst (optional)
	 *         [](Pillar::Entity src, Pillar::Entity dst) {
	 *             if (!src.HasComponent<MyCustomComponent>()) return;
	 *             auto& s = src.GetComponent<MyCustomComponent>();
	 *             auto& d = dst.AddComponent<MyCustomComponent>();
	 *             d.Health = s.Health;
	 *             d.Name = s.Name;
	 *         }
	 *     );
	 * }
	 * ```
	 */
	class PIL_API ComponentRegistry
	{
	public:
		static ComponentRegistry& Get();

		/**
		 * @brief Ensure built-in components are registered
		 * Called automatically by SceneSerializer, Scene::Copy, etc.
		 */
		void EnsureBuiltinsRegistered();

		/**
		 * @brief Register a component type with its serialization functions
		 * @tparam T Component type (used for type checking and identification)
		 * @param jsonKey The key used in JSON for this component
		 * @param serialize Function to serialize the component to JSON
		 * @param deserialize Function to deserialize the component from JSON
		 * @param copy Function to copy the component between entities (optional)
		 */
		template<typename T>
		void Register(
			const std::string& jsonKey,
			ComponentSerializeFunc serialize,
			ComponentDeserializeFunc deserialize,
			ComponentCopyFunc copy = nullptr)
		{
			ComponentRegistration reg;
			reg.Name = jsonKey;
			reg.Serialize = std::move(serialize);
			reg.Deserialize = std::move(deserialize);
			reg.Copy = std::move(copy);

			m_Registrations[jsonKey] = std::move(reg);
			m_TypeToKey[std::type_index(typeid(T))] = jsonKey;
		}

		/**
		 * @brief Get all registered components
		 */
		const std::unordered_map<std::string, ComponentRegistration>& GetRegistrations() const
		{
			return m_Registrations;
		}

		/**
		 * @brief Get registration by JSON key
		 */
		const ComponentRegistration* GetRegistration(const std::string& jsonKey) const
		{
			auto it = m_Registrations.find(jsonKey);
			if (it != m_Registrations.end())
				return &it->second;
			return nullptr;
		}

		/**
		 * @brief Check if a component type is registered
		 */
		template<typename T>
		bool IsRegistered() const
		{
			return m_TypeToKey.find(std::type_index(typeid(T))) != m_TypeToKey.end();
		}

		/**
		 * @brief Get the JSON key for a component type
		 */
		template<typename T>
		const std::string& GetJsonKey() const
		{
			static std::string empty;
			auto it = m_TypeToKey.find(std::type_index(typeid(T)));
			if (it != m_TypeToKey.end())
				return it->second;
			return empty;
		}

		/**
		 * @brief Get number of registered components
		 */
		size_t GetRegistrationCount() const { return m_Registrations.size(); }

	private:
		ComponentRegistry() = default;
		~ComponentRegistry() = default;
		ComponentRegistry(const ComponentRegistry&) = delete;
		ComponentRegistry& operator=(const ComponentRegistry&) = delete;

		std::unordered_map<std::string, ComponentRegistration> m_Registrations;
		std::unordered_map<std::type_index, std::string> m_TypeToKey;
		bool m_BuiltinsRegistered = false;
	};

	// Forward declaration - implemented in BuiltinComponentRegistrations.cpp
	void RegisterBuiltinComponents();

} // namespace Pillar
