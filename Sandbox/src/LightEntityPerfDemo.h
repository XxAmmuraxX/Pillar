#pragma once

#include "Pillar.h"
#include "Pillar/Renderer/Renderer2D.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"
#include "Pillar/ECS/Components/Gameplay/XPGemComponent.h"
#include "Pillar/ECS/Systems/VelocityIntegrationSystem.h"
#include "Pillar/ECS/Systems/XPCollectionSystem.h"
#include <imgui.h>
#include <memory>
#include <random>

class LightEntityPerfDemo : public Pillar::Layer
{
public:
	LightEntityPerfDemo()
		: Layer("LightEntityPerfDemo"),
		  m_CameraController(16.0f / 9.0f, false) // No rotation for perf demo
	{
	}

	void OnAttach() override
	{
		Layer::OnAttach();
		PIL_INFO("Light Entity Performance Demo attached!");

		// Create scene
		m_Scene = std::make_unique<Pillar::Scene>();

		// Create systems
		m_VelocityIntegrationSystem = new Pillar::VelocityIntegrationSystem();
		m_XPCollectionSystem = new Pillar::XPCollectionSystem();

		m_VelocityIntegrationSystem->OnAttach(m_Scene.get());
		m_XPCollectionSystem->OnAttach(m_Scene.get());

		// Create player (for XP gem attraction)
		CreatePlayer();

		// Start with moderate amount
		SpawnLightEntities(1000);

		PIL_INFO("Light entity perf demo initialized with {} entities", m_EntityCount);
	}

	void OnDetach() override
	{
		delete m_VelocityIntegrationSystem;
		delete m_XPCollectionSystem;

		Layer::OnDetach();
	}

	void OnUpdate(float dt) override
	{
		m_FrameTime = dt * 1000.0f; // Convert to ms

		// Update camera
		m_CameraController.OnUpdate(dt);

		// Update systems
		auto sysStart = std::chrono::high_resolution_clock::now();
		m_VelocityIntegrationSystem->OnUpdate(dt);
		m_XPCollectionSystem->OnUpdate(dt);
		auto sysEnd = std::chrono::high_resolution_clock::now();
		m_SystemTime = std::chrono::duration<float, std::milli>(sysEnd - sysStart).count();

		// Render
		Pillar::Renderer::SetClearColor({ 0.05f, 0.05f, 0.08f, 1.0f });
		Pillar::Renderer::Clear();

		auto renderStart = std::chrono::high_resolution_clock::now();
		Pillar::Renderer2D::BeginScene(m_CameraController.GetCamera());
		DrawEntities();
		Pillar::Renderer2D::EndScene();
		auto renderEnd = std::chrono::high_resolution_clock::now();
		m_RenderTime = std::chrono::duration<float, std::milli>(renderEnd - renderStart).count();

		// Update entity count
		m_EntityCount = m_Scene->GetRegistry().storage<entt::entity>().in_use();
	}

	void OnEvent(Pillar::Event& event) override
	{
		m_CameraController.OnEvent(event);
	}

	void OnImGuiRender() override
	{
		ImGui::Begin("Light Entity Performance");
		
		ImGui::Text("Stress Test: Pure ECS Light Entities");
		ImGui::Separator();

		// Performance stats
		ImGui::Text("Entity Count: %zu", m_EntityCount);
		ImGui::Text("Frame Time: %.2f ms (%.0f FPS)", m_FrameTime, 1000.0f / m_FrameTime);
		ImGui::Text("System Time: %.2f ms", m_SystemTime);
		ImGui::Text("Render Time: %.2f ms", m_RenderTime);
		
		// Color-coded performance
		if (m_FrameTime < 16.67f)
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "Performance: EXCELLENT (60+ FPS)");
		else if (m_FrameTime < 33.33f)
			ImGui::TextColored(ImVec4(1, 1, 0, 1), "Performance: GOOD (30-60 FPS)");
		else
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "Performance: POOR (<30 FPS)");

		ImGui::Separator();

		// Spawn controls
		ImGui::Text("Spawn Light Entities:");
		if (ImGui::Button("+ 100")) SpawnLightEntities(100);
		ImGui::SameLine();
		if (ImGui::Button("+ 500")) SpawnLightEntities(500);
		ImGui::SameLine();
		if (ImGui::Button("+ 1000")) SpawnLightEntities(1000);
		
		if (ImGui::Button("+ 5000")) SpawnLightEntities(5000);
		ImGui::SameLine();
		if (ImGui::Button("+ 10000")) SpawnLightEntities(10000);

		ImGui::Separator();
		
		if (ImGui::Button("Clear All"))
		{
			ClearAll();
		}

		ImGui::Separator();
		ImGui::Text("Camera: WASD to move, Scroll to zoom");

		ImGui::End();
	}

private:
	void CreatePlayer()
	{
		m_Player = m_Scene->CreateEntity("Player");
		auto& transform = m_Player.GetComponent<Pillar::TransformComponent>();
		transform.Position = glm::vec2(0.0f, 0.0f);
	}

	void SpawnLightEntities(int count)
	{
		PIL_INFO("Spawning {} light entities...", count);

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<float> posX(-20.0f, 20.0f);
		std::uniform_real_distribution<float> posY(-12.0f, 12.0f);
		std::uniform_real_distribution<float> vel(-5.0f, 5.0f);

		for (int i = 0; i < count; ++i)
		{
			auto entity = m_Scene->CreateEntity("Particle");
			auto& transform = entity.GetComponent<Pillar::TransformComponent>();
			transform.Position = glm::vec2(posX(gen), posY(gen));

			auto& velocity = entity.AddComponent<Pillar::VelocityComponent>();
			velocity.Velocity = glm::vec2(vel(gen), vel(gen));
			velocity.MaxSpeed = 10.0f;

			// Add XP gem for spatial hash testing
			entity.AddComponent<Pillar::XPGemComponent>(1);
		}

		PIL_INFO("Spawned {} entities. Total: {}", count, m_Scene->GetRegistry().storage<entt::entity>().in_use());
	}

	void DrawEntities()
	{
		// Draw player
		if (m_Player)
		{
			auto& transform = m_Player.GetComponent<Pillar::TransformComponent>();
			Pillar::Renderer2D::DrawQuad(transform.Position, { 1.0f, 1.0f }, { 0.2f, 0.8f, 0.3f, 1.0f });
		}

		// Draw particles/gems
		auto view = m_Scene->GetRegistry().view<Pillar::TransformComponent, Pillar::VelocityComponent>();
		for (auto entity : view)
		{
			auto& transform = view.get<Pillar::TransformComponent>(entity);
			
			// Color based on speed
			float speed = glm::length(view.get<Pillar::VelocityComponent>(entity).Velocity);
			float t = speed / 10.0f;
			glm::vec4 color = glm::mix(
				glm::vec4(0.3f, 0.3f, 0.8f, 1.0f), // Blue for slow
				glm::vec4(1.0f, 0.3f, 0.3f, 1.0f), // Red for fast
				t
			);

			Pillar::Renderer2D::DrawQuad(transform.Position, { 0.15f, 0.15f }, color);
		}
	}

	void ClearAll()
	{
		PIL_INFO("Clearing all entities...");
		m_Scene->GetRegistry().clear();
		CreatePlayer();
		PIL_INFO("Cleared. Remaining: {}", m_Scene->GetRegistry().storage<entt::entity>().in_use());
	}

private:
	std::unique_ptr<Pillar::Scene> m_Scene;
	Pillar::OrthographicCameraController m_CameraController;

	// Systems
	Pillar::VelocityIntegrationSystem* m_VelocityIntegrationSystem = nullptr;
	Pillar::XPCollectionSystem* m_XPCollectionSystem = nullptr;

	// Player for attraction
	Pillar::Entity m_Player;

	// Performance metrics
	size_t m_EntityCount = 0;
	float m_FrameTime = 0.0f;
	float m_SystemTime = 0.0f;
	float m_RenderTime = 0.0f;
};
