#include <gtest/gtest.h>
// SpecializedPoolsTests: tests BulletPool and ParticlePool behavior including
// spawning, initialization, transform/velocity setup, return-to-pool and high-volume.
#include "Pillar/ECS/SpecializedPools.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"
#include "Pillar/ECS/Components/Gameplay/BulletComponent.h"
#include "Pillar/ECS/Components/Gameplay/ParticleComponent.h"

using namespace Pillar;

// ============================================================================
// BulletPool Tests
// ============================================================================

class BulletPoolTests : public ::testing::Test
{
protected:
	void SetUp() override
	{
		m_Scene = std::make_unique<Scene>();
	}

	void TearDown() override
	{
		m_Scene.reset();
	}

	std::unique_ptr<Scene> m_Scene;
};

TEST_F(BulletPoolTests, Init_PreallocatesBullets)
{
	BulletPool pool;
	pool.Init(m_Scene.get(), 100);

	EXPECT_EQ(pool.GetAvailableCount(), 100);
	EXPECT_EQ(pool.GetTotalCount(), 100);
}

TEST_F(BulletPoolTests, SpawnBullet_ReturnsValidEntity)
{
	BulletPool pool;
	pool.Init(m_Scene.get(), 10);

	Entity owner = m_Scene->CreateEntity("Player");
	Entity bullet = pool.SpawnBullet(
		glm::vec2(0.0f, 0.0f),
		glm::vec2(1.0f, 0.0f),
		500.0f,
		owner,
		25.0f,
		3.0f
	);

	EXPECT_TRUE(bullet);
	EXPECT_EQ(pool.GetAvailableCount(), 9);
	EXPECT_EQ(pool.GetActiveCount(), 1);
}

TEST_F(BulletPoolTests, SpawnBullet_SetsTransform)
{
	BulletPool pool;
	pool.Init(m_Scene.get(), 10);

	Entity owner = m_Scene->CreateEntity();
	glm::vec2 spawnPos(100.0f, 50.0f);
	Entity bullet = pool.SpawnBullet(spawnPos, glm::vec2(1, 0), 100.0f, owner);

	EXPECT_TRUE(bullet.HasComponent<TransformComponent>());
	auto& transform = bullet.GetComponent<TransformComponent>();
	EXPECT_FLOAT_EQ(transform.Position.x, spawnPos.x);
	EXPECT_FLOAT_EQ(transform.Position.y, spawnPos.y);
}

TEST_F(BulletPoolTests, SpawnBullet_SetsVelocity)
{
	BulletPool pool;
	pool.Init(m_Scene.get(), 10);

	Entity owner = m_Scene->CreateEntity();
	glm::vec2 direction = glm::normalize(glm::vec2(1.0f, 1.0f));
	float speed = 200.0f;

	Entity bullet = pool.SpawnBullet(glm::vec2(0), direction, speed, owner);

	EXPECT_TRUE(bullet.HasComponent<VelocityComponent>());
	auto& vel = bullet.GetComponent<VelocityComponent>();
	glm::vec2 expectedVel = direction * speed;
	EXPECT_NEAR(vel.Velocity.x, expectedVel.x, 0.01f);
	EXPECT_NEAR(vel.Velocity.y, expectedVel.y, 0.01f);
}

TEST_F(BulletPoolTests, SpawnBullet_SetsBulletComponent)
{
	BulletPool pool;
	pool.Init(m_Scene.get(), 10);

	Entity owner = m_Scene->CreateEntity("Player");
	float damage = 50.0f;
	float lifetime = 2.5f;

	Entity bullet = pool.SpawnBullet(
		glm::vec2(0), glm::vec2(1, 0), 100.0f, owner, damage, lifetime
	);

	EXPECT_TRUE(bullet.HasComponent<BulletComponent>());
	auto& bulletComp = bullet.GetComponent<BulletComponent>();
	EXPECT_EQ(bulletComp.Owner, owner);
	EXPECT_FLOAT_EQ(bulletComp.Damage, damage);
	EXPECT_FLOAT_EQ(bulletComp.Lifetime, lifetime);
	EXPECT_FLOAT_EQ(bulletComp.TimeAlive, 0.0f);
}

TEST_F(BulletPoolTests, ReturnBullet_ReturnsToPool)
{
	BulletPool pool;
	pool.Init(m_Scene.get(), 10);

	Entity owner = m_Scene->CreateEntity();
	Entity bullet = pool.SpawnBullet(glm::vec2(0), glm::vec2(1, 0), 100.0f, owner);
	EXPECT_EQ(pool.GetActiveCount(), 1);

	pool.ReturnBullet(bullet);
	EXPECT_EQ(pool.GetActiveCount(), 0);
	EXPECT_EQ(pool.GetAvailableCount(), 10);
}

TEST_F(BulletPoolTests, Clear_RemovesAllBullets)
{
	BulletPool pool;
	pool.Init(m_Scene.get(), 50);

	Entity owner = m_Scene->CreateEntity();
	for (int i = 0; i < 20; i++)
	{
		pool.SpawnBullet(glm::vec2(i, 0), glm::vec2(1, 0), 100.0f, owner);
	}

	pool.Clear();
	EXPECT_EQ(pool.GetAvailableCount(), 0);
	EXPECT_EQ(pool.GetTotalCount(), 0);
}

// ============================================================================
// ParticlePool Tests
// ============================================================================

class ParticlePoolTests : public ::testing::Test
{
protected:
	void SetUp() override
	{
		m_Scene = std::make_unique<Scene>();
	}

	void TearDown() override
	{
		m_Scene.reset();
	}

	std::unique_ptr<Scene> m_Scene;
};

TEST_F(ParticlePoolTests, Init_PreallocatesParticles)
{
	ParticlePool pool;
	pool.Init(m_Scene.get(), 500);

	EXPECT_EQ(pool.GetAvailableCount(), 500);
	EXPECT_EQ(pool.GetTotalCount(), 500);
}

TEST_F(ParticlePoolTests, SpawnParticle_ReturnsValidEntity)
{
	ParticlePool pool;
	pool.Init(m_Scene.get(), 100);

	Entity particle = pool.SpawnParticle(
		glm::vec2(10.0f, 20.0f),
		glm::vec2(5.0f, -10.0f),
		glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
		0.2f,
		2.0f
	);

	EXPECT_TRUE(particle);
	EXPECT_EQ(pool.GetActiveCount(), 1);
}

TEST_F(ParticlePoolTests, SpawnParticle_SetsTransform)
{
	ParticlePool pool;
	pool.Init(m_Scene.get(), 10);

	glm::vec2 position(50.0f, 75.0f);
	float size = 0.5f;

	Entity particle = pool.SpawnParticle(position, glm::vec2(0), glm::vec4(1), size, 1.0f);

	EXPECT_TRUE(particle.HasComponent<TransformComponent>());
	auto& transform = particle.GetComponent<TransformComponent>();
	EXPECT_FLOAT_EQ(transform.Position.x, position.x);
	EXPECT_FLOAT_EQ(transform.Position.y, position.y);
}

TEST_F(ParticlePoolTests, SpawnParticle_SetsVelocity)
{
	ParticlePool pool;
	pool.Init(m_Scene.get(), 10);

	glm::vec2 velocity(100.0f, -50.0f);
	Entity particle = pool.SpawnParticle(glm::vec2(0), velocity, glm::vec4(1), 0.1f, 1.0f);

	EXPECT_TRUE(particle.HasComponent<VelocityComponent>());
	auto& vel = particle.GetComponent<VelocityComponent>();
	EXPECT_FLOAT_EQ(vel.Velocity.x, velocity.x);
	EXPECT_FLOAT_EQ(vel.Velocity.y, velocity.y);
}

TEST_F(ParticlePoolTests, SpawnParticle_SetsParticleComponent)
{
	ParticlePool pool;
	pool.Init(m_Scene.get(), 10);

	float lifetime = 3.5f;
	Entity particle = pool.SpawnParticle(glm::vec2(0), glm::vec2(0), glm::vec4(1), 0.1f, lifetime);

	EXPECT_TRUE(particle.HasComponent<ParticleComponent>());
	auto& comp = particle.GetComponent<ParticleComponent>();
	EXPECT_FLOAT_EQ(comp.Lifetime, lifetime);
	EXPECT_FLOAT_EQ(comp.Age, 0.0f);
	EXPECT_FALSE(comp.Dead);
}

TEST_F(ParticlePoolTests, ReturnParticle_ReturnsToPool)
{
	ParticlePool pool;
	pool.Init(m_Scene.get(), 100);

	Entity particle = pool.SpawnParticle(glm::vec2(0), glm::vec2(0), glm::vec4(1), 0.1f, 1.0f);
	EXPECT_EQ(pool.GetActiveCount(), 1);

	pool.ReturnParticle(particle);
	EXPECT_EQ(pool.GetActiveCount(), 0);
}

TEST_F(ParticlePoolTests, HighVolume_ManyParticles)
{
	ParticlePool pool;
	pool.Init(m_Scene.get(), 1000);

	std::vector<Entity> particles;
	for (int i = 0; i < 1000; i++)
	{
		particles.push_back(pool.SpawnParticle(
			glm::vec2(i * 0.1f, 0.0f),
			glm::vec2(0, -10.0f),
			glm::vec4(1.0f),
			0.1f,
			1.0f
		));
	}

	EXPECT_EQ(pool.GetActiveCount(), 1000);
	EXPECT_EQ(pool.GetAvailableCount(), 0);

	// Return half
	for (int i = 0; i < 500; i++)
	{
		pool.ReturnParticle(particles[i]);
	}

	EXPECT_EQ(pool.GetActiveCount(), 500);
	EXPECT_EQ(pool.GetAvailableCount(), 500);
}
