#pragma once

#include "EditorPanel.h"
#include "../SpriteSheetMetadata.h"
#include "../TexturePackerImporter.h"
#include "../AsepriteImporter.h"
#include "Pillar/Renderer/Texture.h"
#include <glm/glm.hpp>
#include <imgui.h>
#include <memory>
#include <string>

namespace PillarEditor {

    /**
     * @brief Represents a single frame in the sprite sheet frame library
     */
    struct FrameData
    {
        int Index;                      // Sequential index in library
        int Column;                     // Grid column
        int Row;                        // Grid row
        glm::vec2 UVMin;               // Bottom-left UV coordinate
        glm::vec2 UVMax;               // Top-right UV coordinate
        std::string Name;              // Optional frame name
        
        FrameData(int index, int col, int row, const glm::vec2& uvMin, const glm::vec2& uvMax)
            : Index(index), Column(col), Row(row), UVMin(uvMin), UVMax(uvMax)
        {
            Name = "Frame " + std::to_string(index);
        }
    };

    /**
     * @brief Visual editor for selecting sprite sheet frames
     * 
     * Features:
     * - Display texture with grid overlay
     * - Mouse-based cell selection
     * - Grid configuration (columns, rows, cell size)
     * - Apply UV coordinates to selected sprite
     */
    class SpriteSheetEditorPanel : public EditorPanel
    {
    public:
        SpriteSheetEditorPanel();
        virtual ~SpriteSheetEditorPanel() = default;

        virtual void OnImGuiRender() override;

        // Load a texture to edit
        void LoadTexture(const std::string& path);
        void LoadTexture(std::shared_ptr<Pillar::Texture2D> texture);

        // Grid configuration
        void SetGridColumns(int cols) { m_GridColumns = cols; }
        void SetGridRows(int rows) { m_GridRows = rows; }
        void SetCellSize(int width, int height) { m_CellWidth = width; m_CellHeight = height; }
        void SetPadding(int padding) { m_Padding = padding; }
        void SetSpacing(int spacing) { m_Spacing = spacing; }

        // Selection
        glm::vec2 GetSelectedCellMin() const { return m_SelectedCellMin; }
        glm::vec2 GetSelectedCellMax() const { return m_SelectedCellMax; }
        bool HasSelection() const { return m_HasSelection; }

    private:
        void RenderTextureWithGrid();
        void HandleMouseInput();
        void AutoDetectGrid();
        void ApplyPreset(int cellSize);
        void SaveMetadata();
        void LoadMetadata();
        void RenderPresetButtons();
        
        // Drag helpers
        void HandleGridDragging(const ImVec2& imagePos, const ImVec2& displaySize);
        int FindNearestVerticalLine(float mouseX, const ImVec2& imagePos, float cellDisplayWidth, float paddingDisplayX, float spacingDisplayX);
        int FindNearestHorizontalLine(float mouseY, const ImVec2& imagePos, float cellDisplayHeight, float paddingDisplayY, float spacingDisplayY);
        void DrawDragHandles(const ImVec2& imagePos, const ImVec2& displaySize, float cellDisplayWidth, float cellDisplayHeight, 
                           float paddingDisplayX, float paddingDisplayY, float spacingDisplayX, float spacingDisplayY);
        
        // Frame library helpers
        void RenderFrameLibrary();
        void AddCurrentFrameToLibrary();
        void RemoveFrame(int index);
        void ClearFrameLibrary();
        void ExportToAnimationClip();
        void ImportFromTexturePacker();
        void LoadTexturePackerFrames(const std::vector<TexturePackerFrame>& frames);
        void ImportFromAseprite();
        void LoadAsepriteFrames(const std::vector<Pillar::AsepriteFrameData>& frames);
        void CreateAnimationClipsFromTags(const std::vector<Pillar::AsepriteAnimationTag>& tags, const std::vector<Pillar::AsepriteFrameData>& frames);

        // Texture
        std::shared_ptr<Pillar::Texture2D> m_Texture;
        std::string m_TexturePath;

        // Metadata
        SpriteSheetMetadata m_Metadata;
        bool m_MetadataChanged = false;

        // Grid configuration
        int m_GridColumns = 8;
        int m_GridRows = 8;
        int m_CellWidth = 32;
        int m_CellHeight = 32;
        int m_Padding = 0;     // Padding around entire sheet
        int m_Spacing = 0;     // Spacing between cells

        // Selection state
        bool m_HasSelection = false;
        int m_SelectedCol = -1;
        int m_SelectedRow = -1;
        glm::vec2 m_SelectedCellMin = { 0.0f, 0.0f };
        glm::vec2 m_SelectedCellMax = { 1.0f, 1.0f };

        // View state
        float m_Zoom = 1.0f;
        glm::vec2 m_Pan = { 0.0f, 0.0f };
        
        // Drag state for interactive grid adjustment
        enum class DragMode { None, VerticalLine, HorizontalLine };
        DragMode m_DragMode = DragMode::None;
        int m_DraggedLineIndex = -1;
        float m_DragStartMousePos = 0.0f;
        float m_DragStartCellSize = 0.0f;
        
        // Frame library
        std::vector<FrameData> m_FrameLibrary;
        int m_NextFrameIndex = 0;
        int m_HoveredFrameIndex = -1;
    };

} // namespace PillarEditor
