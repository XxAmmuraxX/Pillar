#include "SceneManager.h"
#include "SceneSerializer.h"
#include "Pillar/Logger.h"

namespace Pillar {

	std::string SceneManager::s_EmptyString;

	SceneManager& SceneManager::Get()
	{
		static SceneManager instance;
		return instance;
	}

	std::shared_ptr<Scene> SceneManager::CreateScene(const std::string& name)
	{
		if (m_Scenes.find(name) != m_Scenes.end())
		{
			PIL_CORE_WARN("Scene '{}' already exists!", name);
			return m_Scenes[name];
		}

		auto scene = std::make_shared<Scene>(name);
		m_Scenes[name] = scene;
		PIL_CORE_INFO("Created scene '{}'", name);

		if (!m_ActiveScene)
		{
			m_ActiveScene = scene;
		}

		return scene;
	}

	std::shared_ptr<Scene> SceneManager::GetScene(const std::string& name)
	{
		auto it = m_Scenes.find(name);
		if (it != m_Scenes.end())
			return it->second;
		return nullptr;
	}

	bool SceneManager::HasScene(const std::string& name) const
	{
		return m_Scenes.find(name) != m_Scenes.end();
	}

	bool SceneManager::RemoveScene(const std::string& name)
	{
		auto it = m_Scenes.find(name);
		if (it == m_Scenes.end())
		{
			PIL_CORE_WARN("Scene '{}' not found!", name);
			return false;
		}

		if (m_ActiveScene == it->second)
		{
			PIL_CORE_WARN("Cannot remove active scene '{}'!", name);
			return false;
		}

		m_Scenes.erase(it);
		PIL_CORE_INFO("Removed scene '{}'", name);
		return true;
	}

	void SceneManager::Clear()
	{
		m_Scenes.clear();
		m_ActiveScene = nullptr;
		m_PendingScene.clear();
		m_IsTransitioning = false;
		m_OnSceneChange = nullptr;
		m_OnSceneLoaded = nullptr;
		PIL_CORE_INFO("Cleared all scenes");
	}

	bool SceneManager::SetActiveScene(const std::string& name)
	{
		auto scene = GetScene(name);
		if (!scene)
		{
			PIL_CORE_ERROR("Scene '{}' not found!", name);
			return false;
		}
		return SetActiveScene(scene);
	}

	bool SceneManager::SetActiveScene(std::shared_ptr<Scene> scene)
	{
		if (!scene)
		{
			PIL_CORE_ERROR("Cannot set null scene as active!");
			return false;
		}

		std::string oldSceneName = m_ActiveScene ? m_ActiveScene->GetName() : "";
		
		if (m_ActiveScene && m_ActiveScene->IsPlaying())
		{
			m_ActiveScene->OnRuntimeStop();
		}

		m_ActiveScene = scene;
		PIL_CORE_INFO("Active scene changed to '{}'", scene->GetName());

		if (m_OnSceneChange)
		{
			m_OnSceneChange(oldSceneName, scene->GetName());
		}

		return true;
	}

	const std::string& SceneManager::GetActiveSceneName() const
	{
		if (m_ActiveScene)
			return m_ActiveScene->GetName();
		return s_EmptyString;
	}

	void SceneManager::RequestSceneChange(const std::string& sceneName)
	{
		if (!HasScene(sceneName))
		{
			PIL_CORE_ERROR("Cannot transition to scene '{}' - not loaded!", sceneName);
			return;
		}

		m_PendingScene = sceneName;
		m_IsTransitioning = true;
		PIL_CORE_INFO("Scene change requested to '{}'", sceneName);
	}

	void SceneManager::LoadSceneAsync(const std::string& filepath, const std::string& sceneName)
	{
		// In a full implementation, this would use async loading
		// For now, we'll do synchronous loading
		LoadScene(filepath, sceneName);
	}

	void SceneManager::OnUpdate(float deltaTime)
	{
		// Handle pending scene changes at safe point in frame
		if (m_IsTransitioning)
		{
			ProcessPendingSceneChange();
		}

		// Update active scene
		if (m_ActiveScene)
		{
			m_ActiveScene->OnUpdate(deltaTime);
		}
	}

	void SceneManager::ProcessPendingSceneChange()
	{
		if (m_PendingScene.empty())
		{
			m_IsTransitioning = false;
			return;
		}

		SetActiveScene(m_PendingScene);
		m_PendingScene.clear();
		m_IsTransitioning = false;
	}

	bool SceneManager::LoadScene(const std::string& filepath, const std::string& sceneName)
	{
		std::string name = sceneName.empty() ? filepath : sceneName;
		
		auto scene = CreateScene(name);
		SceneSerializer serializer(scene.get());
		
		if (!serializer.Deserialize(filepath))
		{
			RemoveScene(name);
			return false;
		}

		scene->SetFilePath(filepath);
		
		if (m_OnSceneLoaded)
		{
			m_OnSceneLoaded(scene);
		}

		PIL_CORE_INFO("Loaded scene '{}' from '{}'", name, filepath);
		return true;
	}

	bool SceneManager::SaveScene(const std::string& filepath)
	{
		if (!m_ActiveScene)
		{
			PIL_CORE_ERROR("No active scene to save!");
			return false;
		}
		return SaveScene(m_ActiveScene, filepath);
	}

	bool SceneManager::SaveScene(std::shared_ptr<Scene> scene, const std::string& filepath)
	{
		SceneSerializer serializer(scene.get());
		bool success = serializer.Serialize(filepath);
		
		if (success)
		{
			scene->SetFilePath(filepath);
			PIL_CORE_INFO("Saved scene '{}' to '{}'", scene->GetName(), filepath);
		}
		
		return success;
	}

	std::vector<std::string> SceneManager::GetSceneNames() const
	{
		std::vector<std::string> names;
		names.reserve(m_Scenes.size());
		for (const auto& [name, scene] : m_Scenes)
		{
			names.push_back(name);
		}
		return names;
	}

} // namespace Pillar
