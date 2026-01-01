#pragma once

#include "Pillar.h"
#include "Pillar/ECS/SpecializedPools.h"
#include "Pillar/ECS/Systems/ParticleSystem.h"
#include "Pillar/ECS/Systems/VelocityIntegrationSystem.h"
#include "Pillar/Renderer/Renderer2DBackend.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Gameplay/ParticleComponent.h"
#include "Pillar/ECS/Components/Rendering/SpriteComponent.h"
#include "Pillar/ECS/Systems/SpriteRenderSystem.h"
#include <imgui.h>
#include <memory>

/**
 * @brief Demo layer showcasing the particle system (Phase 1)
 * 
 * Features demonstrated:
 * 1. Particle pool usage
 * 2. Particle lifetime and fading
 * 3. Velocity and gravity
 * 4. Batch rendering of particles
 * 5. Interactive spawning
 */
class ParticleSystemDemo : public Pillar::Layer
{
public:
	ParticleSystemDemo()
		: Layer("ParticleSystemDemo"),
		  m_CameraController(16.0f / 9.0f, true)
	{
	}

	void OnAttach() override
	{
		Layer::OnAttach();
		PIL_INFO("Particle System Demo attached!");

		// Create scene
		m_Scene = std::make_unique<Pillar::Scene>();

		// Initialize particle pool (1000 particles)
		m_ParticlePool.Init(m_Scene.get(), 1000);

		// Create systems
		m_ParticleSystem = new Pillar::ParticleSystem();
		m_VelocitySystem = new Pillar::VelocityIntegrationSystem();
		m_SpriteRenderSystem = new Pillar::SpriteRenderSystem();

		m_ParticleSystem->OnAttach(m_Scene.get());
		m_VelocitySystem->OnAttach(m_Scene.get());
		m_SpriteRenderSystem->OnAttach(m_Scene.get());

		// CRITICAL: Set the particle pool so system can return dead particles
		m_ParticleSystem->SetParticlePool(&m_ParticlePool);

		PIL_INFO("Particle system initialized!");
	}

	void OnDetach() override
	{
		delete m_ParticleSystem;
		delete m_VelocitySystem;
		delete m_SpriteRenderSystem;

		Layer::OnDetach();
	}

	void OnUpdate(float dt) override
	{
		// Update camera
		m_CameraController.OnUpdate(dt);

		// Handle input
		HandleInput(dt);

		// Update systems
		m_ParticleSystem->OnUpdate(dt);
		m_VelocitySystem->OnUpdate(dt);

		// Render
		Pillar::Renderer::SetClearColor({ 0.05f, 0.05f, 0.1f, 1.0f });
		Pillar::Renderer::Clear();

		Pillar::Renderer2DBackend::ResetStats();
		Pillar::Renderer2DBackend::BeginScene(m_CameraController.GetCamera());

		// Render all sprites (particles)
		m_SpriteRenderSystem->OnUpdate(dt);

		Pillar::Renderer2DBackend::EndScene();
	}

	void OnEvent(Pillar::Event& event) override
	{
		m_CameraController.OnEvent(event);

		if (event.GetEventType() == Pillar::EventType::MouseButtonPressed)
		{
			auto& mouseEvent = static_cast<Pillar::MouseButtonPressedEvent&>(event);
			if (mouseEvent.GetMouseButton() == PIL_MOUSE_BUTTON_LEFT)
			{
				SpawnParticles(10, false);
			}
			else if (mouseEvent.GetMouseButton() == PIL_MOUSE_BUTTON_RIGHT)
			{
				SpawnParticles(50, true); // Burst!
			}
		}
	}

	void OnImGuiRender() override
	{
		ImGui::Begin("Particle System Demo");

		ImGui::Text("Phase 1: Core Particle System");
		ImGui::Separator();

		// Pool statistics
		ImGui::Text("Particle Pool:");
		ImGui::Text("  Active: %zu", m_ParticlePool.GetActiveCount());
		ImGui::Text("  Available: %zu", m_ParticlePool.GetAvailableCount());
		ImGui::Text("  Total: %zu", m_ParticlePool.GetTotalCount());

		ImGui::Separator();

		// System statistics
		ImGui::Text("Particle System:");
		ImGui::Text("  Active Particles: %u", m_ParticleSystem->GetActiveParticleCount());
		ImGui::Text("  Dead Particles: %u", m_ParticleSystem->GetDeadParticleCount());

		ImGui::Separator();

		// Renderer statistics
		ImGui::Text("Renderer:");
		ImGui::Text("  Draw Calls: %u", Pillar::Renderer2DBackend::GetDrawCallCount());
		ImGui::Text("  Quads: %u", Pillar::Renderer2DBackend::GetQuadCount());

		ImGui::Separator();

		// Controls
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Controls:");
		ImGui::BulletText("Left Click: Spawn 10 particles");
		ImGui::BulletText("Right Click: Spawn 50 particles (burst)");
		ImGui::BulletText("Space: Spawn 5 particles/sec");
		ImGui::BulletText("C: Clear all particles");
		ImGui::BulletText("WASD: Move camera");
		ImGui::BulletText("Mouse Wheel: Zoom");

		ImGui::Separator();

		// Particle settings
		ImGui::Text("Particle Settings:");
		ImGui::SliderFloat("Lifetime", &m_ParticleLifetime, 0.1f, 5.0f);
		ImGui::SliderFloat("Size", &m_ParticleSize, 0.05f, 0.5f);
		ImGui::SliderFloat("Speed", &m_ParticleSpeed, 1.0f, 20.0f);
		ImGui::ColorEdit4("Start Color", &m_StartColor.r);

		ImGui::End();
	}

private:
	void HandleInput(float dt)
	{
		// Continuous spawning with Space
		if (Pillar::Input::IsKeyPressed(PIL_KEY_SPACE))
		{
			m_SpawnAccumulator += dt;
			if (m_SpawnAccumulator >= 0.2f) // 5 per second
			{
				SpawnParticles(1, false);
				m_SpawnAccumulator = 0.0f;
			}
		}

		// Clear particles with C
		if (Pillar::Input::IsKeyPressed(PIL_KEY_C))
		{
			m_ParticlePool.Clear();
			PIL_INFO("Cleared all particles");
		}
	}

	void SpawnParticles(int count, bool burst)
	{
		// Get mouse position in world space
		auto mousePair = Pillar::Input::GetMousePosition();
		glm::vec2 screenPos(mousePair.first, mousePair.second);
		glm::vec2 worldPos = ScreenToWorld(screenPos);

		for (int i = 0; i < count; ++i)
		{
			// Random direction
			float angle = (rand() % 360) * (3.14159f / 180.0f);
			if (!burst)
			{
				// Spread for normal spawn
				angle += (rand() % 60 - 30) * (3.14159f / 180.0f);
			}

			glm::vec2 direction(cos(angle), sin(angle));

			// Random speed variation
			float speed = m_ParticleSpeed + (rand() % 100 - 50) / 100.0f * 2.0f;
			glm::vec2 velocity = direction * speed;

			// Random color variation
			glm::vec4 color = m_StartColor;
			color.r += (rand() % 20 - 10) / 100.0f;
			color.g += (rand() % 20 - 10) / 100.0f;
			color.b += (rand() % 20 - 10) / 100.0f;
			color = glm::clamp(color, 0.0f, 1.0f);

			// Spawn particle
			m_ParticlePool.SpawnParticle(
				worldPos,
				velocity,
				color,
				m_ParticleSize,
				m_ParticleLifetime
			);
		}
	}

	glm::vec2 ScreenToWorld(const glm::vec2& screenPos)
	{
		// Get window size (hardcoded for now, should get from Application)
		float windowWidth = 1600.0f;
		float windowHeight = 900.0f;

		// Convert screen to NDC (-1 to 1)
		glm::vec2 ndc;
		ndc.x = (2.0f * screenPos.x) / windowWidth - 1.0f;
		ndc.y = 1.0f - (2.0f * screenPos.y) / windowHeight;

		// Convert NDC to world space using camera
		auto& camera = m_CameraController.GetCamera();
		glm::mat4 invViewProj = glm::inverse(camera.GetViewProjectionMatrix());
		glm::vec4 worldPos = invViewProj * glm::vec4(ndc, 0.0f, 1.0f);

		return glm::vec2(worldPos.x, worldPos.y);
	}

private:
	std::unique_ptr<Pillar::Scene> m_Scene;
	Pillar::OrthographicCameraController m_CameraController;

	// Particle pool
	Pillar::ParticlePool m_ParticlePool;

	// Systems
	Pillar::ParticleSystem* m_ParticleSystem = nullptr;
	Pillar::VelocityIntegrationSystem* m_VelocitySystem = nullptr;
	Pillar::SpriteRenderSystem* m_SpriteRenderSystem = nullptr;

	// Settings
	float m_ParticleLifetime = 2.0f;
	float m_ParticleSize = 0.1f;
	float m_ParticleSpeed = 8.0f;
	glm::vec4 m_StartColor = { 1.0f, 0.5f, 0.2f, 1.0f }; // Orange

	// State
	float m_SpawnAccumulator = 0.0f;
};
