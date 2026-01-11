#include "AnimationLibraryManager.h"
#include "Pillar/ECS/Systems/AnimationSystem.h"
#include "Pillar/Utils/AnimationLoader.h"
#include "Pillar/Logger.h"
#include "../Panels/ConsolePanel.h"

#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>

namespace PillarEditor {

	AnimationLibraryManager::AnimationLibraryManager()
	{
		// Default to assets/animations/ directory
		m_AnimationsDirectory = "assets/animations";
	}

	void AnimationLibraryManager::Initialize(Pillar::AnimationSystem* animSystem)
	{
		m_AnimSystem = animSystem;
		
		if (!m_AnimSystem)
		{
			PIL_CORE_ERROR("AnimationLibraryManager: Cannot initialize with null AnimationSystem");
			return;
		}

		// Auto-scan and load clips on initialization
		if (std::filesystem::exists(m_AnimationsDirectory))
		{
			ScanForClips(m_AnimationsDirectory);
			size_t loadedCount = LoadAllClips();
			
			PIL_CORE_INFO("AnimationLibraryManager initialized: {0} clips loaded from {1}", 
				loadedCount, m_AnimationsDirectory.string());
			
			ConsolePanel::Log("Animation Library: Loaded " + std::to_string(loadedCount) + 
				" clips from " + m_AnimationsDirectory.string(), LogLevel::Info);
		}
		else
		{
			PIL_CORE_WARN("Animation directory not found: {0}", m_AnimationsDirectory.string());
			ConsolePanel::Log("Animation directory not found: " + m_AnimationsDirectory.string() + 
				" - Create this folder to store .anim.json files", LogLevel::Warn);
		}
	}

	void AnimationLibraryManager::ScanForClips(const std::filesystem::path& directory)
	{
		m_ClipFiles.clear();

		if (!std::filesystem::exists(directory))
		{
			PIL_CORE_WARN("Cannot scan directory (does not exist): {0}", directory.string());
			return;
		}

		ScanDirectoryRecursive(directory);

		PIL_CORE_INFO("Found {0} animation files in {1}", m_ClipFiles.size(), directory.string());
	}

	void AnimationLibraryManager::ScanDirectoryRecursive(const std::filesystem::path& directory)
	{
		try
		{
			for (const auto& entry : std::filesystem::directory_iterator(directory))
			{
				if (entry.is_directory())
				{
					// Recursively scan subdirectories
					ScanDirectoryRecursive(entry.path());
				}
				else if (entry.is_regular_file() && IsAnimationFile(entry.path()))
				{
					m_ClipFiles.push_back(entry.path());
				}
			}
		}
		catch (const std::filesystem::filesystem_error& e)
		{
			PIL_CORE_ERROR("Error scanning directory {0}: {1}", directory.string(), e.what());
		}
	}

	size_t AnimationLibraryManager::LoadAllClips()
	{
		if (!m_AnimSystem)
		{
			PIL_CORE_ERROR("Cannot load clips: AnimationSystem not initialized");
			return 0;
		}

		size_t successCount = 0;
		size_t failCount = 0;

		// First, get the list of clips already loaded (to avoid duplicates)
		auto& existingClips = m_AnimSystem->GetAllClips();

		for (const auto& filepath : m_ClipFiles)
		{
			if (m_AnimSystem->LoadAnimationClip(filepath.string()))
			{
				successCount++;
			}
			else
			{
				failCount++;
				ConsolePanel::Log("Failed to load animation: " + filepath.string(), LogLevel::Error);
			}
		}

		// Build mapping of clip names to file paths
		// Compare before and after to find newly loaded clips
		auto& currentClips = m_AnimSystem->GetAllClips();
		for (const auto& [clipName, clip] : currentClips)
		{
			if (existingClips.find(clipName) == existingClips.end())
			{
				// This is a newly loaded clip, find its source file
				for (const auto& filepath : m_ClipFiles)
				{
					// Quick check: read just the name field from JSON
					try
					{
						std::ifstream file(filepath.string());
						if (file.is_open())
						{
							nlohmann::json j;
							file >> j;
							if (j.contains("name") && j["name"].get<std::string>() == clipName)
							{
								m_ClipNameToFilePath[clipName] = filepath.string();
								break;
							}
						}
					}
					catch (...) { }
				}
			}
		}

		if (failCount > 0)
		{
			PIL_CORE_WARN("AnimationLibraryManager: {0} clips loaded, {1} failed", successCount, failCount);
		}

		return successCount;
	}

	std::string AnimationLibraryManager::GetClipFilePath(const std::string& clipName) const
	{
		auto it = m_ClipNameToFilePath.find(clipName);
		if (it != m_ClipNameToFilePath.end())
		{
			return it->second;
		}
		return "";
	}

	bool AnimationLibraryManager::ReloadClip(const std::filesystem::path& filepath)
	{
		if (!m_AnimSystem)
		{
			PIL_CORE_ERROR("Cannot reload clip: AnimationSystem not initialized");
			return false;
		}

		if (!std::filesystem::exists(filepath))
		{
			PIL_CORE_ERROR("Cannot reload clip: File not found: {0}", filepath.string());
			return false;
		}

		// Read clip name from JSON
		std::string clipName;
		try
		{
			std::ifstream file(filepath.string());
			if (file.is_open())
			{
				nlohmann::json j;
				file >> j;
				if (j.contains("name"))
				{
					clipName = j["name"].get<std::string>();
				}
			}
		}
		catch (...) { }

		// Reload the clip (will replace existing one with same name)
		bool success = m_AnimSystem->LoadAnimationClip(filepath.string());
		
		if (success && !clipName.empty())
		{
			// Update filepath mapping
			m_ClipNameToFilePath[clipName] = filepath.string();
			
			PIL_CORE_INFO("Hot-reloaded animation clip: {0}", filepath.string());
			ConsolePanel::Log("Reloaded animation: " + filepath.filename().string(), LogLevel::Info);
		}

		return success;
	}

	std::vector<std::string> AnimationLibraryManager::GetAllClipNames() const
	{
		std::vector<std::string> names;

		if (!m_AnimSystem)
			return names;

		const auto& clips = m_AnimSystem->GetAllClips();
		names.reserve(clips.size());

		for (const auto& [name, clip] : clips)
		{
			names.push_back(name);
		}

		// Sort alphabetically for consistent UI display
		std::sort(names.begin(), names.end());

		return names;
	}

	Pillar::AnimationClip* AnimationLibraryManager::GetClip(const std::string& name) const
	{
		if (!m_AnimSystem)
			return nullptr;
		
		return m_AnimSystem->GetClip(name);
	}

	void AnimationLibraryManager::SetAnimationsDirectory(const std::filesystem::path& directory)
	{
		m_AnimationsDirectory = directory;
		PIL_CORE_INFO("Animation directory set to: {0}", directory.string());
	}

	bool AnimationLibraryManager::IsAnimationFile(const std::filesystem::path& filepath)
	{
		// Check for .anim.json extension
		std::string extension = filepath.extension().string();
		std::string filename = filepath.filename().string();

		// Must have .json extension
		if (extension != ".json")
			return false;

		// Must end with .anim.json
		if (filename.size() >= 10)  // ".anim.json" is 10 characters
		{
			std::string suffix = filename.substr(filename.size() - 10);
			return suffix == ".anim.json";
		}

		return false;
	}

	void AnimationLibraryManager::Update()
	{
		if (!m_FileWatchingEnabled || !m_AnimSystem)
			return;

		// Check files periodically, not every frame
		m_FileCheckTimer += 0.016f;  // Approximate frame time
		if (m_FileCheckTimer >= m_FileCheckInterval)
		{
			m_FileCheckTimer = 0.0f;
			CheckForFileChanges();
		}
	}

	void AnimationLibraryManager::CheckForFileChanges()
	{
		if (!std::filesystem::exists(m_AnimationsDirectory))
			return;

		bool anyChanges = false;

		for (const auto& filepath : m_ClipFiles)
		{
			if (!std::filesystem::exists(filepath))
			{
				// File was deleted - could handle this case
				continue;
			}

			try
			{
				auto lastWriteTime = std::filesystem::last_write_time(filepath);
				std::string pathStr = filepath.string();

				// Check if we have a cached write time
				auto it = m_LastWriteTimes.find(pathStr);
				if (it == m_LastWriteTimes.end())
				{
					// First time seeing this file, just cache it
					m_LastWriteTimes[pathStr] = lastWriteTime;
				}
				else if (it->second != lastWriteTime)
				{
					// File was modified - hot-reload it
					PIL_CORE_INFO("Detected change in animation file: {}", pathStr);
					
					if (ReloadClip(filepath))
					{
						ConsolePanel::Log("Hot-reloaded animation: " + filepath.filename().string(), LogLevel::Info);
						anyChanges = true;
					}

					// Update cached write time
					m_LastWriteTimes[pathStr] = lastWriteTime;
				}
			}
			catch (const std::filesystem::filesystem_error& e)
			{
				PIL_CORE_ERROR("Error checking file {}: {}", filepath.string(), e.what());
			}
		}

		// Also check for new files that weren't in our list
		ScanDirectoryRecursive(m_AnimationsDirectory);
		
		// Check if any new files were added
		for (const auto& filepath : m_ClipFiles)
		{
			std::string pathStr = filepath.string();
			if (m_LastWriteTimes.find(pathStr) == m_LastWriteTimes.end())
			{
				// New file discovered
				PIL_CORE_INFO("Discovered new animation file: {}", pathStr);
				
				if (m_AnimSystem->LoadAnimationClip(pathStr))
				{
					ConsolePanel::Log("Loaded new animation: " + filepath.filename().string(), LogLevel::Info);
					anyChanges = true;
				}

				// Cache the write time
				try
				{
					m_LastWriteTimes[pathStr] = std::filesystem::last_write_time(filepath);
				}
				catch (...) { }
			}
		}

		if (anyChanges)
		{
			PIL_CORE_INFO("Animation library updated");
		}
	}
		// 
		// TODO: Implement file system watcher (std::filesystem or platform-specific API)
		// - Watch m_AnimationsDirectory for file modifications
		// - Call ReloadClip() when .anim.json files change
		// - Throttle updates (e.g., max once per 500ms per file)

} // namespace PillarEditor
