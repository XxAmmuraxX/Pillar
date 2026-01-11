#include "ViewportPanel.h"
#include "../SelectionContext.h"
#include "../EditorLayer.h"
#include "../Commands/TransformCommand.h"
#include "../EditorConstants.h"
#include "ConsolePanel.h"
#include "Pillar/Renderer/Renderer.h"
#include "Pillar/Renderer/Renderer2DBackend.h"
#include "Pillar/Renderer/RenderCommand.h"
#include "Pillar/ECS/Components/Core/TagComponent.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Physics/ColliderComponent.h"
#include "Pillar/ECS/Components/Physics/RigidbodyComponent.h"
#include "Pillar/ECS/Components/Rendering/SpriteComponent.h"
#include "Pillar/ECS/Components/Rendering/CameraComponent.h"
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

    using namespace Constants;

    ViewportPanel::ViewportPanel(EditorLayer* editorLayer)
        : EditorPanel("Viewport")
        , m_EditorLayer(editorLayer)
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
        
        // Handle viewport keyboard shortcuts (only when focused and not typing)
        if (m_ViewportFocused && !ImGui::GetIO().WantTextInput && !m_GizmoInUse)
        {
            // L - Toggle entity labels
            if (Pillar::Input::IsKeyPressed(PIL_KEY_L))
            {
                m_ShowEntityLabels = !m_ShowEntityLabels;
            }
            
            // C - Toggle collider gizmos
            if (Pillar::Input::IsKeyPressed(PIL_KEY_C))
            {
                m_ShowColliderGizmos = !m_ShowColliderGizmos;
            }
            
            // X - Toggle rigidbody gizmos
            if (Pillar::Input::IsKeyPressed(PIL_KEY_X))
            {
                m_ShowRigidbodyGizmos = !m_ShowRigidbodyGizmos;
            }
            
            // Arrow key nudging
            glm::vec2 nudge(0.0f);
            bool hasNudge = false;
            
            // Determine nudge direction
            if (Pillar::Input::IsKeyPressed(PIL_KEY_LEFT))
            {
                nudge.x = -1.0f;
                hasNudge = true;
            }
            else if (Pillar::Input::IsKeyPressed(PIL_KEY_RIGHT))
            {
                nudge.x = 1.0f;
                hasNudge = true;
            }
            
            if (Pillar::Input::IsKeyPressed(PIL_KEY_UP))
            {
                nudge.y = 1.0f;
                hasNudge = true;
            }
            else if (Pillar::Input::IsKeyPressed(PIL_KEY_DOWN))
            {
                nudge.y = -1.0f;
                hasNudge = true;
            }
            
            // Apply nudge if arrow key was pressed and we have selected entities
            if (hasNudge && m_SelectionContext && !m_SelectionContext->GetSelection().empty())
            {
                // Determine nudge speed based on modifiers
                float nudgeSpeed = 0.1f;  // Default: 0.1 units
                
                if (Pillar::Input::IsKeyPressed(PIL_KEY_LEFT_SHIFT) || 
                    Pillar::Input::IsKeyPressed(PIL_KEY_RIGHT_SHIFT))
                {
                    nudgeSpeed = 1.0f;  // Shift: Fast (1.0 unit)
                }
                else if (Pillar::Input::IsKeyPressed(PIL_KEY_LEFT_CONTROL) || 
                         Pillar::Input::IsKeyPressed(PIL_KEY_RIGHT_CONTROL))
                {
                    nudgeSpeed = 0.01f;  // Ctrl: Precise (0.01 unit)
                }
                
                nudge *= nudgeSpeed;
                ApplyNudge(nudge);
            }
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
        Pillar::RenderCommand::SetClearColor({ Viewport::BACKGROUND_COLOR.x, Viewport::BACKGROUND_COLOR.y, Viewport::BACKGROUND_COLOR.z, Viewport::BACKGROUND_COLOR.w });
        Pillar::RenderCommand::Clear();

        if (m_Scene)
        {
            // Determine which camera to use based on editor state
            bool useGameCamera = m_EditorLayer && 
                                m_EditorLayer->GetEditorState() == PillarEditor::EditorState::Play;
            
            Pillar::OrthographicCamera* activeCamera = &m_EditorCamera.GetCamera();
            
            // Try to find primary game camera if in play mode
            if (useGameCamera)
            {
                auto cameraView = m_Scene->GetRegistry().view<Pillar::CameraComponent, Pillar::TransformComponent>();
                for (auto entity : cameraView)
                {
                    auto& camera = cameraView.get<Pillar::CameraComponent>(entity);
                    if (camera.Primary)
                    {
                        auto& transform = cameraView.get<Pillar::TransformComponent>(entity);
                        float aspectRatio = m_ViewportSize.x / m_ViewportSize.y;
                        
                        // Update game camera projection based on camera component
                        float orthoLeft = -camera.OrthographicSize * aspectRatio * 0.5f;
                        float orthoRight = camera.OrthographicSize * aspectRatio * 0.5f;
                        float orthoBottom = -camera.OrthographicSize * 0.5f;
                        float orthoTop = camera.OrthographicSize * 0.5f;
                        m_GameCamera.SetProjection(orthoLeft, orthoRight, orthoBottom, orthoTop);
                        
                        // Update game camera transform
                        m_GameCamera.SetPosition(glm::vec3(transform.Position, 0.0f));
                        m_GameCamera.SetRotation(transform.Rotation);
                        
                        activeCamera = &m_GameCamera;
                        break;
                    }
                }
            }
            
            Pillar::Renderer2DBackend::BeginScene(*activeCamera);

            // Draw grid first (will be behind entities)
            DrawGrid();

            // Render all entities with TransformComponent
            auto view = m_Scene->GetRegistry().view<Pillar::TagComponent, Pillar::TransformComponent>();
            
            // Sort entities by Z-index for proper layering
            std::vector<entt::entity> sortedEntities(view.begin(), view.end());
            std::sort(sortedEntities.begin(), sortedEntities.end(),
                [this](entt::entity a, entt::entity b) {
                    auto* spriteA = m_Scene->GetRegistry().try_get<Pillar::SpriteComponent>(a);
                    auto* spriteB = m_Scene->GetRegistry().try_get<Pillar::SpriteComponent>(b);
                    
                    // Entities without sprites go first (lower Z)
                    if (!spriteA && spriteB) return true;
                    if (spriteA && !spriteB) return false;
                    if (!spriteA && !spriteB) return false; // Keep original order
                    
                    // Sort by Z-index (use final Z-index from layer system)
                    return spriteA->GetFinalZIndex() < spriteB->GetFinalZIndex();
                });
            
            for (auto entity : sortedEntities)
            {
                auto& tag = m_Scene->GetRegistry().get<Pillar::TagComponent>(entity);
                auto& transform = m_Scene->GetRegistry().get<Pillar::TransformComponent>(entity);

                // Check if entity has a SpriteComponent
                auto* spriteComp = m_Scene->GetRegistry().try_get<Pillar::SpriteComponent>(entity);
                
                // Skip invisible sprites (respects layer visibility)
                if (spriteComp && !spriteComp->Visible)
                    continue;
                
                // Determine color and size
                glm::vec4 color;
                glm::vec2 size;
                
                if (spriteComp)
                {
                    // Use sprite component data
                    color = spriteComp->Color;
                    size = spriteComp->Size * transform.Scale;
                }
                else
                {
                    // Fallback to colored quad based on entity type
                    color = GetEntityColor(tag.Tag);
                    size = GetEntitySize(tag.Tag, transform.Scale);
                }

                // Check if entity is selected
                bool isSelected = false;
                if (m_SelectionContext)
                {
                    Pillar::Entity e(entity, m_Scene.get());
                    isSelected = m_SelectionContext->IsSelected(e);
                }

                // Draw entity (with rotation if needed)
                if (spriteComp && spriteComp->Texture)
                {
                    // Draw textured sprite with UV coordinates and flip flags
                    // Use vec3 position with sprite's Z-index for depth sorting
                    glm::vec3 position3D(transform.Position.x, transform.Position.y, spriteComp->ZIndex);
                    
                    if (std::abs(transform.Rotation) > 0.001f)
                    {
                        Pillar::Renderer2DBackend::DrawRotatedQuad(
                            position3D, size, transform.Rotation,
                            color, spriteComp->Texture,
                            spriteComp->TexCoordMin, spriteComp->TexCoordMax,
                            spriteComp->FlipX, spriteComp->FlipY
                        );
                    }
                    else
                    {
                        Pillar::Renderer2DBackend::DrawQuad(
                            position3D, size,
                            color, spriteComp->Texture,
                            spriteComp->TexCoordMin, spriteComp->TexCoordMax,
                            spriteComp->FlipX, spriteComp->FlipY
                        );
                    }
                }
                else
                {
                    // Draw colored quad
                    if (std::abs(transform.Rotation) > 0.001f)
                    {
                        Pillar::Renderer2DBackend::DrawRotatedQuad(transform.Position, size, transform.Rotation, color);
                    }
                    else
                    {
                        Pillar::Renderer2DBackend::DrawQuad(transform.Position, size, color);
                    }
                }

                // Draw selection highlight (on top of entity with thicker outline)
                if (isSelected)
                {
                    // Draw a thick border by drawing 4 rectangles around the entity
                    glm::vec4 outlineColor = { Viewport::SELECTION_COLOR.x, Viewport::SELECTION_COLOR.y, Viewport::SELECTION_COLOR.z, Viewport::SELECTION_COLOR.w };  // Bright orange
                    float borderThickness = 0.08f;
                    float rotation = transform.Rotation;
                    
                    // Calculate rotated offset vectors
                    float cosR = std::cos(rotation);
                    float sinR = std::sin(rotation);
                    
                    // For rotated entities, we need to offset in the entity's local space
                    if (std::abs(rotation) > 0.001f)
                    {
                        // Calculate half extents
                        float halfWidth = size.x / 2.0f;
                        float halfHeight = size.y / 2.0f;
                        float offset = borderThickness / 2.0f;
                        
                        // Top border - offset in local Y direction
                        glm::vec2 topOffset = glm::vec2(-sinR * (halfHeight + offset), cosR * (halfHeight + offset));
                        Pillar::Renderer2DBackend::DrawRotatedQuad(
                            transform.Position + topOffset,
                            glm::vec2(size.x + borderThickness * 2.0f, borderThickness),
                            rotation, outlineColor
                        );
                        
                        // Bottom border - offset in local -Y direction
                        glm::vec2 bottomOffset = glm::vec2(sinR * (halfHeight + offset), -cosR * (halfHeight + offset));
                        Pillar::Renderer2DBackend::DrawRotatedQuad(
                            transform.Position + bottomOffset,
                            glm::vec2(size.x + borderThickness * 2.0f, borderThickness),
                            rotation, outlineColor
                        );
                        
                        // Left border - offset in local -X direction
                        glm::vec2 leftOffset = glm::vec2(-cosR * (halfWidth + offset), -sinR * (halfWidth + offset));
                        Pillar::Renderer2DBackend::DrawRotatedQuad(
                            transform.Position + leftOffset,
                            glm::vec2(borderThickness, size.y),
                            rotation, outlineColor
                        );
                        
                        // Right border - offset in local X direction
                        glm::vec2 rightOffset = glm::vec2(cosR * (halfWidth + offset), sinR * (halfWidth + offset));
                        Pillar::Renderer2DBackend::DrawRotatedQuad(
                            transform.Position + rightOffset,
                            glm::vec2(borderThickness, size.y),
                            rotation, outlineColor
                        );
                    }
                    else
                    {
                        // Non-rotated borders (faster)
                        // Top border
                        Pillar::Renderer2DBackend::DrawQuad(
                            glm::vec2(transform.Position.x, transform.Position.y + size.y / 2.0f + borderThickness / 2.0f),
                            glm::vec2(size.x + borderThickness * 2.0f, borderThickness),
                            outlineColor
                        );
                        
                        // Bottom border
                        Pillar::Renderer2DBackend::DrawQuad(
                            glm::vec2(transform.Position.x, transform.Position.y - size.y / 2.0f - borderThickness / 2.0f),
                            glm::vec2(size.x + borderThickness * 2.0f, borderThickness),
                            outlineColor
                        );
                        
                        // Left border
                        Pillar::Renderer2DBackend::DrawQuad(
                            glm::vec2(transform.Position.x - size.x / 2.0f - borderThickness / 2.0f, transform.Position.y),
                            glm::vec2(borderThickness, size.y),
                            outlineColor
                        );
                        
                        // Right border
                        Pillar::Renderer2DBackend::DrawQuad(
                            glm::vec2(transform.Position.x + size.x / 2.0f + borderThickness / 2.0f, transform.Position.y),
                            glm::vec2(borderThickness, size.y),
                            outlineColor
                        );
                    }
                }
            }

            // Draw collider gizmos (if enabled)
            if (m_ShowColliderGizmos)
                DrawColliderGizmos();

            // Draw rigidbody gizmos (if enabled)
            if (m_ShowRigidbodyGizmos)
                DrawRigidbodyGizmos();

            Pillar::Renderer2DBackend::EndScene();
        }
        else
        {
            // No scene - just show empty viewport with hint
            Pillar::Renderer2DBackend::BeginScene(m_EditorCamera.GetCamera());
            DrawGrid();
            Pillar::Renderer2DBackend::EndScene();
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
            Pillar::Renderer2DBackend::DrawQuad(
                glm::vec2(x, camPos.y),
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
            Pillar::Renderer2DBackend::DrawQuad(
                glm::vec2(camPos.x, y),
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
        
        // Handle viewport resize (with threshold to avoid unnecessary resizes)
        uint32_t newWidth = (uint32_t)viewportPanelSize.x;
        uint32_t newHeight = (uint32_t)viewportPanelSize.y;
        uint32_t currentWidth = m_Framebuffer->GetSpecification().Width;
        uint32_t currentHeight = m_Framebuffer->GetSpecification().Height;
        
        if (newWidth > 0 && newHeight > 0 && (newWidth != currentWidth || newHeight != currentHeight))
        {
            m_ViewportSize = { (float)newWidth, (float)newHeight };
            m_Framebuffer->Resize(newWidth, newHeight);
            m_EditorCamera.SetViewportSize((float)newWidth, (float)newHeight);
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

        // Draw entity name labels for selected entities
        DrawEntityLabels();

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
        transform = glm::rotate(transform, tc.Rotation, glm::vec3(0.0f, 0.0f, 1.0f));  // tc.Rotation is already in radians
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

        // Track gizmo state for undo/redo
        bool isCurrentlyUsing = ImGuizmo::IsUsing();
        
        // Capture initial state when gizmo manipulation starts
        if (isCurrentlyUsing && !m_GizmoInUse)
        {
            m_GizmoInUse = true;
            m_GizmoStartPosition = tc.Position;
            m_GizmoStartRotation = tc.Rotation;
            m_GizmoStartScale = tc.Scale;
        }

        // If gizmo was used, decompose the matrix back to transform
        if (isCurrentlyUsing)
        {
            glm::vec3 translation, rotation, scale;
            glm::quat rotationQuat;
            glm::vec3 skew;
            glm::vec4 perspective;
            glm::decompose(transform, scale, rotationQuat, translation, skew, perspective);

            // Extract rotation in degrees
            glm::vec3 eulerRotation = glm::eulerAngles(rotationQuat);
            float rotationRadians = eulerRotation.z;
            
            // Normalize rotation to -PI to PI range (radians)
            while (rotationRadians > glm::pi<float>())
                rotationRadians -= glm::two_pi<float>();
            while (rotationRadians < -glm::pi<float>())
                rotationRadians += glm::two_pi<float>();
            
            // Update transform component - force Z to 0 for 2D
            tc.Position = glm::vec2(translation.x, translation.y);
            tc.Rotation = rotationRadians;  // Store in radians
            tc.Scale = glm::vec2(scale.x, scale.y);
        }
        
        // Create undo command when gizmo manipulation ends
        if (!isCurrentlyUsing && m_GizmoInUse)
        {
            m_GizmoInUse = false;
            
            // Check if transform actually changed
            bool changed = (tc.Position != m_GizmoStartPosition) ||
                          (tc.Rotation != m_GizmoStartRotation) ||
                          (tc.Scale != m_GizmoStartScale);
            
            if (changed && m_EditorLayer)
            {
                // Create command with old and new states
                std::vector<TransformCommand::TransformState> oldStates;
                std::vector<TransformCommand::TransformState> newStates;
                
                oldStates.push_back({
                    static_cast<entt::entity>(selectedEntity),
                    m_GizmoStartPosition,
                    m_GizmoStartRotation,
                    m_GizmoStartScale
                });
                
                newStates.push_back({
                    static_cast<entt::entity>(selectedEntity),
                    tc.Position,
                    tc.Rotation,
                    tc.Scale
                });
                
                // Determine action name based on gizmo mode
                std::string actionName = "Transform";
                if (m_GizmoMode == GizmoMode::Translate)
                    actionName = "Move";
                else if (m_GizmoMode == GizmoMode::Rotate)
                    actionName = "Rotate";
                else if (m_GizmoMode == GizmoMode::Scale)
                    actionName = "Scale";
                
                auto command = std::make_unique<TransformCommand>(
                    m_Scene.get(), oldStates, newStates, actionName);
                m_EditorLayer->GetCommandHistory().ExecuteCommand(std::move(command));
            }
        }
    }

    void ViewportPanel::DrawGizmoToolbar()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 6));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 4));

        ImGuiWindowFlags toolbarFlags = ImGuiWindowFlags_NoScrollbar | 
                                        ImGuiWindowFlags_NoScrollWithMouse;

        ImGui::Begin("Transform Tools", nullptr, toolbarFlags);

        float buttonWidth = 90.0f;
        float buttonHeight = 28.0f;
        
        // None/Select button (Q)
        {
            bool selected = m_GizmoMode == GizmoMode::None;
            if (selected)
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
            
            if (ImGui::Button("Select (Q)", ImVec2(buttonWidth, buttonHeight)))
                m_GizmoMode = GizmoMode::None;
            
            if (selected)
                ImGui::PopStyleColor();
                
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Select Mode (Q)\nClick to select entities");
        }
        
        ImGui::SameLine();
        
        // Translate button (W)
        {
            bool selected = m_GizmoMode == GizmoMode::Translate;
            if (selected)
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
            
            if (ImGui::Button("Move (W)", ImVec2(buttonWidth, buttonHeight)))
                m_GizmoMode = GizmoMode::Translate;
            
            if (selected)
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
            
            if (ImGui::Button("Rotate (E)", ImVec2(buttonWidth, buttonHeight)))
                m_GizmoMode = GizmoMode::Rotate;
            
            if (selected)
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
            
            if (ImGui::Button("Scale (R)", ImVec2(buttonWidth, buttonHeight)))
                m_GizmoMode = GizmoMode::Scale;
            
            if (selected)
                ImGui::PopStyleColor();
                
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Scale Mode (R)\\nResize entity");
        }

        ImGui::End();
        ImGui::PopStyleVar(2);
    }

    void ViewportPanel::DrawEntityLabels()
    {
        if (!m_ShowEntityLabels || !m_SelectionContext || !m_SelectionContext->HasSelection())
            return;

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        
        auto& selected = m_SelectionContext->GetSelection();
        for (auto entityHandle : selected)
        {
            Pillar::Entity entity{ entityHandle, m_Scene.get() };
            if (entity && entity.HasComponent<Pillar::TagComponent>() && 
                entity.HasComponent<Pillar::TransformComponent>())
            {
                auto& tag = entity.GetComponent<Pillar::TagComponent>();
                auto& transform = entity.GetComponent<Pillar::TransformComponent>();
                
                DrawEntityNameLabel(transform.Position, tag.Tag);
            }
        }
    }

    void ViewportPanel::DrawEntityNameLabel(const glm::vec2& worldPos, const std::string& name)
    {
        // Convert world position to screen coordinates
        ImVec2 screenPos = WorldToScreenImGui(worldPos);
        
        // Get ImGui draw list
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        
        // Calculate text size
        ImVec2 textSize = ImGui::CalcTextSize(name.c_str());
        
        // Position text above entity (offset by 20 pixels)
        ImVec2 textPos = ImVec2(screenPos.x - textSize.x * 0.5f, screenPos.y - textSize.y - 25.0f);
        
        // Draw background rectangle with rounded corners
        ImVec2 bgMin = ImVec2(textPos.x - 4.0f, textPos.y - 2.0f);
        ImVec2 bgMax = ImVec2(textPos.x + textSize.x + 4.0f, textPos.y + textSize.y + 2.0f);
        drawList->AddRectFilled(bgMin, bgMax, IM_COL32(0, 0, 0, 180), 3.0f);
        drawList->AddRect(bgMin, bgMax, IM_COL32(255, 180, 0, 200), 3.0f, 0, 1.0f);
        
        // Draw text
        drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), name.c_str());
    }

    ImVec2 ViewportPanel::WorldToScreenImGui(const glm::vec2& worldPos)
    {
        // Get camera matrices
        glm::mat4 viewProj = m_EditorCamera.GetCamera().GetViewProjectionMatrix();
        
        // Convert world to NDC (Normalized Device Coordinates)
        glm::vec4 clipSpace = viewProj * glm::vec4(worldPos.x, worldPos.y, 0.0f, 1.0f);
        glm::vec3 ndc = glm::vec3(clipSpace) / clipSpace.w;
        
        // Convert NDC to screen space (0 to viewport size)
        float screenX = (ndc.x + 1.0f) * 0.5f * m_ViewportSize.x;
        float screenY = (1.0f - ndc.y) * 0.5f * m_ViewportSize.y;
        
        // Add viewport bounds offset to get absolute window position
        return ImVec2(m_ViewportBounds[0].x + screenX, m_ViewportBounds[0].y + screenY);
    }

    void ViewportPanel::ApplyNudge(const glm::vec2& nudge)
    {
        if (!m_SelectionContext || !m_Scene || !m_EditorLayer)
            return;

        auto& selected = m_SelectionContext->GetSelection();
        if (selected.empty())
            return;

        // Store old and new transform states for undo/redo
        std::vector<TransformCommand::TransformState> oldStates;
        std::vector<TransformCommand::TransformState> newStates;

        auto& registry = m_Scene->GetRegistry();

        for (auto entityHandle : selected)
        {
            if (!registry.valid(entityHandle))
                continue;

            if (registry.all_of<Pillar::TransformComponent>(entityHandle))
            {
                auto& transform = registry.get<Pillar::TransformComponent>(entityHandle);

                // Store old state
                TransformCommand::TransformState oldState;
                oldState.EntityID = entityHandle;
                oldState.Position = transform.Position;
                oldState.Rotation = transform.Rotation;
                oldState.Scale = transform.Scale;
                oldStates.push_back(oldState);

                // Apply nudge
                transform.Position += nudge;
                transform.Dirty = true;

                // Store new state
                TransformCommand::TransformState newState;
                newState.EntityID = entityHandle;
                newState.Position = transform.Position;
                newState.Rotation = transform.Rotation;
                newState.Scale = transform.Scale;
                newStates.push_back(newState);
            }
        }

        // Create and execute undo command
        if (!oldStates.empty())
        {
            auto command = std::make_unique<TransformCommand>(
                m_Scene.get(), oldStates, newStates, "Nudge Entity"
            );
            m_EditorLayer->GetCommandHistory().ExecuteCommand(std::move(command));
        }
    }

    void ViewportPanel::DrawColliderGizmos()
    {
        if (!m_Scene)
            return;

        // Get all entities with colliders
        auto view = m_Scene->GetRegistry().view<Pillar::TransformComponent, Pillar::ColliderComponent>();

        for (auto entity : view)
        {
            auto& transform = view.get<Pillar::TransformComponent>(entity);
            auto& collider = view.get<Pillar::ColliderComponent>(entity);

            // Determine color based on selection and sensor status
            glm::vec4 color;
            Pillar::Entity e(entity, m_Scene.get());
            bool isSelected = m_SelectionContext && m_SelectionContext->IsSelected(e);

            if (collider.IsSensor)
            {
                // Sensors are yellow/orange
                color = isSelected ? glm::vec4(1.0f, 0.8f, 0.0f, 0.6f) : glm::vec4(1.0f, 0.8f, 0.0f, 0.3f);
            }
            else
            {
                // Regular colliders: green when selected, blue otherwise
                color = isSelected ? glm::vec4(0.0f, 1.0f, 0.0f, 0.6f) : glm::vec4(0.0f, 0.5f, 1.0f, 0.4f);
            }

            // Rotate the collider offset by the entity's rotation
            float cosR = glm::cos(transform.Rotation);
            float sinR = glm::sin(transform.Rotation);
            glm::vec2 rotatedOffset = glm::vec2(
                collider.Offset.x * cosR - collider.Offset.y * sinR,
                collider.Offset.x * sinR + collider.Offset.y * cosR
            );

            // Calculate world position (transform + rotated offset)
            glm::vec2 worldPos = transform.Position + rotatedOffset;

            // Draw based on shape type
            switch (collider.Type)
            {
            case Pillar::ColliderType::Circle:
            {
                Pillar::Renderer2DBackend::DrawCircle(worldPos, collider.Radius, color, 32, 2.0f);
                break;
            }

            case Pillar::ColliderType::Box:
            {
                glm::vec2 size = collider.HalfExtents * 2.0f;
                DrawWireBox(worldPos, size, transform.Rotation, color);
                break;
            }

            case Pillar::ColliderType::Polygon:
            {
                // Draw polygon outline
                if (collider.Vertices.size() >= 3)
                {
                    for (size_t i = 0; i < collider.Vertices.size(); i++)
                    {
                        size_t nextIdx = (i + 1) % collider.Vertices.size();
                        
                        // Rotate vertices by transform rotation
                        float cosR = glm::cos(transform.Rotation);
                        float sinR = glm::sin(transform.Rotation);
                        
                        glm::vec2 v1 = collider.Vertices[i];
                        glm::vec2 v2 = collider.Vertices[nextIdx];
                        
                        glm::vec2 rotatedV1 = glm::vec2(
                            v1.x * cosR - v1.y * sinR,
                            v1.x * sinR + v1.y * cosR
                        );
                        glm::vec2 rotatedV2 = glm::vec2(
                            v2.x * cosR - v2.y * sinR,
                            v2.x * sinR + v2.y * cosR
                        );
                        
                        glm::vec2 start = worldPos + rotatedV1;
                        glm::vec2 end = worldPos + rotatedV2;
                        
                        Pillar::Renderer2DBackend::DrawLine(start, end, color, 2.0f);
                    }
                }
                break;
            }
            }
        }
    }

    void ViewportPanel::DrawRigidbodyGizmos()
    {
        if (!m_Scene)
            return;

        bool isPlaying = m_EditorLayer && m_EditorLayer->GetEditorState() == EditorState::Play;

        // Get all entities with rigidbodies
        auto view = m_Scene->GetRegistry().view<Pillar::TransformComponent, Pillar::RigidbodyComponent>();

        for (auto entity : view)
        {
            auto& transform = view.get<Pillar::TransformComponent>(entity);
            auto& rb = view.get<Pillar::RigidbodyComponent>(entity);

            Pillar::Entity e(entity, m_Scene.get());
            bool isSelected = m_SelectionContext && m_SelectionContext->IsSelected(e);

            // Determine body type color
            glm::vec4 bodyColor;
            switch (rb.BodyType)
            {
            case b2_staticBody:
                // Static: Gray
                bodyColor = glm::vec4(0.7f, 0.7f, 0.7f, 0.9f);
                break;
            case b2_kinematicBody:
                // Kinematic: Cyan/Blue
                bodyColor = glm::vec4(0.3f, 0.7f, 1.0f, 0.9f);
                break;
            case b2_dynamicBody:
                // Dynamic: Bright Green
                bodyColor = glm::vec4(0.4f, 1.0f, 0.4f, 0.9f);
                break;
            }

            // Modify color based on body state (play mode only)
            if (isPlaying && rb.Body)
            {
                if (!rb.Body->IsEnabled())
                {
                    // Inactive: Bright Red
                    bodyColor = glm::vec4(1.0f, 0.3f, 0.3f, 0.9f);
                }
                else if (!rb.Body->IsAwake())
                {
                    // Sleeping: Desaturate and dim
                    bodyColor.r *= 0.5f;
                    bodyColor.g *= 0.5f;
                    bodyColor.b *= 0.5f;
                    bodyColor.a = 0.6f;
                }
            }

            // Make selected bodies even brighter with white outline
            if (isSelected)
            {
                bodyColor.a = 1.0f;
            }

            // === BODY TYPE INDICATOR ===
            // Draw a small circle or icon at the entity center
            float indicatorSize = 0.12f;

            // Draw dark background for contrast
            glm::vec4 bgColor(0.0f, 0.0f, 0.0f, 0.6f);

            if (rb.BodyType == b2_staticBody)
            {
                // Static: Filled square (immovable)
                Pillar::Renderer2DBackend::DrawCircle(transform.Position, indicatorSize * 1.2f, bgColor, 4, 0.0f);
                Pillar::Renderer2DBackend::DrawCircle(transform.Position, indicatorSize, bodyColor, 4, 0.0f);
            }
            else if (rb.BodyType == b2_kinematicBody)
            {
                // Kinematic: Filled diamond shape (controlled movement)
                float halfSize = indicatorSize;
                glm::vec2 top = transform.Position + glm::vec2(0.0f, halfSize);
                glm::vec2 right = transform.Position + glm::vec2(halfSize, 0.0f);
                glm::vec2 bottom = transform.Position + glm::vec2(0.0f, -halfSize);
                glm::vec2 left = transform.Position + glm::vec2(-halfSize, 0.0f);

                // Background
                Pillar::Renderer2DBackend::DrawLine(top, right, bgColor, 4.0f);
                Pillar::Renderer2DBackend::DrawLine(right, bottom, bgColor, 4.0f);
                Pillar::Renderer2DBackend::DrawLine(bottom, left, bgColor, 4.0f);
                Pillar::Renderer2DBackend::DrawLine(left, top, bgColor, 4.0f);

                // Main shape
                Pillar::Renderer2DBackend::DrawLine(top, right, bodyColor, 3.0f);
                Pillar::Renderer2DBackend::DrawLine(right, bottom, bodyColor, 3.0f);
                Pillar::Renderer2DBackend::DrawLine(bottom, left, bodyColor, 3.0f);
                Pillar::Renderer2DBackend::DrawLine(left, top, bodyColor, 3.0f);
            }
            else // Dynamic
            {
                // Dynamic: Filled circle (fully simulated)
                Pillar::Renderer2DBackend::DrawCircle(transform.Position, indicatorSize * 1.3f, bgColor, 16, 0.0f);
                Pillar::Renderer2DBackend::DrawCircle(transform.Position, indicatorSize, bodyColor, 16, 0.0f);
            }

            // === VELOCITY VECTOR (Play Mode, Dynamic Bodies Only) ===
            if (isPlaying && rb.Body && rb.BodyType == b2_dynamicBody)
            {
                b2Vec2 linearVel = rb.Body->GetLinearVelocity();
                float speed = linearVel.Length();

                // Only draw if moving significantly
                if (speed > 0.5f)
                {
                    // Scale velocity for visibility (clamp to reasonable length)
                    float arrowLength = glm::min(speed * 0.05f, 1.5f);
                    glm::vec2 velDir(linearVel.x, linearVel.y);
                    if (speed > 0.0f)
                        velDir = glm::normalize(velDir);

                    glm::vec2 arrowEnd = transform.Position + velDir * arrowLength;

                    // Color based on speed (green -> yellow -> red)
                    glm::vec4 velColor;
                    if (speed < 10.0f)
                        velColor = glm::vec4(0.4f, 1.0f, 0.4f, 1.0f); // Bright Green: slow
                    else if (speed < 20.0f)
                        velColor = glm::vec4(1.0f, 1.0f, 0.2f, 1.0f); // Bright Yellow: medium
                    else
                        velColor = glm::vec4(1.0f, 0.4f, 0.2f, 1.0f); // Bright Red: fast

                    // Draw background shadow for contrast
                    glm::vec4 shadowColor(0.0f, 0.0f, 0.0f, 0.7f);
                    Pillar::Renderer2DBackend::DrawLine(transform.Position, arrowEnd, shadowColor, 4.0f);

                    // Draw arrow shaft (thicker, more visible)
                    Pillar::Renderer2DBackend::DrawLine(transform.Position, arrowEnd, velColor, 3.0f);

                    // Draw filled arrowhead (triangle)
                    float arrowheadSize = 0.15f;
                    glm::vec2 perpendicular(-velDir.y, velDir.x);
                    glm::vec2 arrowBase = arrowEnd - velDir * arrowheadSize;
                    glm::vec2 arrowLeft = arrowBase + perpendicular * arrowheadSize * 0.6f;
                    glm::vec2 arrowRight = arrowBase - perpendicular * arrowheadSize * 0.6f;

                    // Shadow
                    Pillar::Renderer2DBackend::DrawLine(arrowEnd, arrowLeft, shadowColor, 4.0f);
                    Pillar::Renderer2DBackend::DrawLine(arrowEnd, arrowRight, shadowColor, 4.0f);
                    Pillar::Renderer2DBackend::DrawLine(arrowLeft, arrowRight, shadowColor, 4.0f);

                    // Main arrowhead
                    Pillar::Renderer2DBackend::DrawLine(arrowEnd, arrowLeft, velColor, 3.0f);
                    Pillar::Renderer2DBackend::DrawLine(arrowEnd, arrowRight, velColor, 3.0f);
                    Pillar::Renderer2DBackend::DrawLine(arrowLeft, arrowRight, velColor, 3.0f);
                }

                // === CENTER OF MASS INDICATOR ===
                // Draw filled circle at center of mass
                b2Vec2 com = rb.Body->GetWorldCenter();
                glm::vec2 comPos(com.x, com.y);
                float comSize = 0.06f;
                glm::vec4 comColor(1.0f, 0.6f, 0.1f, 1.0f); // Bright Orange

                // Dark background for contrast
                Pillar::Renderer2DBackend::DrawCircle(comPos, comSize * 1.5f, glm::vec4(0.0f, 0.0f, 0.0f, 0.7f), 12, 0.0f);
                // Filled circle
                Pillar::Renderer2DBackend::DrawCircle(comPos, comSize, comColor, 12, 0.0f);
            }
        }
    }

    void ViewportPanel::DrawWireBox(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
        {
            // For axis-aligned boxes, use DrawRect for efficiency
            if (rotation == 0.0f)
            {
                Pillar::Renderer2DBackend::DrawRect(position, size, color, 2.0f);
                return;
            }

            // For rotated boxes, manually draw four lines
            float halfWidth = size.x * 0.5f;
            float halfHeight = size.y * 0.5f;

            // Calculate rotated corner positions
            float cosR = glm::cos(rotation);
            float sinR = glm::sin(rotation);

            // Local corner positions (relative to center)
            glm::vec2 corners[4] = {
                { -halfWidth, -halfHeight },  // Bottom-left
                {  halfWidth, -halfHeight },  // Bottom-right
                {  halfWidth,  halfHeight },  // Top-right
                { -halfWidth,  halfHeight }   // Top-left
            };

            // Rotate and translate corners to world space
            glm::vec2 worldCorners[4];
            for (int i = 0; i < 4; i++)
            {
                worldCorners[i] = glm::vec2(
                    position.x + corners[i].x * cosR - corners[i].y * sinR,
                    position.y + corners[i].x * sinR + corners[i].y * cosR
                );
            }

            // Draw four edges
            for (int i = 0; i < 4; i++)
            {
                int nextIdx = (i + 1) % 4;
                Pillar::Renderer2DBackend::DrawLine(worldCorners[i], worldCorners[nextIdx], color, 2.0f);
            }
        }
    }


