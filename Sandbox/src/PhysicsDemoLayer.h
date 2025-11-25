#pragma once

#include "Pillar.h"
#include "Pillar/Renderer/Renderer2D.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Core/TagComponent.h"
#include "Pillar/ECS/Components/Physics/RigidbodyComponent.h"
#include "Pillar/ECS/Components/Physics/ColliderComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"
#include "Pillar/ECS/Components/Gameplay/BulletComponent.h"
#include "Pillar/ECS/Components/Gameplay/XPGemComponent.h"
#include "Pillar/ECS/Systems/PhysicsSystem.h"
#include "Pillar/ECS/Systems/PhysicsSyncSystem.h"
#include "Pillar/ECS/Systems/VelocityIntegrationSystem.h"
#include "Pillar/ECS/Systems/BulletCollisionSystem.h"
#include "Pillar/ECS/Systems/XPCollectionSystem.h"
#include <imgui.h>
#include <memory>

class PhysicsDemoLayer : public Pillar::Layer
{
public:
	PhysicsDemoLayer()
		: Layer("PhysicsDemoLayer"),
		  m_CameraController(16.0f / 9.0f, true)
	{
	}

	void OnAttach() override
	{
		Layer::OnAttach();
		PIL_INFO("Physics Demo Layer attached!");

		// Create scene
		m_Scene = std::make_unique<Pillar::Scene>();

		// Create systems
		m_PhysicsSystem = new Pillar::PhysicsSystem(glm::vec2(0.0f, 0.0f)); // No gravity (top-down)
		m_PhysicsSyncSystem = new Pillar::PhysicsSyncSystem();
		m_VelocityIntegrationSystem = new Pillar::VelocityIntegrationSystem();
		m_BulletCollisionSystem = new Pillar::BulletCollisionSystem(m_PhysicsSystem);
		m_XPCollectionSystem = new Pillar::XPCollectionSystem();

		m_PhysicsSystem->OnAttach(m_Scene.get());
		m_PhysicsSyncSystem->OnAttach(m_Scene.get());
		m_VelocityIntegrationSystem->OnAttach(m_Scene.get());
		m_BulletCollisionSystem->OnAttach(m_Scene.get());
		m_XPCollectionSystem->OnAttach(m_Scene.get());

		// Tell scene about physics system (for cleanup)
		m_Scene->SetPhysicsSystem(m_PhysicsSystem);

		// Create world
		CreateWalls();
		CreatePlayer();
		CreateEnemies(5);
		CreateXPGems(20);

		PIL_INFO("Physics demo initialized!");
	}

	void OnDetach() override
	{
		delete m_PhysicsSystem;
		delete m_PhysicsSyncSystem;
		delete m_VelocityIntegrationSystem;
		delete m_BulletCollisionSystem;
		delete m_XPCollectionSystem;

		Layer::OnDetach();
	}

	void OnUpdate(float dt) override
	{
		// Update camera
		m_CameraController.OnUpdate(dt);

		// Handle player input
		HandlePlayerInput(dt);

		// Update systems in correct order
		m_PhysicsSystem->OnUpdate(dt);
		m_PhysicsSyncSystem->OnUpdate(dt);
		m_VelocityIntegrationSystem->OnUpdate(dt);
		m_BulletCollisionSystem->OnUpdate(dt);
		m_XPCollectionSystem->OnUpdate(dt);

		// Render
		Pillar::Renderer::SetClearColor({ 0.1f, 0.1f, 0.15f, 1.0f });
		Pillar::Renderer::Clear();

		Pillar::Renderer2D::BeginScene(m_CameraController.GetCamera());

		// Draw all entities
		DrawEntities();

		Pillar::Renderer2D::EndScene();
	}

	void OnEvent(Pillar::Event& event) override
	{
		m_CameraController.OnEvent(event);

		if (event.GetEventType() == Pillar::EventType::MouseButtonPressed)
		{
			auto& mouseEvent = static_cast<Pillar::MouseButtonPressedEvent&>(event);
			if (mouseEvent.GetMouseButton() == PIL_MOUSE_BUTTON_LEFT)
			{
				ShootBullet();
			}
		}
	}

	void OnImGuiRender() override
	{
		// ImGui window removed - less clutter, more visibility
		// Stats can be re-enabled if needed for debugging
	}

private:
	void CreateWalls()
	{
		// Bottom wall
		auto wall = m_Scene->CreateEntity("Wall");
		auto& transform = wall.GetComponent<Pillar::TransformComponent>();
		transform.Position = glm::vec2(0.0f, -8.0f);
		wall.AddComponent<Pillar::RigidbodyComponent>(b2_staticBody);
		wall.AddComponent<Pillar::ColliderComponent>(Pillar::ColliderComponent::Box(glm::vec2(15.0f, 0.5f)));

		// Top wall
		wall = m_Scene->CreateEntity("Wall");
		transform = wall.GetComponent<Pillar::TransformComponent>();
		transform.Position = glm::vec2(0.0f, 8.0f);
		wall.AddComponent<Pillar::RigidbodyComponent>(b2_staticBody);
		wall.AddComponent<Pillar::ColliderComponent>(Pillar::ColliderComponent::Box(glm::vec2(15.0f, 0.5f)));

		// Left wall
		wall = m_Scene->CreateEntity("Wall");
		transform = wall.GetComponent<Pillar::TransformComponent>();
		transform.Position = glm::vec2(-15.0f, 0.0f);
		wall.AddComponent<Pillar::RigidbodyComponent>(b2_staticBody);
		wall.AddComponent<Pillar::ColliderComponent>(Pillar::ColliderComponent::Box(glm::vec2(0.5f, 8.0f)));

		// Right wall
		wall = m_Scene->CreateEntity("Wall");
		transform = wall.GetComponent<Pillar::TransformComponent>();
		transform.Position = glm::vec2(15.0f, 0.0f);
		wall.AddComponent<Pillar::RigidbodyComponent>(b2_staticBody);
		wall.AddComponent<Pillar::ColliderComponent>(Pillar::ColliderComponent::Box(glm::vec2(0.5f, 8.0f)));
	}

	void CreatePlayer()
	{
		m_Player = m_Scene->CreateEntity("Player");
		auto& transform = m_Player.GetComponent<Pillar::TransformComponent>();
		transform.Position = glm::vec2(0.0f, 0.0f);

		m_Player.AddComponent<Pillar::RigidbodyComponent>(b2_dynamicBody);
		auto& rb = m_Player.GetComponent<Pillar::RigidbodyComponent>();
		rb.FixedRotation = true; // Don't rotate

		m_Player.AddComponent<Pillar::ColliderComponent>(Pillar::ColliderComponent::Circle(0.5f));
	}

	void CreateEnemies(int count)
	{
		for (int i = 0; i < count; ++i)
		{
			float angle = (i / (float)count) * 2.0f * 3.14159f;
			float radius = 5.0f;
			glm::vec2 pos(cos(angle) * radius, sin(angle) * radius);

			auto enemy = m_Scene->CreateEntity("Enemy");
			auto& transform = enemy.GetComponent<Pillar::TransformComponent>();
			transform.Position = pos;

			enemy.AddComponent<Pillar::RigidbodyComponent>(b2_dynamicBody);
			enemy.AddComponent<Pillar::ColliderComponent>(Pillar::ColliderComponent::Circle(0.4f));
		}
	}

	void CreateXPGems(int count)
	{
		for (int i = 0; i < count; ++i)
		{
			float x = ((rand() % 200) - 100) / 10.0f; // -10 to 10
			float y = ((rand() % 140) - 70) / 10.0f;  // -7 to 7

			auto gem = m_Scene->CreateEntity("XPGem");
			auto& transform = gem.GetComponent<Pillar::TransformComponent>();
			transform.Position = glm::vec2(x, y);

			gem.AddComponent<Pillar::VelocityComponent>();
			gem.AddComponent<Pillar::XPGemComponent>(1);
		}
	}

	void HandlePlayerInput(float dt)
	{
		if (!m_Player)
			return;

		auto& rb = m_Player.GetComponent<Pillar::RigidbodyComponent>();
		if (!rb.Body)
			return;

		glm::vec2 input(0.0f);
		// Use arrow keys for player movement (camera uses WASD)
		if (Pillar::Input::IsKeyPressed(PIL_KEY_UP)) input.y += 1.0f;
		if (Pillar::Input::IsKeyPressed(PIL_KEY_DOWN)) input.y -= 1.0f;
		if (Pillar::Input::IsKeyPressed(PIL_KEY_LEFT)) input.x -= 1.0f;
		if (Pillar::Input::IsKeyPressed(PIL_KEY_RIGHT)) input.x += 1.0f;

		if (glm::length(input) > 0.0f)
		{
			input = glm::normalize(input) * m_PlayerSpeed;
			rb.Body->SetLinearVelocity(b2Vec2(input.x, input.y));
		}
		else
		{
			rb.Body->SetLinearVelocity(b2Vec2(0, 0));
		}

		// Reset demo
		if (Pillar::Input::IsKeyPressed(PIL_KEY_R))
		{
			ResetDemo();
		}
	}

	void ShootBullet()
	{
		if (!m_Player)
			return;

		auto& playerTransform = m_Player.GetComponent<Pillar::TransformComponent>();

		// Get mouse position in world space
		auto mousePos = Pillar::Input::GetMousePosition();
		auto& camera = m_CameraController.GetCamera();

		// Convert screen to world coordinates (simplified - just use right direction for now)
		glm::vec2 direction(1.0f, 0.0f); // Shoot right as a simple demo

		// Create bullet
		auto bullet = m_Scene->CreateEntity("Bullet");
		auto& transform = bullet.GetComponent<Pillar::TransformComponent>();
		transform.Position = playerTransform.Position + direction * 0.6f; // Offset from player

		bullet.AddComponent<Pillar::VelocityComponent>(direction * m_BulletSpeed);
		bullet.AddComponent<Pillar::BulletComponent>(m_Player, 10.0f);

		PIL_CORE_TRACE("Bullet shot!");
	}

	void DrawEntities()
	{
		// Draw walls
		auto wallView = m_Scene->GetRegistry().view<Pillar::TagComponent, Pillar::TransformComponent, Pillar::ColliderComponent>();
		for (auto entity : wallView)
		{
			auto& tag = wallView.get<Pillar::TagComponent>(entity);
			if (tag.Tag != "Wall")
				continue;

			auto& transform = wallView.get<Pillar::TransformComponent>(entity);
			auto& collider = wallView.get<Pillar::ColliderComponent>(entity);

			glm::vec2 size = collider.Type == Pillar::ColliderType::Box ?
				collider.HalfExtents * 2.0f : glm::vec2(collider.Radius * 2.0f);

			Pillar::Renderer2D::DrawQuad(transform.Position, size, { 0.3f, 0.3f, 0.3f, 1.0f });
		}

		// Draw player
		if (m_Player)
		{
			auto& transform = m_Player.GetComponent<Pillar::TransformComponent>();
			Pillar::Renderer2D::DrawQuad(transform.Position, { 1.0f, 1.0f }, { 0.2f, 0.8f, 0.3f, 1.0f });
		}

		// Draw enemies
		auto enemyView = m_Scene->GetRegistry().view<Pillar::TagComponent, Pillar::TransformComponent>();
		for (auto entity : enemyView)
		{
			auto& tag = enemyView.get<Pillar::TagComponent>(entity);
			if (tag.Tag != "Enemy")
				continue;

			auto& transform = enemyView.get<Pillar::TransformComponent>(entity);
			Pillar::Renderer2D::DrawQuad(transform.Position, { 0.8f, 0.8f }, { 0.9f, 0.2f, 0.2f, 1.0f });
		}

		// Draw XP gems
		auto gemView = m_Scene->GetRegistry().view<Pillar::TransformComponent, Pillar::XPGemComponent>();
		for (auto entity : gemView)
		{
			auto& transform = gemView.get<Pillar::TransformComponent>(entity);
			auto& gem = gemView.get<Pillar::XPGemComponent>(entity);

			glm::vec4 color = gem.IsAttracted ?
				glm::vec4(1.0f, 1.0f, 0.2f, 1.0f) :  // Yellow when attracted
				glm::vec4(0.8f, 0.8f, 0.2f, 1.0f);   // Dim yellow otherwise

			Pillar::Renderer2D::DrawQuad(transform.Position, { 0.3f, 0.3f }, color);
		}

		// Draw bullets
		auto bulletView = m_Scene->GetRegistry().view<Pillar::TransformComponent, Pillar::BulletComponent>();
		for (auto entity : bulletView)
		{
			auto& transform = bulletView.get<Pillar::TransformComponent>(entity);
			Pillar::Renderer2D::DrawQuad(transform.Position, { 0.2f, 0.2f }, { 1.0f, 0.5f, 0.0f, 1.0f });
		}
	}

	void ResetDemo()
	{
		PIL_INFO("Resetting demo...");

		// Clear all entities
		m_Scene->GetRegistry().clear();
		m_Player = Pillar::Entity();

		// Recreate world
		CreateWalls();
		CreatePlayer();
		CreateEnemies(5);
		CreateXPGems(20);
	}

private:
	std::unique_ptr<Pillar::Scene> m_Scene;
	Pillar::OrthographicCameraController m_CameraController;

	// Systems
	Pillar::PhysicsSystem* m_PhysicsSystem = nullptr;
	Pillar::PhysicsSyncSystem* m_PhysicsSyncSystem = nullptr;
	Pillar::VelocityIntegrationSystem* m_VelocityIntegrationSystem = nullptr;
	Pillar::BulletCollisionSystem* m_BulletCollisionSystem = nullptr;
	Pillar::XPCollectionSystem* m_XPCollectionSystem = nullptr;

	// Player
	Pillar::Entity m_Player;

	// Game parameters
	float m_PlayerSpeed = 5.0f;
	float m_BulletSpeed = 15.0f;
};
