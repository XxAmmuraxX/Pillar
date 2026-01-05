#pragma once

#include "EditorPanel.h"
#include "Pillar/Renderer/Texture.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>

namespace PillarEditor {

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

        // Texture
        std::shared_ptr<Pillar::Texture2D> m_Texture;
        std::string m_TexturePath;

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
    };

} // namespace PillarEditor
