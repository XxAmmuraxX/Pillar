#pragma once

#include "Pillar/ECS/Components/Rendering/AnimationClip.h"
#include "Pillar/Renderer/Texture.h"
#include <memory>
#include <string>
#include <vector>

namespace Pillar {
	class AnimationSystem;
	class Scene;
}

namespace PillarEditor {

	class AnimationLibraryManager;

	/**
	 * @brief Visual timeline editor for creating and editing animation clips
	 * 
	 * Features:
	 * - Timeline view with frame thumbnails
	 * - Frame duration editing (visual handles)
	 * - Clip properties editor (name, loop, speed)
	 * - Animation preview with playback controls
	 * - Event marker system
	 * - Integration with sprite sheet editor and inspector
	 */
	class AnimationEditorPanel
	{
	public:
		AnimationEditorPanel();
		~AnimationEditorPanel() = default;

		/**
		 * @brief Initialize the panel
		 * @param animSystem Pointer to engine's AnimationSystem
		 * @param libraryManager Pointer to AnimationLibraryManager
		 */
		void Initialize(Pillar::AnimationSystem* animSystem, AnimationLibraryManager* libraryManager);

		/**
		 * @brief Render the panel UI
		 */
		void OnImGuiRender();

		/**
		 * @brief Update method (for animation preview)
		 * @param dt Delta time
		 */
		void Update(float dt);

		/**
		 * @brief Set panel visibility
		 */
		void SetVisible(bool visible) { m_Visible = visible; }
		bool IsVisible() const { return m_Visible; }

		/**
		 * @brief Open a specific animation clip in the editor
		 * @param clipName Name of the clip to edit
		 */
		void OpenClip(const std::string& clipName);

		/**
		 * @brief Create new animation clip from frame library
		 * @param frames List of animation frames
		 * @param texture Shared texture for all frames
		 * @param texturePath Path to texture file
		 */
		void CreateFromFrames(
			const std::vector<Pillar::AnimationFrame>& frames,
			std::shared_ptr<Pillar::Texture2D> texture,
			const std::string& texturePath
		);

		/**
		 * @brief Create a new empty animation clip
		 */
		void CreateNewClip();

		/**
		 * @brief Save current clip to file
		 */
		bool SaveClip();

		/**
		 * @brief Save current clip with new name
		 */
		bool SaveClipAs(const std::string& filepath);

		// Undo/Redo command interface (public for command implementations)
		struct EditorCommand
		{
			virtual ~EditorCommand() = default;
			virtual void Execute() = 0;
			virtual void Undo() = 0;
			virtual std::string GetDescription() const = 0;
		};

	private:
		// State
		bool m_Visible = false;
		Pillar::AnimationSystem* m_AnimSystem = nullptr;
		AnimationLibraryManager* m_LibraryManager = nullptr;

		// Current clip being edited
		Pillar::AnimationClip m_CurrentClip;
		std::string m_CurrentClipFilePath;
		bool m_ClipModified = false;
		bool m_HasClipLoaded = false;

		// Preview state
		bool m_PreviewPlaying = false;
		int m_PreviewFrame = 0;
		float m_PreviewTime = 0.0f;
		std::shared_ptr<Pillar::Texture2D> m_PreviewTexture;

		// Timeline state
		float m_TimelineZoom = 100.0f;  // Pixels per second
		float m_TimelineScroll = 0.0f;
		int m_SelectedFrameIndex = -1;

		// UI Layout
		float m_LeftPanelWidth = 250.0f;
		float m_TimelineHeight = 150.0f;
		float m_PreviewPanelHeight = 300.0f;

		// Undo/Redo system
		std::vector<std::unique_ptr<EditorCommand>> m_UndoStack;
		std::vector<std::unique_ptr<EditorCommand>> m_RedoStack;
		static constexpr int MAX_UNDO_HISTORY = 50;

		void ExecuteCommand(std::unique_ptr<EditorCommand> command);
		void Undo();
		void Redo();
		void ClearUndoHistory();

		// UI Rendering Methods
		void RenderToolbar();
		void RenderClipProperties();
		void RenderClipLibrary();
		void RenderTimeline();
		void RenderPreviewControls();
		void RenderPreviewViewport();
		void RenderFrameProperties();

		// Timeline interaction
		void HandleTimelineInput();
		void HandleKeyboardInput();
		void DrawFrame(int frameIndex, float posX, float posY, float width, float height);
		void DrawTimelineRuler(float startX, float width, float height);

		// Frame operations
		void AddFrame();
		void DeleteFrame(int index);
		void DeleteSelectedFrame();
		void DuplicateFrame(int index);
		void MoveFrame(int fromIndex, int toIndex);

		// Preview update
		void UpdatePreview(float dt);
		void AdvancePreviewFrame();

		// File operations
		std::string GenerateDefaultClipPath();  // Note: non-const, modifies m_CurrentClip.Name
		void MarkModified();
		bool PromptUnsavedChanges();

		// Validation
		bool ValidateClip() const;
	};

} // namespace PillarEditor
