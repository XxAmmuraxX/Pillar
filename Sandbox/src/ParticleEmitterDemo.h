#pragma once

#include "Pillar.h"
#include "Pillar/ECS/SpecializedPools.h"
#include "Pillar/ECS/Systems/ParticleSystem.h"
#include "Pillar/ECS/Systems/ParticleEmitterSystem.h"
#include "Pillar/ECS/Systems/VelocityIntegrationSystem.h"
#include "Pillar/Renderer/Renderer2DBackend.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Gameplay/ParticleComponent.h"
#include "Pillar/ECS/Components/Gameplay/ParticleEmitterComponent.h"
#include "Pillar/ECS/Components/Rendering/SpriteComponent.h"
#include "Pillar/ECS/Systems/SpriteRenderSystem.h"
#include <imgui.h>
#include <memory>

/**
 * @brief Demo layer showcasing Phase 2 particle emitter system
 * 
 * Features demonstrated:
 * 1. Continuous particle emission (particles/sec)
 * 2. Burst emission (one-shot spawning)
 * 3. Multiple emission shapes (point, circle, box, cone)
 * 4. Randomization (position, velocity, lifetime, size, color)
 * 5. Emitter presets (explosion, fire, smoke, magic)
 */
class ParticleEmitterDemo : public Pillar::Layer
{
public:
	ParticleEmitterDemo()
		: Layer("ParticleEmitterDemo"),
		  m_CameraController(16.0f / 9.0f, true)
	{
	}

	void OnAttach() override
	{
		Layer::OnAttach();
		PIL_INFO("Particle Emitter Demo attached!");

		// Create scene
		m_Scene = std::make_unique<Pillar::Scene>();

		// Initialize particle pool (2000 particles for emitters)
		m_ParticlePool.Init(m_Scene.get(), 2000);

		// Create systems
		m_ParticleSystem = new Pillar::ParticleSystem();
		m_ParticleEmitterSystem = new Pillar::ParticleEmitterSystem();
		m_VelocitySystem = new Pillar::VelocityIntegrationSystem();
		m_SpriteRenderSystem = new Pillar::SpriteRenderSystem();

		m_ParticleSystem->OnAttach(m_Scene.get());
		m_ParticleEmitterSystem->OnAttach(m_Scene.get());
		m_VelocitySystem->OnAttach(m_Scene.get());
		m_SpriteRenderSystem->OnAttach(m_Scene.get());

		// Wire up particle pools
		m_ParticleSystem->SetParticlePool(&m_ParticlePool);
		m_ParticleEmitterSystem->SetParticlePool(&m_ParticlePool);

		PIL_INFO("Particle emitter system initialized!");
	}

	void OnDetach() override
	{
		delete m_ParticleSystem;
		delete m_ParticleEmitterSystem;
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
		m_ParticleEmitterSystem->OnUpdate(dt);
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
				// Spawn emitter at mouse position
				auto mousePair = Pillar::Input::GetMousePosition();
				glm::vec2 screenPos(mousePair.first, mousePair.second);
				glm::vec2 worldPos = ScreenToWorld(screenPos);
				
				SpawnEmitter(worldPos, m_CurrentPreset);
			}
		}
	}

	void OnImGuiRender() override
	{
		ImGui::Begin("Particle Emitter Demo (Phase 2)");

		ImGui::Text("Phase 2: Particle Emitter System");
		ImGui::Separator();

		// Pool statistics
		ImGui::Text("Particle Pool:");
		ImGui::Text("  Active: %zu", m_ParticlePool.GetActiveCount());
		ImGui::Text("  Available: %zu", m_ParticlePool.GetAvailableCount());
		ImGui::Text("  Total: %zu", m_ParticlePool.GetTotalCount());

		ImGui::Separator();

		// System statistics
		ImGui::Text("Systems:");
		ImGui::Text("  Active Emitters: %u", m_ParticleEmitterSystem->GetEmitterCount());
		ImGui::Text("  Spawned This Frame: %u", m_ParticleEmitterSystem->GetParticlesSpawnedThisFrame());
		ImGui::Text("  Active Particles: %u", m_ParticleSystem->GetActiveParticleCount());

		ImGui::Separator();

		// Renderer statistics
		ImGui::Text("Renderer:");
		ImGui::Text("  Draw Calls: %u", Pillar::Renderer2DBackend::GetDrawCallCount());
		ImGui::Text("  Quads: %u", Pillar::Renderer2DBackend::GetQuadCount());

		ImGui::Separator();

		// Emitter presets
		ImGui::Text("Emitter Presets:");
		if (ImGui::RadioButton("Explosion", m_CurrentPreset == EmitterPreset::Explosion))
			m_CurrentPreset = EmitterPreset::Explosion;
		ImGui::SameLine();
		if (ImGui::RadioButton("Fire", m_CurrentPreset == EmitterPreset::Fire))
			m_CurrentPreset = EmitterPreset::Fire;
		ImGui::SameLine();
		if (ImGui::RadioButton("Smoke", m_CurrentPreset == EmitterPreset::Smoke))
			m_CurrentPreset = EmitterPreset::Smoke;
		ImGui::SameLine();
		if (ImGui::RadioButton("Magic", m_CurrentPreset == EmitterPreset::Magic))
			m_CurrentPreset = EmitterPreset::Magic;

		ImGui::Separator();

		// Controls
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Controls:");
		ImGui::BulletText("Left Click: Spawn emitter (selected preset)");
		ImGui::BulletText("C: Clear all particles and emitters");
		ImGui::BulletText("WASD: Move camera");
		ImGui::BulletText("Mouse Wheel: Zoom");

		ImGui::End();
	}

private:
	enum class EmitterPreset
	{
		Explosion,
		Fire,
		Smoke,
		Magic
	};

	void HandleInput(float dt)
	{
		// Clear with C
		if (Pillar::Input::IsKeyPressed(PIL_KEY_C))
		{
			// Clear pool
			m_ParticlePool.Clear();
			
			// Destroy all emitter entities
			m_Scene->GetRegistry().clear();
			
			PIL_INFO("Cleared all particles and emitters");
		}
	}

	void SpawnEmitter(const glm::vec2& position, EmitterPreset preset)
	{
		Pillar::Entity emitter = m_Scene->CreateEntity("Emitter");
		auto& transform = emitter.GetComponent<Pillar::TransformComponent>();
		transform.Position = position;

		auto& emitterComp = emitter.AddComponent<Pillar::ParticleEmitterComponent>();

		switch (preset)
		{
		case EmitterPreset::Explosion:
			// Burst of 200 particles in all directions
			emitterComp.BurstMode = true;
			emitterComp.BurstCount = 200;
			emitterComp.Shape = Pillar::EmissionShape::Point;
			emitterComp.Direction = glm::vec2(0.0f, 1.0f);
			emitterComp.DirectionSpread = 180.0f; // Full 360°
			emitterComp.Speed = 8.0f;
			emitterComp.SpeedVariance = 4.0f;
			emitterComp.Lifetime = 1.5f;
			emitterComp.LifetimeVariance = 0.5f;
			emitterComp.Size = 0.15f;
			emitterComp.SizeVariance = 0.05f;
			emitterComp.StartColor = glm::vec4(1.0f, 0.7f, 0.2f, 1.0f); // Orange
			emitterComp.ColorVariance = glm::vec4(0.2f, 0.2f, 0.1f, 0.0f);
			emitterComp.FadeOut = true;
			emitterComp.Gravity = glm::vec2(0.0f, -5.0f);
			PIL_INFO("Spawned Explosion emitter at ({}, {})", position.x, position.y);
			break;

		case EmitterPreset::Fire:
			// Continuous upward emission
			emitterComp.EmissionRate = 50.0f;
			emitterComp.Shape = Pillar::EmissionShape::Circle;
			emitterComp.ShapeSize = glm::vec2(0.3f); // Small circle
			emitterComp.Direction = glm::vec2(0.0f, 1.0f);
			emitterComp.DirectionSpread = 15.0f;
			emitterComp.Speed = 4.0f;
			emitterComp.SpeedVariance = 1.0f;
			emitterComp.Lifetime = 1.0f;
			emitterComp.LifetimeVariance = 0.3f;
			emitterComp.Size = 0.2f;
			emitterComp.SizeVariance = 0.05f;
			emitterComp.StartColor = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f); // Orange
			emitterComp.ColorVariance = glm::vec4(0.3f, 0.2f, 0.0f, 0.0f);
			emitterComp.FadeOut = true;
			emitterComp.Gravity = glm::vec2(0.0f, 1.0f); // Slight upward drift
			PIL_INFO("Spawned Fire emitter at ({}, {})", position.x, position.y);
			break;

		case EmitterPreset::Smoke:
			// Continuous upward emission with gray particles
			emitterComp.EmissionRate = 20.0f;
			emitterComp.Shape = Pillar::EmissionShape::Box;
			emitterComp.ShapeSize = glm::vec2(0.5f, 0.1f);
			emitterComp.Direction = glm::vec2(0.0f, 1.0f);
			emitterComp.DirectionSpread = 20.0f;
			emitterComp.Speed = 2.0f;
			emitterComp.SpeedVariance = 0.5f;
			emitterComp.Lifetime = 2.5f;
			emitterComp.LifetimeVariance = 0.5f;
			emitterComp.Size = 0.3f;
			emitterComp.SizeVariance = 0.1f;
			emitterComp.StartColor = glm::vec4(0.5f, 0.5f, 0.5f, 0.8f); // Gray
			emitterComp.ColorVariance = glm::vec4(0.1f, 0.1f, 0.1f, 0.0f);
			emitterComp.FadeOut = true;
			emitterComp.ScaleOverTime = true;
			emitterComp.EndScale = 2.0f; // Expand
			emitterComp.Gravity = glm::vec2(0.0f, 0.5f); // Float up slowly
			PIL_INFO("Spawned Smoke emitter at ({}, {})", position.x, position.y);
			break;

		case EmitterPreset::Magic:
			// Sparkle effect with random colors
			emitterComp.EmissionRate = 30.0f;
			emitterComp.Shape = Pillar::EmissionShape::Circle;
			emitterComp.ShapeSize = glm::vec2(0.5f);
			emitterComp.Direction = glm::vec2(0.0f, 1.0f);
			emitterComp.DirectionSpread = 180.0f; // All directions
			emitterComp.Speed = 3.0f;
			emitterComp.SpeedVariance = 2.0f;
			emitterComp.Lifetime = 1.2f;
			emitterComp.LifetimeVariance = 0.4f;
			emitterComp.Size = 0.1f;
			emitterComp.SizeVariance = 0.03f;
			emitterComp.StartColor = glm::vec4(0.5f, 0.3f, 1.0f, 1.0f); // Purple
			emitterComp.ColorVariance = glm::vec4(0.5f, 0.5f, 0.0f, 0.0f); // High variance
			emitterComp.FadeOut = true;
			emitterComp.RotateOverTime = true;
			emitterComp.RotationSpeed = 360.0f;
			emitterComp.Gravity = glm::vec2(0.0f, -1.0f);
			PIL_INFO("Spawned Magic emitter at ({}, {})", position.x, position.y);
			break;
		}
	}

	glm::vec2 ScreenToWorld(const glm::vec2& screenPos)
	{
		// Get window size (hardcoded for now)
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
	Pillar::ParticleEmitterSystem* m_ParticleEmitterSystem = nullptr;
	Pillar::VelocityIntegrationSystem* m_VelocitySystem = nullptr;
	Pillar::SpriteRenderSystem* m_SpriteRenderSystem = nullptr;

	// Preset selection
	EmitterPreset m_CurrentPreset = EmitterPreset::Fire;
};
