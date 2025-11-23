#pragma once

#include "Pillar.h"
#include "Pillar/Renderer/Renderer2D.h"
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>

class ExampleLayer : public Pillar::Layer
{
public:
	ExampleLayer() 
		: Layer("ExampleLayer"), m_Camera(-1.6f, 1.6f, -0.9f, 0.9f)
	{
		m_Camera.SetPosition({ 0.0f, 0.0f, 0.0f });
	}

	void OnAttach() override 
	{ 
		Layer::OnAttach();
		PIL_INFO("ExampleLayer attached - Testing Renderer2D and Texture");
		
		// Load texture
		m_Texture = Pillar::Texture2D::Create("pillar_logo.png");
		
		PIL_INFO("Renderer2D test initialized with texture!");
	}
	
	void OnDetach() override 
	{ 
		Layer::OnDetach();
	}
	
    void OnUpdate(float dt) override
    {
		// Camera controls
		float cameraSpeed = 2.0f * dt;
		if (Pillar::Input::IsKeyPressed(PIL_KEY_A))
			m_Camera.SetPosition(m_Camera.GetPosition() + glm::vec3(-cameraSpeed, 0.0f, 0.0f));
		if (Pillar::Input::IsKeyPressed(PIL_KEY_D))
			m_Camera.SetPosition(m_Camera.GetPosition() + glm::vec3(cameraSpeed, 0.0f, 0.0f));
		if (Pillar::Input::IsKeyPressed(PIL_KEY_W))
			m_Camera.SetPosition(m_Camera.GetPosition() + glm::vec3(0.0f, cameraSpeed, 0.0f));
		if (Pillar::Input::IsKeyPressed(PIL_KEY_S))
			m_Camera.SetPosition(m_Camera.GetPosition() + glm::vec3(0.0f, -cameraSpeed, 0.0f));

		// Camera rotation
		float rotationSpeed = 50.0f * dt;
		if (Pillar::Input::IsKeyPressed(PIL_KEY_Q))
			m_Camera.SetRotation(m_Camera.GetRotation() + rotationSpeed);
		if (Pillar::Input::IsKeyPressed(PIL_KEY_E))
			m_Camera.SetRotation(m_Camera.GetRotation() - rotationSpeed);

		// Update animation time
		m_Time += dt;
		
		// Clear screen
		Pillar::Renderer::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
		Pillar::Renderer::Clear();

		// Begin Renderer2D scene
		Pillar::Renderer2D::BeginScene(m_Camera);

		// Test 1: Draw colored quads
		Pillar::Renderer2D::DrawQuad({ -0.75f, 0.5f }, { 0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f, 1.0f });
		Pillar::Renderer2D::DrawQuad({ 0.75f, 0.5f }, { 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f, 1.0f });
		Pillar::Renderer2D::DrawQuad({ -0.75f, -0.5f }, { 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f });
		
		// Test 2: Draw textured quad
		Pillar::Renderer2D::DrawQuad({ 0.75f, -0.5f }, { 0.5f, 0.5f }, m_Texture);
		
		// Test 3: Draw animated textured quad in center
		float scale = 0.8f + sin(m_Time * 2.0f) * 0.2f;
		Pillar::Renderer2D::DrawQuad({ 0.0f, 0.0f, 0.1f }, { scale, scale }, m_Texture);
		
		// Test 4: Draw textured quad with tint color
		glm::vec4 tint = { 1.0f, 0.5f + sin(m_Time) * 0.5f, 0.5f, 1.0f };
		Pillar::Renderer2D::DrawQuad({ 0.0f, -1.2f }, { 0.6f, 0.3f }, m_Texture, tint);

		Pillar::Renderer2D::EndScene();
    }
    
    void OnEvent(Pillar::Event& event) override
    {
        if (event.GetEventType() == Pillar::EventType::KeyPressed)
        {
			Pillar::KeyPressedEvent& keyEvent = static_cast<Pillar::KeyPressedEvent&>(event);
			if (keyEvent.GetRepeatCount() == 0)  // Only log on first press
			{
				PIL_INFO("Key Pressed: {0}", keyEvent.GetKeyCode());
			}
		}
	}
	
	void OnImGuiRender() override 
	{
		// Ensure we're using the correct ImGui context (DLL boundary issue)
		ImGui::SetCurrentContext(Pillar::ImGuiLayer::GetImGuiContext());
		
		ImGui::Begin("Renderer2D Test");
		ImGui::Text("Renderer2D and Texture Test");
		ImGui::Text("Camera Controls: WASD to move, Q/E to rotate");
		ImGui::Text("Time: %.2f", m_Time);
		if (m_Texture)
		{
			ImGui::Text("Texture Size: %dx%d", m_Texture->GetWidth(), m_Texture->GetHeight());
		}
		else
		{
			ImGui::Text("Texture: Failed to load!");
		}
		ImGui::End();
	}

private:
	std::shared_ptr<Pillar::Texture2D> m_Texture;
	Pillar::OrthographicCamera m_Camera;
	float m_Time = 0.0f;
};
