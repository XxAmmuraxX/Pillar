#include "SpriteSheetEditorPanel.h"
#include "ConsolePanel.h"
#include "InspectorPanel.h"
#include "../SelectionContext.h"
#include "Pillar/ECS/Components/Rendering/SpriteComponent.h"
#include <imgui.h>
#include <algorithm>

namespace PillarEditor {

    SpriteSheetEditorPanel::SpriteSheetEditorPanel()
    {
    }

    void SpriteSheetEditorPanel::OnImGuiRender()
    {
        if (!m_Visible)
            return;

        ImGui::Begin("Sprite Sheet Editor", &m_Visible);

        // Toolbar
        if (ImGui::Button("Load Texture..."))
        {
            // TODO: Open file dialog or texture browser
            ConsolePanel::Log("Texture loading dialog not yet implemented", LogLevel::Info);
        }

        ImGui::SameLine();
        if (ImGui::Button("Auto-Detect Grid"))
        {
            AutoDetectGrid();
        }

        ImGui::SameLine();
        if (m_Texture)
        {
            ImGui::Text("Texture: %dx%d", m_Texture->GetWidth(), m_Texture->GetHeight());
        }
        else
        {
            ImGui::TextDisabled("No texture loaded");
        }

        ImGui::Separator();

        // Grid configuration
        ImGui::Text("Grid Configuration");
        ImGui::SliderInt("Columns", &m_GridColumns, 1, 32);
        ImGui::SliderInt("Rows", &m_GridRows, 1, 32);
        
        ImGui::Separator();
        
        if (ImGui::InputInt("Cell Width", &m_CellWidth))
            m_CellWidth = std::max(1, m_CellWidth);
        if (ImGui::InputInt("Cell Height", &m_CellHeight))
            m_CellHeight = std::max(1, m_CellHeight);
        
        ImGui::InputInt("Padding", &m_Padding);
        ImGui::InputInt("Spacing", &m_Spacing);

        ImGui::Separator();

        // Selection info
        if (m_HasSelection)
        {
            ImGui::Text("Selected Cell: [%d, %d]", m_SelectedCol, m_SelectedRow);
            ImGui::Text("UV Min: (%.3f, %.3f)", m_SelectedCellMin.x, m_SelectedCellMin.y);
            ImGui::Text("UV Max: (%.3f, %.3f)", m_SelectedCellMax.x, m_SelectedCellMax.y);
            
            if (ImGui::Button("Apply to Selected Sprite"))
            {
                if (m_SelectionContext && m_SelectionContext->HasSelection())
                {
                    Pillar::Entity entity = m_SelectionContext->GetPrimarySelection();
                    if (entity && entity.HasComponent<Pillar::SpriteComponent>())
                    {
                        auto& sprite = entity.GetComponent<Pillar::SpriteComponent>();
                        sprite.TexCoordMin = m_SelectedCellMin;
                        sprite.TexCoordMax = m_SelectedCellMax;
                        
                        // Also set the texture if loaded
                        if (m_Texture)
                        {
                            sprite.Texture = m_Texture;
                            sprite.TexturePath = m_TexturePath;
                        }
                        
                        ConsolePanel::Log("Applied sprite sheet frame to sprite", LogLevel::Info);
                    }
                    else
                    {
                        ConsolePanel::Log("Selected entity has no SpriteComponent", LogLevel::Warn);
                    }
                }
                else
                {
                    ConsolePanel::Log("No entity selected", LogLevel::Warn);
                }
            }
        }
        else
        {
            ImGui::TextDisabled("No cell selected - click on a grid cell below");
        }

        ImGui::Separator();

        // Texture viewer with grid
        if (m_Texture)
        {
            RenderTextureWithGrid();
        }
        else
        {
            ImGui::TextDisabled("Load a texture to begin editing");
        }

        ImGui::End();
    }

    void SpriteSheetEditorPanel::LoadTexture(const std::string& path)
    {
        try
        {
            m_Texture = Pillar::Texture2D::Create(path);
            m_TexturePath = path;
            AutoDetectGrid();
            ConsolePanel::Log("Loaded sprite sheet: " + path, LogLevel::Info);
        }
        catch (const std::exception& e)
        {
            ConsolePanel::Log("Failed to load sprite sheet: " + std::string(e.what()), LogLevel::Error);
            m_Texture = nullptr;
            m_TexturePath.clear();
        }
    }

    void SpriteSheetEditorPanel::LoadTexture(std::shared_ptr<Pillar::Texture2D> texture)
    {
        m_Texture = texture;
        m_TexturePath = "";
        AutoDetectGrid();
    }

    void SpriteSheetEditorPanel::RenderTextureWithGrid()
    {
        if (!m_Texture)
            return;

        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        ImVec2 canvasSize = ImGui::GetContentRegionAvail();
        canvasSize.y = std::max(canvasSize.y, 300.0f);

        // Calculate display size (maintain aspect ratio)
        float aspectRatio = (float)m_Texture->GetWidth() / (float)m_Texture->GetHeight();
        float displayWidth = canvasSize.x * m_Zoom;
        float displayHeight = displayWidth / aspectRatio;
        
        if (displayHeight > canvasSize.y)
        {
            displayHeight = canvasSize.y * m_Zoom;
            displayWidth = displayHeight * aspectRatio;
        }

        ImVec2 displaySize(displayWidth, displayHeight);
        ImVec2 imagePos = ImVec2(canvasPos.x + m_Pan.x, canvasPos.y + m_Pan.y);

        // Draw texture
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddImage(
            (void*)(intptr_t)m_Texture->GetRendererID(),
            imagePos,
            ImVec2(imagePos.x + displaySize.x, imagePos.y + displaySize.y),
            ImVec2(0, 1), ImVec2(1, 0)
        );

        // Draw grid
        int texWidth = m_Texture->GetWidth();
        int texHeight = m_Texture->GetHeight();
        float cellDisplayWidth = displaySize.x / m_GridColumns;
        float cellDisplayHeight = displaySize.y / m_GridRows;

        ImU32 gridColor = IM_COL32(255, 255, 0, 128);
        ImU32 selectedColor = IM_COL32(0, 255, 0, 200);

        // Vertical lines
        for (int col = 0; col <= m_GridColumns; col++)
        {
            float x = imagePos.x + col * cellDisplayWidth;
            drawList->AddLine(
                ImVec2(x, imagePos.y),
                ImVec2(x, imagePos.y + displaySize.y),
                gridColor, 1.0f
            );
        }

        // Horizontal lines
        for (int row = 0; row <= m_GridRows; row++)
        {
            float y = imagePos.y + row * cellDisplayHeight;
            drawList->AddLine(
                ImVec2(imagePos.x, y),
                ImVec2(imagePos.x + displaySize.x, y),
                gridColor, 1.0f
            );
        }

        // Highlight selected cell
        if (m_HasSelection && m_SelectedCol >= 0 && m_SelectedRow >= 0)
        {
            float x = imagePos.x + m_SelectedCol * cellDisplayWidth;
            float y = imagePos.y + m_SelectedRow * cellDisplayHeight;
            drawList->AddRect(
                ImVec2(x, y),
                ImVec2(x + cellDisplayWidth, y + cellDisplayHeight),
                selectedColor, 0.0f, 0, 3.0f
            );
        }

        // Handle mouse input for cell selection
        ImGui::SetCursorScreenPos(canvasPos);
        ImGui::InvisibleButton("canvas", canvasSize);
        
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            ImVec2 mousePos = ImGui::GetMousePos();
            ImVec2 relativePos = ImVec2(mousePos.x - imagePos.x, mousePos.y - imagePos.y);
            
            // Check if click is within texture bounds
            if (relativePos.x >= 0 && relativePos.x <= displaySize.x &&
                relativePos.y >= 0 && relativePos.y <= displaySize.y)
            {
                // Calculate grid cell
                int col = (int)(relativePos.x / cellDisplayWidth);
                int row = (int)(relativePos.y / cellDisplayHeight);
                
                // Clamp to valid range
                col = std::clamp(col, 0, m_GridColumns - 1);
                row = std::clamp(row, 0, m_GridRows - 1);
                
                m_SelectedCol = col;
                m_SelectedRow = row;
                m_HasSelection = true;
                
                // Calculate UV coordinates
                float cellWidth = (float)texWidth / m_GridColumns;
                float cellHeight = (float)texHeight / m_GridRows;
                
                float pixelMinX = col * cellWidth + m_Padding + m_Spacing * col;
                float pixelMinY = row * cellHeight + m_Padding + m_Spacing * row;
                float pixelMaxX = pixelMinX + cellWidth - m_Spacing;
                float pixelMaxY = pixelMinY + cellHeight - m_Spacing;
                
                m_SelectedCellMin = glm::vec2(pixelMinX / texWidth, 1.0f - pixelMaxY / texHeight);
                m_SelectedCellMax = glm::vec2(pixelMaxX / texWidth, 1.0f - pixelMinY / texHeight);
                
                ConsolePanel::Log("Selected cell [" + std::to_string(col) + ", " + std::to_string(row) + "]", 
                                LogLevel::Info);
            }
        }

        // Zoom controls
        ImGui::SetCursorScreenPos(ImVec2(canvasPos.x + 10, canvasPos.y + canvasSize.y - 30));
        ImGui::Text("Zoom: %.1fx", m_Zoom);
        
        // Mouse wheel zoom
        if (ImGui::IsItemHovered())
        {
            float wheel = ImGui::GetIO().MouseWheel;
            if (wheel != 0.0f)
            {
                m_Zoom += wheel * 0.1f;
                m_Zoom = std::clamp(m_Zoom, 0.25f, 4.0f);
            }
        }
    }

    void SpriteSheetEditorPanel::AutoDetectGrid()
    {
        if (!m_Texture)
            return;

        // Simple auto-detection: try common grid sizes
        int width = m_Texture->GetWidth();
        int height = m_Texture->GetHeight();

        // Try common sprite sizes
        int commonSizes[] = { 8, 16, 32, 64, 128 };
        
        for (int size : commonSizes)
        {
            if (width % size == 0 && height % size == 0)
            {
                m_GridColumns = width / size;
                m_GridRows = height / size;
                m_CellWidth = size;
                m_CellHeight = size;
                
                ConsolePanel::Log("Auto-detected " + std::to_string(m_GridColumns) + "x" + 
                                std::to_string(m_GridRows) + " grid (" + 
                                std::to_string(size) + "x" + std::to_string(size) + " cells)", 
                                LogLevel::Info);
                return;
            }
        }
        
        // Fallback: 8x8 grid
        m_GridColumns = 8;
        m_GridRows = 8;
        m_CellWidth = width / 8;
        m_CellHeight = height / 8;
        
        ConsolePanel::Log("Using default 8x8 grid", LogLevel::Info);
    }

} // namespace PillarEditor
