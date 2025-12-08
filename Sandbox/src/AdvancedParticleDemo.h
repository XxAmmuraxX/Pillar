#pragma once

#include "Pillar.h"
#include "Pillar/ECS/SpecializedPools.h"
#include "Pillar/ECS/Systems/ParticleEmitterSystem.h"
#include "Pillar/ECS/Systems/ParticleSystem.h"
#include "Pillar/ECS/Systems/VelocityIntegrationSystem.h"
#include "Pillar/ECS/Systems/SpriteRenderSystem.h"
#include "Pillar/ECS/Components/Gameplay/ParticleComponent.h"
#include "Pillar/ECS/Components/Gameplay/ParticleEmitterComponent.h"
#include "Pillar/ECS/Components/Gameplay/ParticleAnimationCurves.h"
#include "Pillar/Renderer/OrthographicCameraController.h"
#include <memory>

/**
 * @brief Phase 3 advanced particle demo with gradients and curves
 * 
 * Features:
 * - Color gradients (3+ color stops)
 * - Animation curves (EaseIn, EaseOut, EaseInOut, Bounce)
 * - Advanced effects showcasing non-linear interpolation
 * 
 * Controls:
 * - Left Click: Spawn FireworkEmitter (burst with curves)
 * - Right Click: Spawn MagicSparkleEmitter (gradient with ease-out)
 * - Space: Spawn FlameEmitter (3-color gradient)
 * - C: Clear all emitters
 * - WASD/QE: Camera controls
 */
class AdvancedParticleDemo : public Pillar::Layer
{
public:
	AdvancedParticleDemo()
		: Layer("AdvancedParticleDemo")
		, m_CameraController(16.0f / 9.0f, true)
	{
	}

	void OnAttach() override
	{
		PIL_INFO("AdvancedParticleDemo: Starting Phase 3 demo");

		// Create scene
		m_Scene = Pillar::SceneManager::CreateScene("AdvancedParticleDemo");
		Pillar::SceneManager::SetActiveScene(m_Scene);

		// Initialize particle pool (3000 for advanced effects)
		m_ParticlePool.Init(m_Scene, 3000);

		// Create and register systems
		auto particleEmitterSystem = std::make_shared<Pillar::ParticleEmitterSystem>();
		particleEmitterSystem->SetParticlePool(&m_ParticlePool);
		m_Scene->RegisterSystem(particleEmitterSystem);

		auto particleSystem = std::make_shared<Pillar::ParticleSystem>();
		particleSystem->SetParticlePool(&m_ParticlePool);
		m_Scene->RegisterSystem(particleSystem);

		auto velocitySystem = std::make_shared<Pillar::VelocityIntegrationSystem>();
		m_Scene->RegisterSystem(velocitySystem);

		auto spriteRenderSystem = std::make_shared<Pillar::SpriteRenderSystem>();
		m_Scene->RegisterSystem(spriteRenderSystem);

		// Pre-create gradients and curves for reuse
		CreateSharedEffects();

		PIL_INFO("AdvancedParticleDemo: Initialized with pool capacity {}", m_ParticlePool.GetTotalCount());
	}

	void OnDetach() override
	{
		// Clean up shared effects
		delete m_FireworkGradient;
		delete m_FlameGradient;
		delete m_SparkleGradient;
		delete m_EaseOutCurve;
		delete m_EaseInOutCurve;
		delete m_BounceCurve;

		PIL_INFO("AdvancedParticleDemo: Detached");
	}

	void OnUpdate(float dt) override
	{
		// Update camera
		m_CameraController.OnUpdate(dt);

		// Update scene systems
		if (m_Scene)
		{
			m_Scene->OnUpdate(dt);
		}
	}

	void OnRender() override
	{
		// Clear screen
		Pillar::RenderCommand::SetClearColor({ 0.05f, 0.05f, 0.1f, 1.0f });
		Pillar::RenderCommand::Clear();

		// Render scene
		Pillar::Renderer2D::BeginScene(m_CameraController.GetCamera());

		if (m_Scene)
		{
			auto spriteSystem = m_Scene->GetSystem<Pillar::SpriteRenderSystem>();
			if (spriteSystem)
			{
				spriteSystem->Render();
			}
		}

		Pillar::Renderer2D::EndScene();
	}

	void OnEvent(Pillar::Event& event) override
	{
		m_CameraController.OnEvent(event);

		// Mouse button events
		Pillar::EventDispatcher dispatcher(event);
		dispatcher.Dispatch<Pillar::MouseButtonPressedEvent>(BIND_EVENT_FN(OnMouseButtonPressed));
		dispatcher.Dispatch<Pillar::KeyPressedEvent>(BIND_EVENT_FN(OnKeyPressed));
	}

	void OnImGuiRender() override
	{
		ImGui::Begin("Advanced Particle Demo (Phase 3)");

		// Pool statistics
		ImGui::Text("Particle Pool:");
		ImGui::Text("  Active: %zu", m_ParticlePool.GetActiveCount());
		ImGui::Text("  Available: %zu", m_ParticlePool.GetAvailableCount());
		ImGui::Text("  Total: %zu", m_ParticlePool.GetTotalCount());

		ImGui::Separator();

		// System statistics
		if (m_Scene)
		{
			auto emitterSystem = m_Scene->GetSystem<Pillar::ParticleEmitterSystem>();
			auto particleSystem = m_Scene->GetSystem<Pillar::ParticleSystem>();

			if (emitterSystem)
			{
				ImGui::Text("Emitters: %u", emitterSystem->GetEmitterCount());
				ImGui::Text("Spawned This Frame: %u", emitterSystem->GetParticlesSpawnedThisFrame());
			}

			if (particleSystem)
			{
				ImGui::Text("Active Particles: %u", particleSystem->GetActiveParticleCount());
			}
		}

		ImGui::Separator();

		// Controls help
		ImGui::Text("Controls:");
		ImGui::BulletText("Left Click: Firework (bounce curve)");
		ImGui::BulletText("Right Click: Magic Sparkle (ease-out)");
		ImGui::BulletText("Space: Flame (3-color gradient)");
		ImGui::BulletText("C: Clear all emitters");

		ImGui::Separator();

		// Renderer statistics
		ImGui::Text("Renderer:");
		ImGui::Text("  Draw Calls: %u", Pillar::Renderer2DBackend::GetStats().DrawCalls);
		ImGui::Text("  Quads: %u", Pillar::Renderer2DBackend::GetStats().QuadCount);

		ImGui::End();
	}

private:
	void CreateSharedEffects()
	{
		// === Color Gradients ===
		
		// Firework: White -> Orange -> Red (explosion effect)
		m_FireworkGradient = new Pillar::ColorGradient(
			glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),  // 0.0 - Bright white
			glm::vec4(1.0f, 0.5f, 0.1f, 0.8f),  // 0.5 - Orange
			glm::vec4(0.8f, 0.1f, 0.0f, 0.0f)   // 1.0 - Dark red, transparent
		);

		// Flame: Yellow -> Orange -> Red -> Black (realistic fire)
		m_FlameGradient = new Pillar::ColorGradient();
		m_FlameGradient->Stops.push_back({ 0.0f, glm::vec4(1.0f, 1.0f, 0.3f, 1.0f) });  // Yellow
		m_FlameGradient->Stops.push_back({ 0.3f, glm::vec4(1.0f, 0.5f, 0.0f, 1.0f) });  // Orange
		m_FlameGradient->Stops.push_back({ 0.7f, glm::vec4(0.8f, 0.1f, 0.0f, 0.6f) });  // Red
		m_FlameGradient->Stops.push_back({ 1.0f, glm::vec4(0.2f, 0.0f, 0.0f, 0.0f) });  // Black fade

		// Magic Sparkle: Purple -> Pink -> White (magical effect)
		m_SparkleGradient = new Pillar::ColorGradient(
			glm::vec4(0.5f, 0.0f, 1.0f, 1.0f),  // 0.0 - Deep purple
			glm::vec4(1.0f, 0.3f, 0.8f, 0.8f),  // 0.5 - Pink
			glm::vec4(1.0f, 1.0f, 1.0f, 0.0f)   // 1.0 - White fade
		);

		// === Animation Curves ===
		m_EaseOutCurve = new Pillar::AnimationCurve(Pillar::CurveType::EaseOut, 1.0f);
		m_EaseInOutCurve = new Pillar::AnimationCurve(Pillar::CurveType::EaseInOut, 1.0f);
		m_BounceCurve = new Pillar::AnimationCurve(Pillar::CurveType::Bounce, 1.0f);
	}

	void SpawnFireworkEmitter(const glm::vec2& position)
	{
		// Burst emitter with bounce curve for dramatic explosion
		auto emitter = m_Scene->CreateEntity();
		auto& transform = emitter.GetComponent<Pillar::TransformComponent>();
		transform.Position = glm::vec3(position.x, position.y, 0.0f);

		auto& emitterComp = emitter.AddComponent<Pillar::ParticleEmitterComponent>();
		emitterComp.Enabled = true;
		emitterComp.BurstMode = true;
		emitterComp.BurstCount = 150;

		// Explosion pattern
		emitterComp.Shape = Pillar::EmissionShape::Circle;
		emitterComp.ShapeSize = glm::vec2(0.2f);
		emitterComp.Direction = glm::vec2(0.0f, 1.0f);
		emitterComp.DirectionSpread = 180.0f;  // Full 360°
		emitterComp.Speed = 8.0f;
		emitterComp.SpeedVariance = 4.0f;

		// Particle properties
		emitterComp.Lifetime = 1.5f;
		emitterComp.LifetimeVariance = 0.5f;
		emitterComp.Size = 0.15f;
		emitterComp.SizeVariance = 0.05f;
		emitterComp.StartColor = glm::vec4(1.0f);  // Will be overridden by gradient

		// Visual effects - use gradient and bounce curve
		emitterComp.FadeOut = true;
		emitterComp.ScaleOverTime = true;
		emitterComp.RotateOverTime = false;
		emitterComp.EndScale = 0.3f;
		emitterComp.Gravity = glm::vec2(0.0f, -4.0f);

		// Phase 3: Advanced features
		emitterComp.UseColorGradient = true;
		emitterComp.ColorGradientPtr = m_FireworkGradient;
		emitterComp.SizeCurvePtr = m_BounceCurve;  // Bouncy size animation

		PIL_INFO("Spawned Firework emitter at ({}, {})", position.x, position.y);
	}

	void SpawnMagicSparkleEmitter(const glm::vec2& position)
	{
		// Continuous sparkle effect with ease-out curve
		auto emitter = m_Scene->CreateEntity();
		auto& transform = emitter.GetComponent<Pillar::TransformComponent>();
		transform.Position = glm::vec3(position.x, position.y, 0.0f);

		auto& emitterComp = emitter.AddComponent<Pillar::ParticleEmitterComponent>();
		emitterComp.Enabled = true;
		emitterComp.EmissionRate = 40.0f;  // Continuous emission

		// Sparkle pattern
		emitterComp.Shape = Pillar::EmissionShape::Circle;
		emitterComp.ShapeSize = glm::vec2(0.5f);
		emitterComp.Direction = glm::vec2(0.0f, 1.0f);
		emitterComp.DirectionSpread = 180.0f;
		emitterComp.Speed = 3.0f;
		emitterComp.SpeedVariance = 2.0f;

		// Particle properties
		emitterComp.Lifetime = 1.2f;
		emitterComp.LifetimeVariance = 0.4f;
		emitterComp.Size = 0.12f;
		emitterComp.SizeVariance = 0.04f;

		// Visual effects - gradient + ease-out for smooth sparkle
		emitterComp.FadeOut = true;
		emitterComp.ScaleOverTime = false;
		emitterComp.RotateOverTime = true;
		emitterComp.RotationSpeed = 360.0f;
		emitterComp.Gravity = glm::vec2(0.0f, -1.0f);

		// Phase 3: Advanced features
		emitterComp.UseColorGradient = true;
		emitterComp.ColorGradientPtr = m_SparkleGradient;
		emitterComp.RotationCurvePtr = m_EaseOutCurve;  // Smooth rotation slowdown

		PIL_INFO("Spawned Magic Sparkle emitter at ({}, {})", position.x, position.y);
	}

	void SpawnFlameEmitter(const glm::vec2& position)
	{
		// Flame effect with 4-color gradient
		auto emitter = m_Scene->CreateEntity();
		auto& transform = emitter.GetComponent<Pillar::TransformComponent>();
		transform.Position = glm::vec3(position.x, position.y, 0.0f);

		auto& emitterComp = emitter.AddComponent<Pillar::ParticleEmitterComponent>();
		emitterComp.Enabled = true;
		emitterComp.EmissionRate = 50.0f;

		// Flame pattern - upward with slight spread
		emitterComp.Shape = Pillar::EmissionShape::Circle;
		emitterComp.ShapeSize = glm::vec2(0.3f);
		emitterComp.Direction = glm::vec2(0.0f, 1.0f);
		emitterComp.DirectionSpread = 15.0f;
		emitterComp.Speed = 4.0f;
		emitterComp.SpeedVariance = 1.0f;

		// Particle properties
		emitterComp.Lifetime = 1.0f;
		emitterComp.LifetimeVariance = 0.3f;
		emitterComp.Size = 0.25f;
		emitterComp.SizeVariance = 0.08f;

		// Visual effects - 4-color flame gradient + ease-in-out
		emitterComp.FadeOut = true;
		emitterComp.ScaleOverTime = true;
		emitterComp.RotateOverTime = false;
		emitterComp.EndScale = 1.5f;  // Expand as it rises
		emitterComp.Gravity = glm::vec2(0.0f, 1.0f);  // Rise upward

		// Phase 3: Advanced features
		emitterComp.UseColorGradient = true;
		emitterComp.ColorGradientPtr = m_FlameGradient;
		emitterComp.SizeCurvePtr = m_EaseInOutCurve;  // Smooth size transition

		PIL_INFO("Spawned Flame emitter at ({}, {})", position.x, position.y);
	}

	void ClearAllEmitters()
	{
		if (!m_Scene)
			return;

		// Destroy all emitter entities
		auto& registry = m_Scene->GetRegistry();
		auto view = registry.view<Pillar::ParticleEmitterComponent>();

		std::vector<Pillar::Entity> toDestroy;
		for (auto entityHandle : view)
		{
			toDestroy.push_back(Pillar::Entity(entityHandle, m_Scene));
		}

		for (auto entity : toDestroy)
		{
			m_Scene->DestroyEntity(entity);
		}

		PIL_INFO("Cleared {} emitters", toDestroy.size());
	}

	bool OnMouseButtonPressed(Pillar::MouseButtonPressedEvent& e)
	{
		// Get mouse world position
		auto [mouseX, mouseY] = Pillar::Input::GetMousePosition();
		glm::vec2 worldPos = m_CameraController.GetCamera().ScreenToWorld(mouseX, mouseY);

		if (e.GetMouseButton() == PIL_MOUSE_BUTTON_LEFT)
		{
			// Left click - Firework
			SpawnFireworkEmitter(worldPos);
			return true;
		}
		else if (e.GetMouseButton() == PIL_MOUSE_BUTTON_RIGHT)
		{
			// Right click - Magic Sparkle
			SpawnMagicSparkleEmitter(worldPos);
			return true;
		}

		return false;
	}

	bool OnKeyPressed(Pillar::KeyPressedEvent& e)
	{
		if (e.GetKeyCode() == PIL_KEY_SPACE)
		{
			// Space - Flame at center
			SpawnFlameEmitter(m_CameraController.GetCamera().GetPosition());
			return true;
		}
		else if (e.GetKeyCode() == PIL_KEY_C)
		{
			// C - Clear all
			ClearAllEmitters();
			return true;
		}

		return false;
	}

private:
	Pillar::Scene* m_Scene = nullptr;
	Pillar::ParticlePool m_ParticlePool;
	Pillar::OrthographicCameraController m_CameraController;

	// Shared gradients and curves (reused across emitters)
	Pillar::ColorGradient* m_FireworkGradient = nullptr;
	Pillar::ColorGradient* m_FlameGradient = nullptr;
	Pillar::ColorGradient* m_SparkleGradient = nullptr;
	Pillar::AnimationCurve* m_EaseOutCurve = nullptr;
	Pillar::AnimationCurve* m_EaseInOutCurve = nullptr;
	Pillar::AnimationCurve* m_BounceCurve = nullptr;
};
