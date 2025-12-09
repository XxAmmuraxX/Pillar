#include "ViewportPanel.h"
#include "../SelectionContext.h"
#include "Pillar/Renderer/Renderer.h"
#include "Pillar/Renderer/Renderer2D.h"
#include "Pillar/Renderer/RenderCommand.h"
#include "Pillar/ECS/Components/Core/TagComponent.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Physics/ColliderComponent.h"
#include <imgui.h>
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

                // Draw selection highlight (behind entity)
                if (isSelected)
                {
                    glm::vec4 outlineColor = { 1.0f, 0.6f, 0.0f, 0.8f };  // Orange
                    glm::vec2 outlineSize = size + glm::vec2(0.15f);
                    Pillar::Renderer2D::DrawQuad(
                        glm::vec3(transform.Position, -0.01f),
                        outlineSize,
                        outlineColor
                    );
                }

                // Draw entity
                Pillar::Renderer2D::DrawQuad(transform.Position, size, color);
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

        // Show viewport info overlay using SetCursorPos (simpler, doesn't create child window)
        ImGui::SetCursorPos(ImVec2(10, 30));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 0.8f));
        ImGui::Text("Zoom: %.1fx", 1.0f / m_EditorCamera.GetZoomLevel());
        glm::vec3 pos = m_EditorCamera.GetPosition();
        ImGui::SetCursorPos(ImVec2(10, 48));
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

}
