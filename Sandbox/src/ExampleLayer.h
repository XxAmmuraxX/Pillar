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
		: Layer("ExampleLayer")
	{
	}

	void OnAttach() override 
	{ 
		Layer::OnAttach();
		PIL_INFO("ExampleLayer attached - Testing Renderer2D and Texture");
		
		// Enable keyboard controls for demo camera (WASD movement, Q/E rotation)
		m_CameraController.SetKeyboardControlEnabled(true);
		
		// Load texture
		m_Texture = Pillar::Texture2D::Create("pillar_logo.png");
		
		PIL_INFO("Renderer2D test initialized with texture!");
	}

    void OnUpdate(float dt) override
    {
		// Update camera controller
		m_CameraController.OnUpdate(dt);

		// Update animation time
		m_Time += dt;
		
		// Clear screen
		Pillar::Renderer2D::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
		Pillar::Renderer2D::Clear();

		// Begin Renderer2D scene with camera from controller
		Pillar::Renderer2D::BeginScene(m_CameraController.GetCamera());

		// Test 1: Draw colored quads
		Pillar::Renderer2D::DrawQuad({ -0.75f, 0.5f }, { 0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f, 1.0f });
		Pillar::Renderer2D::DrawQuad({ 0.75f, 0.5f }, { 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f, 1.0f });
		Pillar::Renderer2D::DrawQuad({ -0.75f, -0.5f }, { 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f });
		
		// Test 2: Draw textured quad
		Pillar::Renderer2D::DrawQuad({ 0.75f, -0.5f }, { 0.5f, 0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f }, m_Texture);
		
		// Test 3: Draw animated textured quad in center
		float scale = 0.8f + sin(m_Time * 2.0f) * 0.2f;
		Pillar::Renderer2D::DrawQuad({ 0.0f, 0.0f, 0.1f }, { scale, scale }, { 1.0f, 1.0f, 1.0f, 1.0f }, m_Texture);
		
		// Test 4: Draw textured quad with tint color
		glm::vec4 tint = { 1.0f, 0.5f + sin(m_Time) * 0.5f, 0.5f, 1.0f };
		Pillar::Renderer2D::DrawQuad({ 0.0f, -1.2f }, { 0.6f, 0.3f }, tint, m_Texture);

		Pillar::Renderer2D::EndScene();
    }
    
    void OnEvent(Pillar::Event& event) override
    {
		// Pass events to camera controller (for zoom and resize)
		m_CameraController.OnEvent(event);

        if (event.GetEventType() == Pillar::EventType::KeyPressed)
        {
			const auto& keyEvent = static_cast<const Pillar::KeyPressedEvent&>(event);
			if (keyEvent.GetRepeatCount() == 0)  // Only log on first press
			{
				PIL_INFO("Key Pressed: {0}", keyEvent.GetKeyCode());
			}
		}
	}
	
	void OnImGuiRender() override 
	{
		ImGui::Begin("Pillar Engine - Renderer2D Demo");
		
		// Credits section
		ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Developed by:");
		ImGui::BulletText("Ayse Sila Solak");
		ImGui::BulletText("Chika Libuku");
		ImGui::BulletText("Omar Akkawi");
		ImGui::Spacing();
		ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.4f, 1.0f), "Supervisor:");
		ImGui::BulletText("Dr hab. inz. Jerzy Balicki, prof. PW");
		ImGui::Separator();
		
		// Camera debug panel
		ImGui::Text("Camera Controls");
		ImGui::BulletText("WASD: Move camera");
		ImGui::BulletText("Q/E: Rotate camera");
		ImGui::BulletText("Mouse Wheel: Zoom");
		ImGui::Separator();
		
		// Camera stats
		const auto& camera = m_CameraController.GetCamera();
		glm::vec3 pos = camera.GetPosition();
		float rotation = camera.GetRotation();
		float zoom = m_CameraController.GetZoomLevel();
		
		ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
		ImGui::Text("Camera Rotation: %.2f deg", rotation);
		ImGui::Text("Zoom Level: %.2fx", zoom);
		ImGui::Separator();
		
		// Camera Settings (adjustable)
		ImGui::Text("Camera Settings:");
		
		float baseTranslationSpeed = m_CameraController.GetBaseTranslationSpeed();
		if (ImGui::SliderFloat("Move Speed", &baseTranslationSpeed, 1.0f, 20.0f))
		{
			m_CameraController.SetBaseTranslationSpeed(baseTranslationSpeed);
		}
		
		float rotationSpeed = m_CameraController.GetRotationSpeed();
		if (ImGui::SliderFloat("Rotation Speed", &rotationSpeed, 10.0f, 360.0f))
		{
			m_CameraController.SetRotationSpeed(rotationSpeed);
		}
		
		float zoomSpeed = m_CameraController.GetZoomSpeed();
		if (ImGui::SliderFloat("Zoom Speed", &zoomSpeed, 0.1f, 1.0f))
		{
			m_CameraController.SetZoomSpeed(zoomSpeed);
		}
		
		if (ImGui::Button("Reset Camera"))
		{
			m_CameraController.GetCamera().SetPosition({ 0.0f, 0.0f, 0.0f });
			m_CameraController.GetCamera().SetRotation(0.0f);
			m_CameraController.SetZoomLevel(1.0f);
			m_CameraController.SetBaseTranslationSpeed(5.0f);
		}
		
		ImGui::Separator();
		ImGui::Text("Scene Time: %.2f s", m_Time);
		
		if (m_Texture)
		{
			ImGui::Text("Texture Size: %dx%d", m_Texture->GetWidth(), m_Texture->GetHeight());
		}
		else
		{
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Texture: Failed to load!");
		}
		
		ImGui::Separator();
		
		// Renderer2D Statistics (NEW!)
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Renderer2D Statistics:");
		auto stats = Pillar::Renderer2D::GetStats();
		ImGui::Text("Draw Calls: %u", stats.DrawCalls);
		ImGui::Text("Quads: %u", stats.QuadCount);
		ImGui::Text("Vertices: %u", stats.VertexCount);
		ImGui::Text("Batches: %u", stats.BatchCount);
		ImGui::Text("Texture Switches: %u", stats.TextureSwitches);
		ImGui::Text("Buffer Uploads: %u", stats.BufferUploads);
		ImGui::Text("Flush Count: %u", stats.FlushCount);
		ImGui::Separator();
		ImGui::Text("Batch Efficiency: %.2f quads/batch", stats.GetBatchEfficiency());
		ImGui::Text("Avg Quads/Draw: %.2f", stats.GetAverageQuadsPerDraw());
		ImGui::Text("Avg Vertices/Draw: %.2f", stats.GetAverageVerticesPerDraw());
		ImGui::Separator();
		ImGui::Text("Total Quads Rendered: %u", stats.TotalQuadsRendered);
		ImGui::Text("Total Draw Calls: %u", stats.TotalDrawCalls);
		ImGui::Text("Peak Vertices: %u", stats.PeakVertices);
		ImGui::Text("Peak Quads: %u", stats.PeakQuads);
		
		ImGui::End();
	}

private:
	std::shared_ptr<Pillar::Texture2D> m_Texture;
	Pillar::OrthographicCameraController m_CameraController{ 16.0f / 9.0f, true };
	float m_Time = 0.0f;
};
