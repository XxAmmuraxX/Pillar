#include "ViewportPanel.h"
#include "../SelectionContext.h"
#include "ConsolePanel.h"
#include "Pillar/Renderer/Renderer.h"
#include "Pillar/Renderer/Renderer2D.h"
#include "Pillar/Renderer/RenderCommand.h"
#include "Pillar/ECS/Components/Core/TagComponent.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Physics/ColliderComponent.h"
#include "Pillar/Events/MouseEvent.h"
#include "Pillar/Input.h"
#include "Pillar/KeyCodes.h"
#include <imgui.h>
#include <ImGuizmo.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <algorithm>
#include <cmath>

namespace PillarEditor {

    ViewportPanel::ViewportPanel()
        : EditorPanel("Viewport")
    {
        // Create framebuffer with initial size
        Pillar::FramebufferSpecification spec;
        spec.Width = 1280;
        spec.Height = 720;
        m_Framebuffer = Pillar::Framebuffer::Create(spec);

        // Initialize camera at origin with good default zoom
        m_EditorCamera.SetViewportSize(1280.0f, 720.0f);
        m_EditorCamera.SetZoomLevel(5.0f);  // Start zoomed out to see more of the scene
        m_EditorCamera.SetPosition({ 0.0f, 0.0f, 0.0f });
    }

    void ViewportPanel::OnUpdate(float deltaTime)
    {
        // Update camera for panning (needs to track mouse even when just hovered)
        // Panning requires continuous updates when middle mouse is held
        if (m_ViewportHovered)
        {
            m_EditorCamera.OnUpdate(deltaTime);
        }
    }

    void ViewportPanel::OnEvent(Pillar::Event& e)
    {
        // Handle scroll events for zoom when viewport is hovered
        if (m_ViewportHovered)
        {
            m_EditorCamera.OnEvent(e);
            
            // Handle mouse clicks for entity picking
            Pillar::EventDispatcher dispatcher(e);
            dispatcher.Dispatch<Pillar::MouseButtonPressedEvent>(
                [this](Pillar::MouseButtonPressedEvent& event) { return OnMouseButtonPressed(event); }
            );
        }
    }

    void ViewportPanel::RenderScene()
    {
        if (!m_Framebuffer)
            return;

        // Bind framebuffer and render scene
        m_Framebuffer->Bind();

        // Dark gray background for editor viewport
        Pillar::RenderCommand::SetClearColor({ 0.12f, 0.12f, 0.15f, 1.0f });
        Pillar::RenderCommand::Clear();

        if (m_Scene)
        {
            Pillar::Renderer2D::BeginScene(m_EditorCamera.GetCamera());

            // Draw grid (subtle)
            DrawGrid();

            // Render all entities with TransformComponent
            auto view = m_Scene->GetRegistry().view<Pillar::TagComponent, Pillar::TransformComponent>();
            
            for (auto entity : view)
            {
                auto& tag = view.get<Pillar::TagComponent>(entity);
                auto& transform = view.get<Pillar::TransformComponent>(entity);

                // Determine color based on entity type
                glm::vec4 color = GetEntityColor(tag.Tag);
                glm::vec2 size = GetEntitySize(tag.Tag, transform.Scale);

                // Check if entity is selected
                bool isSelected = false;
                if (m_SelectionContext)
                {
                    Pillar::Entity e(entity, m_Scene.get());
                    isSelected = m_SelectionContext->IsSelected(e);
                }

                // Draw entity first
                Pillar::Renderer2D::DrawQuad(transform.Position, size, color);

                // Draw selection highlight (on top of entity with thicker outline)
                if (isSelected)
                {
                    // Draw a thick border by drawing 4 rectangles around the entity
                    glm::vec4 outlineColor = { 1.0f, 0.7f, 0.0f, 1.0f };  // Bright orange
                    float borderThickness = 0.08f;
                    
                    // Top border
                    Pillar::Renderer2D::DrawQuad(
                        glm::vec3(transform.Position.x, transform.Position.y + size.y / 2.0f + borderThickness / 2.0f, 0.01f),
                        glm::vec2(size.x + borderThickness * 2.0f, borderThickness),
                        outlineColor
                    );
                    
                    // Bottom border
                    Pillar::Renderer2D::DrawQuad(
                        glm::vec3(transform.Position.x, transform.Position.y - size.y / 2.0f - borderThickness / 2.0f, 0.01f),
                        glm::vec2(size.x + borderThickness * 2.0f, borderThickness),
                        outlineColor
                    );
                    
                    // Left border
                    Pillar::Renderer2D::DrawQuad(
                        glm::vec3(transform.Position.x - size.x / 2.0f - borderThickness / 2.0f, transform.Position.y, 0.01f),
                        glm::vec2(borderThickness, size.y),
                        outlineColor
                    );
                    
                    // Right border
                    Pillar::Renderer2D::DrawQuad(
                        glm::vec3(transform.Position.x + size.x / 2.0f + borderThickness / 2.0f, transform.Position.y, 0.01f),
                        glm::vec2(borderThickness, size.y),
                        outlineColor
                    );
                }
            }

            Pillar::Renderer2D::EndScene();
        }
        else
        {
            // No scene - just show empty viewport with hint
            Pillar::Renderer2D::BeginScene(m_EditorCamera.GetCamera());
            DrawGrid();
            Pillar::Renderer2D::EndScene();
        }

        m_Framebuffer->Unbind();
    }

    void ViewportPanel::DrawGrid()
    {
        // Draw a subtle grid for reference
        // Adjust grid based on zoom level for better visibility
        float zoomLevel = m_EditorCamera.GetZoomLevel();
        float gridSize = 1.0f;
        
        // Determine grid extent based on zoom (render more grid when zoomed out)
        float gridExtent = std::max(20.0f, zoomLevel * 3.0f);
        
        glm::vec4 gridColor = { 0.2f, 0.2f, 0.22f, 0.4f };
        glm::vec4 axisColorX = { 0.5f, 0.2f, 0.2f, 0.6f };  // Red for X axis
        glm::vec4 axisColorY = { 0.2f, 0.5f, 0.2f, 0.6f };  // Green for Y axis

        // Get camera position to center grid around it
        glm::vec3 camPos = m_EditorCamera.GetPosition();
        float startX = std::floor(camPos.x - gridExtent);
        float endX = std::ceil(camPos.x + gridExtent);
        float startY = std::floor(camPos.y - gridExtent);
        float endY = std::ceil(camPos.y + gridExtent);

        // Vertical lines
        for (float x = startX; x <= endX; x += gridSize)
        {
            bool isYAxis = std::abs(x) < 0.001f;
            glm::vec4 color = isYAxis ? axisColorY : gridColor;
            float thickness = isYAxis ? 0.04f : 0.015f;
            Pillar::Renderer2D::DrawQuad(
                glm::vec3(x, camPos.y, -0.1f),
                glm::vec2(thickness, gridExtent * 2.0f),
                color
            );
        }

        // Horizontal lines
        for (float y = startY; y <= endY; y += gridSize)
        {
            bool isXAxis = std::abs(y) < 0.001f;
            glm::vec4 color = isXAxis ? axisColorX : gridColor;
            float thickness = isXAxis ? 0.04f : 0.015f;
            Pillar::Renderer2D::DrawQuad(
                glm::vec3(camPos.x, y, -0.1f),
                glm::vec2(gridExtent * 2.0f, thickness),
                color
            );
        }
    }

    glm::vec4 ViewportPanel::GetEntityColor(const std::string& tag)
    {
        // Color based on entity type/tag
        if (tag == "Player" || tag.find("Player") != std::string::npos)
            return { 0.2f, 0.7f, 0.3f, 1.0f };  // Green
        
        if (tag == "Enemy" || tag.find("Enemy") != std::string::npos)
            return { 0.8f, 0.2f, 0.2f, 1.0f };  // Red
        
        if (tag.find("XP") != std::string::npos || tag.find("Gem") != std::string::npos)
            return { 0.9f, 0.9f, 0.2f, 1.0f };  // Yellow
        
        if (tag == "Ground" || tag.find("Ground") != std::string::npos ||
            tag == "Wall" || tag.find("Wall") != std::string::npos)
            return { 0.4f, 0.35f, 0.3f, 1.0f };  // Brown
        
        if (tag == "Camera" || tag.find("Camera") != std::string::npos)
            return { 0.3f, 0.5f, 0.8f, 1.0f };  // Blue
        
        if (tag.find("Bullet") != std::string::npos)
            return { 1.0f, 0.5f, 0.0f, 1.0f };  // Orange
        
        // Default gray
        return { 0.5f, 0.5f, 0.55f, 1.0f };
    }

    glm::vec2 ViewportPanel::GetEntitySize(const std::string& tag, const glm::vec2& scale)
    {
        // Some entities have default sizes
        if (tag.find("XP") != std::string::npos || tag.find("Gem") != std::string::npos)
            return { 0.3f, 0.3f };
        
        if (tag.find("Bullet") != std::string::npos)
            return { 0.2f, 0.2f };
        
        // Use transform scale
        return scale;
    }

    void ViewportPanel::OnImGuiRender()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Viewport");

        // Track viewport focus/hover state BEFORE rendering image
        // This is important for proper event handling
        m_ViewportFocused = ImGui::IsWindowFocused();
        m_ViewportHovered = ImGui::IsWindowHovered();

        // Get available content region size
        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        
        // Ensure minimum size
        viewportPanelSize.x = std::max(viewportPanelSize.x, 100.0f);
        viewportPanelSize.y = std::max(viewportPanelSize.y, 100.0f);
        
        // Handle viewport resize
        if (m_ViewportSize.x != viewportPanelSize.x || m_ViewportSize.y != viewportPanelSize.y)
        {
            m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };
            m_Framebuffer->Resize((uint32_t)viewportPanelSize.x, (uint32_t)viewportPanelSize.y);
            m_EditorCamera.SetViewportSize(viewportPanelSize.x, viewportPanelSize.y);
        }

        // Get viewport bounds for mouse picking later
        ImVec2 viewportMinRegion = ImGui::GetWindowContentRegionMin();
        ImVec2 viewportMaxRegion = ImGui::GetWindowContentRegionMax();
        ImVec2 viewportOffset = ImGui::GetWindowPos();
        m_ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
        m_ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

        // Render framebuffer texture
        uint32_t textureID = m_Framebuffer->GetColorAttachmentRendererID();
        // Flip Y coordinates for OpenGL texture (UV: 0,1 to 1,0)
        ImGui::Image((void*)(intptr_t)textureID, viewportPanelSize, ImVec2(0, 1), ImVec2(1, 0));

        // Draw gizmos overlay
        DrawGizmos();

        // Draw gizmo toolbar
        ImGui::SetCursorPos(ImVec2(10, 10));
        DrawGizmoToolbar();

        // Show viewport info overlay using SetCursorPos (simpler, doesn't create child window)
        ImGui::SetCursorPos(ImVec2(10, 60));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 0.8f));
        ImGui::Text("Zoom: %.1fx", 1.0f / m_EditorCamera.GetZoomLevel());
        glm::vec3 pos = m_EditorCamera.GetPosition();
        ImGui::SetCursorPos(ImVec2(10, 78));
        ImGui::Text("Pos: %.1f, %.1f", pos.x, pos.y);
        ImGui::PopStyleColor();

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void ViewportPanel::ResetCamera()
    {
        m_EditorCamera.SetPosition({ 0.0f, 0.0f, 0.0f });
        m_EditorCamera.SetZoomLevel(5.0f);
    }

    bool ViewportPanel::OnMouseButtonPressed(Pillar::MouseButtonPressedEvent& e)
    {
        // Only handle left mouse button
        if (e.GetMouseButton() != 0) // 0 = left button
            return false;

        // Don't pick if ImGuizmo is being used or wants mouse input
        if (ImGuizmo::IsOver() || ImGuizmo::IsUsing())
            return false;

        // Don't pick if we're panning (middle mouse) or if viewport isn't focused
        if (Pillar::Input::IsMouseButtonPressed(2) || !m_ViewportFocused)
            return false;

        // Get mouse position using ImGui (which gives us the correct position relative to the window)
        ImVec2 mousePos = ImGui::GetMousePos();
        glm::vec2 screenPos = { mousePos.x, mousePos.y };

        // Check if mouse is within viewport bounds
        if (screenPos.x < m_ViewportBounds[0].x || screenPos.x > m_ViewportBounds[1].x ||
            screenPos.y < m_ViewportBounds[0].y || screenPos.y > m_ViewportBounds[1].y)
        {
            return false;
        }

        // Convert to viewport-relative coordinates (0 to viewport size)
        glm::vec2 viewportRelativePos = screenPos - m_ViewportBounds[0];
        
        // Convert to world space
        glm::vec2 worldPos = ScreenToWorld(viewportRelativePos);

        // Find entity at this position
        Pillar::Entity clickedEntity = GetEntityAtWorldPosition(worldPos);

        // Check if Ctrl is held for multi-select
        bool ctrlHeld = Pillar::Input::IsKeyPressed(341) || Pillar::Input::IsKeyPressed(345); // Left/Right Ctrl

        if (clickedEntity && m_SelectionContext)
        {
            if (ctrlHeld)
            {
                // Toggle selection
                if (m_SelectionContext->IsSelected(clickedEntity))
                    m_SelectionContext->RemoveFromSelection(clickedEntity);
                else
                    m_SelectionContext->AddToSelection(clickedEntity);
            }
            else
            {
                // Replace selection
                m_SelectionContext->Select(clickedEntity);
            }
            
            return true;
        }
        else if (!ctrlHeld && m_SelectionContext)
        {
            // Clicked on empty space - clear selection
            m_SelectionContext->ClearSelection();
        }

        return false;
    }

    glm::vec2 ViewportPanel::ScreenToWorld(const glm::vec2& screenPos)
    {
        // Normalize to 0-1 range
        glm::vec2 normalized = screenPos / m_ViewportSize;
        
        // Convert to -1 to 1 range (NDC)
        normalized = normalized * 2.0f - 1.0f;
        
        // Flip Y axis (screen space has Y down, world space has Y up)
        normalized.y = -normalized.y;
        
        // Get camera properties
        float zoom = m_EditorCamera.GetZoomLevel();
        float aspectRatio = m_ViewportSize.x / m_ViewportSize.y;
        glm::vec3 cameraPos = m_EditorCamera.GetPosition();
        
        // Convert to world space
        glm::vec2 worldPos;
        worldPos.x = normalized.x * zoom * aspectRatio + cameraPos.x;
        worldPos.y = normalized.y * zoom + cameraPos.y;
        
        return worldPos;
    }

    Pillar::Entity ViewportPanel::GetEntityAtWorldPosition(const glm::vec2& worldPos)
    {
        if (!m_Scene)
            return {};

        // Iterate through all entities with transform and check for intersection
        auto view = m_Scene->GetRegistry().view<Pillar::TagComponent, Pillar::TransformComponent>();
        
        // Store entities that intersect, we'll pick the one rendered on top (last in the list)
        Pillar::Entity selectedEntity;
        
        for (auto entity : view)
        {
            auto& tag = view.get<Pillar::TagComponent>(entity);
            auto& transform = view.get<Pillar::TransformComponent>(entity);
            
            // Get entity size
            glm::vec2 size = GetEntitySize(tag.Tag, transform.Scale);
            
            // Calculate AABB bounds
            glm::vec2 minBounds = transform.Position - (size * 0.5f);
            glm::vec2 maxBounds = transform.Position + (size * 0.5f);
            
            // Check if world position is inside the AABB
            if (worldPos.x >= minBounds.x && worldPos.x <= maxBounds.x &&
                worldPos.y >= minBounds.y && worldPos.y <= maxBounds.y)
            {
                // Entity intersects - keep checking others (last one wins = on top)
                selectedEntity = Pillar::Entity(entity, m_Scene.get());
            }
        }
        
        return selectedEntity;
    }

    void ViewportPanel::DrawGizmos()
    {
        // Only draw gizmos if we have a selection
        if (!m_SelectionContext || !m_SelectionContext->HasSelection())
            return;

        auto selectedEntity = m_SelectionContext->GetPrimarySelection();
        if (!selectedEntity || !selectedEntity.HasComponent<Pillar::TransformComponent>())
            return;

        // Don't draw gizmo if in "None" mode
        if (m_GizmoMode == GizmoMode::None)
            return;

        // Get transform component
        auto& tc = selectedEntity.GetComponent<Pillar::TransformComponent>();

        // Setup ImGuizmo for this window
        ImGuizmo::SetOrthographic(true);
        ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
        ImGuizmo::SetRect(m_ViewportBounds[0].x, m_ViewportBounds[0].y, 
                         m_ViewportSize.x, m_ViewportSize.y);

        // Get camera matrices
        const auto& camera = m_EditorCamera.GetCamera();
        glm::mat4 viewMatrix = camera.GetViewMatrix();
        glm::mat4 projectionMatrix = camera.GetProjectionMatrix();

        // Create transform matrix from 2D transform
        glm::mat4 transform = glm::mat4(1.0f);
        transform = glm::translate(transform, glm::vec3(tc.Position.x, tc.Position.y, 0.0f));
        transform = glm::rotate(transform, glm::radians(tc.Rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        transform = glm::scale(transform, glm::vec3(tc.Scale.x, tc.Scale.y, 1.0f));

        // Determine gizmo operation
        ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
        if (m_GizmoMode == GizmoMode::Translate)
            operation = ImGuizmo::TRANSLATE;
        else if (m_GizmoMode == GizmoMode::Rotate)
            operation = ImGuizmo::ROTATE;
        else if (m_GizmoMode == GizmoMode::Scale)
            operation = ImGuizmo::SCALE;

        // For 2D, we only want to manipulate X and Y axes (hide Z axis)
        ImGuizmo::MODE mode = ImGuizmo::LOCAL;
        
        // Manipulate the gizmo
        bool snapping = Pillar::Input::IsKeyPressed(PIL_KEY_LEFT_CONTROL) || 
                       Pillar::Input::IsKeyPressed(PIL_KEY_RIGHT_CONTROL);
        float snapValue = 0.5f; // 0.5 units for translate, 15 degrees for rotate, 0.5 for scale
        
        if (operation == ImGuizmo::ROTATE)
            snapValue = 15.0f;
        
        float snapValues[3] = { snapValue, snapValue, snapValue };

        // For 2D mode: The gizmo is viewed from looking down the Z axis
        // In orthographic projection, only X and Y axes will be prominently visible
        // The Z axis (blue) will appear as a dot/circle which we can ignore
        
        ImGuizmo::Manipulate(glm::value_ptr(viewMatrix), glm::value_ptr(projectionMatrix),
            operation, mode, glm::value_ptr(transform),
            nullptr, snapping ? snapValues : nullptr);

        // If gizmo was used, decompose the matrix back to transform
        if (ImGuizmo::IsUsing())
        {
            glm::vec3 translation, rotation, scale;
            glm::quat rotationQuat;
            glm::vec3 skew;
            glm::vec4 perspective;
            glm::decompose(transform, scale, rotationQuat, translation, skew, perspective);

            // Extract rotation in degrees
            glm::vec3 eulerRotation = glm::eulerAngles(rotationQuat);
            
            // Update transform component - force Z to 0 for 2D
            tc.Position = glm::vec2(translation.x, translation.y);
            tc.Rotation = glm::degrees(eulerRotation.z);
            tc.Scale = glm::vec2(scale.x, scale.y);
        }
    }

    void ViewportPanel::DrawGizmoToolbar()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
        
        // Background for toolbar
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 0.8f));

        ImGui::Begin("##gizmotoolbar", nullptr, 
            ImGuiWindowFlags_NoDecoration | 
            ImGuiWindowFlags_NoScrollbar | 
            ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_AlwaysAutoResize);

        float buttonSize = 32.0f;
        
        // None/Select button (Q)
        {
            bool selected = m_GizmoMode == GizmoMode::None;
            if (selected)
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
            else
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 0.9f));
            
            if (ImGui::Button("Q##SelectMode", ImVec2(buttonSize, buttonSize)))
                m_GizmoMode = GizmoMode::None;
            
            ImGui::PopStyleColor();
                
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Select Mode (Q)\nNo gizmo, selection only");
        }
        
        ImGui::SameLine();
        
        // Translate button (W)
        {
            bool selected = m_GizmoMode == GizmoMode::Translate;
            if (selected)
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
            else
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 0.9f));
            
            if (ImGui::Button("W##TranslateMode", ImVec2(buttonSize, buttonSize)))
                m_GizmoMode = GizmoMode::Translate;
            
            ImGui::PopStyleColor();
                
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Translate Mode (W)\nMove entity position");
        }
        
        ImGui::SameLine();
        
        // Rotate button (E)
        {
            bool selected = m_GizmoMode == GizmoMode::Rotate;
            if (selected)
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
            else
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 0.9f));
            
            if (ImGui::Button("E##RotateMode", ImVec2(buttonSize, buttonSize)))
                m_GizmoMode = GizmoMode::Rotate;
            
            ImGui::PopStyleColor();
                
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Rotate Mode (E)\nRotate entity");
        }
        
        ImGui::SameLine();
        
        // Scale button (R)
        {
            bool selected = m_GizmoMode == GizmoMode::Scale;
            if (selected)
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
            else
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 0.9f));
            
            if (ImGui::Button("R##ScaleMode", ImVec2(buttonSize, buttonSize)))
                m_GizmoMode = GizmoMode::Scale;
            
            ImGui::PopStyleColor();
                
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Scale Mode (R)\nResize entity");
        }

        ImGui::End();
        
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(4);
    }

}
