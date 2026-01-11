#include "SpriteSheetEditorPanel.h"
#include "ConsolePanel.h"
#include "InspectorPanel.h"
#include "../SelectionContext.h"
#include "../Utils/FileDialog.h"
#include "Pillar/ECS/Components/Rendering/SpriteComponent.h"
#include <imgui.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <filesystem>

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
            auto filePath = FileDialog::OpenFile("Image Files (*.png;*.jpg;*.jpeg;*.bmp;*.tga)\0*.png;*.jpg;*.jpeg;*.bmp;*.tga\0All Files (*.*)\0*.*\0");
            if (filePath.has_value())
            {
                LoadTexture(filePath.value());
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Auto-Detect Grid"))
        {
            AutoDetectGrid();
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Import TexturePacker"))
        {
            ImportFromTexturePacker();
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Import Aseprite"))
        {
            ImportFromAseprite();
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
        
        // Preset buttons
        RenderPresetButtons();
        
        ImGui::Separator();
        
        bool gridChanged = false;
        gridChanged |= ImGui::SliderInt("Columns", &m_GridColumns, 1, 32);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Number of sprite columns in the sheet");
            
        gridChanged |= ImGui::SliderInt("Rows", &m_GridRows, 1, 32);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Number of sprite rows in the sheet");
        
        ImGui::Separator();
        
        if (ImGui::InputInt("Cell Width (px)", &m_CellWidth))
        {
            m_CellWidth = std::max(1, m_CellWidth);
            gridChanged = true;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Width of each individual sprite in pixels");
            
        if (ImGui::InputInt("Cell Height (px)", &m_CellHeight))
        {
            m_CellHeight = std::max(1, m_CellHeight);
            gridChanged = true;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Height of each individual sprite in pixels");
        
        gridChanged |= ImGui::InputInt("Padding", &m_Padding);
        gridChanged |= ImGui::InputInt("Spacing", &m_Spacing);
        
        // Auto-save when grid changes
        if (gridChanged)
        {
            m_MetadataChanged = true;
            SaveMetadata();
        }

        ImGui::Separator();

        // Selection info
        if (m_HasSelection)
        {
            ImGui::Text("Selected Cell: [%d, %d]", m_SelectedCol, m_SelectedRow);
            ImGui::Text("UV Min: (%.3f, %.3f)", m_SelectedCellMin.x, m_SelectedCellMin.y);
            ImGui::Text("UV Max: (%.3f, %.3f)", m_SelectedCellMax.x, m_SelectedCellMax.y);
            
            if (ImGui::Button("Apply to Selected Sprite"))
            {
                // Debug logging
                if (!m_SelectionContext)
                {
                    ConsolePanel::Log("ERROR: SelectionContext is null (panel not initialized correctly)", LogLevel::Error);
                }
                else if (!m_SelectionContext->HasSelection())
                {
                    ConsolePanel::Log("No entity selected. Select an entity in Scene Hierarchy or Viewport first.", LogLevel::Warn);
                }
                else
                {
                    Pillar::Entity entity = m_SelectionContext->GetPrimarySelection();
                    if (!entity)
                    {
                        ConsolePanel::Log("Selected entity is invalid (null)", LogLevel::Error);
                    }
                    else if (!entity.HasComponent<Pillar::SpriteComponent>())
                    {
                        ConsolePanel::Log("Selected entity does not have a SpriteComponent. Add one in Inspector.", LogLevel::Warn);
                    }
                    else
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
                        
                        ConsolePanel::Log("✅ Applied frame [" + std::to_string(m_SelectedCol) + ", " + std::to_string(m_SelectedRow) + "] to sprite", LogLevel::Info);
                    }
                }
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Apply selected cell's UV coordinates to the currently selected entity's SpriteComponent.\nNo need to add to Frame Library first.");
            
            ImGui::SameLine();
            if (ImGui::Button("Add to Frame Library"))
            {
                AddCurrentFrameToLibrary();
            }
        }
        else
        {
            ImGui::TextDisabled("No cell selected - click on a grid cell below");
        }
        
        ImGui::Separator();
        
        // Frame library section
        RenderFrameLibrary();
        
        ImGui::Separator();

        // Texture viewer with grid
        if (m_Texture)
        {
            RenderTextureWithGrid();
        }
        else
        {
            // Create a visible drag-drop zone
            ImVec2 dropZoneSize = ImGui::GetContentRegionAvail();
            dropZoneSize.x = std::max(dropZoneSize.x, 100.0f);  // Ensure minimum width
            dropZoneSize.y = std::max(dropZoneSize.y, 200.0f);  // Ensure minimum height
            
            ImVec2 dropZonePos = ImGui::GetCursorScreenPos();
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            
            // Draw dashed border rectangle
            ImU32 borderColor = IM_COL32(128, 128, 128, 200);
            ImU32 bgColor = IM_COL32(40, 40, 40, 100);
            
            drawList->AddRectFilled(dropZonePos, 
                ImVec2(dropZonePos.x + dropZoneSize.x, dropZonePos.y + dropZoneSize.y), 
                bgColor);
            
            // Dashed border effect
            float dashLength = 10.0f;
            float gapLength = 5.0f;
            for (float x = dropZonePos.x; x < dropZonePos.x + dropZoneSize.x; x += dashLength + gapLength)
            {
                drawList->AddLine(
                    ImVec2(x, dropZonePos.y),
                    ImVec2(std::min(x + dashLength, dropZonePos.x + dropZoneSize.x), dropZonePos.y),
                    borderColor, 2.0f);
                drawList->AddLine(
                    ImVec2(x, dropZonePos.y + dropZoneSize.y),
                    ImVec2(std::min(x + dashLength, dropZonePos.x + dropZoneSize.x), dropZonePos.y + dropZoneSize.y),
                    borderColor, 2.0f);
            }
            for (float y = dropZonePos.y; y < dropZonePos.y + dropZoneSize.y; y += dashLength + gapLength)
            {
                drawList->AddLine(
                    ImVec2(dropZonePos.x, y),
                    ImVec2(dropZonePos.x, std::min(y + dashLength, dropZonePos.y + dropZoneSize.y)),
                    borderColor, 2.0f);
                drawList->AddLine(
                    ImVec2(dropZonePos.x + dropZoneSize.x, y),
                    ImVec2(dropZonePos.x + dropZoneSize.x, std::min(y + dashLength, dropZonePos.y + dropZoneSize.y)),
                    borderColor, 2.0f);
            }
            
            // Center text
            const char* text1 = "Drag and drop texture here";
            const char* text2 = "or click 'Load Texture...' button above";
            const char* text3 = "Supported: .png, .jpg, .bmp, .tga";
            
            ImVec2 textSize1 = ImGui::CalcTextSize(text1);
            ImVec2 textSize2 = ImGui::CalcTextSize(text2);
            ImVec2 textSize3 = ImGui::CalcTextSize(text3);
            
            ImGui::SetCursorScreenPos(ImVec2(
                dropZonePos.x + (dropZoneSize.x - textSize1.x) * 0.5f,
                dropZonePos.y + (dropZoneSize.y - textSize1.y) * 0.5f - 30.0f
            ));
            ImGui::TextDisabled("%s", text1);
            
            ImGui::SetCursorScreenPos(ImVec2(
                dropZonePos.x + (dropZoneSize.x - textSize2.x) * 0.5f,
                dropZonePos.y + (dropZoneSize.y - textSize2.y) * 0.5f
            ));
            ImGui::TextDisabled("%s", text2);
            
            ImGui::SetCursorScreenPos(ImVec2(
                dropZonePos.x + (dropZoneSize.x - textSize3.x) * 0.5f,
                dropZonePos.y + (dropZoneSize.y - textSize3.y) * 0.5f + 30.0f
            ));
            ImGui::TextDisabled("%s", text3);
            
            // Invisible button for drag-drop
            ImGui::SetCursorScreenPos(dropZonePos);
            ImGui::InvisibleButton("##dropzone", dropZoneSize);
            
            // Drag-and-drop target
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                {
                    const char* path = (const char*)payload->Data;
                    std::string pathStr(path);
                    
                    // Check if it's an image file
                    std::string ext = std::filesystem::path(pathStr).extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    
                    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga")
                    {
                        LoadTexture(pathStr);
                    }
                    else
                    {
                        ConsolePanel::Log("Invalid file type. Drag an image file (.png, .jpg, .bmp, .tga)", LogLevel::Warn);
                    }
                }
                ImGui::EndDragDropTarget();
            }
        }

        ImGui::End();
    }

    void SpriteSheetEditorPanel::LoadTexture(const std::string& path)
    {
        try
        {
            m_Texture = Pillar::Texture2D::Create(path);
            m_TexturePath = path;
            
            // Try to load existing metadata, otherwise auto-detect
            LoadMetadata();
            if (!m_Metadata.IsValid())
            {
                AutoDetectGrid();
            }
            
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
        
        // Calculate cell display size accounting for padding and spacing
        // Total spacing is between cells only, not at edges
        float totalSpacingWidth = m_Spacing * (m_GridColumns - 1);
        float totalSpacingHeight = m_Spacing * (m_GridRows - 1);
        
        // Available space after padding and spacing
        float availableWidth = texWidth - (m_Padding * 2) - totalSpacingWidth;
        float availableHeight = texHeight - (m_Padding * 2) - totalSpacingHeight;
        
        // Cell size in pixels (texture space)
        float cellPixelWidth = availableWidth / m_GridColumns;
        float cellPixelHeight = availableHeight / m_GridRows;
        
        // Convert to display space
        float cellDisplayWidth = (cellPixelWidth / texWidth) * displaySize.x;
        float cellDisplayHeight = (cellPixelHeight / texHeight) * displaySize.y;
        float paddingDisplayX = displaySize.x * m_Padding / texWidth;
        float paddingDisplayY = displaySize.y * m_Padding / texHeight;
        float spacingDisplayX = displaySize.x * m_Spacing / texWidth;
        float spacingDisplayY = displaySize.y * m_Spacing / texHeight;

        ImU32 gridColor = IM_COL32(255, 255, 0, 128);
        ImU32 selectedColor = IM_COL32(0, 255, 0, 200);

        // Vertical lines
        for (int col = 0; col <= m_GridColumns; col++)
        {
            float x = imagePos.x + paddingDisplayX + col * (cellDisplayWidth + (col < m_GridColumns ? spacingDisplayX : 0.0f));
            
            // Calculate proper line endpoints
            float lineStartY = imagePos.y + paddingDisplayY;
            float lineEndY = imagePos.y + paddingDisplayY + m_GridRows * cellDisplayHeight + (m_GridRows - 1) * spacingDisplayY;
            
            drawList->AddLine(
                ImVec2(x, lineStartY),
                ImVec2(x, lineEndY),
                gridColor, 1.0f
            );
        }

        // Horizontal lines
        for (int row = 0; row <= m_GridRows; row++)
        {
            float y = imagePos.y + paddingDisplayY + row * (cellDisplayHeight + (row < m_GridRows ? spacingDisplayY : 0.0f));
            
            // Calculate proper line endpoints
            float lineStartX = imagePos.x + paddingDisplayX;
            float lineEndX = imagePos.x + paddingDisplayX + m_GridColumns * cellDisplayWidth + (m_GridColumns - 1) * spacingDisplayX;
            
            drawList->AddLine(
                ImVec2(lineStartX, y),
                ImVec2(lineEndX, y),
                gridColor, 1.0f
            );
        }

        // Highlight selected cell
        if (m_HasSelection && m_SelectedCol >= 0 && m_SelectedRow >= 0)
        {
            float x = imagePos.x + paddingDisplayX + m_SelectedCol * (cellDisplayWidth + spacingDisplayX);
            float y = imagePos.y + paddingDisplayY + m_SelectedRow * (cellDisplayHeight + spacingDisplayY);
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
            
            // Check if click is within texture bounds (accounting for padding)
            if (relativePos.x >= paddingDisplayX && relativePos.x <= displaySize.x - paddingDisplayX &&
                relativePos.y >= paddingDisplayY && relativePos.y <= displaySize.y - paddingDisplayY)
            {
                // Calculate grid cell (account for padding and spacing)
                float adjustedX = relativePos.x - paddingDisplayX;
                float adjustedY = relativePos.y - paddingDisplayY;
                
                int col = (int)(adjustedX / (cellDisplayWidth + spacingDisplayX));
                int row = (int)(adjustedY / (cellDisplayHeight + spacingDisplayY));
                
                // Clamp to valid range
                col = std::clamp(col, 0, m_GridColumns - 1);
                row = std::clamp(row, 0, m_GridRows - 1);
                
                m_SelectedCol = col;
                m_SelectedRow = row;
                m_HasSelection = true;
                
                // Calculate UV coordinates (account for padding and spacing)
                float pixelMinX = m_Padding + col * (m_CellWidth + m_Spacing);
                float pixelMinY = m_Padding + row * (m_CellHeight + m_Spacing);
                float pixelMaxX = pixelMinX + m_CellWidth;
                float pixelMaxY = pixelMinY + m_CellHeight;
                
                m_SelectedCellMin = glm::vec2(pixelMinX / texWidth, 1.0f - pixelMaxY / texHeight);
                m_SelectedCellMax = glm::vec2(pixelMaxX / texWidth, 1.0f - pixelMinY / texHeight);
                
                ConsolePanel::Log("Selected cell [" + std::to_string(col) + ", " + std::to_string(row) + "]", 
                                LogLevel::Info);
            }
        }
        
        // Draw drag handles and handle grid dragging
        DrawDragHandles(imagePos, displaySize, cellDisplayWidth, cellDisplayHeight, 
                       paddingDisplayX, paddingDisplayY, spacingDisplayX, spacingDisplayY);
        HandleGridDragging(imagePos, displaySize);

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

    void SpriteSheetEditorPanel::ApplyPreset(int cellSize)
    {
        if (!m_Texture)
            return;

        int width = m_Texture->GetWidth();
        int height = m_Texture->GetHeight();

        m_GridColumns = width / cellSize;
        m_GridRows = height / cellSize;
        m_CellWidth = cellSize;
        m_CellHeight = cellSize;
        m_Padding = 0;
        m_Spacing = 0;

        m_MetadataChanged = true;
        SaveMetadata();

        ConsolePanel::Log("Applied " + std::to_string(cellSize) + "x" + std::to_string(cellSize) + 
                         " preset (" + std::to_string(m_GridColumns) + "x" + std::to_string(m_GridRows) + 
                         " grid)", LogLevel::Info);
    }

    void SpriteSheetEditorPanel::SaveMetadata()
    {
        if (m_TexturePath.empty() || !m_Texture)
            return;

        m_Metadata.Columns = m_GridColumns;
        m_Metadata.Rows = m_GridRows;
        m_Metadata.CellSize = { (float)m_CellWidth, (float)m_CellHeight };
        m_Metadata.Padding = { (float)m_Padding, (float)m_Padding };
        m_Metadata.Spacing = { (float)m_Spacing, (float)m_Spacing };
        m_Metadata.TextureSize = { (float)m_Texture->GetWidth(), (float)m_Texture->GetHeight() };

        std::string metadataPath = SpriteSheetMetadata::GetMetadataPath(m_TexturePath);
        if (m_Metadata.SaveToFile(metadataPath))
        {
            m_MetadataChanged = false;
        }
    }

    void SpriteSheetEditorPanel::LoadMetadata()
    {
        if (m_TexturePath.empty())
            return;

        std::string metadataPath = SpriteSheetMetadata::GetMetadataPath(m_TexturePath);
        if (m_Metadata.LoadFromFile(metadataPath))
        {
            // Apply loaded metadata to grid settings
            m_GridColumns = m_Metadata.Columns;
            m_GridRows = m_Metadata.Rows;
            m_CellWidth = (int)m_Metadata.CellSize.x;
            m_CellHeight = (int)m_Metadata.CellSize.y;
            m_Padding = (int)m_Metadata.Padding.x;
            m_Spacing = (int)m_Metadata.Spacing.x;

            ConsolePanel::Log("Loaded grid configuration from metadata", LogLevel::Info);
        }
    }

    void SpriteSheetEditorPanel::RenderPresetButtons()
    {
        ImGui::Text("Presets:");
        ImGui::SameLine();

        int presets[] = { 8, 16, 32, 64, 128 };
        const char* presetLabels[] = { "8x8", "16x16", "32x32", "64x64", "128x128" };

        for (int i = 0; i < 5; i++)
        {
            if (ImGui::Button(presetLabels[i]))
            {
                ApplyPreset(presets[i]);
            }
            if (i < 4) ImGui::SameLine();
        }
    }

    void SpriteSheetEditorPanel::DrawDragHandles(const ImVec2& imagePos, const ImVec2& displaySize, 
                                                  float cellDisplayWidth, float cellDisplayHeight,
                                                  float paddingDisplayX, float paddingDisplayY, 
                                                  float spacingDisplayX, float spacingDisplayY)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImU32 handleColor = IM_COL32(255, 128, 0, 200); // Orange handles
        ImU32 handleHoverColor = IM_COL32(255, 200, 0, 255); // Bright orange on hover
        float handleRadius = 4.0f;

        ImVec2 mousePos = ImGui::GetMousePos();
        
        // Draw handles on vertical lines (except first and last which are borders)
        for (int col = 1; col < m_GridColumns; col++)
        {
            float x = imagePos.x + paddingDisplayX + col * (cellDisplayWidth + spacingDisplayX);
            float y = imagePos.y + displaySize.y / 2.0f;
            
            // Check if mouse is near this handle
            float dist = std::abs(mousePos.x - x);
            bool isHovered = dist < 8.0f && std::abs(mousePos.y - y) < displaySize.y / 2.0f;
            
            drawList->AddCircleFilled(ImVec2(x, y), handleRadius, isHovered ? handleHoverColor : handleColor);
            
            // Change cursor when hovering
            if (isHovered)
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            }
        }

        // Draw handles on horizontal lines (except first and last)
        for (int row = 1; row < m_GridRows; row++)
        {
            float x = imagePos.x + displaySize.x / 2.0f;
            float y = imagePos.y + paddingDisplayY + row * (cellDisplayHeight + spacingDisplayY);
            
            // Check if mouse is near this handle
            float dist = std::abs(mousePos.y - y);
            bool isHovered = dist < 8.0f && std::abs(mousePos.x - x) < displaySize.x / 2.0f;
            
            drawList->AddCircleFilled(ImVec2(x, y), handleRadius, isHovered ? handleHoverColor : handleColor);
            
            // Change cursor when hovering
            if (isHovered)
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
            }
        }
    }

    int SpriteSheetEditorPanel::FindNearestVerticalLine(float mouseX, const ImVec2& imagePos, 
                                                         float cellDisplayWidth, float paddingDisplayX, 
                                                         float spacingDisplayX)
    {
        const float threshold = 8.0f;
        
        for (int col = 1; col < m_GridColumns; col++)
        {
            float x = imagePos.x + paddingDisplayX + col * (cellDisplayWidth + spacingDisplayX);
            if (std::abs(mouseX - x) < threshold)
            {
                return col;
            }
        }
        
        return -1;
    }

    int SpriteSheetEditorPanel::FindNearestHorizontalLine(float mouseY, const ImVec2& imagePos, 
                                                           float cellDisplayHeight, float paddingDisplayY, 
                                                           float spacingDisplayY)
    {
        const float threshold = 8.0f;
        
        for (int row = 1; row < m_GridRows; row++)
        {
            float y = imagePos.y + paddingDisplayY + row * (cellDisplayHeight + spacingDisplayY);
            if (std::abs(mouseY - y) < threshold)
            {
                return row;
            }
        }
        
        return -1;
    }

    void SpriteSheetEditorPanel::HandleGridDragging(const ImVec2& imagePos, const ImVec2& displaySize)
    {
        if (!m_Texture)
            return;

        ImVec2 mousePos = ImGui::GetMousePos();
        int texWidth = m_Texture->GetWidth();
        int texHeight = m_Texture->GetHeight();
        
        // Calculate display metrics
        float totalPaddingWidth = m_Padding * 2 + m_Spacing * (m_GridColumns - 1);
        float totalPaddingHeight = m_Padding * 2 + m_Spacing * (m_GridRows - 1);
        float effectiveWidth = displaySize.x * (1.0f - totalPaddingWidth / texWidth);
        float effectiveHeight = displaySize.y * (1.0f - totalPaddingHeight / texHeight);
        
        float cellDisplayWidth = effectiveWidth / m_GridColumns;
        float cellDisplayHeight = effectiveHeight / m_GridRows;
        float paddingDisplayX = displaySize.x * m_Padding / texWidth;
        float paddingDisplayY = displaySize.y * m_Padding / texHeight;
        float spacingDisplayX = displaySize.x * m_Spacing / texWidth;
        float spacingDisplayY = displaySize.y * m_Spacing / texHeight;

        // Start dragging
        if (m_DragMode == DragMode::None && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            int verticalLine = FindNearestVerticalLine(mousePos.x, imagePos, cellDisplayWidth, paddingDisplayX, spacingDisplayX);
            int horizontalLine = FindNearestHorizontalLine(mousePos.y, imagePos, cellDisplayHeight, paddingDisplayY, spacingDisplayY);
            
            if (verticalLine != -1)
            {
                m_DragMode = DragMode::VerticalLine;
                m_DraggedLineIndex = verticalLine;
                m_DragStartMousePos = mousePos.x;
                m_DragStartCellSize = (float)m_CellWidth;
            }
            else if (horizontalLine != -1)
            {
                m_DragMode = DragMode::HorizontalLine;
                m_DraggedLineIndex = horizontalLine;
                m_DragStartMousePos = mousePos.y;
                m_DragStartCellSize = (float)m_CellHeight;
            }
        }

        // Update during drag
        if (m_DragMode != DragMode::None && ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            if (m_DragMode == DragMode::VerticalLine)
            {
                // Calculate new cell width based on drag
                float dragDelta = mousePos.x - m_DragStartMousePos;
                float pixelDelta = dragDelta * (texWidth / displaySize.x);
                
                // Adjust cell width
                int newCellWidth = (int)(m_DragStartCellSize + pixelDelta / m_DraggedLineIndex);
                newCellWidth = std::clamp(newCellWidth, 4, texWidth / 2); // Min 4px, max half texture width
                
                if (newCellWidth != m_CellWidth)
                {
                    m_CellWidth = newCellWidth;
                    // Recalculate columns to fit texture
                    m_GridColumns = (texWidth - m_Padding * 2 + m_Spacing) / (m_CellWidth + m_Spacing);
                    m_GridColumns = std::max(1, m_GridColumns);
                }
            }
            else if (m_DragMode == DragMode::HorizontalLine)
            {
                // Calculate new cell height based on drag
                float dragDelta = mousePos.y - m_DragStartMousePos;
                float pixelDelta = dragDelta * (texHeight / displaySize.y);
                
                // Adjust cell height
                int newCellHeight = (int)(m_DragStartCellSize + pixelDelta / m_DraggedLineIndex);
                newCellHeight = std::clamp(newCellHeight, 4, texHeight / 2);
                
                if (newCellHeight != m_CellHeight)
                {
                    m_CellHeight = newCellHeight;
                    // Recalculate rows to fit texture
                    m_GridRows = (texHeight - m_Padding * 2 + m_Spacing) / (m_CellHeight + m_Spacing);
                    m_GridRows = std::max(1, m_GridRows);
                }
            }
        }

        // End dragging
        if (m_DragMode != DragMode::None && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            m_DragMode = DragMode::None;
            m_DraggedLineIndex = -1;
            
            // Save metadata after drag
            m_MetadataChanged = true;
            SaveMetadata();
            
            ConsolePanel::Log("Grid adjusted to " + std::to_string(m_GridColumns) + "x" + 
                            std::to_string(m_GridRows) + " (" + std::to_string(m_CellWidth) + "x" + 
                            std::to_string(m_CellHeight) + "px cells)", LogLevel::Info);
        }
    }

    void SpriteSheetEditorPanel::RenderFrameLibrary()
    {
        ImGui::Text("Frame Library (%zu frames)", m_FrameLibrary.size());
        
        ImGui::SameLine();
        if (ImGui::Button("Clear All"))
        {
            ClearFrameLibrary();
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Export to Animation"))
        {
            ExportToAnimationClip();
        }
        
        // Frame list with thumbnails
        ImGui::BeginChild("FrameList", ImVec2(0, 200), true);
        
        if (m_Texture)
        {
            float thumbnailSize = 64.0f;
            int columns = std::max(1, (int)(ImGui::GetContentRegionAvail().x / (thumbnailSize + 10)));
            
            for (size_t i = 0; i < m_FrameLibrary.size(); i++)
            {
                const auto& frame = m_FrameLibrary[i];
                
                ImGui::PushID((int)i);
                
                // Thumbnail button (flip V coordinate for OpenGL)
                ImVec2 uvMin = ImVec2(frame.UVMin.x, frame.UVMax.y);
                ImVec2 uvMax = ImVec2(frame.UVMax.x, frame.UVMin.y);
                
                bool isHovered = ((int)i == m_HoveredFrameIndex);
                ImVec4 tintColor = isHovered ? ImVec4(1, 1, 0, 1) : ImVec4(1, 1, 1, 1);
                
                if (ImGui::ImageButton(
                    ("frame_" + std::to_string(i)).c_str(),
                    (ImTextureID)(intptr_t)m_Texture->GetRendererID(),
                    ImVec2(thumbnailSize, thumbnailSize),
                    uvMin, uvMax,
                    ImVec4(0, 0, 0, 1), tintColor))
                {
                    // Click to select this frame on grid
                    m_SelectedCol = frame.Column;
                    m_SelectedRow = frame.Row;
                    m_SelectedCellMin = frame.UVMin;
                    m_SelectedCellMax = frame.UVMax;
                    m_HasSelection = true;
                }
                
                // Track hover
                if (ImGui::IsItemHovered())
                {
                    m_HoveredFrameIndex = (int)i;
                    ImGui::SetTooltip("%s\nGrid: [%d, %d]\nUV: (%.3f, %.3f) - (%.3f, %.3f)",
                                     frame.Name.c_str(),
                                     frame.Column, frame.Row,
                                     frame.UVMin.x, frame.UVMin.y,
                                     frame.UVMax.x, frame.UVMax.y);
                }
                
                // Remove button
                ImGui::SameLine(0, 4);
                if (ImGui::SmallButton("X"))
                {
                    RemoveFrame((int)i);
                    ImGui::PopID();
                    break; // Exit loop since we modified the vector
                }
                
                // Layout columns
                if ((i + 1) % columns != 0)
                    ImGui::SameLine();
                
                ImGui::PopID();
            }
        }
        else
        {
            ImGui::TextDisabled("No texture loaded");
        }
        
        ImGui::EndChild();
        
        // Reset hover when not over any frame
        if (!ImGui::IsWindowHovered())
        {
            m_HoveredFrameIndex = -1;
        }
    }

    void SpriteSheetEditorPanel::AddCurrentFrameToLibrary()
    {
        if (!m_HasSelection || !m_Texture)
        {
            ConsolePanel::Log("No frame selected to add", LogLevel::Warn);
            return;
        }
        
        // Check if frame already exists
        for (const auto& frame : m_FrameLibrary)
        {
            if (frame.Column == m_SelectedCol && frame.Row == m_SelectedRow)
            {
                ConsolePanel::Log("Frame [" + std::to_string(m_SelectedCol) + ", " + 
                                std::to_string(m_SelectedRow) + "] already in library", LogLevel::Warn);
                return;
            }
        }
        
        // Add frame
        m_FrameLibrary.emplace_back(
            m_NextFrameIndex++,
            m_SelectedCol,
            m_SelectedRow,
            m_SelectedCellMin,
            m_SelectedCellMax
        );
        
        ConsolePanel::Log("Added frame [" + std::to_string(m_SelectedCol) + ", " + 
                        std::to_string(m_SelectedRow) + "] to library (total: " + 
                        std::to_string(m_FrameLibrary.size()) + ")", LogLevel::Info);
    }

    void SpriteSheetEditorPanel::RemoveFrame(int index)
    {
        if (index >= 0 && index < (int)m_FrameLibrary.size())
        {
            m_FrameLibrary.erase(m_FrameLibrary.begin() + index);
            ConsolePanel::Log("Removed frame from library", LogLevel::Info);
        }
    }

    void SpriteSheetEditorPanel::ClearFrameLibrary()
    {
        m_FrameLibrary.clear();
        m_NextFrameIndex = 0;
        m_HoveredFrameIndex = -1;
        ConsolePanel::Log("Cleared frame library", LogLevel::Info);
    }

    void SpriteSheetEditorPanel::ExportToAnimationClip()
    {
        if (m_FrameLibrary.empty())
        {
            ConsolePanel::Log("No frames to export", LogLevel::Warn);
            return;
        }
        
        if (m_TexturePath.empty())
        {
            ConsolePanel::Log("Cannot export: no texture path", LogLevel::Error);
            return;
        }
        
        // Create animation clip JSON in AnimationLoader format
        try
        {
            nlohmann::json animJson;
            
            // Extract filename from path for animation name
            std::filesystem::path texPath(m_TexturePath);
            std::string animName = texPath.stem().string() + "_animation";
            
            // Animation metadata (matches AnimationLoader format)
            animJson["name"] = animName;
            animJson["loop"] = true;
            animJson["playbackSpeed"] = 1.0f;
            
            // Frames array (per-frame format with texturePath and duration)
            nlohmann::json framesArray = nlohmann::json::array();
            for (const auto& frame : m_FrameLibrary)
            {
                nlohmann::json frameJson;
                frameJson["texturePath"] = m_TexturePath;  // Required by AnimationLoader
                frameJson["duration"] = 0.1f;              // Per-frame duration (100ms default)
                frameJson["uvMin"] = { frame.UVMin.x, frame.UVMin.y };
                frameJson["uvMax"] = { frame.UVMax.x, frame.UVMax.y };
                framesArray.push_back(frameJson);
            }
            animJson["frames"] = framesArray;
            
            // Events array (empty by default, can be edited later)
            animJson["events"] = nlohmann::json::array();
            
            // Save to assets/animations/ directory (auto-create if needed)
            std::filesystem::path animDir = "assets/animations";
            if (!std::filesystem::exists(animDir))
            {
                std::filesystem::create_directories(animDir);
                PIL_INFO("Created animations directory: {}", animDir.string());
            }
            
            // Generate unique filename in animations folder
            std::string animPath = (animDir / (animName + ".anim.json")).string();
            
            // Add number suffix if file already exists
            int counter = 1;
            while (std::filesystem::exists(animPath))
            {
                animPath = (animDir / (animName + "_" + std::to_string(counter) + ".anim.json")).string();
                counter++;
            }
            
            std::ofstream file(animPath);
            if (!file.is_open())
            {
                ConsolePanel::Log("Failed to create animation file: " + animPath, LogLevel::Error);
                return;
            }
            
            file << animJson.dump(2);  // 2-space indent for consistency with AnimationLoader
            file.close();
            
            ConsolePanel::Log("Exported animation clip '" + animName + "' with " + 
                            std::to_string(m_FrameLibrary.size()) + " frames to: " + animPath, LogLevel::Info);
            ConsolePanel::Log("Animation format is compatible with AnimationSystem", LogLevel::Info);
        }
        catch (const std::exception& e)
        {
            ConsolePanel::Log("Failed to export animation: " + std::string(e.what()), LogLevel::Error);
        }
    }

    void SpriteSheetEditorPanel::ImportFromTexturePacker()
    {
        // TODO: Replace with proper file dialog
        // For now, log instructions
        ConsolePanel::Log("TexturePacker Import: Please ensure .json file is in same directory as texture", LogLevel::Info);
        ConsolePanel::Log("Usage: Place TexturePacker .json next to texture, then use 'Import TexturePacker' button", LogLevel::Info);
        
        // Try to find TexturePacker JSON in same directory as texture
        if (m_TexturePath.empty())
        {
            ConsolePanel::Log("No texture loaded. Load a texture first.", LogLevel::Warn);
            return;
        }
        
        std::filesystem::path texPath(m_TexturePath);
        std::string jsonPath = texPath.parent_path().string() + "/" + texPath.stem().string() + ".json";
        
        // Check if file exists
        if (!std::filesystem::exists(jsonPath))
        {
            ConsolePanel::Log("TexturePacker JSON not found: " + jsonPath, LogLevel::Error);
            ConsolePanel::Log("Tip: Export from TexturePacker with JSON (Hash) format", LogLevel::Info);
            return;
        }
        
        // Parse file
        TexturePackerImporter importer;
        if (!importer.ParseFile(jsonPath))
        {
            ConsolePanel::Log("Failed to import: " + importer.GetError(), LogLevel::Error);
            return;
        }
        
        // Load texture from metadata if different
        const auto& metadata = importer.GetMetadata();
        if (!metadata.ImagePath.empty() && metadata.ImagePath != m_TexturePath)
        {
            LoadTexture(metadata.ImagePath);
        }
        
        // Import frames into library
        LoadTexturePackerFrames(importer.GetFrames());
    }

    void SpriteSheetEditorPanel::LoadTexturePackerFrames(const std::vector<TexturePackerFrame>& frames)
    {
        if (frames.empty())
        {
            ConsolePanel::Log("No frames to import", LogLevel::Warn);
            return;
        }
        
        // Clear existing frame library
        ClearFrameLibrary();
        
        // Import all frames
        for (const auto& tpFrame : frames)
        {
            FrameData frame(
                m_NextFrameIndex++,
                -1,  // No grid column (direct UV coords from TexturePacker)
                -1,  // No grid row
                tpFrame.UVMin,
                tpFrame.UVMax
            );
            
            // Use TexturePacker frame name
            frame.Name = tpFrame.Name;
            
            // Add metadata about rotation/trimming
            if (tpFrame.Rotated)
            {
                frame.Name += " [ROTATED]";
            }
            if (tpFrame.Trimmed)
            {
                frame.Name += " [TRIMMED]";
            }
            
            m_FrameLibrary.push_back(frame);
        }
        
        ConsolePanel::Log("Imported " + std::to_string(frames.size()) + " frames from TexturePacker", LogLevel::Info);
    }

    void SpriteSheetEditorPanel::ImportFromAseprite()
    {
        // Try to find Aseprite JSON in same directory as texture
        if (m_TexturePath.empty())
        {
            ConsolePanel::Log("No texture loaded. Load a texture first.", LogLevel::Warn);
            return;
        }
        
        std::filesystem::path texPath(m_TexturePath);
        std::string jsonPath = texPath.parent_path().string() + "/" + texPath.stem().string() + ".json";
        
        // Check if file exists
        if (!std::filesystem::exists(jsonPath))
        {
            ConsolePanel::Log("Aseprite JSON not found: " + jsonPath, LogLevel::Error);
            ConsolePanel::Log("Tip: Export from Aseprite with 'JSON Data' format", LogLevel::Info);
            return;
        }
        
        // Parse file
        Pillar::AsepriteImporter importer;
        if (!importer.ParseFile(jsonPath))
        {
            ConsolePanel::Log("Failed to import: " + importer.GetErrorMessage(), LogLevel::Error);
            return;
        }
        
        // Load texture from metadata if different
        const auto& metadata = importer.GetMetadata();
        if (!metadata.ImagePath.empty() && metadata.ImagePath != m_TexturePath)
        {
            LoadTexture(metadata.ImagePath);
        }
        
        // Import frames into library
        LoadAsepriteFrames(importer.GetFrames());
        
        // Auto-create animation clips from tags
        if (!importer.GetAnimationTags().empty())
        {
            CreateAnimationClipsFromTags(importer.GetAnimationTags(), importer.GetFrames());
        }
    }

    void SpriteSheetEditorPanel::LoadAsepriteFrames(const std::vector<Pillar::AsepriteFrameData>& frames)
    {
        if (frames.empty())
        {
            ConsolePanel::Log("No frames to import", LogLevel::Warn);
            return;
        }
        
        // Clear existing frame library
        ClearFrameLibrary();
        
        // Import all frames (preserving Aseprite frame order)
        for (const auto& aseFrame : frames)
        {
            FrameData frame(
                m_NextFrameIndex++,
                -1,  // No grid column (direct UV coords from Aseprite)
                -1,  // No grid row
                aseFrame.UVMin,
                aseFrame.UVMax
            );
            
            // Use Aseprite frame name
            frame.Name = aseFrame.FrameName;
            
            // Add duration metadata as suffix
            frame.Name += " (" + std::to_string(aseFrame.DurationMs) + "ms)";
            
            m_FrameLibrary.push_back(frame);
        }
        
        ConsolePanel::Log("Imported " + std::to_string(frames.size()) + " frames from Aseprite", LogLevel::Info);
    }

    void SpriteSheetEditorPanel::CreateAnimationClipsFromTags(const std::vector<Pillar::AsepriteAnimationTag>& tags, const std::vector<Pillar::AsepriteFrameData>& frames)
    {
        if (tags.empty())
        {
            ConsolePanel::Log("No animation tags found in Aseprite file", LogLevel::Info);
            return;
        }
        
        ConsolePanel::Log("Creating animation clips from " + std::to_string(tags.size()) + " tags...", LogLevel::Info);
        
        for (const auto& tag : tags)
        {
            // Build animation clip JSON
            nlohmann::json animJson;
            animJson["name"] = tag.Name;
            animJson["loop"] = (tag.Direction == "pingpong" || tag.Direction == "forward"); // Loop by default
            animJson["fps"] = 10.0f; // Default FPS, will be overridden by frame durations
            
            nlohmann::json framesArray = nlohmann::json::array();
            
            // Add frames in range [FromFrame, ToFrame]
            for (int i = tag.FromFrame; i <= tag.ToFrame && i < static_cast<int>(frames.size()); ++i)
            {
                const auto& aseFrame = frames[i];
                
                nlohmann::json frameJson;
                frameJson["uvMin"] = { aseFrame.UVMin.x, aseFrame.UVMin.y };
                frameJson["uvMax"] = { aseFrame.UVMax.x, aseFrame.UVMax.y };
                
                // Convert duration from milliseconds to seconds
                frameJson["duration"] = aseFrame.DurationMs / 1000.0f;
                
                framesArray.push_back(frameJson);
            }
            
            animJson["frames"] = framesArray;
            
            // Special handling for "pingpong" direction
            if (tag.Direction == "pingpong")
            {
                // Add reverse sequence (excluding first and last to avoid duplicate)
                for (int i = tag.ToFrame - 1; i > tag.FromFrame && i < static_cast<int>(frames.size()); --i)
                {
                    const auto& aseFrame = frames[i];
                    
                    nlohmann::json frameJson;
                    frameJson["uvMin"] = { aseFrame.UVMin.x, aseFrame.UVMin.y };
                    frameJson["uvMax"] = { aseFrame.UVMax.x, aseFrame.UVMax.y };
                    frameJson["duration"] = aseFrame.DurationMs / 1000.0f;
                    
                    framesArray.push_back(frameJson);
                }
                animJson["frames"] = framesArray;
            }
            
            // Reverse frames if direction is "reverse"
            if (tag.Direction == "reverse")
            {
                std::reverse(framesArray.begin(), framesArray.end());
                animJson["frames"] = framesArray;
            }
            
            // Save to file
            std::string filename = tag.Name + ".anim.json";
            std::ofstream outFile(filename);
            if (outFile.is_open())
            {
                outFile << animJson.dump(4);
                outFile.close();
                ConsolePanel::Log("Created animation clip: " + filename, LogLevel::Info);
            }
            else
            {
                ConsolePanel::Log("Failed to save animation clip: " + filename, LogLevel::Error);
            }
        }
    }

} // namespace PillarEditor
