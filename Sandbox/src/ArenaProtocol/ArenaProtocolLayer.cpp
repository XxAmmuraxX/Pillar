#include "ArenaProtocolLayer.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Core/TagComponent.h"
#include "Pillar/ECS/Components/Rendering/SpriteComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"
#include "Pillar/ECS/Components/Gameplay/BulletComponent.h"
#include "Pillar/ECS/Components/Gameplay/XPGemComponent.h"
#include "Pillar/Renderer/Renderer2DBackend.h"
#include "Pillar/Renderer/RenderCommand.h"
#include "Pillar/Audio/AudioClip.h"
#include "Pillar/Utils/AssetManager.h"
#include "Pillar/Input.h"
#include "Pillar/KeyCodes.h"
#include "Pillar/Application.h"
#include <imgui.h>
#include <glm/gtc/constants.hpp>
#include <algorithm>

namespace Pillar {

	ArenaProtocolLayer::ArenaProtocolLayer()
		: Layer("ArenaProtocol"), m_CameraController(1920.0f / 1080.0f, true)
	{
	}

	void ArenaProtocolLayer::OnAttach()
	{
		PIL_INFO("=== Arena Protocol: Technical Showcase ===");
		PIL_INFO("Initializing game systems...");

		// Create scene
		m_Scene = std::make_shared<Scene>("ArenaProtocol");

		// Initialize all systems
		InitializeSystems();
		InitializeAudio();
		InitializePlayer();
		InitializeBulletPool();
		SetupCallbacks();

		// Load initial chamber (Enemy Gauntlet for action)
		m_ChamberManager->LoadChamber(ChamberType::EnemyGauntlet);

		// Start wave system
		m_WaveSystem->StartWave(1);

		// Set camera position
		m_CameraController.GetCamera().SetPosition({ 0.0f, 0.0f, 0.0f });
		m_CameraController.SetZoomLevel(15.0f);

		m_GameState = GameState::Playing;
		PIL_INFO("Arena Protocol initialized successfully!");
	}

	void ArenaProtocolLayer::OnDetach()
	{
		// Audio cleanup handled by Application destructor
		// Scene cleanup will destroy AudioSourceComponents
		
		m_Scene.reset();
		m_BulletPool.reset();
		m_EntityFactory.reset();
		m_ChamberManager.reset();
	}

	void ArenaProtocolLayer::InitializeSystems()
	{
		// Create factory
		m_EntityFactory = std::make_unique<EntityFactory>();
		m_EntityFactory->Initialize(m_Scene.get(), "Sandbox/assets");

		// Create game systems
		m_EnemyAISystem = std::make_unique<EnemyAISystem>();
		m_BossAISystem = std::make_unique<BossAISystem>();
		m_BulletSystem = std::make_unique<BulletSystem>();
		m_XPCollectionSystem = std::make_unique<ArenaProtocol::XPCollectionSystem>();
		m_WaveSystem = std::make_unique<WaveSystem>();

		// Chamber manager (initialized after player creation)
		m_ChamberManager = std::make_unique<ChamberManager>();
	}

	void ArenaProtocolLayer::InitializePlayer()
	{
		m_Player = m_EntityFactory->CreatePlayer({ 0.0f, -5.0f });
		
		// Initialize chamber manager with player reference
		m_ChamberManager->Initialize(m_Scene.get(), m_EntityFactory.get(), m_Player);
	}

	void ArenaProtocolLayer::InitializeBulletPool()
	{
		m_BulletPool = std::make_unique<ObjectPool>();

		// Set init callback BEFORE Init() - this is called for each entity during creation
		m_BulletPool->SetInitCallback([this](Entity entity) {
			// Add all required components for bullets
			auto& transform = entity.AddComponent<TransformComponent>();
			transform.Position = glm::vec2(-1000.0f, -1000.0f); // Off-screen

			auto& sprite = entity.AddComponent<SpriteComponent>();
			sprite.Texture = m_EntityFactory->GetBulletTexture();
			sprite.Color = { 0.0f, 1.0f, 1.0f, 0.0f }; // Hidden initially (alpha = 0)
			sprite.Size = { 0.2f, 0.4f };

			entity.AddComponent<VelocityComponent>();

			auto& bc = entity.AddComponent<BulletComponent>();
			bc.Lifetime = 3.0f;
			bc.Damage = 10.0f;
		});

		// Set reset callback - called when bullet is returned to pool
		m_BulletPool->SetResetCallback([](Entity entity) {
			if (entity.HasComponent<TransformComponent>())
			{
				auto& transform = entity.GetComponent<TransformComponent>();
				transform.Position = glm::vec2(-1000.0f, -1000.0f);
			}
			if (entity.HasComponent<SpriteComponent>())
			{
				auto& sprite = entity.GetComponent<SpriteComponent>();
				sprite.Color.a = 0.0f; // Hide
			}
			if (entity.HasComponent<BulletComponent>())
			{
				auto& bullet = entity.GetComponent<BulletComponent>();
				bullet.TimeAlive = 0.0f;
				bullet.HitsRemaining = 1;
			}
		});

		// Now initialize - each entity will get components via the callback
		m_BulletPool->Init(m_Scene.get(), 500);
		PIL_INFO("Bullet pool initialized with 500 pre-configured bullets");
	}

	void ArenaProtocolLayer::InitializeAudio()
	{
		// Resolve SFX paths via AssetManager (works from build output and editor)
		auto loadClip = [](const std::string& name) -> std::shared_ptr<AudioClip>
		{
			std::string path = AssetManager::GetSFXPath(name);
			auto clip = AudioClip::Create(path);
			if (!clip)
			{
				PIL_WARN("Audio: Failed to load clip '{}' (resolved path: {})", name, path);
			}
			return clip;
		};

		m_ShootSFX = loadClip("shoot.wav");
		m_EnemyShootSFX = loadClip("enemy_shoot.wav");
		m_HitSFX = loadClip("hit.wav");
		m_ExplosionSFX = loadClip("explosion.wav");
		m_PickupSFX = loadClip("pickup.wav");
		m_DashSFX = loadClip("dash.wav");

		// Wire pickup SFX into XP collection system if available
		if (m_XPCollectionSystem)
		{
			m_XPCollectionSystem->SetPickupSFX(m_PickupSFX);
		}
	}

	void ArenaProtocolLayer::SetupCallbacks()
	{
		// Wave system callbacks
		m_WaveSystem->SetWaveCompleteCallback([this](int wave) {
			OnWaveComplete(wave);
		});

		m_WaveSystem->SetSpawnCallback([this](const std::string& type, const glm::vec2& pos) {
			SpawnEnemy(type, pos);
		});

		// Boss system callbacks
		m_BossAISystem->SetSpawnCallback([this](const glm::vec2& pos) {
			SpawnEnemy("Drone", pos);
		});

		m_BossAISystem->SetFireBulletCallback([this](const glm::vec2& pos, const glm::vec2& dir, Entity owner) {
			return FireBullet(pos, dir, owner, false);
		});
	}

	void ArenaProtocolLayer::OnUpdate(float deltaTime)
	{
		m_GameTime += deltaTime;

		if (m_GameState == GameState::Paused || m_GameState == GameState::GameOver || m_GameState == GameState::Victory)
		{
			// Still render, but don't update game logic
			RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.15f, 1.0f });
			RenderCommand::Clear();
			
			// Render scene
			Renderer2DBackend::BeginScene(m_CameraController.GetCamera());
			auto view = m_Scene->GetRegistry().view<TransformComponent, SpriteComponent>();
			for (auto entity : view)
			{
				auto [transform, sprite] = view.get<TransformComponent, SpriteComponent>(entity);
				if (sprite.Color.a > 0.0f)
				{
					if (transform.Rotation != 0.0f)
						Renderer2DBackend::DrawRotatedQuad(transform.Position, sprite.Size, transform.Rotation, sprite.Color, sprite.Texture);
					else
						Renderer2DBackend::DrawQuad(transform.Position, sprite.Size, sprite.Color, sprite.Texture);
				}
			}
			Renderer2DBackend::EndScene();
			return;
		}

		// Handle input
		HandleInput(deltaTime);

		// Update game systems
		UpdatePlayer(deltaTime);
		HandleShooting(deltaTime);
		
		// Update enemies and boss
		if (m_BossMode && m_Boss.IsValid())
		{
			UpdateBoss(deltaTime);
		}
		else
		{
			m_WaveSystem->OnUpdate(m_Scene.get(), m_Enemies, deltaTime);
		}
		
		UpdateEnemies(deltaTime);

		// Update bullets
		m_BulletSystem->OnUpdate(m_Scene.get(), m_BulletPool.get(), deltaTime);

		// Process collisions
		ProcessBulletCollisions();
		ProcessPlayerCollisions(deltaTime);

		// Update XP collection
		m_XPCollectionSystem->OnUpdate(m_Scene.get(), m_Player, deltaTime);

		// Update chamber manager (handles dead enemy cleanup)
		m_ChamberManager->OnUpdate(deltaTime);

		// Cleanup and check state
		CleanupDeadEntities();
		CheckVictoryCondition();

		// Update camera
		UpdateCamera(deltaTime);
		UpdateCameraEffects(deltaTime);

		// Render
		RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.15f, 1.0f });
		RenderCommand::Clear();

		Renderer2DBackend::BeginScene(m_CameraController.GetCamera());

		// Render all sprites (sorted by ZIndex)
		auto view = m_Scene->GetRegistry().view<TransformComponent, SpriteComponent>();
		
		// Collect and sort entities
		std::vector<std::pair<float, entt::entity>> sortedEntities;
		for (auto entity : view)
		{
			auto& sprite = view.get<SpriteComponent>(entity);
			if (sprite.Color.a > 0.0f)
			{
				sortedEntities.push_back({ sprite.ZIndex, entity });
			}
		}
		std::sort(sortedEntities.begin(), sortedEntities.end(), 
			[](const auto& a, const auto& b) { return a.first < b.first; });

		// Render sorted
		for (const auto& [zindex, entity] : sortedEntities)
		{
			auto [transform, sprite] = view.get<TransformComponent, SpriteComponent>(entity);
			
			// Use rotated quad for entities with rotation, else use regular quad
			if (transform.Rotation != 0.0f)
			{
				Renderer2DBackend::DrawRotatedQuad(transform.Position, sprite.Size, transform.Rotation, sprite.Color, sprite.Texture);
			}
			else
			{
				Renderer2DBackend::DrawQuad(transform.Position, sprite.Size, sprite.Color, sprite.Texture);
			}
		}

		Renderer2DBackend::EndScene();
	}

	void ArenaProtocolLayer::UpdatePlayer(float deltaTime)
	{
		if (!m_Player.IsValid()) return;

		auto& transform = m_Player.GetComponent<TransformComponent>();
		auto& velocity = m_Player.GetComponent<VelocityComponent>();
		auto& stats = m_Player.GetComponent<PlayerStatsComponent>();

		// Check for death
		if (stats.Health <= 0.0f)
		{
			OnPlayerDeath();
			return;
		}

		// Movement
		glm::vec2 moveDir(0.0f);
		if (Input::IsKeyPressed(PIL_KEY_W)) moveDir.y += 1.0f;
		if (Input::IsKeyPressed(PIL_KEY_S)) moveDir.y -= 1.0f;
		if (Input::IsKeyPressed(PIL_KEY_A)) moveDir.x -= 1.0f;
		if (Input::IsKeyPressed(PIL_KEY_D)) moveDir.x += 1.0f;

		float speed = 6.0f;
		if (glm::length(moveDir) > 0.0f)
		{
			moveDir = glm::normalize(moveDir);
			velocity.Velocity = moveDir * speed;
		}
		else
		{
			velocity.Velocity = glm::vec2(0.0f);
		}

		// Calculate new position
		glm::vec2 newPos = transform.Position;
		newPos.x += velocity.Velocity.x * deltaTime;
		newPos.y += velocity.Velocity.y * deltaTime;

		// Check collision with obstacles before applying movement
		float playerRadius = 0.5f;
		
		auto obstacleView = m_Scene->GetRegistry().view<TransformComponent, ObstacleComponent>();
		for (auto entity : obstacleView)
		{
			auto& obstacleTransform = obstacleView.get<TransformComponent>(entity);
			auto& obstacle = obstacleView.get<ObstacleComponent>(entity);
			
			if (!obstacle.BlocksMovement) continue;

			glm::vec2 obstaclePos = obstacleTransform.Position;
			glm::vec2 obstacleSize = obstacleTransform.Scale;
			
			// AABB collision with circle (player)
			float halfWidth = obstacleSize.x * 0.5f;
			float halfHeight = obstacleSize.y * 0.5f;
			
			// Find closest point on rectangle to player center
			float closestX = glm::clamp(newPos.x, obstaclePos.x - halfWidth, obstaclePos.x + halfWidth);
			float closestY = glm::clamp(newPos.y, obstaclePos.y - halfHeight, obstaclePos.y + halfHeight);
			
			float distanceX = newPos.x - closestX;
			float distanceY = newPos.y - closestY;
			float distanceSquared = (distanceX * distanceX) + (distanceY * distanceY);
			
			if (distanceSquared < (playerRadius * playerRadius))
			{
				// Collision detected - push player out
				float distance = sqrtf(distanceSquared);
				if (distance > 0.0001f)
				{
					float overlap = playerRadius - distance;
					newPos.x += (distanceX / distance) * overlap;
					newPos.y += (distanceY / distance) * overlap;
				}
				else
				{
					// Player center is exactly on the edge - use move direction to push out
					if (glm::length(moveDir) > 0.0f)
					{
						newPos = transform.Position; // Revert
					}
				}
			}
		}

		// Apply movement
		transform.Position = newPos;
		transform.Dirty = true;

		// Mouse aiming - convert screen space to world space
		auto [mouseX, mouseY] = Input::GetMousePosition();
		
		// Get actual window dimensions from Application
		Window& window = Application::Get().GetWindow();
		float windowWidth = static_cast<float>(window.GetWidth());
		float windowHeight = static_cast<float>(window.GetHeight());
		
		// Convert screen coordinates to world space accounting for camera
		glm::vec2 screenCenter = { windowWidth * 0.5f, windowHeight * 0.5f };
		glm::vec2 mouseOffset = { mouseX - screenCenter.x, screenCenter.y - mouseY };
		
		// Scale by zoom level and aspect ratio to get world space offset
		float zoomLevel = m_CameraController.GetZoomLevel();
		float aspectRatio = windowWidth / windowHeight;
		glm::vec2 worldMouseOffset = {
			mouseOffset.x * (aspectRatio * zoomLevel) / screenCenter.x,
			mouseOffset.y * zoomLevel / screenCenter.y
		};
		
		// Calculate world mouse position
		glm::vec3 camPos = m_CameraController.GetCamera().GetPosition();
		glm::vec2 worldMousePos = { camPos.x + worldMouseOffset.x, camPos.y + worldMouseOffset.y };
		
		// Calculate angle from player to mouse position
		glm::vec2 playerPos = { transform.Position.x, transform.Position.y };
		glm::vec2 toMouse = worldMousePos - playerPos;
		
		// Only update rotation if mouse is far enough from player
		if (glm::length(toMouse) > 0.01f)
		{
			float angle = atan2f(toMouse.y, toMouse.x);
			transform.Rotation = angle;
		}

		// Dash cooldown
		if (stats.DashCooldownTimer > 0.0f)
		{
			stats.DashCooldownTimer -= deltaTime;
			if (stats.DashCooldownTimer <= 0.0f)
				stats.CanDash = true;
		}

		// Dash ability
		if (Input::IsKeyPressed(PIL_KEY_SPACE) && stats.CanDash)
		{
			if (glm::length(velocity.Velocity) > 0.0f)
			{
				glm::vec2 dashDir = glm::normalize(velocity.Velocity);
				
				// Check dash doesn't go through walls
				glm::vec2 dashTarget = transform.Position;
				dashTarget.x += dashDir.x * 3.0f;
				dashTarget.y += dashDir.y * 3.0f;
				
				// Simple dash collision check - raycast in steps
				bool dashBlocked = false;
				for (float t = 0.1f; t <= 1.0f; t += 0.1f)
				{
					glm::vec2 checkPos = glm::mix(glm::vec2(transform.Position), dashTarget, t);
					for (auto entity : obstacleView)
					{
						auto& obstacleTransform = obstacleView.get<TransformComponent>(entity);
						auto& obstacle = obstacleView.get<ObstacleComponent>(entity);
						if (!obstacle.BlocksMovement) continue;
						
						glm::vec2 obstaclePos = obstacleTransform.Position;
						glm::vec2 obstacleSize = obstacleTransform.Scale;
						float halfWidth = obstacleSize.x * 0.5f + playerRadius;
						float halfHeight = obstacleSize.y * 0.5f + playerRadius;
						
						if (checkPos.x > obstaclePos.x - halfWidth &&
							checkPos.x < obstaclePos.x + halfWidth &&
							checkPos.y > obstaclePos.y - halfHeight &&
							checkPos.y < obstaclePos.y + halfHeight)
						{
							// Clamp dash to just before obstacle
							dashTarget = glm::mix(glm::vec2(transform.Position), dashTarget, glm::max(0.0f, t - 0.1f));
							dashBlocked = true;
							break;
						}
					}
					if (dashBlocked) break;
				}
				
				transform.Position = dashTarget;
				stats.CanDash = false;
				stats.DashCooldownTimer = stats.DashCooldown;
				m_CameraEffects.TriggerShake(0.2f, 0.1f);
				auto playClip = [](const std::shared_ptr<AudioClip>& clip) { if (clip) clip->Play(); };
				playClip(m_DashSFX);
			}
		}
	}

	void ArenaProtocolLayer::UpdateEnemies(float deltaTime)
	{
		if (!m_Player.IsValid()) return;

		// Use the AI system
		m_EnemyAISystem->OnUpdate(m_Scene.get(), m_Player, deltaTime);

		// Handle turret shooting
		auto& playerTransform = m_Player.GetComponent<TransformComponent>();
		glm::vec2 playerPos = { playerTransform.Position.x, playerTransform.Position.y };

		auto turretView = m_Scene->GetRegistry().view<TransformComponent, TurretAIComponent>();
		for (auto entity : turretView)
		{
			Entity turret(entity, m_Scene.get());
			auto& transform = turretView.get<TransformComponent>(entity);
			auto& turretAI = turretView.get<TurretAIComponent>(entity);

			glm::vec2 turretPos = { transform.Position.x, transform.Position.y };
			float distance = glm::length(playerPos - turretPos);

			if (distance < turretAI.Range && turretAI.FireTimer <= 0.0f)
			{
				glm::vec2 direction = glm::normalize(playerPos - turretPos);
				FireBullet(turretPos, direction, turret, false);
				turretAI.FireTimer = 1.0f / turretAI.FireRate;
			}
		}
	}

	void ArenaProtocolLayer::UpdateBoss(float deltaTime)
	{
		if (!m_Boss.IsValid() || !m_Player.IsValid()) return;

		m_BossAISystem->OnUpdate(m_Scene.get(), m_Boss, m_Player, deltaTime);

		// Check boss death
		if (m_Boss.HasComponent<HealthComponent>())
		{
			auto& health = m_Boss.GetComponent<HealthComponent>();
			if (health.IsDead)
			{
				OnBossDefeated();
			}
		}
	}

	void ArenaProtocolLayer::UpdateCamera(float deltaTime)
	{
		if (!m_Player.IsValid()) return;

		auto& playerPos = m_Player.GetComponent<TransformComponent>().Position;
		auto& camera = m_CameraController.GetCamera();

		// Smooth camera follow
		glm::vec3 currentPos = camera.GetPosition();
		glm::vec3 targetPos = { playerPos.x, playerPos.y, 0.0f };
		glm::vec3 newPos = currentPos + (targetPos - currentPos) * 5.0f * deltaTime;
		
		m_BaseCameraPosition = newPos;
		camera.SetPosition(newPos + glm::vec3(m_CameraEffects.ShakeOffset, 0.0f));
	}

	void ArenaProtocolLayer::UpdateCameraEffects(float deltaTime)
	{
		// Screen shake
		if (m_CameraEffects.ShakeActive)
		{
			m_CameraEffects.ShakeTimer -= deltaTime;
			if (m_CameraEffects.ShakeTimer <= 0.0f)
			{
				m_CameraEffects.ShakeActive = false;
				m_CameraEffects.ShakeOffset = glm::vec2(0.0f);
			}
			else
			{
				float intensity = m_CameraEffects.ShakeMaxIntensity * 
					(m_CameraEffects.ShakeTimer / m_CameraEffects.ShakeDuration);
				m_CameraEffects.ShakeOffset = {
					((rand() % 100) / 100.0f - 0.5f) * 2.0f * intensity,
					((rand() % 100) / 100.0f - 0.5f) * 2.0f * intensity
				};
			}
		}

		// Zoom effect
		if (m_CameraEffects.CurrentZoom != m_CameraEffects.TargetZoom)
		{
			float diff = m_CameraEffects.TargetZoom - m_CameraEffects.CurrentZoom;
			m_CameraEffects.CurrentZoom += diff * m_CameraEffects.ZoomSpeed * deltaTime;
			m_CameraController.SetZoomLevel(15.0f * m_CameraEffects.CurrentZoom);
		}
	}

	void ArenaProtocolLayer::HandleInput(float deltaTime)
	{
		// Only handle mouse wheel zoom, not WASD (player handles WASD)
		// m_CameraController.OnUpdate(deltaTime); // Disabled - we handle camera follow manually
	}

	void ArenaProtocolLayer::HandleShooting(float deltaTime)
	{
		m_FireTimer -= deltaTime;
		m_SecondaryFireTimer -= deltaTime;

		if (!m_Player.IsValid()) return;

		auto& weapon = m_Player.GetComponent<WeaponComponent>();
		auto& transform = m_Player.GetComponent<TransformComponent>();
		glm::vec2 fireDirection = { cosf(transform.Rotation), sinf(transform.Rotation) };

		// Primary fire (left click)
		if (Input::IsMouseButtonPressed(PIL_MOUSE_BUTTON_LEFT) && m_FireTimer <= 0.0f)
		{
			glm::vec2 muzzlePos = { 
				transform.Position.x + fireDirection.x * weapon.MuzzleOffset.x,
				transform.Position.y + fireDirection.y * weapon.MuzzleOffset.x
			};

			// Handle different weapon types
			switch (weapon.Type)
			{
			case WeaponType::Primary:
				FireBullet(muzzlePos, fireDirection, m_Player, true);
				m_BulletsFired++;
				break;

			case WeaponType::Spread:
				for (int i = 0; i < weapon.SpreadCount; i++)
				{
					float angleOffset = (i - weapon.SpreadCount / 2) * glm::radians(weapon.SpreadAngle);
					float newAngle = transform.Rotation + angleOffset;
					glm::vec2 dir = { cosf(newAngle), sinf(newAngle) };
					FireBullet(muzzlePos, dir, m_Player, true);
				}
				m_BulletsFired += weapon.SpreadCount;
				break;

			case WeaponType::Rapid:
				FireBullet(muzzlePos, fireDirection, m_Player, true);
				m_BulletsFired++;
				break;

			case WeaponType::Rocket:
				{
					Entity bullet = FireBullet(muzzlePos, fireDirection, m_Player, true);
					if (bullet.IsValid())
					{
						auto& bc = bullet.GetComponent<BulletComponent>();
						bc.Damage = weapon.Damage;
					}
					m_BulletsFired++;
				}
				break;
			}

			m_FireTimer = 1.0f / weapon.FireRate;
		}

		// Secondary fire (right click) - always spread shot, independent cooldown
		if (Input::IsMouseButtonPressed(PIL_MOUSE_BUTTON_RIGHT) && m_SecondaryFireTimer <= 0.0f)
		{
			glm::vec2 muzzlePos = { 
				transform.Position.x + fireDirection.x * 0.5f,
				transform.Position.y + fireDirection.y * 0.5f
			};

			// Fire spread shot (5 bullets in a cone)
			for (int i = -2; i <= 2; i++)
			{
				float angleOffset = i * glm::radians(10.0f);
				float newAngle = transform.Rotation + angleOffset;
				glm::vec2 dir = { cosf(newAngle), sinf(newAngle) };
				FireBullet(muzzlePos, dir, m_Player, true);
			}

			m_SecondaryFireTimer = 0.4f; // Slightly longer cooldown for spread
			m_BulletsFired += 5;
		}
	}

	Entity ArenaProtocolLayer::FireBullet(const glm::vec2& position, const glm::vec2& direction, Entity owner, bool isPlayerBullet)
	{
		Entity bullet = m_BulletPool->Acquire();
		
		if (!bullet.IsValid())
		{
			PIL_WARN("Bullet pool exhausted!");
			return Entity();
		}

		// Ensure bullet has required components
		if (!bullet.HasComponent<TransformComponent>())
		{
			PIL_ERROR("Bullet missing TransformComponent!");
			m_BulletPool->Release(bullet);
			return Entity();
		}

		auto& transform = bullet.GetComponent<TransformComponent>();
		transform.Position = position;
		transform.Rotation = atan2f(direction.y, direction.x);
		transform.Dirty = true;

		if (!bullet.HasComponent<VelocityComponent>())
			bullet.AddComponent<VelocityComponent>();
		auto& velocity = bullet.GetComponent<VelocityComponent>();
		velocity.Velocity = direction * 25.0f;

		if (!bullet.HasComponent<BulletComponent>())
			bullet.AddComponent<BulletComponent>();
		auto& bulletComp = bullet.GetComponent<BulletComponent>();
		bulletComp.Owner = owner;
		bulletComp.TimeAlive = 0.0f;
		bulletComp.Damage = isPlayerBullet ? 10.0f : 15.0f;
		bulletComp.Lifetime = 3.0f;
		bulletComp.HitsRemaining = 1;

		if (!bullet.HasComponent<SpriteComponent>())
			bullet.AddComponent<SpriteComponent>();
		auto& sprite = bullet.GetComponent<SpriteComponent>();
		sprite.Texture = isPlayerBullet ? m_EntityFactory->GetBulletTexture() : m_EntityFactory->GetEnemyBulletTexture();
		sprite.Color = isPlayerBullet ? glm::vec4(0.0f, 1.0f, 1.0f, 1.0f) : glm::vec4(1.0f, 0.3f, 0.0f, 1.0f);
		sprite.Size = { 0.2f, 0.4f };

		// Play fire SFX
		auto playClip = [](const std::shared_ptr<AudioClip>& clip) { if (clip) clip->Play(); };
		if (owner == m_Player)
			playClip(m_ShootSFX);
		else
			playClip(m_EnemyShootSFX);

		return bullet;
	}

	void ArenaProtocolLayer::SpawnEnemy(const std::string& type, const glm::vec2& position)
	{
		Entity enemy;

		if (type == "Drone")
		{
			enemy = m_EntityFactory->CreateDrone(position, m_Player);
		}
		else if (type == "Turret")
		{
			enemy = m_EntityFactory->CreateTurret(position);
		}
		else if (type == "Charger")
		{
			enemy = m_EntityFactory->CreateCharger(position);
		}

		if (enemy.IsValid())
		{
			m_Enemies.push_back(enemy);
		}
	}

	void ArenaProtocolLayer::CreateExplosion(const glm::vec2& position, float scale)
	{
		m_EntityFactory->CreateExplosion(position, scale);
		m_CameraEffects.TriggerShake(0.3f * scale, 0.15f);
		auto playClip = [](const std::shared_ptr<AudioClip>& clip) { if (clip) clip->Play(); };
		playClip(m_ExplosionSFX);
	}

	void ArenaProtocolLayer::ProcessBulletCollisions()
	{
		auto bulletView = m_Scene->GetRegistry().view<BulletComponent, TransformComponent>();

		for (auto entity : bulletView)
		{
			Entity bullet(entity, m_Scene.get());

			if (m_BulletPool->IsInPool(bullet))
				continue;

			bool hit = false;

			// Check enemy collisions (player bullets)
			hit = m_BulletSystem->CheckBulletEnemyCollision(bullet, m_Scene.get(), m_Player, m_Enemies);

			// Check player collision (enemy bullets)
			if (!hit)
			{
				hit = m_BulletSystem->CheckBulletPlayerCollision(bullet, m_Player);
				if (hit)
				{
					m_DamageTaken += (int)bullet.GetComponent<BulletComponent>().Damage;
					m_CameraEffects.TriggerShake(0.2f, 0.1f);
				}
			}

			// Check obstacle collisions
			if (!hit)
			{
				hit = m_BulletSystem->CheckBulletObstacleCollision(bullet, m_Scene.get());
			}

			// Release bullet if it hit something
			if (hit)
			{
				auto playClip = [](const std::shared_ptr<AudioClip>& clip) { if (clip) clip->Play(); };
				playClip(m_HitSFX);
				bullet.GetComponent<SpriteComponent>().Color.a = 0.0f;
				m_BulletPool->Release(bullet);
			}
		}
	}

	void ArenaProtocolLayer::ProcessPlayerCollisions(float deltaTime)
	{
		if (!m_Player.IsValid()) return;

		// Update contact damage cooldown
		if (m_ContactDamageCooldown > 0.0f)
			m_ContactDamageCooldown -= deltaTime;

		auto& playerTransform = m_Player.GetComponent<TransformComponent>();
		auto& playerStats = m_Player.GetComponent<PlayerStatsComponent>();
		glm::vec2 playerPos = { playerTransform.Position.x, playerTransform.Position.y };

		// Check collision with chargers (contact damage)
		auto chargerView = m_Scene->GetRegistry().view<TransformComponent, ChargerAIComponent>();
		for (auto entity : chargerView)
		{
			auto& chargerAI = chargerView.get<ChargerAIComponent>(entity);
			if (!chargerAI.IsCharging) continue;

			auto& chargerTransform = chargerView.get<TransformComponent>(entity);
			glm::vec2 chargerPos = { chargerTransform.Position.x, chargerTransform.Position.y };
			float distance = glm::length(playerPos - chargerPos);

			if (distance < 1.0f) // Contact radius
			{
				playerStats.Health -= 20.0f;
				m_DamageTaken += 20;
				m_CameraEffects.TriggerShake(0.5f, 0.2f);

				// Knockback
				glm::vec2 knockback = glm::normalize(playerPos - chargerPos) * 2.0f;
				playerTransform.Position.x += knockback.x;
				playerTransform.Position.y += knockback.y;

				// Stop charger
				chargerAI.IsCharging = false;
				chargerAI.WindupTimer = 0.0f;
			}
		}

		// Check collision with drones (contact damage with cooldown)
		if (m_ContactDamageCooldown <= 0.0f)
		{
			for (auto& enemy : m_Enemies)
			{
				if (!enemy.IsValid()) continue;
				if (!enemy.HasComponent<EnemyAIComponent>()) continue;

				auto& enemyTransform = enemy.GetComponent<TransformComponent>();
				glm::vec2 enemyPos = { enemyTransform.Position.x, enemyTransform.Position.y };
				float distance = glm::length(playerPos - enemyPos);

				if (distance < 0.8f)
				{
					playerStats.Health -= 10.0f; // Fixed contact damage
					m_DamageTaken += 10;
					m_ContactDamageCooldown = 0.5f; // Half second invulnerability
					m_CameraEffects.TriggerShake(0.15f, 0.08f);
					break; // Only take damage from one enemy per cooldown
				}
			}
		}
	}

	void ArenaProtocolLayer::CleanupDeadEntities()
	{
		auto it = m_Enemies.begin();
		while (it != m_Enemies.end())
		{
			if (!it->IsValid())
			{
				it = m_Enemies.erase(it);
				continue;
			}

			if (it->HasComponent<HealthComponent>())
			{
				auto& health = it->GetComponent<HealthComponent>();
				if (health.IsDead)
				{
					auto& transform = it->GetComponent<TransformComponent>();
					
					// Spawn XP and explosion
					int xpValue = 1;
					auto& tag = it->GetComponent<TagComponent>();
					if (tag.Tag.find("Turret") != std::string::npos)
						xpValue = 5;
					else if (tag.Tag.find("Charger") != std::string::npos)
						xpValue = 3;

					m_EntityFactory->CreateXPGem({ transform.Position.x, transform.Position.y }, xpValue);
					CreateExplosion({ transform.Position.x, transform.Position.y }, 0.5f);
					
					m_Scene->DestroyEntity(*it);
					it = m_Enemies.erase(it);
					m_EnemiesKilled++;
					continue;
				}
			}
			++it;
		}
	}

	void ArenaProtocolLayer::OnPlayerDeath()
	{
		PIL_WARN("=== GAME OVER ===");
		m_GameState = GameState::GameOver;
		
		if (m_Player.IsValid())
		{
			auto& transform = m_Player.GetComponent<TransformComponent>();
			CreateExplosion({ transform.Position.x, transform.Position.y }, 2.0f);
		}
	}

	void ArenaProtocolLayer::OnBossDefeated()
	{
		PIL_INFO("=== BOSS DEFEATED! ===");
		
		if (m_Boss.IsValid())
		{
			auto& transform = m_Boss.GetComponent<TransformComponent>();
			CreateExplosion({ transform.Position.x, transform.Position.y }, 3.0f);
			
			// Spawn lots of XP
			for (int i = 0; i < 20; i++)
			{
				float angle = (i / 20.0f) * 2.0f * glm::pi<float>();
				float dist = 2.0f + (rand() % 30) / 10.0f;
				glm::vec2 pos = { transform.Position.x + cosf(angle) * dist,
								 transform.Position.y + sinf(angle) * dist };
				m_EntityFactory->CreateXPGem(pos, 5);
			}
			
			m_Scene->DestroyEntity(m_Boss);
			m_Boss = Entity();
		}
		
		m_BossMode = false;
		m_GameState = GameState::Victory;
	}

	void ArenaProtocolLayer::OnWaveComplete(int waveNumber)
	{
		PIL_INFO("Wave {} complete! Enemies killed: {}", waveNumber, m_EnemiesKilled);
		
		// Heal player slightly between waves
		if (m_Player.IsValid() && m_Player.HasComponent<PlayerStatsComponent>())
		{
			auto& stats = m_Player.GetComponent<PlayerStatsComponent>();
			stats.Health = std::min(stats.Health + 20.0f, stats.MaxHealth);
		}

		// After wave 5, spawn boss
		if (waveNumber >= 5 && !m_BossMode)
		{
			PIL_INFO("=== BOSS INCOMING! ===");
			m_Boss = m_EntityFactory->CreateBoss({ 0.0f, 12.0f });
			m_Enemies.push_back(m_Boss);
			m_BossMode = true;
			m_CameraEffects.TriggerShake(0.5f, 1.0f);
		}
	}

	void ArenaProtocolLayer::CheckVictoryCondition()
	{
		// Victory when boss is defeated (handled in OnBossDefeated)
	}

	void ArenaProtocolLayer::RestartGame()
	{
		PIL_INFO("Restarting game...");
		
		// Clear all enemies
		for (auto& enemy : m_Enemies)
		{
			if (enemy.IsValid())
				m_Scene->DestroyEntity(enemy);
		}
		m_Enemies.clear();

		// Reset player
		if (m_Player.IsValid())
		{
			auto& transform = m_Player.GetComponent<TransformComponent>();
			transform.Position = { 0.0f, -5.0f };
			
			auto& stats = m_Player.GetComponent<PlayerStatsComponent>();
			stats.Health = stats.MaxHealth;
			stats.XP = 0;
			stats.Level = 1;
		}
		else
		{
			InitializePlayer();
		}

		// Reset stats
		m_BulletsFired = 0;
		m_EnemiesKilled = 0;
		m_DamageTaken = 0;
		m_GameTime = 0.0f;
		m_BossMode = false;
		m_Boss = Entity();
		m_FireTimer = 0.0f;
		m_SecondaryFireTimer = 0.0f;
		m_ContactDamageCooldown = 0.0f;

		// Reload chamber and restart waves
		m_ChamberManager->LoadChamber(ChamberType::EnemyGauntlet);
		m_WaveSystem->StartWave(1);

		m_GameState = GameState::Playing;
	}

	void ArenaProtocolLayer::OnImGuiRender()
	{
		RenderGameUI();

		if (m_ShowDebugPanel)
			RenderDebugPanel();

		if (m_ShowSystemsPanel)
			RenderSystemsPanel();

		if (m_ShowChamberSelect)
			RenderChamberSelect();

		if (m_GameState == GameState::GameOver)
			RenderGameOverScreen();

		if (m_GameState == GameState::Victory)
			RenderVictoryScreen();
	}

	void ArenaProtocolLayer::RenderGameUI()
	{
		// Top-left: Health and XP
		ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_Always);
		ImGui::Begin("Status", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground);

		if (m_Player.IsValid() && m_Player.HasComponent<PlayerStatsComponent>())
		{
			auto& stats = m_Player.GetComponent<PlayerStatsComponent>();
			
			ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "HP: %.0f / %.0f", stats.Health, stats.MaxHealth);
			ImGui::ProgressBar(stats.Health / stats.MaxHealth, ImVec2(180, 15), "");
			
			ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "XP: %d / %d", stats.XP, stats.XPToNextLevel);
			ImGui::ProgressBar((float)stats.XP / stats.XPToNextLevel, ImVec2(180, 10), "");
			
			ImGui::Text("Level %d", stats.Level);
		}

		ImGui::End();

		// Top-right: Wave info
		ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 160, 10), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(150, 60), ImGuiCond_Always);
		ImGui::Begin("Wave", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground);

		if (m_BossMode)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "BOSS FIGHT!");
		}
		else
		{
			ImGui::Text("Wave %d", m_WaveSystem->GetCurrentWave());
		}
		ImGui::Text("Enemies: %zu", m_Enemies.size());

		ImGui::End();
	}

	void ArenaProtocolLayer::RenderDebugPanel()
	{
		ImGui::Begin("Arena Protocol Debug", &m_ShowDebugPanel);

		ImGui::Text("Game Time: %.1f s", m_GameTime);
		ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
		ImGui::Separator();

		// Stats
		ImGui::Text("Bullets Fired: %d", m_BulletsFired);
		ImGui::Text("Enemies Killed: %d", m_EnemiesKilled);
		ImGui::Text("Damage Taken: %d", m_DamageTaken);
		ImGui::Separator();

		// Object pool stats
		ImGui::Text("Bullet Pool:");
		ImGui::Text("  Active: %zu", m_BulletPool->GetActiveCount());
		ImGui::Text("  Available: %zu", m_BulletPool->GetAvailableCount());
		ImGui::Separator();

		// Entity counts
		ImGui::Text("Entities: %zu", m_Scene->GetEntityCount());
		ImGui::Text("Enemies: %zu", m_Enemies.size());
		ImGui::Separator();

		// Controls
		ImGui::Text("Controls:");
		ImGui::BulletText("WASD - Move");
		ImGui::BulletText("Mouse - Aim");
		ImGui::BulletText("Left Click - Shoot");
		ImGui::BulletText("Right Click - Spread Shot");
		ImGui::BulletText("Space - Dash");
		ImGui::BulletText("F1 - Toggle Debug");
		ImGui::BulletText("F2 - Systems Panel");
		ImGui::BulletText("F3 - Chamber Select");
		ImGui::BulletText("F5 - Spawn Drone");
		ImGui::BulletText("R - Restart (when dead)");

		ImGui::End();
	}

	void ArenaProtocolLayer::RenderSystemsPanel()
	{
		ImGui::Begin("Systems", &m_ShowSystemsPanel);

		// Camera info
		ImGui::Text("Camera:");
		auto pos = m_CameraController.GetCamera().GetPosition();
		ImGui::Text("  Position: (%.1f, %.1f)", pos.x, pos.y);
		ImGui::Text("  Zoom: %.1f", m_CameraController.GetZoomLevel());
		ImGui::Text("  Shake: %s", m_CameraEffects.ShakeActive ? "Active" : "None");
		ImGui::Separator();

		// Wave system
		ImGui::Text("Wave System:");
		ImGui::Text("  Current Wave: %d", m_WaveSystem->GetCurrentWave());
		ImGui::Text("  In Progress: %s", m_WaveSystem->IsWaveInProgress() ? "Yes" : "No");
		ImGui::Separator();

		// Weapon info
		if (m_Player.IsValid() && m_Player.HasComponent<WeaponComponent>())
		{
			auto& weapon = m_Player.GetComponent<WeaponComponent>();
			ImGui::Text("Weapon:");
			const char* weaponNames[] = { "Primary", "Spread", "Rapid", "Rocket" };
			ImGui::Text("  Type: %s", weaponNames[(int)weapon.Type]);
			ImGui::Text("  Fire Rate: %.1f/s", weapon.FireRate);
			ImGui::Text("  Damage: %.0f", weapon.Damage);
			
			// Weapon selection buttons
			if (ImGui::Button("Primary")) weapon.Type = WeaponType::Primary;
			ImGui::SameLine();
			if (ImGui::Button("Spread")) weapon.Type = WeaponType::Spread;
			ImGui::SameLine();
			if (ImGui::Button("Rapid")) weapon.Type = WeaponType::Rapid;
			ImGui::SameLine();
			if (ImGui::Button("Rocket")) weapon.Type = WeaponType::Rocket;
		}

		ImGui::End();
	}

	void ArenaProtocolLayer::RenderChamberSelect()
	{
		ImGui::Begin("Chamber Select", &m_ShowChamberSelect);

		const char* chambers[] = {
			"Hub", "Movement & Physics", "Shooting Range", "Enemy Gauntlet",
			"Particle Test", "Animation Test", "Audio Test", "Boss Arena", "Stress Test"
		};

		ImGui::Combo("Chamber", &m_SelectedChamber, chambers, 9);

		if (ImGui::Button("Load Chamber"))
		{
			m_Enemies.clear();
			m_BossMode = false;
			m_Boss = Entity();
			m_ChamberManager->LoadChamber((ChamberType)m_SelectedChamber);
			
			if ((ChamberType)m_SelectedChamber == ChamberType::EnemyGauntlet)
			{
				m_WaveSystem->StartWave(1);
			}
			else if ((ChamberType)m_SelectedChamber == ChamberType::BossArena)
			{
				m_Boss = m_EntityFactory->CreateBoss({ 0.0f, 10.0f });
				m_Enemies.push_back(m_Boss);
				m_BossMode = true;
			}
			
			// Copy enemies from chamber manager
			for (const auto& enemy : m_ChamberManager->GetEnemies())
			{
				m_Enemies.push_back(enemy);
			}
		}

		ImGui::End();
	}

	void ArenaProtocolLayer::RenderGameOverScreen()
	{
		ImVec2 center = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
		ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::Begin("Game Over", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);

		ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "GAME OVER");
		ImGui::Separator();
		ImGui::Text("Time Survived: %.1f s", m_GameTime);
		ImGui::Text("Enemies Killed: %d", m_EnemiesKilled);
		ImGui::Text("Waves Completed: %d", m_WaveSystem->GetCurrentWave() - 1);
		ImGui::Separator();

		if (ImGui::Button("Restart (R)") || Input::IsKeyPressed(PIL_KEY_R))
		{
			RestartGame();
		}

		ImGui::End();
	}

	void ArenaProtocolLayer::RenderVictoryScreen()
	{
		ImVec2 center = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
		ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::Begin("Victory", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);

		ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "VICTORY!");
		ImGui::Separator();
		ImGui::Text("Time: %.1f s", m_GameTime);
		ImGui::Text("Enemies Killed: %d", m_EnemiesKilled);
		ImGui::Text("Damage Taken: %d", m_DamageTaken);
		
		if (m_Player.IsValid() && m_Player.HasComponent<PlayerStatsComponent>())
		{
			auto& stats = m_Player.GetComponent<PlayerStatsComponent>();
			ImGui::Text("Final Level: %d", stats.Level);
		}
		ImGui::Separator();

		if (ImGui::Button("Play Again (R)") || Input::IsKeyPressed(PIL_KEY_R))
		{
			RestartGame();
		}

		ImGui::End();
	}

	void ArenaProtocolLayer::OnEvent(Event& e)
	{
		m_CameraController.OnEvent(e);

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& e) { return OnKeyPressed(e); });
		dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& e) { return OnMouseButtonPressed(e); });
	}

	bool ArenaProtocolLayer::OnKeyPressed(KeyPressedEvent& e)
	{
		switch (e.GetKeyCode())
		{
		case PIL_KEY_F1:
			m_ShowDebugPanel = !m_ShowDebugPanel;
			return true;
		case PIL_KEY_F2:
			m_ShowSystemsPanel = !m_ShowSystemsPanel;
			return true;
		case PIL_KEY_F3:
			m_ShowChamberSelect = !m_ShowChamberSelect;
			return true;
		case PIL_KEY_F5:
			{
				float angle = (rand() % 360) * glm::pi<float>() / 180.0f;
				SpawnEnemy("Drone", { cosf(angle) * 10.0f, sinf(angle) * 10.0f });
			}
			return true;
		case PIL_KEY_F6:
			{
				float angle = (rand() % 360) * glm::pi<float>() / 180.0f;
				SpawnEnemy("Charger", { cosf(angle) * 10.0f, sinf(angle) * 10.0f });
			}
			return true;
		case PIL_KEY_ESCAPE:
			if (m_GameState == GameState::Playing)
				m_GameState = GameState::Paused;
			else if (m_GameState == GameState::Paused)
				m_GameState = GameState::Playing;
			return true;
		case PIL_KEY_R:
			if (m_GameState == GameState::GameOver || m_GameState == GameState::Victory)
			{
				RestartGame();
			}
			return true;
		}
		return false;
	}

	bool ArenaProtocolLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		return false;
	}

} // namespace Pillar
