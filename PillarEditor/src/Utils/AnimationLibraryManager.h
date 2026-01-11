#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <functional>
#include <unordered_map>

namespace Pillar {
	class AnimationSystem;
	struct AnimationClip;
}

namespace PillarEditor {

	/**
	 * @brief Manages the animation clip library in the editor
	 * 
	 * Responsibilities:
	 * - Auto-scan assets/animations/ folder for .anim.json files
	 * - Load all animation clips on editor startup
	 * - Watch for file changes and hot-reload clips
	 * - Provide clip discovery and query functionality
	 */
	class AnimationLibraryManager
	{
	public:
		AnimationLibraryManager();
		~AnimationLibraryManager() = default;

		/**
		 * @brief Initialize the library manager with an animation system
		 * @param animSystem Pointer to the engine's AnimationSystem
		 */
		void Initialize(Pillar::AnimationSystem* animSystem);

		/**
		 * @brief Scan a directory for .anim.json files
		 * @param directory Path to scan (defaults to assets/animations/)
		 */
		void ScanForClips(const std::filesystem::path& directory);

		/**
		 * @brief Load all discovered animation clips
		 * @return Number of clips successfully loaded
		 */
		size_t LoadAllClips();

		/**
		 * @brief Reload a specific animation clip from disk
		 * @param filepath Path to the .anim.json file
		 * @return True if reloaded successfully
		 */
		bool ReloadClip(const std::filesystem::path& filepath);

		/**
		 * @brief Get all discovered clip file paths
		 */
		const std::vector<std::filesystem::path>& GetClipFiles() const { return m_ClipFiles; }

		/**
		 * @brief Get names of all loaded clips
		 */
		std::vector<std::string> GetAllClipNames() const;

		/**
		 * @brief Get a clip by name (delegates to AnimationSystem)
		 * @param name Name of the clip
		 * @return Pointer to clip, or nullptr if not found
		 */
		Pillar::AnimationClip* GetClip(const std::string& name) const;

		/**
		 * @brief Set the directory to scan for animations
		 * @param directory Path to animations directory
		 */
		void SetAnimationsDirectory(const std::filesystem::path& directory);

		/**
		 * @brief Get the current animations directory
		 */
		const std::filesystem::path& GetAnimationsDirectory() const { return m_AnimationsDirectory; }

		/**
		 * @brief Check if a file is an animation clip
		 * @param filepath Path to check
		 */
		static bool IsAnimationFile(const std::filesystem::path& filepath);

		/**
		 * @brief Update method for checking file changes (called each frame)
		 */
		void Update();

		/**
		 * @brief Enable/disable file watching for hot-reload
		 */
		void SetFileWatchingEnabled(bool enabled) { m_FileWatchingEnabled = enabled; }
		bool IsFileWatchingEnabled() const { return m_FileWatchingEnabled; }

	public:
		/**
		 * @brief Get the filepath for a clip by name
		 * @param clipName Name of the clip
		 * @return Filepath to the .anim.json file, or empty string if not found
		 */
		std::string GetClipFilePath(const std::string& clipName) const;

	private:
		Pillar::AnimationSystem* m_AnimSystem = nullptr;
		std::filesystem::path m_AnimationsDirectory;
		std::vector<std::filesystem::path> m_ClipFiles;

		// Mapping of clip name to file path (for tracking which file a clip came from)
		std::unordered_map<std::string, std::string> m_ClipNameToFilePath;

		// File watching for hot-reload
		bool m_FileWatchingEnabled = true;
		float m_FileCheckTimer = 0.0f;
		float m_FileCheckInterval = 1.0f;  // Check files every 1 second
		std::unordered_map<std::string, std::filesystem::file_time_type> m_LastWriteTimes;

		/**
		 * @brief Recursively scan directory for animation files
		 */
		void ScanDirectoryRecursive(const std::filesystem::path& directory);

		/**
		 * @brief Check for file changes and hot-reload modified clips
		 */
		void CheckForFileChanges();
	};

} // namespace PillarEditor
