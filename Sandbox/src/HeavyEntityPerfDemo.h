#pragma once

#include "Pillar.h"
#include "Pillar/Renderer/Renderer2D.h"
#include "Pillar/Renderer/Renderer2DBackend.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Physics/RigidbodyComponent.h"
#include "Pillar/ECS/Components/Physics/ColliderComponent.h"
#include "Pillar/ECS/Systems/PhysicsSystem.h"
#include "Pillar/ECS/Systems/PhysicsSyncSystem.h"
#include <imgui.h>
#include <memory>
#include <random>

class HeavyEntityPerfDemo : public Pillar::Layer
{
public:
	HeavyEntityPerfDemo()
		: Layer("HeavyEntityPerfDemo"),
		  m_CameraController(16.0f / 9.0f, false) // No rotation for perf demo
	{
	}

	void OnAttach() override
	{
		Layer::OnAttach();
		PIL_INFO("Heavy Entity Performance Demo attached!");

		// Create scene
		m_Scene = std::make_unique<Pillar::Scene>();

		// Create systems
		m_PhysicsSystem = new Pillar::PhysicsSystem(glm::vec2(0.0f, -9.81f)); // With gravity
		m_PhysicsSyncSystem = new Pillar::PhysicsSyncSystem();

		m_PhysicsSystem->OnAttach(m_Scene.get());
		m_PhysicsSyncSystem->OnAttach(m_Scene.get());

		m_Scene->SetPhysicsSystem(m_PhysicsSystem);

		// Create boundaries
		CreateBoundaries();

		// Start with moderate amount
		SpawnHeavyEntities(50);

		PIL_INFO("Heavy entity perf demo initialized with {} entities", m_EntityCount);
	}

	void OnDetach() override
	{
		delete m_PhysicsSystem;
		delete m_PhysicsSyncSystem;

		Layer::OnDetach();
	}

	void OnUpdate(float dt) override
	{
		m_FrameTime = dt * 1000.0f; // Convert to ms

		// Update camera
		m_CameraController.OnUpdate(dt);

		// Update physics systems
		auto sysStart = std::chrono::high_resolution_clock::now();
		m_PhysicsSystem->OnUpdate(dt);
		m_PhysicsSyncSystem->OnUpdate(dt);
		auto sysEnd = std::chrono::high_resolution_clock::now();
		m_SystemTime = std::chrono::duration<float, std::milli>(sysEnd - sysStart).count();

		// Render
		Pillar::Renderer::SetClearColor({ 0.05f, 0.05f, 0.08f, 1.0f });
		Pillar::Renderer::Clear();

		auto renderStart = std::chrono::high_resolution_clock::now();
		Pillar::Renderer2DBackend::ResetStats();
		Pillar::Renderer2DBackend::BeginScene(m_CameraController.GetCamera());
		DrawEntities();
		Pillar::Renderer2DBackend::EndScene();
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
		ImGui::Begin("Heavy Entity Performance");
		
		ImGui::Text("Stress Test: Box2D Physics Bodies");
		ImGui::Separator();

		// Performance stats
		ImGui::Text("Entity Count: %zu", m_EntityCount);
		ImGui::Text("Frame Time: %.2f ms (%.0f FPS)", m_FrameTime, 1000.0f / m_FrameTime);
		ImGui::Text("Physics Time: %.2f ms", m_SystemTime);
		ImGui::Text("Render Time: %.2f ms", m_RenderTime);
		
		// Renderer stats
		ImGui::Separator();
		ImGui::Text("Renderer Statistics:");
		ImGui::Text("  Draw Calls: %u", Pillar::Renderer2DBackend::GetDrawCallCount());
		ImGui::Text("  Quads Rendered: %u", Pillar::Renderer2DBackend::GetQuadCount());
		
		// Color-coded performance
		if (m_FrameTime < 16.67f)
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "Performance: EXCELLENT (60+ FPS)");
		else if (m_FrameTime < 33.33f)
			ImGui::TextColored(ImVec4(1, 1, 0, 1), "Performance: GOOD (30-60 FPS)");
		else
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "Performance: POOR (<30 FPS)");

		ImGui::Separator();

		// Spawn controls
		ImGui::Text("Spawn Physics Bodies:");
		if (ImGui::Button("+ 10")) SpawnHeavyEntities(10);
		ImGui::SameLine();
		if (ImGui::Button("+ 25")) SpawnHeavyEntities(25);
		ImGui::SameLine();
		if (ImGui::Button("+ 50")) SpawnHeavyEntities(50);
		
		if (ImGui::Button("+ 100")) SpawnHeavyEntities(100);
		ImGui::SameLine();
		if (ImGui::Button("+ 250")) SpawnHeavyEntities(250);

		ImGui::Separator();

		// Entity type selection
		ImGui::Text("Spawn Type:");
		ImGui::RadioButton("Dynamic (affected by gravity)", &m_SpawnType, 0);
		ImGui::RadioButton("Kinematic (no gravity)", &m_SpawnType, 1);

		ImGui::Separator();
		
		if (ImGui::Button("Clear All"))
		{
			ClearAll();
		}

		ImGui::Separator();
		ImGui::Text("Camera: WASD to move, Scroll to zoom");
		ImGui::Text("Watch entities fall and collide!");

		ImGui::End();
	}

private:
	void CreateBoundaries()
	{
		// Bottom
		auto wall = m_Scene->CreateEntity("Ground");
		auto& transform = wall.GetComponent<Pillar::TransformComponent>();
		transform.Position = glm::vec2(0.0f, -10.0f);
		wall.AddComponent<Pillar::RigidbodyComponent>(b2_staticBody);
		wall.AddComponent<Pillar::ColliderComponent>(Pillar::ColliderComponent::Box(glm::vec2(25.0f, 1.0f)));

		// Left wall
		wall = m_Scene->CreateEntity("Wall");
		transform = wall.GetComponent<Pillar::TransformComponent>();
		transform.Position = glm::vec2(-25.0f, 0.0f);
		wall.AddComponent<Pillar::RigidbodyComponent>(b2_staticBody);
		wall.AddComponent<Pillar::ColliderComponent>(Pillar::ColliderComponent::Box(glm::vec2(1.0f, 15.0f)));

		// Right wall
		wall = m_Scene->CreateEntity("Wall");
		transform = wall.GetComponent<Pillar::TransformComponent>();
		transform.Position = glm::vec2(25.0f, 0.0f);
		wall.AddComponent<Pillar::RigidbodyComponent>(b2_staticBody);
		wall.AddComponent<Pillar::ColliderComponent>(Pillar::ColliderComponent::Box(glm::vec2(1.0f, 15.0f)));
	}

	void SpawnHeavyEntities(int count)
	{
		PIL_INFO("Spawning {} heavy entities...", count);

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<float> posX(-20.0f, 20.0f);
		std::uniform_real_distribution<float> posY(5.0f, 15.0f); // Spawn above ground
		std::uniform_real_distribution<float> size(0.3f, 0.8f);
		std::uniform_int_distribution<int> shapeType(0, 1); // 0=circle, 1=box

		b2BodyType bodyType = (m_SpawnType == 0) ? b2_dynamicBody : b2_kinematicBody;

		for (int i = 0; i < count; ++i)
		{
			auto entity = m_Scene->CreateEntity("PhysicsBody");
			auto& transform = entity.GetComponent<Pillar::TransformComponent>();
			transform.Position = glm::vec2(posX(gen), posY(gen));

			entity.AddComponent<Pillar::RigidbodyComponent>(bodyType);

			// Random shape
			if (shapeType(gen) == 0)
			{
				entity.AddComponent<Pillar::ColliderComponent>(
					Pillar::ColliderComponent::Box(glm::vec2(size(gen), size(gen)))
				);
			}
			else
			{
				float s = size(gen);
				entity.AddComponent<Pillar::ColliderComponent>(
					Pillar::ColliderComponent::Box(glm::vec2(s, s))
				);
			}
		}

		PIL_INFO("Spawned {} entities. Total: {}", count, m_Scene->GetRegistry().storage<entt::entity>().in_use());
	}

	void DrawEntities()
	{
		// Draw all physics bodies
		auto view = m_Scene->GetRegistry().view<Pillar::TransformComponent, Pillar::RigidbodyComponent, Pillar::ColliderComponent>();
		
		for (auto entity : view)
		{
			auto& transform = view.get<Pillar::TransformComponent>(entity);
			auto& rb = view.get<Pillar::RigidbodyComponent>(entity);
			auto& collider = view.get<Pillar::ColliderComponent>(entity);

			// Color based on body type
			glm::vec4 color;
			if (rb.BodyType == b2_staticBody)
				color = glm::vec4(0.3f, 0.3f, 0.3f, 1.0f); // Gray for static
			else if (rb.BodyType == b2_kinematicBody)
				color = glm::vec4(0.3f, 0.5f, 0.8f, 1.0f); // Blue for kinematic
			else
				color = glm::vec4(0.8f, 0.3f, 0.3f, 1.0f); // Red for dynamic

			// Size based on collider
			glm::vec2 size;
			if (collider.Type == Pillar::ColliderType::Circle)
				size = glm::vec2(collider.Radius * 2.0f);
			else
				size = collider.HalfExtents * 2.0f;

			if (transform.Rotation != 0.0f)
				Pillar::Renderer2DBackend::DrawRotatedQuad(transform.Position, size, transform.Rotation, color);
			else
				Pillar::Renderer2DBackend::DrawQuad(transform.Position, size, color);
		}
	}

	void ClearAll()
	{
		PIL_INFO("Clearing all entities...");
		m_Scene->GetRegistry().clear();
		CreateBoundaries();
		PIL_INFO("Cleared. Remaining: {}", m_Scene->GetRegistry().storage<entt::entity>().in_use());
	}

private:
	std::unique_ptr<Pillar::Scene> m_Scene;
	Pillar::OrthographicCameraController m_CameraController;

	// Systems
	Pillar::PhysicsSystem* m_PhysicsSystem = nullptr;
	Pillar::PhysicsSyncSystem* m_PhysicsSyncSystem = nullptr;

	// UI state
	int m_SpawnType = 0; // 0=dynamic, 1=kinematic

	// Performance metrics
	size_t m_EntityCount = 0;
	float m_FrameTime = 0.0f;
	float m_SystemTime = 0.0f;
	float m_RenderTime = 0.0f;
};
