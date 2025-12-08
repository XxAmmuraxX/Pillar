#pragma once

#include "Pillar.h"
#include "Pillar/ECS/SpecializedPools.h"
#include "Pillar/Renderer/Renderer2D.h"
#include "Pillar/Renderer/Renderer2DBackend.h"
#include <memory>

/**
 * @brief Example layer demonstrating object pooling with bullets and particles
 * 
 * This example shows:
 * 1. How to initialize object pools
 * 2. How to spawn entities from pools
 * 3. How to return entities to pools
 * 4. Performance benefits of object pooling
 */
class ObjectPoolDemo : public Pillar::Layer
{
public:
	ObjectPoolDemo() 
		: Layer("ObjectPoolDemo"),
		  m_CameraController(16.0f / 9.0f, true)
	{
	}

	void OnAttach() override 
	{ 
		Layer::OnAttach();
		PIL_INFO("ObjectPoolDemo attached");

		// Create scene
		m_Scene = new Pillar::Scene();

		// Initialize pools (texture parameters removed since rendering system not complete)
		// Pre-allocate 200 bullets and 1000 particles
		m_BulletPool.Init(m_Scene, 200);
		m_ParticlePool.Init(m_Scene, 1000);

		PIL_INFO("Object pools initialized!");
		PIL_INFO("  Bullets: {0} pre-allocated", m_BulletPool.GetTotalCount());
		PIL_INFO("  Particles: {0} pre-allocated", m_ParticlePool.GetTotalCount());

		// Track active bullets for cleanup
		m_ActiveBullets.reserve(200);
	}

	void OnDetach() override 
	{ 
		// Clean up
		m_BulletPool.Clear();
		m_ParticlePool.Clear();
		delete m_Scene;

		Layer::OnDetach();
	}

	void OnUpdate(float dt) override
	{
		// Update camera
		m_CameraController.OnUpdate(dt);

		// Handle input for spawning bullets
		HandleInput();

		// Update bullet lifetimes
		UpdateBullets(dt);

		// Render
		Render();
	}

	void HandleInput()
	{
		// Spawn bullet on left mouse click
		if (Pillar::Input::IsMouseButtonPressed(PIL_MOUSE_BUTTON_LEFT) && !m_MousePressed)
		{
			m_MousePressed = true;
			SpawnBulletAtMouse();
		}
		else if (!Pillar::Input::IsMouseButtonPressed(PIL_MOUSE_BUTTON_LEFT))
		{
			m_MousePressed = false;
		}

		// Spawn particle burst on right mouse click
		if (Pillar::Input::IsMouseButtonPressed(PIL_MOUSE_BUTTON_RIGHT) && !m_RightMousePressed)
		{
			m_RightMousePressed = true;
			SpawnParticleBurst();
		}
		else if (!Pillar::Input::IsMouseButtonPressed(PIL_MOUSE_BUTTON_RIGHT))
		{
			m_RightMousePressed = false;
		}
	}

	void SpawnBulletAtMouse()
	{
		// Get mouse position in world space
		auto mousePair = Pillar::Input::GetMousePosition();
		glm::vec2 worldPos = ScreenToWorld(glm::vec2(mousePair.first, mousePair.second));

		// Spawn bullet from center of screen toward mouse
		glm::vec2 center(0.0f, 0.0f);
		glm::vec2 direction = glm::normalize(worldPos - center);

		// Use the pool to spawn bullet
		Pillar::Entity bullet = m_BulletPool.SpawnBullet(
			center,              // position
			direction,           // direction
			10.0f,              // speed
			Pillar::Entity(),   // owner (none for demo)
			25.0f,              // damage
			3.0f                // lifetime
		);

		// Track active bullet
		m_ActiveBullets.push_back(bullet);

		// Spawn particles at spawn point for visual effect
		for (int i = 0; i < 5; i++)
		{
			glm::vec2 particleVel = direction * (5.0f + (rand() % 5));
			m_ParticlePool.SpawnParticle(
				center,
				particleVel,
				glm::vec4(1.0f, 0.8f, 0.2f, 1.0f),
				0.05f,
				0.5f
			);
		}

		PIL_INFO("Spawned bullet | Active: {0}/{1}", 
			m_BulletPool.GetActiveCount(), 
			m_BulletPool.GetTotalCount());
	}

	void SpawnParticleBurst()
	{
		// Get mouse position
		auto mousePair = Pillar::Input::GetMousePosition();
		glm::vec2 worldPos = ScreenToWorld(glm::vec2(mousePair.first, mousePair.second));

		// Spawn burst of 50 particles
		for (int i = 0; i < 50; i++)
		{
			// Random direction
			float angle = (rand() % 360) * (3.14159f / 180.0f);
			glm::vec2 direction(cos(angle), sin(angle));
			
			// Random speed
			float speed = 2.0f + (rand() % 100) / 100.0f * 5.0f;

			// Random color
			glm::vec4 color(
				0.5f + (rand() % 50) / 100.0f,
				0.5f + (rand() % 50) / 100.0f,
				0.5f + (rand() % 50) / 100.0f,
				1.0f
			);

			m_ParticlePool.SpawnParticle(
				worldPos,
				direction * speed,
				color,
				0.1f,
				2.0f
			);
		}

		PIL_INFO("Spawned particle burst | Active: {0}/{1}", 
			m_ParticlePool.GetActiveCount(), 
			m_ParticlePool.GetTotalCount());
	}

	void UpdateBullets(float dt)
	{
		// Update bullet lifetimes and return expired bullets to pool
		auto it = m_ActiveBullets.begin();
		while (it != m_ActiveBullets.end())
		{
			auto& bullet = *it;
			auto& bulletComp = bullet.GetComponent<Pillar::BulletComponent>();
			
			bulletComp.TimeAlive += dt;

			// Check if bullet expired
			if (bulletComp.TimeAlive >= bulletComp.Lifetime)
			{
				// Return to pool instead of destroying
				m_BulletPool.ReturnBullet(bullet);
				it = m_ActiveBullets.erase(it);

				PIL_TRACE("Returned bullet to pool | Available: {0}", 
					m_BulletPool.GetAvailableCount());
			}
			else
			{
				++it;
			}
		}
	}

	void Render()
	{
		// Clear screen
		Pillar::Renderer::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
		Pillar::Renderer::Clear();

		// Begin scene
		Pillar::Renderer2D::BeginScene(m_CameraController.GetCamera());

		// Draw crosshair at center
		Pillar::Renderer2DBackend::DrawQuad({ 0.0f, 0.0f }, { 0.1f, 0.1f }, { 1.0f, 0.0f, 0.0f, 1.0f });

		// TODO: Render bullets and particles from scene
		// (Will be implemented when rendering system integration is complete)

		Pillar::Renderer2D::EndScene();
	}

	void OnImGuiRender() override 
	{
		ImGui::Begin("Object Pool Demo");
		
		ImGui::Text("Object Pooling Performance Demo");
		ImGui::Separator();

		// Instructions
		ImGui::Text("Controls:");
		ImGui::BulletText("Left Click: Spawn bullet");
		ImGui::BulletText("Right Click: Spawn particle burst");
		ImGui::BulletText("WASD: Move camera");
		ImGui::Separator();

		// Pool stats
		ImGui::Text("Bullet Pool:");
		ImGui::Text("  Active: %zu", m_BulletPool.GetActiveCount());
		ImGui::Text("  Available: %zu", m_BulletPool.GetAvailableCount());
		ImGui::Text("  Total: %zu", m_BulletPool.GetTotalCount());
		
		ImGui::Separator();

		ImGui::Text("Particle Pool:");
		ImGui::Text("  Active: %zu", m_ParticlePool.GetActiveCount());
		ImGui::Text("  Available: %zu", m_ParticlePool.GetAvailableCount());
		ImGui::Text("  Total: %zu", m_ParticlePool.GetTotalCount());

		ImGui::Separator();

		// Performance benefits
		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Performance Benefits:");
		ImGui::BulletText("Reduced memory allocations");
		ImGui::BulletText("Cache-friendly entity reuse");
		ImGui::BulletText("Predictable performance");
		ImGui::BulletText("Avoid heap fragmentation");

		ImGui::End();
	}

	void OnEvent(Pillar::Event& event) override
	{
		m_CameraController.OnEvent(event);
	}

private:
	glm::vec2 ScreenToWorld(const glm::vec2& screenPos)
	{
		// Simple conversion for demo
		// In production, use proper screen-to-world transformation
		// For now, just normalize screen coordinates to [-1, 1] range
		return screenPos;
	}

private:
	Pillar::Scene* m_Scene = nullptr;
	Pillar::OrthographicCameraController m_CameraController;

	// Object pools
	Pillar::BulletPool m_BulletPool;
	Pillar::ParticlePool m_ParticlePool;

	// Track active bullets for lifetime management
	std::vector<Pillar::Entity> m_ActiveBullets;

	// Input state
	bool m_MousePressed = false;
	bool m_RightMousePressed = false;
};
