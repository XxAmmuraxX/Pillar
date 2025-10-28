#pragma once

#include "Layer.h"
#include "Pillar/Events/ApplicationEvent.h"
#include "Pillar/Events/KeyEvent.h"
#include "Pillar/Events/MouseEvent.h"

struct GLFWwindow;

namespace Pillar {

    class PIL_API ImGuiLayer : public Layer
    {
    public:
        ImGuiLayer();
        ~ImGuiLayer();

        virtual void OnAttach() override;
        virtual void OnDetach() override;
        virtual void OnUpdate(float deltaTime) override;
        virtual void OnImGuiRender() override;
        virtual void OnEvent(Event& event) override;

        void Begin();
        void End();

        void SetBlockEvents(bool block) { m_BlockEvents = block; }
        bool IsBlockingEvents() const { return m_BlockEvents; }

    private:
        bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
        bool OnMouseButtonReleased(MouseButtonReleasedEvent& e);
        bool OnMouseMoved(MouseMovedEvent& e);
        bool OnMouseScrolled(MouseScrolledEvent& e);
        bool OnKeyPressed(KeyPressedEvent& e);
        bool OnKeyReleased(KeyReleasedEvent& e);
        bool OnWindowResize(WindowResizeEvent& e);

    private:
        bool m_BlockEvents = true;
        float m_Time = 0.0f;
    };

}