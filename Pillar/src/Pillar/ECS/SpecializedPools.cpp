#include "SpecializedPools.h"
#include "Scene.h"
#include "Pillar/Logger.h"

namespace Pillar {

// ============================================================================
// BulletPool Implementation
// ============================================================================

void BulletPool::Init(Scene* scene, uint32_t initialCapacity)
{
	PIL_CORE_ASSERT(scene, "Scene cannot be null!");

	m_Scene = scene;

	// Set up initialization callback to add bullet components
	m_Pool.SetInitCallback([this](Entity entity) {
		entity.AddComponent<TransformComponent>();
		entity.AddComponent<VelocityComponent>();
		entity.AddComponent<BulletComponent>();
		// Note: Sprite component will be added when rendering system is implemented
	});

	// Set up reset callback to reset bullet state when returned to pool
	m_Pool.SetResetCallback([](Entity entity) {
		auto& transform = entity.GetComponent<TransformComponent>();
		transform.Position = glm::vec2(0.0f);
		transform.Rotation = 0.0f;
		transform.Dirty = true;

		auto& velocity = entity.GetComponent<VelocityComponent>();
		velocity.Velocity = glm::vec2(0.0f);

		auto& bullet = entity.GetComponent<BulletComponent>();
		bullet.TimeAlive = 0.0f;
		bullet.HitsRemaining = bullet.MaxHits;
	});

	// Initialize the underlying pool
	m_Pool.Init(scene, initialCapacity);

	PIL_CORE_INFO("BulletPool initialized with {0} bullets", initialCapacity);
}

Entity BulletPool::SpawnBullet(
	const glm::vec2& position,
	const glm::vec2& direction,
	float speed,
	Entity owner,
	float damage,
	float lifetime)
{
	// Acquire entity from pool
	Entity bullet = m_Pool.Acquire();

	// Set transform
	auto& transform = bullet.GetComponent<TransformComponent>();
	transform.Position = position;
	transform.Rotation = atan2(direction.y, direction.x); // Orient bullet
	transform.Dirty = true;

	// Set velocity
	glm::vec2 normalizedDir = glm::normalize(direction);
	auto& velocity = bullet.GetComponent<VelocityComponent>();
	velocity.Velocity = normalizedDir * speed;

	// Set bullet properties
	auto& bulletComp = bullet.GetComponent<BulletComponent>();
	bulletComp.Owner = owner;
	bulletComp.Damage = damage;
	bulletComp.Lifetime = lifetime;
	bulletComp.TimeAlive = 0.0f;
	bulletComp.HitsRemaining = bulletComp.MaxHits;

	PIL_CORE_TRACE("BulletPool: Spawned bullet at ({0}, {1})", position.x, position.y);

	return bullet;
}

void BulletPool::ReturnBullet(Entity bullet)
{
	m_Pool.Release(bullet);
}

// ============================================================================
// ParticlePool Implementation
// ============================================================================

void ParticlePool::Init(Scene* scene, uint32_t initialCapacity)
{
	PIL_CORE_ASSERT(scene, "Scene cannot be null!");

	m_Scene = scene;

	// Set up initialization callback
	m_Pool.SetInitCallback([this](Entity entity) {
		entity.AddComponent<TransformComponent>();
		entity.AddComponent<VelocityComponent>();
		// Note: Sprite component will be added when rendering system is implemented
	});

	// Set up reset callback
	m_Pool.SetResetCallback([](Entity entity) {
		auto& transform = entity.GetComponent<TransformComponent>();
		transform.Position = glm::vec2(0.0f);
		transform.Scale = glm::vec2(1.0f);
		transform.Rotation = 0.0f;
		transform.Dirty = true;

		auto& velocity = entity.GetComponent<VelocityComponent>();
		velocity.Velocity = glm::vec2(0.0f);
		velocity.Acceleration = glm::vec2(0.0f);
	});

	// Initialize the underlying pool
	m_Pool.Init(scene, initialCapacity);

	PIL_CORE_INFO("ParticlePool initialized with {0} particles", initialCapacity);
}

Entity ParticlePool::SpawnParticle(
	const glm::vec2& position,
	const glm::vec2& velocity,
	const glm::vec4& color,
	float size,
	float lifetime)
{
	// Acquire entity from pool
	Entity particle = m_Pool.Acquire();

	// Set transform
	auto& transform = particle.GetComponent<TransformComponent>();
	transform.Position = position;
	transform.Scale = glm::vec2(size);
	transform.Dirty = true;

	// Set velocity
	auto& vel = particle.GetComponent<VelocityComponent>();
	vel.Velocity = velocity;
	vel.Acceleration = glm::vec2(0.0f, -9.81f); // Gravity

	PIL_CORE_TRACE("ParticlePool: Spawned particle at ({0}, {1})", position.x, position.y);

	return particle;
}

void ParticlePool::ReturnParticle(Entity particle)
{
	m_Pool.Release(particle);
}

} // namespace Pillar
