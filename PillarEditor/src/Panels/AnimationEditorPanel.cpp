#include "AnimationEditorPanel.h"
#include "ConsolePanel.h"
#include "../Utils/AnimationLibraryManager.h"
#include "Pillar/ECS/Systems/AnimationSystem.h"
#include "Pillar/Utils/AnimationLoader.h"
#include "Pillar/Logger.h"

#include <imgui.h>
#include <glm/glm.hpp>
#include <filesystem>

namespace PillarEditor {

	// ============================================================================
	// Undo/Redo Command Implementations
	// ============================================================================

	class AddFrameCommand : public AnimationEditorPanel::EditorCommand
	{
	public:
		AddFrameCommand(Pillar::AnimationClip& clip, int index, const Pillar::AnimationFrame& frame)
			: m_Clip(clip), m_Index(index), m_Frame(frame) {}

		void Execute() override
		{
			if (m_Index >= 0 && m_Index <= m_Clip.Frames.size())
				m_Clip.Frames.insert(m_Clip.Frames.begin() + m_Index, m_Frame);
		}

		void Undo() override
		{
			if (m_Index >= 0 && m_Index < m_Clip.Frames.size())
				m_Clip.Frames.erase(m_Clip.Frames.begin() + m_Index);
		}

		std::string GetDescription() const override { return "Add Frame"; }

	private:
		Pillar::AnimationClip& m_Clip;
		int m_Index;
		Pillar::AnimationFrame m_Frame;
	};

	class DeleteFrameCommand : public AnimationEditorPanel::EditorCommand
	{
	public:
		DeleteFrameCommand(Pillar::AnimationClip& clip, int index)
			: m_Clip(clip), m_Index(index)
		{
			if (index >= 0 && index < m_Clip.Frames.size())
				m_Frame = m_Clip.Frames[index];
		}

		void Execute() override
		{
			if (m_Index >= 0 && m_Index < m_Clip.Frames.size())
				m_Clip.Frames.erase(m_Clip.Frames.begin() + m_Index);
		}

		void Undo() override
		{
			if (m_Index >= 0 && m_Index <= m_Clip.Frames.size())
				m_Clip.Frames.insert(m_Clip.Frames.begin() + m_Index, m_Frame);
		}

		std::string GetDescription() const override { return "Delete Frame"; }

	private:
		Pillar::AnimationClip& m_Clip;
		int m_Index;
		Pillar::AnimationFrame m_Frame;
	};

	class MoveFrameCommand : public AnimationEditorPanel::EditorCommand
	{
	public:
		MoveFrameCommand(Pillar::AnimationClip& clip, int fromIndex, int toIndex)
			: m_Clip(clip), m_FromIndex(fromIndex), m_ToIndex(toIndex) {}

		void Execute() override
		{
			if (m_FromIndex >= 0 && m_FromIndex < m_Clip.Frames.size() &&
				m_ToIndex >= 0 && m_ToIndex < m_Clip.Frames.size())
			{
				Pillar::AnimationFrame frame = m_Clip.Frames[m_FromIndex];
				m_Clip.Frames.erase(m_Clip.Frames.begin() + m_FromIndex);
				int adjustedTo = (m_FromIndex < m_ToIndex) ? m_ToIndex - 1 : m_ToIndex;
				m_Clip.Frames.insert(m_Clip.Frames.begin() + adjustedTo, frame);
			}
		}

		void Undo() override
		{
			// Undo by moving back
			if (m_FromIndex >= 0 && m_FromIndex < m_Clip.Frames.size() &&
				m_ToIndex >= 0 && m_ToIndex < m_Clip.Frames.size())
			{
				int adjustedTo = (m_FromIndex < m_ToIndex) ? m_ToIndex - 1 : m_ToIndex;
				Pillar::AnimationFrame frame = m_Clip.Frames[adjustedTo];
				m_Clip.Frames.erase(m_Clip.Frames.begin() + adjustedTo);
				m_Clip.Frames.insert(m_Clip.Frames.begin() + m_FromIndex, frame);
			}
		}

		std::string GetDescription() const override { return "Move Frame"; }

	private:
		Pillar::AnimationClip& m_Clip;
		int m_FromIndex;
		int m_ToIndex;
	};

	class ModifyFrameDurationCommand : public AnimationEditorPanel::EditorCommand
	{
	public:
		ModifyFrameDurationCommand(Pillar::AnimationClip& clip, int index, float oldDuration, float newDuration)
			: m_Clip(clip), m_Index(index), m_OldDuration(oldDuration), m_NewDuration(newDuration) {}

		void Execute() override
		{
			if (m_Index >= 0 && m_Index < m_Clip.Frames.size())
				m_Clip.Frames[m_Index].Duration = m_NewDuration;
		}

		void Undo() override
		{
			if (m_Index >= 0 && m_Index < m_Clip.Frames.size())
				m_Clip.Frames[m_Index].Duration = m_OldDuration;
		}

		std::string GetDescription() const override { return "Modify Frame Duration"; }

	private:
		Pillar::AnimationClip& m_Clip;
		int m_Index;
		float m_OldDuration;
		float m_NewDuration;
	};

	// ============================================================================
	// AnimationEditorPanel Implementation
	// ============================================================================

	AnimationEditorPanel::AnimationEditorPanel()
		: m_Visible(true)  // Visible by default
	{
		PIL_CORE_INFO("AnimationEditorPanel created");
	}

	void AnimationEditorPanel::Initialize(Pillar::AnimationSystem* animSystem, AnimationLibraryManager* libraryManager)
	{
		m_AnimSystem = animSystem;
		m_LibraryManager = libraryManager;

		if (!m_AnimSystem)
		{
			PIL_CORE_ERROR("AnimationEditorPanel: Cannot initialize with null AnimationSystem");
		}

		if (!m_LibraryManager)
		{
			PIL_CORE_ERROR("AnimationEditorPanel: Cannot initialize with null AnimationLibraryManager");
		}

		PIL_CORE_INFO("AnimationEditorPanel initialized");
	}

	void AnimationEditorPanel::OnImGuiRender()
	{
		if (!m_Visible)
			return;

		ImGui::SetNextWindowSize(ImVec2(1200, 700), ImGuiCond_FirstUseEver);
		
		if (ImGui::Begin("Animation Editor", &m_Visible))
		{
			// Handle keyboard input
			HandleKeyboardInput();

			// Toolbar at top
			RenderToolbar();
			
			ImGui::Separator();
			
			// Main content area - horizontal split
			ImGui::BeginChild("MainContent", ImVec2(0, 0), false);
			{
				// LEFT PANEL - Properties & Library (fixed width)
				ImGui::BeginChild("LeftPanel", ImVec2(m_LeftPanelWidth, 0), true);
				{
					RenderClipProperties();
					ImGui::Separator();
					RenderClipLibrary();
				}
				ImGui::EndChild();
				
				ImGui::SameLine();
				
				// RIGHT PANEL - Timeline & Preview (takes remaining space)
				ImGui::BeginChild("RightPanel", ImVec2(0, 0), false);
				{
					// Timeline section
					ImGui::Text("Timeline");
					ImGui::BeginChild("Timeline", ImVec2(0, m_TimelineHeight), true);
					{
						RenderTimeline();
					}
					ImGui::EndChild();

					// Frame properties (when frame selected)
					if (m_SelectedFrameIndex >= 0 && m_SelectedFrameIndex < m_CurrentClip.Frames.size())
					{
						RenderFrameProperties();
					}
					
					ImGui::Spacing();
					
					// Preview controls
					RenderPreviewControls();
					
					ImGui::Spacing();
					
					// Preview viewport
					ImGui::Text("Preview");
					ImGui::BeginChild("Preview", ImVec2(0, 0), true);
					{
						RenderPreviewViewport();
					}
					ImGui::EndChild();
				}
				ImGui::EndChild();
			}
			ImGui::EndChild();
		}
		ImGui::End();
	}

	void AnimationEditorPanel::Update(float dt)
	{
		if (m_Visible && m_PreviewPlaying)
		{
			UpdatePreview(dt);
		}
	}

	void AnimationEditorPanel::OpenClip(const std::string& clipName)
	{
		if (!m_AnimSystem)
			return;

		// Check for unsaved changes
		if (m_ClipModified && !PromptUnsavedChanges())
		{
			return;
		}

		// Get clip from system
		Pillar::AnimationClip* clip = m_AnimSystem->GetClip(clipName);
		if (!clip)
		{
			ConsolePanel::Log("Animation clip not found: " + clipName, LogLevel::Error);
			return;
		}

		m_CurrentClip = *clip;
		m_HasClipLoaded = true;
		m_ClipModified = false;
		m_PreviewFrame = 0;
		m_PreviewTime = 0.0f;
		m_PreviewPlaying = false;
		m_SelectedFrameIndex = -1;

		// Get the source filepath from AnimationLibraryManager
		if (m_LibraryManager)
		{
			m_CurrentClipFilePath = m_LibraryManager->GetClipFilePath(clipName);
			if (m_CurrentClipFilePath.empty())
			{
				PIL_CORE_WARN("Could not find source file for clip: {0}", clipName);
			}
		}

		// Clear undo history when loading new clip
		ClearUndoHistory();

		// Load texture if available
		if (!m_CurrentClip.Frames.empty() && !m_CurrentClip.Frames[0].TexturePath.empty())
		{
			m_PreviewTexture = Pillar::Texture2D::Create(m_CurrentClip.Frames[0].TexturePath);
		}

		m_Visible = true;
		ConsolePanel::Log("Opened animation clip: " + clipName, LogLevel::Info);
	}

	void AnimationEditorPanel::CreateFromFrames(
		const std::vector<Pillar::AnimationFrame>& frames,
		std::shared_ptr<Pillar::Texture2D> texture,
		const std::string& texturePath)
	{
		// Check for unsaved changes
		if (m_ClipModified && !PromptUnsavedChanges())
		{
			return;
		}

		// Create new clip from provided frames
		m_CurrentClip = Pillar::AnimationClip();
		m_CurrentClip.Name = "NewAnimation";
		m_CurrentClip.Loop = true;
		m_CurrentClip.PlaybackSpeed = 1.0f;
		m_CurrentClip.Frames = frames;

		m_PreviewTexture = texture;
		m_HasClipLoaded = true;
		m_ClipModified = true;
		m_PreviewFrame = 0;
		m_PreviewTime = 0.0f;
		m_PreviewPlaying = false;
		m_SelectedFrameIndex = -1;

		// Clear undo history when creating new clip
		ClearUndoHistory();

		m_Visible = true;
		ConsolePanel::Log("Created new animation from " + std::to_string(frames.size()) + " frames", LogLevel::Info);
	}

	void AnimationEditorPanel::CreateNewClip()
	{
		// Check for unsaved changes
		if (m_ClipModified && !PromptUnsavedChanges())
		{
			return;
		}

		// Create empty clip
		m_CurrentClip = Pillar::AnimationClip();
		m_CurrentClip.Name = "NewAnimation";
		m_CurrentClip.Loop = true;
		m_CurrentClip.PlaybackSpeed = 1.0f;

		m_PreviewTexture.reset();
		m_HasClipLoaded = true;
		m_ClipModified = true;
		m_PreviewFrame = 0;
		m_PreviewTime = 0.0f;
		m_PreviewPlaying = false;
		m_SelectedFrameIndex = -1;

		// Clear undo history when creating new clip
		ClearUndoHistory();

		m_Visible = true;
		ConsolePanel::Log("Created new empty animation clip", LogLevel::Info);
	}

	bool AnimationEditorPanel::SaveClip()
	{
		if (!ValidateClip())
		{
			ConsolePanel::Log("Cannot save invalid animation clip", LogLevel::Error);
			return false;
		}

		// If no filepath, generate one for new clips
		// For existing clips (loaded via OpenClip), m_CurrentClipFilePath is already set
		if (m_CurrentClipFilePath.empty())
		{
			m_CurrentClipFilePath = GenerateDefaultClipPath();
			PIL_CORE_INFO("Generated new animation file: {0}", m_CurrentClipFilePath);
		}
		else
		{
			PIL_CORE_INFO("Overwriting existing animation file: {0}", m_CurrentClipFilePath);
		}

		// Save to file
		bool success = Pillar::AnimationLoader::SaveToJSON(m_CurrentClip, m_CurrentClipFilePath);
		
		if (success)
		{
			m_ClipModified = false;
			
			// Reload in animation system
			if (m_AnimSystem)
			{
				m_AnimSystem->LoadAnimationClip(m_CurrentClipFilePath);
			}
			
			ConsolePanel::Log("Saved animation: " + m_CurrentClipFilePath, LogLevel::Info);
		}
		else
		{
			ConsolePanel::Log("Failed to save animation: " + m_CurrentClipFilePath, LogLevel::Error);
		}

		return success;
	}

	bool AnimationEditorPanel::SaveClipAs(const std::string& filepath)
	{
		m_CurrentClipFilePath = filepath;
		return SaveClip();
	}

	// ============================================================================
	// UI Rendering Methods
	// ============================================================================

	void AnimationEditorPanel::RenderToolbar()
	{
		if (ImGui::Button("New"))
		{
			CreateNewClip();
		}
		
		ImGui::SameLine();
		if (ImGui::Button("Open"))
		{
			// TODO: File dialog to select .anim.json file
			ConsolePanel::Log("Open dialog not implemented yet - use Clip Library below", LogLevel::Info);
		}
		
		ImGui::SameLine();
		if (ImGui::Button("Save"))
		{
			SaveClip();
		}
		
		ImGui::SameLine();
		if (ImGui::Button("Save As"))
		{
			// TODO: File dialog to specify new filename
			ConsolePanel::Log("Save As dialog not implemented yet", LogLevel::Info);
		}

		ImGui::SameLine();
		ImGui::Spacing();
		ImGui::SameLine();

		// Undo/Redo buttons
		ImGui::BeginDisabled(m_UndoStack.empty());
		if (ImGui::Button("Undo"))
		{
			Undo();
		}
		ImGui::EndDisabled();

		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			if (!m_UndoStack.empty())
				ImGui::SetTooltip("Undo: %s (Ctrl+Z)", m_UndoStack.back()->GetDescription().c_str());
			else
				ImGui::SetTooltip("Nothing to undo");
		}

		ImGui::SameLine();
		ImGui::BeginDisabled(m_RedoStack.empty());
		if (ImGui::Button("Redo"))
		{
			Redo();
		}
		ImGui::EndDisabled();

		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			if (!m_RedoStack.empty())
				ImGui::SetTooltip("Redo: %s (Ctrl+Y)", m_RedoStack.back()->GetDescription().c_str());
			else
				ImGui::SetTooltip("Nothing to redo");
		}

		ImGui::SameLine();
		ImGui::Spacing();
		ImGui::SameLine();

		// Frame management buttons
		if (ImGui::Button("Add Frame"))
		{
			AddFrame();
		}
		ImGui::SameLine();

		bool hasSelection = m_SelectedFrameIndex >= 0 && m_SelectedFrameIndex < m_CurrentClip.Frames.size();
		ImGui::BeginDisabled(!hasSelection);
		if (ImGui::Button("Delete Frame"))
		{
			DeleteSelectedFrame();
		}
		ImGui::EndDisabled();

		ImGui::SameLine();
		ImGui::Spacing();
		ImGui::SameLine();

		// Show current clip name
		ImGui::Text("Clip:");
		ImGui::SameLine();
		if (m_HasClipLoaded)
		{
			ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "%s", m_CurrentClip.Name.c_str());
			if (m_ClipModified)
			{
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "*");
			}
		}
		else
		{
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No clip loaded");
		}
	}

	void AnimationEditorPanel::RenderClipProperties()
	{
		ImGui::Text("Clip Properties");
		ImGui::Separator();
		ImGui::Spacing();

		if (!m_HasClipLoaded)
		{
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No clip loaded");
			return;
		}

		ImGui::PushItemWidth(-1); // Full width for inputs

		// Clip name
		ImGui::Text("Name");
		char nameBuf[256];
		strncpy_s(nameBuf, m_CurrentClip.Name.c_str(), sizeof(nameBuf) - 1);
		if (ImGui::InputText("##ClipName", nameBuf, sizeof(nameBuf)))
		{
			m_CurrentClip.Name = nameBuf;
			MarkModified();
		}

		ImGui::Spacing();

		// Loop setting
		if (ImGui::Checkbox("Loop", &m_CurrentClip.Loop))
		{
			MarkModified();
		}

		ImGui::Spacing();

		// Playback speed
		ImGui::Text("Playback Speed");
		if (ImGui::SliderFloat("##Speed", &m_CurrentClip.PlaybackSpeed, 0.1f, 5.0f, "%.2fx"))
		{
			MarkModified();
		}

		// Stats
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Text("Statistics");
		ImGui::Separator();
		ImGui::Spacing();
		
		ImGui::Columns(2, "ClipStats", false);
		ImGui::Text("Frames");
		ImGui::NextColumn();
		ImGui::Text("%d", m_CurrentClip.GetFrameCount());
		ImGui::NextColumn();
		
		ImGui::Text("Duration");
		ImGui::NextColumn();
		ImGui::Text("%.2fs", m_CurrentClip.GetDuration());
		ImGui::NextColumn();
		
		ImGui::Text("Events");
		ImGui::NextColumn();
		ImGui::Text("%d", (int)m_CurrentClip.Events.size());
		ImGui::Columns(1);

		ImGui::PopItemWidth();
	}

	void AnimationEditorPanel::RenderClipLibrary()
	{
		ImGui::Text("Clip Library");
		ImGui::Separator();

		if (!m_LibraryManager)
		{
			ImGui::TextColored(ImVec4(0.8f, 0.3f, 0.3f, 1.0f), "Library not initialized");
			return;
		}

		auto clipNames = m_LibraryManager->GetAllClipNames();

		if (clipNames.empty())
		{
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No clips found");
			ImGui::TextWrapped("Create clips in Sprite Sheet Editor or place .anim.json files in assets/animations/");
			return;
		}

		ImGui::BeginChild("ClipList", ImVec2(0, 0), true);
		{
			for (const auto& clipName : clipNames)
			{
				bool isCurrentClip = (m_HasClipLoaded && m_CurrentClip.Name == clipName);
				
				if (isCurrentClip)
				{
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
				}

				if (ImGui::Selectable(clipName.c_str(), isCurrentClip))
				{
					OpenClip(clipName);
				}

				if (isCurrentClip)
				{
					ImGui::PopStyleColor();
				}
			}
		}
		ImGui::EndChild();
	}

	void AnimationEditorPanel::RenderTimeline()
	{
		if (!m_HasClipLoaded || m_CurrentClip.Frames.empty())
		{
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No frames to display");
			ImGui::TextWrapped("Add frames by importing from Sprite Sheet Editor or dragging from Content Browser");
			return;
		}

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 canvasPos = ImGui::GetCursorScreenPos();
		ImVec2 canvasSize = ImGui::GetContentRegionAvail();

		// Draw background
		drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
			IM_COL32(30, 30, 30, 255));

		// Draw ruler
		DrawTimelineRuler(canvasPos.x, canvasSize.x, 30.0f);

		// Draw frames and handle interaction
		float currentX = canvasPos.x + 10.0f;
		float frameY = canvasPos.y + 40.0f;
		float frameHeight = canvasSize.y - 50.0f;

		for (int i = 0; i < m_CurrentClip.Frames.size(); ++i)
		{
			const auto& frame = m_CurrentClip.Frames[i];
			float frameWidth = frame.Duration * m_TimelineZoom;

			DrawFrame(i, currentX, frameY, frameWidth, frameHeight);

			// Create invisible button for clicking and drag-drop
			ImGui::SetCursorScreenPos(ImVec2(currentX, frameY));
			ImGui::PushID(i);
			ImGui::InvisibleButton("frameInteract", ImVec2(frameWidth, frameHeight));

			// Handle click to select
			if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
			{
				m_SelectedFrameIndex = i;
			}

			// Drag-drop source (for reordering)
			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
			{
				ImGui::SetDragDropPayload("ANIM_FRAME", &i, sizeof(int));
				ImGui::Text("Frame %d", i);
				ImGui::EndDragDropSource();
			}

			// Drag-drop target (for reordering)
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ANIM_FRAME"))
				{
					int sourceIndex = *(const int*)payload->Data;
					if (sourceIndex != i)
					{
						MoveFrame(sourceIndex, i);
					}
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::PopID();

			currentX += frameWidth + 5.0f;  // Gap between frames
		}

		// Draw playhead
		if (m_PreviewFrame >= 0 && m_PreviewFrame < m_CurrentClip.Frames.size())
		{
			float playheadX = canvasPos.x + 10.0f;
			for (int i = 0; i < m_PreviewFrame; ++i)
			{
				playheadX += m_CurrentClip.Frames[i].Duration * m_TimelineZoom + 5.0f;
			}
			playheadX += m_PreviewTime * m_TimelineZoom;

			drawList->AddLine(
				ImVec2(playheadX, canvasPos.y),
				ImVec2(playheadX, canvasPos.y + canvasSize.y),
				IM_COL32(255, 255, 0, 255), 2.0f
			);
		}

		ImGui::Dummy(canvasSize);  // Reserve space
	}

	void AnimationEditorPanel::RenderPreviewControls()
	{
		if (!m_HasClipLoaded)
			return;

		// Playback buttons
		if (ImGui::Button(m_PreviewPlaying ? "Pause" : "Play"))
		{
			m_PreviewPlaying = !m_PreviewPlaying;
		}

		ImGui::SameLine();
		if (ImGui::Button("Stop"))
		{
			m_PreviewPlaying = false;
			m_PreviewFrame = 0;
			m_PreviewTime = 0.0f;
		}

		ImGui::SameLine();
		if (ImGui::Button("<<"))
		{
			if (m_PreviewFrame > 0)
			{
				m_PreviewFrame--;
				m_PreviewTime = 0.0f;
			}
		}

		ImGui::SameLine();
		if (ImGui::Button(">>"))
		{
			if (m_PreviewFrame < m_CurrentClip.GetFrameCount() - 1)
			{
				m_PreviewFrame++;
				m_PreviewTime = 0.0f;
			}
		}

		ImGui::SameLine();
		ImGui::Text("Frame: %d / %d", m_PreviewFrame + 1, m_CurrentClip.GetFrameCount());
	}

	void AnimationEditorPanel::RenderPreviewViewport()
	{
		if (!m_HasClipLoaded || !m_PreviewTexture)
		{
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No preview available");
			return;
		}

		if (m_CurrentClip.Frames.empty() || m_PreviewFrame >= m_CurrentClip.Frames.size())
		{
			return;
		}

		// Get current frame
		const auto& frame = m_CurrentClip.Frames[m_PreviewFrame];

		// Calculate preview size (maintain aspect ratio)
		ImVec2 availSize = ImGui::GetContentRegionAvail();
		float texWidth = m_PreviewTexture->GetWidth();
		float texHeight = m_PreviewTexture->GetHeight();

		// Calculate UV rect size
		float uvWidth = frame.UVMax.x - frame.UVMin.x;
		float uvHeight = frame.UVMax.y - frame.UVMin.y;
		float framePixelWidth = texWidth * uvWidth;
		float framePixelHeight = texHeight * uvHeight;

		// Scale to fit in available space
		float scale = std::min(availSize.x / framePixelWidth, availSize.y / framePixelHeight);
		scale = std::min(scale, 4.0f);  // Max 4x zoom

		ImVec2 imageSize(framePixelWidth * scale, framePixelHeight * scale);

		// Center the image
		ImVec2 cursorPos = ImGui::GetCursorPos();
		cursorPos.x += (availSize.x - imageSize.x) * 0.5f;
		cursorPos.y += (availSize.y - imageSize.y) * 0.5f;
		ImGui::SetCursorPos(cursorPos);

		// Draw image with UV coordinates (flip V coordinate for OpenGL)
		ImGui::Image(
			(void*)(intptr_t)m_PreviewTexture->GetRendererID(),
			imageSize,
			ImVec2(frame.UVMin.x, frame.UVMax.y),
			ImVec2(frame.UVMax.x, frame.UVMin.y)
		);
	}

	// ============================================================================
	// Timeline Drawing
	// ============================================================================

	void AnimationEditorPanel::DrawFrame(int frameIndex, float posX, float posY, float width, float height)
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		bool isSelected = (frameIndex == m_SelectedFrameIndex);
		bool isCurrentFrame = (frameIndex == m_PreviewFrame);

		// Frame background
		ImU32 bgColor = isCurrentFrame ? IM_COL32(80, 80, 120, 255) :
			isSelected ? IM_COL32(70, 70, 100, 255) :
			IM_COL32(50, 50, 50, 255);

		drawList->AddRectFilled(
			ImVec2(posX, posY),
			ImVec2(posX + width, posY + height),
			bgColor
		);

		// Frame border
		ImU32 borderColor = isSelected ? IM_COL32(255, 255, 0, 255) : IM_COL32(100, 100, 100, 255);
		drawList->AddRect(
			ImVec2(posX, posY),
			ImVec2(posX + width, posY + height),
			borderColor, 0.0f, 0, 2.0f
		);

		// Frame number
		char labelBuf[32];
		snprintf(labelBuf, sizeof(labelBuf), "%d", frameIndex);
		drawList->AddText(ImVec2(posX + 5, posY + 5), IM_COL32(200, 200, 200, 255), labelBuf);

		// Frame duration
		const auto& frame = m_CurrentClip.Frames[frameIndex];
		snprintf(labelBuf, sizeof(labelBuf), "%.2fs", frame.Duration);
		drawList->AddText(ImVec2(posX + 5, posY + height - 20), IM_COL32(150, 150, 150, 255), labelBuf);
	}

	void AnimationEditorPanel::DrawTimelineRuler(float startX, float width, float height)
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 canvasPos = ImGui::GetCursorScreenPos();

		// Ruler background
		drawList->AddRectFilled(
			canvasPos,
			ImVec2(canvasPos.x + width, canvasPos.y + height),
			IM_COL32(40, 40, 40, 255)
		);

		// Time markers every 0.5 seconds
		float markerInterval = 0.5f;  // seconds
		float pixelInterval = markerInterval * m_TimelineZoom;

		for (float time = 0.0f; time < m_CurrentClip.GetDuration(); time += markerInterval)
		{
			float x = canvasPos.x + 10.0f + (time * m_TimelineZoom);

			drawList->AddLine(
				ImVec2(x, canvasPos.y + height - 10),
				ImVec2(x, canvasPos.y + height),
				IM_COL32(150, 150, 150, 255)
			);

			char timeBuf[16];
			snprintf(timeBuf, sizeof(timeBuf), "%.1fs", time);
			drawList->AddText(ImVec2(x + 2, canvasPos.y + 5), IM_COL32(200, 200, 200, 255), timeBuf);
		}
	}

	// ============================================================================
	// Preview Update
	// ============================================================================

	void AnimationEditorPanel::UpdatePreview(float dt)
	{
		if (!m_HasClipLoaded || m_CurrentClip.Frames.empty())
			return;

		if (m_PreviewFrame >= m_CurrentClip.Frames.size())
		{
			m_PreviewFrame = 0;
			m_PreviewTime = 0.0f;
		}

		const auto& currentFrame = m_CurrentClip.Frames[m_PreviewFrame];

		// Advance time
		m_PreviewTime += dt * m_CurrentClip.PlaybackSpeed;

		// Check if we need to advance frame
		if (m_PreviewTime >= currentFrame.Duration)
		{
			AdvancePreviewFrame();
		}
	}

	void AnimationEditorPanel::AdvancePreviewFrame()
	{
		m_PreviewTime = 0.0f;
		m_PreviewFrame++;

		// Handle looping
		if (m_PreviewFrame >= m_CurrentClip.GetFrameCount())
		{
			if (m_CurrentClip.Loop)
			{
				m_PreviewFrame = 0;
			}
			else
			{
				m_PreviewFrame = m_CurrentClip.GetFrameCount() - 1;
				m_PreviewPlaying = false;
			}
		}
	}

	// ============================================================================
	// Utility Methods
	// ============================================================================

	std::string AnimationEditorPanel::GenerateDefaultClipPath()
	{
		std::filesystem::path animDir = "assets/animations";
		
		// Create directory if it doesn't exist
		if (!std::filesystem::exists(animDir))
		{
			std::filesystem::create_directories(animDir);
		}

		// Generate unique filename
		std::string baseName = m_CurrentClip.Name;
		if (baseName.empty())
		{
			baseName = "animation";
		}

		std::filesystem::path filepath = animDir / (baseName + ".anim.json");

		// Add number suffix if file already exists
		int counter = 1;
		while (std::filesystem::exists(filepath))
		{
			filepath = animDir / (baseName + "_" + std::to_string(counter) + ".anim.json");
			counter++;
		}

		// IMPORTANT: Update the clip's internal name to match the filename with suffix
		// This ensures walk_cycle_1.anim.json has Name="walk_cycle_1" internally,
		// preventing collision with walk_cycle.anim.json in the animation library
		if (counter > 1)
		{
			m_CurrentClip.Name = baseName + "_" + std::to_string(counter - 1);
		}

		return filepath.string();
	}

	void AnimationEditorPanel::MarkModified()
	{
		m_ClipModified = true;
	}

	bool AnimationEditorPanel::PromptUnsavedChanges()
	{
		// TODO: Implement proper modal dialog
		// For now, just return true to proceed
		PIL_CORE_WARN("Unsaved changes in animation clip - proceeding anyway (no dialog implemented)");
		return true;
	}

	bool AnimationEditorPanel::ValidateClip() const
	{
		if (m_CurrentClip.Name.empty())
		{
			ConsolePanel::Log("Animation clip must have a name", LogLevel::Error);
			return false;
		}

		if (m_CurrentClip.Frames.empty())
		{
			ConsolePanel::Log("Animation clip must have at least one frame", LogLevel::Error);
			return false;
		}

		return true;
	}

	// ============================================================================
	// Frame Management
	// ============================================================================

	void AnimationEditorPanel::HandleKeyboardInput()
	{
		if (!ImGui::IsWindowFocused())
			return;

		ImGuiIO& io = ImGui::GetIO();

		// Ctrl+Z - Undo
		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z) && !io.KeyShift)
		{
			Undo();
		}

		// Ctrl+Y or Ctrl+Shift+Z - Redo
		if (io.KeyCtrl && (ImGui::IsKeyPressed(ImGuiKey_Y) || (ImGui::IsKeyPressed(ImGuiKey_Z) && io.KeyShift)))
		{
			Redo();
		}

		// Ctrl+S - Save
		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S))
		{
			SaveClip();
		}

		// Delete key - remove selected frame
		if (ImGui::IsKeyPressed(ImGuiKey_Delete))
		{
			DeleteSelectedFrame();
		}

		// Arrow keys - navigate frames
		if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
		{
			if (m_SelectedFrameIndex > 0)
			{
				m_SelectedFrameIndex--;
			}
		}
		if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
		{
			if (m_SelectedFrameIndex < m_CurrentClip.Frames.size() - 1)
			{
				m_SelectedFrameIndex++;
			}
		}

		// Space - play/pause
		if (ImGui::IsKeyPressed(ImGuiKey_Space))
		{
			m_PreviewPlaying = !m_PreviewPlaying;
		}
	}

	void AnimationEditorPanel::AddFrame()
	{
		if (!m_HasClipLoaded)
		{
			ConsolePanel::Log("No clip loaded - create a new clip first", LogLevel::Warn);
			return;
		}

		// Create a default frame
		Pillar::AnimationFrame newFrame;
		newFrame.Duration = 0.1f;
		newFrame.UVMin = glm::vec2(0.0f, 0.0f);
		newFrame.UVMax = glm::vec2(1.0f, 1.0f);

		// If we have existing frames, copy texture path from first frame
		if (!m_CurrentClip.Frames.empty())
		{
			newFrame.TexturePath = m_CurrentClip.Frames[0].TexturePath;
		}

		// Insert after selected frame, or at end if no selection
		int insertIndex = m_SelectedFrameIndex + 1;
		if (insertIndex <= 0 || insertIndex > m_CurrentClip.Frames.size())
		{
			insertIndex = m_CurrentClip.Frames.size();
		}

		// Execute via command system for undo support
		auto command = std::make_unique<AddFrameCommand>(m_CurrentClip, insertIndex, newFrame);
		ExecuteCommand(std::move(command));
		
		m_SelectedFrameIndex = insertIndex;
		ConsolePanel::Log("Added new frame at index " + std::to_string(m_SelectedFrameIndex), LogLevel::Info);
	}

	void AnimationEditorPanel::DeleteFrame(int index)
	{
		if (index < 0 || index >= m_CurrentClip.Frames.size())
			return;

		// Execute via command system for undo support
		auto command = std::make_unique<DeleteFrameCommand>(m_CurrentClip, index);
		ExecuteCommand(std::move(command));

		// Adjust selection
		if (m_SelectedFrameIndex >= m_CurrentClip.Frames.size())
		{
			m_SelectedFrameIndex = m_CurrentClip.Frames.size() - 1;
		}

		// Adjust preview frame
		if (m_PreviewFrame >= m_CurrentClip.Frames.size())
		{
			m_PreviewFrame = std::max(0, (int)m_CurrentClip.Frames.size() - 1);
		}

		ConsolePanel::Log("Deleted frame at index " + std::to_string(index), LogLevel::Info);
	}

	void AnimationEditorPanel::DeleteSelectedFrame()
	{
		if (m_SelectedFrameIndex < 0 || m_SelectedFrameIndex >= m_CurrentClip.Frames.size())
		{
			ConsolePanel::Log("No frame selected", LogLevel::Warn);
			return;
		}

		if (m_CurrentClip.Frames.size() <= 1)
		{
			ConsolePanel::Log("Cannot delete the last frame", LogLevel::Warn);
			return;
		}

		DeleteFrame(m_SelectedFrameIndex);
	}

	void AnimationEditorPanel::DuplicateFrame(int index)
	{
		if (index < 0 || index >= m_CurrentClip.Frames.size())
			return;

		Pillar::AnimationFrame duplicatedFrame = m_CurrentClip.Frames[index];
		m_CurrentClip.Frames.insert(m_CurrentClip.Frames.begin() + index + 1, duplicatedFrame);
		m_SelectedFrameIndex = index + 1;

		MarkModified();
		ConsolePanel::Log("Duplicated frame at index " + std::to_string(index), LogLevel::Info);
	}

	void AnimationEditorPanel::MoveFrame(int fromIndex, int toIndex)
	{
		if (fromIndex < 0 || fromIndex >= m_CurrentClip.Frames.size())
			return;
		if (toIndex < 0 || toIndex >= m_CurrentClip.Frames.size())
			return;
		if (fromIndex == toIndex)
			return;

		// Execute via command system for undo support
		auto command = std::make_unique<MoveFrameCommand>(m_CurrentClip, fromIndex, toIndex);
		ExecuteCommand(std::move(command));

		// Adjust target index for selection
		if (fromIndex < toIndex)
			m_SelectedFrameIndex = toIndex - 1;
		else
			m_SelectedFrameIndex = toIndex;

		ConsolePanel::Log("Moved frame from " + std::to_string(fromIndex) + " to " + std::to_string(toIndex), LogLevel::Info);
	}

	void AnimationEditorPanel::RenderFrameProperties()
	{
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Text("Frame Properties");
		ImGui::Separator();
		ImGui::Spacing();

		if (m_SelectedFrameIndex < 0 || m_SelectedFrameIndex >= m_CurrentClip.Frames.size())
			return;

		auto& frame = m_CurrentClip.Frames[m_SelectedFrameIndex];

	float availWidth = ImGui::GetContentRegionAvail().x;
	ImGui::PushItemWidth(availWidth);

	ImGui::Text("Frame Index: %d", m_SelectedFrameIndex);

	// Duration slider
	float duration = frame.Duration;
	ImGui::Text("Duration");
	ImGui::SetNextItemWidth(availWidth - 200.0f); // Leave room for quick buttons
	if (ImGui::SliderFloat("##FrameDuration", &duration, 0.01f, 2.0f, "%.3f s"))
		{
			frame.Duration = duration;
			MarkModified();
		}

		// Quick duration buttons on same line
		ImGui::SameLine();
		if (ImGui::SmallButton("0.05s"))
		{
			frame.Duration = 0.05f;
			MarkModified();
		}
		ImGui::SameLine();
		if (ImGui::SmallButton("0.1s"))
		{
			frame.Duration = 0.1f;
			MarkModified();
		}
		ImGui::SameLine();
		if (ImGui::SmallButton("0.2s"))
		{
			frame.Duration = 0.2f;
			MarkModified();
		}

		ImGui::Spacing();

		// Texture path (read-only for now)
		ImGui::Text("Texture");
		ImGui::TextWrapped("%s", frame.TexturePath.c_str());

		ImGui::Spacing();

		// UV coordinates (read-only for now) - formatted in columns
		ImGui::Columns(2, "UVCoords", false);
		ImGui::Text("UV Min");
		ImGui::NextColumn();
		ImGui::Text("(%.3f, %.3f)", frame.UVMin.x, frame.UVMin.y);
		ImGui::NextColumn();
		
		ImGui::Text("UV Max");
		ImGui::NextColumn();
		ImGui::Text("(%.3f, %.3f)", frame.UVMax.x, frame.UVMax.y);
		ImGui::Columns(1);

		// Frame actions
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		
		if (ImGui::Button("Duplicate Frame", ImVec2(availWidth, 0)))
		{
			DuplicateFrame(m_SelectedFrameIndex);
		}

		ImGui::PopItemWidth();
	}

	// ============================================================================
	// Undo/Redo System
	// ============================================================================

	void AnimationEditorPanel::ExecuteCommand(std::unique_ptr<EditorCommand> command)
	{
		command->Execute();
		
		m_UndoStack.push_back(std::move(command));
		
		// Limit undo history
		if (m_UndoStack.size() > MAX_UNDO_HISTORY)
		{
			m_UndoStack.erase(m_UndoStack.begin());
		}
		
		// Clear redo stack when new command is executed
		m_RedoStack.clear();
		
		MarkModified();
	}

	void AnimationEditorPanel::Undo()
	{
		if (m_UndoStack.empty())
		{
			ConsolePanel::Log("Nothing to undo", LogLevel::Info);
			return;
		}

		auto command = std::move(m_UndoStack.back());
		m_UndoStack.pop_back();

		command->Undo();
		m_RedoStack.push_back(std::move(command));

		ConsolePanel::Log("Undo: " + m_RedoStack.back()->GetDescription(), LogLevel::Info);
		MarkModified();
	}

	void AnimationEditorPanel::Redo()
	{
		if (m_RedoStack.empty())
		{
			ConsolePanel::Log("Nothing to redo", LogLevel::Info);
			return;
		}

		auto command = std::move(m_RedoStack.back());
		m_RedoStack.pop_back();

		command->Execute();
		m_UndoStack.push_back(std::move(command));

		ConsolePanel::Log("Redo: " + m_UndoStack.back()->GetDescription(), LogLevel::Info);
		MarkModified();
	}

	void AnimationEditorPanel::ClearUndoHistory()
	{
		m_UndoStack.clear();
		m_RedoStack.clear();
	}

} // namespace PillarEditor
