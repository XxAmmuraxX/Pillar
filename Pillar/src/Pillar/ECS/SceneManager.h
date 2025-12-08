#pragma once

#include "Pillar/Core.h"
#include "Scene.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>
#include <vector>

namespace Pillar {

	// Scene transition callback type
	using SceneTransitionCallback = std::function<void(const std::string& fromScene, const std::string& toScene)>;

	class PIL_API SceneManager
	{
	public:
		static SceneManager& Get();

		// Scene management
		std::shared_ptr<Scene> CreateScene(const std::string& name);
		std::shared_ptr<Scene> GetScene(const std::string& name);
		bool HasScene(const std::string& name) const;
		bool RemoveScene(const std::string& name);
		void Clear();

		// Active scene
		bool SetActiveScene(const std::string& name);
		bool SetActiveScene(std::shared_ptr<Scene> scene);
		std::shared_ptr<Scene> GetActiveScene() const { return m_ActiveScene; }
		const std::string& GetActiveSceneName() const;

		// Scene transitions
		void RequestSceneChange(const std::string& sceneName);
		void LoadSceneAsync(const std::string& filepath, const std::string& sceneName = "");
		bool IsTransitioning() const { return m_IsTransitioning; }

		// Update (handles pending scene changes)
		void OnUpdate(float deltaTime);

		// Scene loading/saving
		bool LoadScene(const std::string& filepath, const std::string& sceneName = "");
		bool SaveScene(const std::string& filepath);
		bool SaveScene(std::shared_ptr<Scene> scene, const std::string& filepath);

		// Callbacks
		void SetOnSceneChangeCallback(SceneTransitionCallback callback) { m_OnSceneChange = callback; }
		void SetOnSceneLoadedCallback(std::function<void(std::shared_ptr<Scene>)> callback) { m_OnSceneLoaded = callback; }

		// Statistics
		size_t GetSceneCount() const { return m_Scenes.size(); }
		std::vector<std::string> GetSceneNames() const;

	private:
		SceneManager() = default;
		~SceneManager() = default;
		SceneManager(const SceneManager&) = delete;
		SceneManager& operator=(const SceneManager&) = delete;

		void ProcessPendingSceneChange();

	private:
		std::unordered_map<std::string, std::shared_ptr<Scene>> m_Scenes;
		std::shared_ptr<Scene> m_ActiveScene;
		
		// Scene transition handling
		std::string m_PendingScene;
		bool m_IsTransitioning = false;

		// Callbacks
		SceneTransitionCallback m_OnSceneChange;
		std::function<void(std::shared_ptr<Scene>)> m_OnSceneLoaded;

		static std::string s_EmptyString;
	};

} // namespace Pillar
