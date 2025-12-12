#include "EntityFactory.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Core/TagComponent.h"
#include "Pillar/ECS/Components/Rendering/SpriteComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"
#include "Pillar/ECS/Components/Gameplay/BulletComponent.h"
#include "Pillar/ECS/Components/Gameplay/XPGemComponent.h"
#include "Pillar/ECS/Components/Gameplay/ParticleEmitterComponent.h"
#include "../Components/PlayerStatsComponent.h"
#include "../Components/HealthComponent.h"
#include "../Components/EnemyAIComponent.h"
#include "../Components/TurretAIComponent.h"
#include "../Components/ChargerAIComponent.h"
#include "../Components/BossAIComponent.h"
#include "../Components/WeaponComponent.h"
#include "../Components/ObstacleComponent.h"
#include "../Components/TriggerComponent.h"
#include "../Components/XPGemVisualComponent.h"
#include "Pillar/Logger.h"
#include "Pillar/Utils/AssetManager.h"

namespace Pillar {

	void EntityFactory::Initialize(Scene* scene, const std::string& assetsPath)
	{
		m_Scene = scene;

		// Resolve assets directory using AssetManager so paths work from both build output and workspace
		if (!assetsPath.empty())
		{
			m_AssetsPath = AssetManager::GetAssetPath(assetsPath);
		}
		else
		{
			m_AssetsPath = AssetManager::GetAssetsDirectory();
		}

		LoadTextures(m_AssetsPath);
	}

	void EntityFactory::LoadTextures(const std::string& basePath)
	{
		PIL_INFO("Loading game textures from: {}", basePath);

		// Player textures
		m_Textures["player"] = LoadOrCreateTexture("player", "player_ship.png", 0xFF00FFFF); // Cyan
		m_Textures["player_thrust"] = LoadOrCreateTexture("player_thrust", "player_ship_thrust.png", 0xFF00FFFF);

		// Bullet textures
		m_Textures["bullet"] = LoadOrCreateTexture("bullet", "bullet.png", 0xFFFFFF00); // Yellow
		m_Textures["enemy_bullet"] = LoadOrCreateTexture("enemy_bullet", "enemy_bullet.png", 0xFF0080FF); // Orange

	// Enemy textures
	m_Textures["drone"] = LoadOrCreateTexture("drone", "drone.png", 0xFF0000FF); // Red
	m_Textures["turret"] = LoadOrCreateTexture("turret", "turret.png", 0xFF808080); // Gray
	m_Textures["turret_barrel"] = LoadOrCreateTexture("turret_barrel", "turret_barrel.png", 0xFF606060);
	m_Textures["charger"] = LoadOrCreateTexture("charger", "charger.png", 0xFF00A5FF); // Orange

	// Boss textures
	m_Textures["boss"] = LoadOrCreateTexture("boss", "boss.png", 0xFFFF00FF); // Purple		// Collectibles
		m_Textures["xp_gem_small"] = LoadOrCreateTexture("xp_gem_small", "xp_gem_small.png", 0xFF00FF00); // Green
		m_Textures["xp_gem_medium"] = LoadOrCreateTexture("xp_gem_medium", "xp_gem_medium.png", 0xFFFF0000); // Blue
		m_Textures["xp_gem_large"] = LoadOrCreateTexture("xp_gem_large", "xp_gem_large.png", 0xFFFF00FF); // Purple

		// Environment
		m_Textures["crate"] = LoadOrCreateTexture("crate", "crate.png", 0xFF456789); // Brown
		m_Textures["pillar"] = LoadOrCreateTexture("pillar", "pillar.png", 0xFF888888); // Gray
		m_Textures["wall"] = LoadOrCreateTexture("wall", "wall.png", 0xFF555555);

		// Particles
		m_Textures["particle_circle"] = LoadOrCreateTexture("particle_circle", "particle_circle.png", 0xFFFFFFFF);
		m_Textures["particle_spark"] = LoadOrCreateTexture("particle_spark", "particle_spark.png", 0xFFFFFFFF);

		PIL_INFO("Loaded {} textures", m_Textures.size());
	}

	std::shared_ptr<Texture2D> EntityFactory::LoadOrCreateTexture(const std::string& name, const std::string& filename, uint32_t fallbackColor)
	{
		// Resolve texture path using AssetManager (searches workspace Sandbox/assets/textures and exe assets)
		std::string resolvedPath = AssetManager::GetTexturePath(filename);
		try
		{
			auto texture = Texture2D::Create(resolvedPath);
			if (texture && texture->GetWidth() > 0)
			{
				PIL_TRACE("Loaded texture '{}' from: {}", name, resolvedPath);
				return texture;
			}
		}
		catch (...)
		{
			PIL_WARN("Texture2D::Create threw while loading {} (resolved path: {})", filename, resolvedPath);
		}

		// Create fallback colored texture
		PIL_WARN("Could not load texture '{}', using fallback color", filename);
		auto texture = Texture2D::Create(1, 1);
		texture->SetData(&fallbackColor, sizeof(uint32_t));
		return texture;
	}

	std::shared_ptr<Texture2D> EntityFactory::GetTexture(const std::string& name) const
	{
		auto it = m_Textures.find(name);
		if (it != m_Textures.end())
		{
			return it->second;
		}
		return nullptr;
	}

	Entity EntityFactory::CreatePlayer(const glm::vec2& position)
	{
		Entity player = m_Scene->CreateEntity("Player");

		auto& transform = player.GetComponent<TransformComponent>();
		transform.Position = position;
		transform.Rotation = 0.0f;
		transform.Scale = { 1.0f, 1.0f };

		auto& sprite = player.AddComponent<SpriteComponent>();
		sprite.Texture = GetTexture("player");
		sprite.Color = { 0.0f, 1.0f, 1.0f, 1.0f }; // Cyan tint
		sprite.Size = { 1.0f, 1.0f };

		player.AddComponent<VelocityComponent>();

		auto& stats = player.AddComponent<PlayerStatsComponent>();
		stats.Health = 100.0f;
		stats.MaxHealth = 100.0f;
		stats.XP = 0;
		stats.Level = 1;

		auto& weapon = player.AddComponent<WeaponComponent>(WeaponType::Primary);

		PIL_INFO("Created player at ({:.1f}, {:.1f})", position.x, position.y);
		return player;
	}

	Entity EntityFactory::CreateBullet(const glm::vec2& position, const glm::vec2& direction, Entity owner, bool isPlayerBullet)
	{
		Entity bullet = m_Scene->CreateEntity("Bullet");

		auto& transform = bullet.GetComponent<TransformComponent>();
		transform.Position = position;
		transform.Rotation = atan2f(direction.y, direction.x);
		transform.Scale = { 1.0f, 1.0f };

		auto& sprite = bullet.AddComponent<SpriteComponent>();
		sprite.Texture = isPlayerBullet ? GetTexture("bullet") : GetTexture("enemy_bullet");
		sprite.Color = isPlayerBullet ? glm::vec4(0.0f, 1.0f, 1.0f, 1.0f) : glm::vec4(1.0f, 0.3f, 0.0f, 1.0f);
		sprite.Size = { 0.2f, 0.4f };

		auto& velocity = bullet.AddComponent<VelocityComponent>();
		velocity.Velocity = direction * 25.0f;

		auto& bulletComp = bullet.AddComponent<BulletComponent>();
		bulletComp.Owner = owner;
		bulletComp.Damage = 10.0f;
		bulletComp.Lifetime = 3.0f;
		bulletComp.TimeAlive = 0.0f;

		return bullet;
	}

	Entity EntityFactory::CreateDrone(const glm::vec2& position, Entity target)
	{
		Entity drone = m_Scene->CreateEntity("EnemyDrone");

		auto& transform = drone.GetComponent<TransformComponent>();
		transform.Position = position;
		transform.Rotation = 0.0f;
		transform.Scale = { 1.0f, 1.0f };

		auto& sprite = drone.AddComponent<SpriteComponent>();
		sprite.Texture = GetTexture("drone");
		sprite.Color = { 1.0f, 0.3f, 0.3f, 1.0f }; // Red tint
		sprite.Size = { 0.8f, 0.8f };

		auto& health = drone.AddComponent<HealthComponent>(30.0f);

		auto& ai = drone.AddComponent<EnemyAIComponent>();
		ai.Behavior = AIBehavior::SeekPlayer;
		ai.Speed = 3.0f;
		ai.DetectionRange = 15.0f;
		ai.Target = target;

		PIL_TRACE("Created drone at ({:.1f}, {:.1f})", position.x, position.y);
		return drone;
	}

	Entity EntityFactory::CreateTurret(const glm::vec2& position)
	{
		Entity turret = m_Scene->CreateEntity("EnemyTurret");

		auto& transform = turret.GetComponent<TransformComponent>();
		transform.Position = position;
		transform.Rotation = 0.0f;
		transform.Scale = { 1.0f, 1.0f };

		auto& sprite = turret.AddComponent<SpriteComponent>();
		auto turretTexture = GetTexture("turret");
		if (!turretTexture) {
			PIL_ERROR("Failed to get turret texture!");
		}
		sprite.Texture = turretTexture;
		sprite.Color = { 0.5f, 0.5f, 0.5f, 1.0f }; // Gray
		sprite.Size = { 1.2f, 1.2f };
		sprite.ZIndex = 0.0f; // Base layer

		auto& health = turret.AddComponent<HealthComponent>(80.0f);

		auto& ai = turret.AddComponent<TurretAIComponent>();
		ai.FireRate = 1.0f;
		ai.Range = 12.0f;
		ai.RotationSpeed = 120.0f;

		// Create turret barrel as a child entity
		Entity barrel = m_Scene->CreateEntity("TurretBarrel");
		auto& barrelTransform = barrel.GetComponent<TransformComponent>();
		barrelTransform.Position = position; // Same position as base
		barrelTransform.Rotation = 0.0f;
		barrelTransform.Scale = { 1.0f, 1.0f };

		auto& barrelSprite = barrel.AddComponent<SpriteComponent>();
		auto barrelTexture = GetTexture("turret_barrel");
		if (!barrelTexture) {
			PIL_WARN("Turret barrel texture not found, using base texture");
			barrelTexture = turretTexture; // Fallback
		}
		barrelSprite.Texture = barrelTexture;
		barrelSprite.Color = { 0.6f, 0.6f, 0.6f, 1.0f };
		barrelSprite.Size = { 0.8f, 0.8f };
		barrelSprite.ZIndex = 0.1f; // Render above base

		// Link barrel to turret for synchronized rotation
		// Store barrel reference in TurretAIComponent
		ai.BarrelEntity = barrel;

		PIL_TRACE("Created turret at ({:.1f}, {:.1f})", position.x, position.y);
		return turret;
	}

	Entity EntityFactory::CreateCharger(const glm::vec2& position)
	{
		Entity charger = m_Scene->CreateEntity("EnemyCharger");

		auto& transform = charger.GetComponent<TransformComponent>();
		transform.Position = position;
		transform.Rotation = 0.0f;
		transform.Scale = { 1.0f, 1.0f };

		auto& sprite = charger.AddComponent<SpriteComponent>();
		sprite.Texture = GetTexture("charger");
		sprite.Color = { 1.0f, 0.65f, 0.0f, 1.0f }; // Orange
		sprite.Size = { 0.8f, 1.0f };

		auto& health = charger.AddComponent<HealthComponent>(50.0f);

		auto& ai = charger.AddComponent<ChargerAIComponent>();
		ai.ChargeSpeed = 15.0f;
		ai.WindupTime = 0.5f;
		ai.ChargeDuration = 1.0f;

		charger.AddComponent<VelocityComponent>();

		PIL_TRACE("Created charger at ({:.1f}, {:.1f})", position.x, position.y);
		return charger;
	}

	Entity EntityFactory::CreateBoss(const glm::vec2& position)
	{
		Entity boss = m_Scene->CreateEntity("BossSentinel");

		auto& transform = boss.GetComponent<TransformComponent>();
		transform.Position = position;
		transform.Rotation = 0.0f;
		transform.Scale = { 3.0f, 3.0f };

	auto& sprite = boss.AddComponent<SpriteComponent>();
	auto bossTexture = GetTexture("boss");
	if (!bossTexture) {
		PIL_ERROR("Failed to get boss texture!");
	}
	sprite.Texture = bossTexture;
	sprite.Color = { 0.8f, 0.2f, 0.8f, 1.0f }; // Purple
	sprite.Size = { 3.0f, 3.0f };
	sprite.ZIndex = -1.0f; // Render behind other entities		auto& health = boss.AddComponent<HealthComponent>(500.0f);

		auto& ai = boss.AddComponent<BossAIComponent>();
		ai.Phase = BossPhase::Phase1;
		ai.AttackCooldown = 2.0f;

		// Add particle emitter for ambient effects
		auto& emitter = boss.AddComponent<ParticleEmitterComponent>();
		emitter.Enabled = true;
		emitter.EmissionRate = 5.0f;
		emitter.Shape = EmissionShape::Circle;
		emitter.ShapeSize = { 1.5f, 1.5f };
		emitter.Speed = 0.5f;
		emitter.SpeedVariance = 0.2f;
		emitter.Lifetime = 1.5f;
		emitter.Size = 0.1f;
		emitter.StartColor = { 0.8f, 0.2f, 1.0f, 0.8f };
		emitter.FadeOut = true;

		PIL_INFO("Created boss at ({:.1f}, {:.1f})", position.x, position.y);
		return boss;
	}

	Entity EntityFactory::CreateXPGem(const glm::vec2& position, int xpValue)
	{
		Entity gem = m_Scene->CreateEntity("XPGem");

		auto& transform = gem.GetComponent<TransformComponent>();
		transform.Position = position;
		transform.Rotation = 0.0f;

		// Size based on value
		GemSize size;
		float scale;
		std::string textureName;
		glm::vec4 color;

		if (xpValue >= 10)
		{
			size = GemSize::Large;
			scale = 0.5f;
			textureName = "xp_gem_large";
			color = { 0.8f, 0.2f, 1.0f, 1.0f }; // Purple
		}
		else if (xpValue >= 5)
		{
			size = GemSize::Medium;
			scale = 0.4f;
			textureName = "xp_gem_medium";
			color = { 0.2f, 0.6f, 1.0f, 1.0f }; // Blue
		}
		else
		{
			size = GemSize::Small;
			scale = 0.3f;
			textureName = "xp_gem_small";
			color = { 0.2f, 1.0f, 0.2f, 1.0f }; // Green
		}

		transform.Scale = { scale, scale };

		auto& sprite = gem.AddComponent<SpriteComponent>();
		sprite.Texture = GetTexture(textureName);
		sprite.Color = color;
		sprite.Size = { scale, scale };

		auto& xpComp = gem.AddComponent<XPGemComponent>(xpValue);
		xpComp.AttractionRadius = 3.0f;

		auto& visual = gem.AddComponent<XPGemVisualComponent>(size);
		visual.BasePosition = position;

		gem.AddComponent<VelocityComponent>();

		return gem;
	}

	Entity EntityFactory::CreateObstacle(const glm::vec2& position, const std::string& type, const glm::vec2& size)
	{
		Entity obstacle = m_Scene->CreateEntity("Obstacle_" + type);

		auto& transform = obstacle.GetComponent<TransformComponent>();
		transform.Position = position;
		transform.Rotation = 0.0f;
		transform.Scale = size;

		auto& sprite = obstacle.AddComponent<SpriteComponent>();
		
		ObstacleType obstacleType = ObstacleType::Crate;
		if (type == "crate")
		{
			sprite.Texture = GetTexture("crate");
			sprite.Color = { 0.6f, 0.4f, 0.2f, 1.0f }; // Brown
			obstacleType = ObstacleType::Crate;
		}
		else if (type == "pillar")
		{
			sprite.Texture = GetTexture("pillar");
			sprite.Color = { 0.5f, 0.5f, 0.5f, 1.0f }; // Gray
			obstacleType = ObstacleType::Pillar;
		}
		else if (type == "wall")
		{
			sprite.Texture = GetTexture("wall");
			sprite.Color = { 0.3f, 0.3f, 0.35f, 1.0f }; // Dark gray
			obstacleType = ObstacleType::Wall;
		}

		sprite.Size = size;

		auto& obstacleComp = obstacle.AddComponent<ObstacleComponent>(obstacleType, type == "crate");

		return obstacle;
	}

	Entity EntityFactory::CreateTriggerZone(const glm::vec2& position, const glm::vec2& size, const std::string& eventName)
	{
		Entity trigger = m_Scene->CreateEntity("Trigger_" + eventName);

		auto& transform = trigger.GetComponent<TransformComponent>();
		transform.Position = position;
		transform.Scale = size;

		auto& triggerComp = trigger.AddComponent<TriggerComponent>();
		triggerComp.OnEnterEvent = eventName;

		// No sprite for triggers (invisible)

		return trigger;
	}

	Entity EntityFactory::CreateExplosion(const glm::vec2& position, float scale)
	{
		Entity explosion = m_Scene->CreateEntity("ExplosionFX");

		auto& transform = explosion.GetComponent<TransformComponent>();
		transform.Position = position;
		transform.Scale = { scale, scale };

		auto& emitter = explosion.AddComponent<ParticleEmitterComponent>();
		emitter.Enabled = true;
		emitter.BurstMode = true;
		emitter.BurstCount = (int)(50 * scale);
		emitter.Shape = EmissionShape::Circle;
		emitter.Direction = { 0.0f, 0.0f };
		emitter.DirectionSpread = 360.0f;
		emitter.Speed = 8.0f * scale;
		emitter.SpeedVariance = 4.0f;
		emitter.Lifetime = 0.8f;
		emitter.LifetimeVariance = 0.2f;
		emitter.Size = 0.3f * scale;
		emitter.SizeVariance = 0.1f;
		emitter.StartColor = { 1.0f, 0.8f, 0.2f, 1.0f }; // Orange-yellow
		emitter.FadeOut = true;
		emitter.ScaleOverTime = true;
		emitter.EndScale = 0.0f;
		emitter.Gravity = { 0.0f, -2.0f };
		emitter.TexturePath = "particle_circle";

		return explosion;
	}

} // namespace Pillar
