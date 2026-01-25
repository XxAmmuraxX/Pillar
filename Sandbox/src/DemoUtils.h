#pragma once

#include "Pillar.h"
#include "Pillar/Renderer/Renderer2D.h"
#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

/**
 * @brief Common utility functions for demo layers to reduce code duplication
 */
namespace DemoUtils
{
    /**
     * @brief Convert screen coordinates to world coordinates using camera
     * @param screenPos Screen position in pixels
     * @param camera The camera to use for conversion
     * @param windowWidth Viewport width in pixels
     * @param windowHeight Viewport height in pixels
     * @return World position as glm::vec2
     */
    inline glm::vec2 ScreenToWorld(
        const glm::vec2& screenPos,
        const Pillar::OrthographicCamera& camera,
        float windowWidth = 1600.0f,
        float windowHeight = 900.0f)
    {
        // Convert screen to NDC (-1 to 1)
        glm::vec2 ndc;
        ndc.x = (2.0f * screenPos.x) / windowWidth - 1.0f;
        ndc.y = 1.0f - (2.0f * screenPos.y) / windowHeight;

        // Convert NDC to world space using camera
        glm::mat4 invViewProj = glm::inverse(camera.GetViewProjectionMatrix());
        glm::vec4 worldPos = invViewProj * glm::vec4(ndc, 0.0f, 1.0f);

        return glm::vec2(worldPos.x, worldPos.y);
    }

    /**
     * @brief Render performance statistics UI panel
     * @param title Panel title
     * @param entityCount Number of entities
     * @param frameTime Frame time in milliseconds
     * @param systemTime System update time in milliseconds
     * @param renderTime Render time in milliseconds
     */
    inline void RenderPerformanceStats(
        const char* title,
        size_t entityCount,
        float frameTime,
        float systemTime,
        float renderTime)
    {
        ImGui::Text("%s", title);
        ImGui::Separator();

        // Performance stats
        ImGui::Text("Entity Count: %zu", entityCount);
        ImGui::Text("Frame Time: %.2f ms (%.0f FPS)", frameTime, 1000.0f / frameTime);
        ImGui::Text("System Time: %.2f ms", systemTime);
        ImGui::Text("Render Time: %.2f ms", renderTime);
        
        // Renderer stats
        ImGui::Separator();
        ImGui::Text("Renderer Statistics:");
        ImGui::Text("  Draw Calls: %u", Pillar::Renderer2D::GetDrawCallCount());
        ImGui::Text("  Quads Rendered: %u", Pillar::Renderer2D::GetQuadCount());
        
        // Color-coded performance
        if (frameTime < 16.67f)
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Performance: EXCELLENT (60+ FPS)");
        else if (frameTime < 33.33f)
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Performance: GOOD (30-60 FPS)");
        else
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Performance: POOR (<30 FPS)");
    }

    /**
     * @brief Render particle pool statistics
     * @param pool The particle pool
     */
    template<typename PoolT>
    inline void RenderPoolStats(const PoolT& pool)
    {
        ImGui::Text("Particle Pool:");
        ImGui::Text("  Active: %zu", pool.GetActiveCount());
        ImGui::Text("  Available: %zu", pool.GetAvailableCount());
        ImGui::Text("  Total: %zu", pool.GetTotalCount());
    }

    /**
     * @brief Render renderer statistics (draw calls, quads)
     */
    inline void RenderRendererStats()
    {
        ImGui::Text("Renderer:");
        ImGui::Text("  Draw Calls: %u", Pillar::Renderer2D::GetDrawCallCount());
        ImGui::Text("  Quads: %u", Pillar::Renderer2D::GetQuadCount());
    }

    /**
     * @brief Render camera controls help text
     */
    inline void RenderCameraHelpText()
    {
        ImGui::Text("Camera: WASD to move, Scroll to zoom");
    }

} // namespace DemoUtils
