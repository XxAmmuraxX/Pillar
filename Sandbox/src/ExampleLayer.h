#pragma once

#include "Pillar.h"


class ExampleLayer : public Pillar::Layer
{
public:
	ExampleLayer() : Layer("ExampleLayer") {}
	void OnAttach() override { Layer::OnAttach(); }
	void OnDetach() override { Layer::OnDetach(); }
    void OnUpdate(float dt) override
    {
        // Test keyboard input using Pillar keycodes
        if (Pillar::Input::IsKeyPressed(PIL_KEY_SPACE))
        {
            auto [x, y] = Pillar::Input::GetMousePosition();
            PIL_INFO("SPACE pressed! Mouse at ({0}, {1})", x, y);
        }

        if (Pillar::Input::IsKeyPressed(PIL_KEY_W))
            PIL_INFO("W key pressed");

        if (Pillar::Input::IsKeyPressed(PIL_KEY_A))
            PIL_INFO("A key pressed");

        if (Pillar::Input::IsKeyPressed(PIL_KEY_S))
            PIL_INFO("S key pressed");

        if (Pillar::Input::IsKeyPressed(PIL_KEY_D))
            PIL_INFO("D key pressed");

        // Test mouse buttons
        if (Pillar::Input::IsMouseButtonPressed(PIL_MOUSE_BUTTON_LEFT))
        {
            auto [x, y] = Pillar::Input::GetMousePosition();
            PIL_INFO("Left mouse clicked at ({0}, {1})", x, y);
        }
    }
    void OnEvent(Pillar::Event& event) override
    {
        if (event.GetEventType() == Pillar::EventType::KeyPressed)
        {
			Pillar::KeyPressedEvent& keyEvent = static_cast<Pillar::KeyPressedEvent&>(event);
			PIL_INFO("Key Pressed: {0} (repeats: {1})", keyEvent.GetKeyCode(), keyEvent.GetRepeatCount());
		}
	}
	void OnImGuiRender() override {}
};

