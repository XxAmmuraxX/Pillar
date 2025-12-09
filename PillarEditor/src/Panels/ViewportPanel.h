#pragma once

#include "EditorPanel.h"
#include "../EditorCamera.h"
#include "Pillar/Renderer/Framebuffer.h"
#include "Pillar/Application.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>

namespace PillarEditor {

    class ViewportPanel : public EditorPanel
    {
    public:
        ViewportPanel();

        virtual void OnImGuiRender() override;
        virtual void OnUpdate(float deltaTime) override;
        virtual void OnEvent(Pillar::Event& e) override;

        void RenderScene();  // Called before OnImGuiRender to render to framebuffer
        void ResetCamera();  // Reset camera to origin

        EditorCamera& GetCamera() { return m_EditorCamera; }
        const EditorCamera& GetCamera() const { return m_EditorCamera; }

        bool IsViewportFocused() const { return m_ViewportFocused; }
        bool IsViewportHovered() const { return m_ViewportHovered; }

        glm::vec2 GetViewportSize() const { return m_ViewportSize; }
        const glm::vec2* GetViewportBounds() const { return m_ViewportBounds; }

    private:
        void DrawGrid();
        glm::vec4 GetEntityColor(const std::string& tag);
        glm::vec2 GetEntitySize(const std::string& tag, const glm::vec2& scale);

    private:
        std::shared_ptr<Pillar::Framebuffer> m_Framebuffer;
        EditorCamera m_EditorCamera;

        glm::vec2 m_ViewportSize = { 1280.0f, 720.0f };
        glm::vec2 m_ViewportBounds[2] = {};

        bool m_ViewportFocused = false;
        bool m_ViewportHovered = false;
    };

}
