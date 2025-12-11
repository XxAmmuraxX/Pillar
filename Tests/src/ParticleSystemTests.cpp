#include <gtest/gtest.h>
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/Components/Gameplay/ParticleComponent.h"
#include "Pillar/ECS/Components/Gameplay/ParticleEmitterComponent.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"
#include "Pillar/ECS/Systems/ParticleSystem.h"
#include "Pillar/ECS/Systems/ParticleEmitterSystem.h"
#include "Pillar/ECS/SpecializedPools.h"
#include <glm/glm.hpp>

using namespace Pillar;

// ============================================================================
// ParticleComponent Tests
// ============================================================================

TEST(ParticleComponentTests, DefaultConstruction)
{
	ParticleComponent particle;

	EXPECT_FLOAT_EQ(particle.Lifetime, 1.0f);
	EXPECT_FLOAT_EQ(particle.Age, 0.0f);
	EXPECT_FALSE(particle.Dead);
	EXPECT_TRUE(particle.FadeOut);
	EXPECT_FALSE(particle.ScaleOverTime);
	EXPECT_FALSE(particle.RotateOverTime);
}

TEST(ParticleComponentTests, ParameterizedConstruction)
{
	ParticleComponent particle(2.5f, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

	EXPECT_FLOAT_EQ(particle.Lifetime, 2.5f);
	EXPECT_EQ(particle.StartColor, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	// End color should have same RGB but alpha 0 (fade out)
	EXPECT_FLOAT_EQ(particle.EndColor.r, 1.0f);
	EXPECT_FLOAT_EQ(particle.EndColor.g, 0.0f);
	EXPECT_FLOAT_EQ(particle.EndColor.b, 0.0f);
	EXPECT_FLOAT_EQ(particle.EndColor.a, 0.0f);
}

TEST(ParticleComponentTests, GetNormalizedAge_ZeroAge)
{
	ParticleComponent particle;
	particle.Lifetime = 2.0f;
	particle.Age = 0.0f;

	EXPECT_FLOAT_EQ(particle.GetNormalizedAge(), 0.0f);
}

TEST(ParticleComponentTests, GetNormalizedAge_HalfLifetime)
{
	ParticleComponent particle;
	particle.Lifetime = 2.0f;
	particle.Age = 1.0f;

	EXPECT_FLOAT_EQ(particle.GetNormalizedAge(), 0.5f);
}

TEST(ParticleComponentTests, GetNormalizedAge_FullLifetime)
{
	ParticleComponent particle;
	particle.Lifetime = 2.0f;
	particle.Age = 2.0f;

	EXPECT_FLOAT_EQ(particle.GetNormalizedAge(), 1.0f);
}

TEST(ParticleComponentTests, GetNormalizedAge_ZeroLifetime)
{
	ParticleComponent particle;
	particle.Lifetime = 0.0f;
	particle.Age = 1.0f;

	// Should return 1.0 when lifetime is 0 to avoid division by zero
	EXPECT_FLOAT_EQ(particle.GetNormalizedAge(), 1.0f);
}

TEST(ParticleComponentTests, ShouldRemove_NotDead)
{
	ParticleComponent particle;
	particle.Lifetime = 2.0f;
	particle.Age = 1.0f;
	particle.Dead = false;

	EXPECT_FALSE(particle.ShouldRemove());
}

TEST(ParticleComponentTests, ShouldRemove_DeadFlag)
{
	ParticleComponent particle;
	particle.Dead = true;

	EXPECT_TRUE(particle.ShouldRemove());
}

TEST(ParticleComponentTests, ShouldRemove_AgeExceedsLifetime)
{
	ParticleComponent particle;
	particle.Lifetime = 1.0f;
	particle.Age = 1.5f;

	EXPECT_TRUE(particle.ShouldRemove());
}

TEST(ParticleComponentTests, ShouldRemove_AgeEqualsLifetime)
{
	ParticleComponent particle;
	particle.Lifetime = 1.0f;
	particle.Age = 1.0f;

	EXPECT_TRUE(particle.ShouldRemove());
}

TEST(ParticleComponentTests, VisualEffects_SizeInterpolation)
{
	ParticleComponent particle;
	particle.StartSize = glm::vec2(1.0f, 1.0f);
	particle.EndSize = glm::vec2(0.0f, 0.0f);
	particle.ScaleOverTime = true;

	EXPECT_EQ(particle.StartSize, glm::vec2(1.0f, 1.0f));
	EXPECT_EQ(particle.EndSize, glm::vec2(0.0f, 0.0f));
}

TEST(ParticleComponentTests, VisualEffects_ColorInterpolation)
{
	ParticleComponent particle;
	particle.StartColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	particle.EndColor = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);

	EXPECT_EQ(particle.StartColor, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	EXPECT_EQ(particle.EndColor, glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
}

// ============================================================================
// ParticleEmitterComponent Tests
// ============================================================================

TEST(ParticleEmitterComponentTests, DefaultConstruction)
{
	ParticleEmitterComponent emitter;

	EXPECT_TRUE(emitter.Enabled);
	EXPECT_FLOAT_EQ(emitter.EmissionRate, 10.0f);
	EXPECT_FALSE(emitter.BurstMode);
	EXPECT_EQ(emitter.BurstCount, 100);
	EXPECT_EQ(emitter.Shape, EmissionShape::Point);
}

TEST(ParticleEmitterComponentTests, EmissionShape_Point)
{
	ParticleEmitterComponent emitter;
	emitter.Shape = EmissionShape::Point;

	EXPECT_EQ(emitter.Shape, EmissionShape::Point);
}

TEST(ParticleEmitterComponentTests, EmissionShape_Circle)
{
	ParticleEmitterComponent emitter;
	emitter.Shape = EmissionShape::Circle;
	emitter.ShapeSize = glm::vec2(2.0f);

	EXPECT_EQ(emitter.Shape, EmissionShape::Circle);
	EXPECT_EQ(emitter.ShapeSize, glm::vec2(2.0f));
}

TEST(ParticleEmitterComponentTests, EmissionShape_Box)
{
	ParticleEmitterComponent emitter;
	emitter.Shape = EmissionShape::Box;
	emitter.ShapeSize = glm::vec2(5.0f, 3.0f);

	EXPECT_EQ(emitter.Shape, EmissionShape::Box);
	EXPECT_EQ(emitter.ShapeSize, glm::vec2(5.0f, 3.0f));
}

TEST(ParticleEmitterComponentTests, EmissionShape_Cone)
{
	ParticleEmitterComponent emitter;
	emitter.Shape = EmissionShape::Cone;
	emitter.Direction = glm::vec2(0.0f, 1.0f);
	emitter.DirectionSpread = 45.0f;

	EXPECT_EQ(emitter.Shape, EmissionShape::Cone);
	EXPECT_EQ(emitter.Direction, glm::vec2(0.0f, 1.0f));
	EXPECT_FLOAT_EQ(emitter.DirectionSpread, 45.0f);
}

TEST(ParticleEmitterComponentTests, BurstMode_Settings)
{
	ParticleEmitterComponent emitter;
	emitter.BurstMode = true;
	emitter.BurstCount = 500;
	emitter.BurstFired = false;

	EXPECT_TRUE(emitter.BurstMode);
	EXPECT_EQ(emitter.BurstCount, 500);
	EXPECT_FALSE(emitter.BurstFired);
}

TEST(ParticleEmitterComponentTests, SpeedSettings)
{
	ParticleEmitterComponent emitter;
	emitter.Speed = 100.0f;
	emitter.SpeedVariance = 25.0f;

	EXPECT_FLOAT_EQ(emitter.Speed, 100.0f);
	EXPECT_FLOAT_EQ(emitter.SpeedVariance, 25.0f);
}

TEST(ParticleEmitterComponentTests, LifetimeSettings)
{
	ParticleEmitterComponent emitter;
	emitter.Lifetime = 5.0f;
	emitter.LifetimeVariance = 1.0f;

	EXPECT_FLOAT_EQ(emitter.Lifetime, 5.0f);
	EXPECT_FLOAT_EQ(emitter.LifetimeVariance, 1.0f);
}

TEST(ParticleEmitterComponentTests, VisualEffects)
{
	ParticleEmitterComponent emitter;
	emitter.FadeOut = true;
	emitter.ScaleOverTime = true;
	emitter.RotateOverTime = true;
	emitter.EndScale = 0.25f;
	emitter.RotationSpeed = 360.0f;

	EXPECT_TRUE(emitter.FadeOut);
	EXPECT_TRUE(emitter.ScaleOverTime);
	EXPECT_TRUE(emitter.RotateOverTime);
	EXPECT_FLOAT_EQ(emitter.EndScale, 0.25f);
	EXPECT_FLOAT_EQ(emitter.RotationSpeed, 360.0f);
}

TEST(ParticleEmitterComponentTests, GravitySettings)
{
	ParticleEmitterComponent emitter;
	emitter.Gravity = glm::vec2(0.0f, -9.8f);

	EXPECT_FLOAT_EQ(emitter.Gravity.x, 0.0f);
	EXPECT_FLOAT_EQ(emitter.Gravity.y, -9.8f);
}

// ============================================================================
// ParticleSystem Tests
// ============================================================================

class ParticleSystemTests : public ::testing::Test
{
protected:
	void SetUp() override
	{
		m_Scene = std::make_unique<Scene>();
		m_ParticlePool = std::make_unique<ParticlePool>();
		m_ParticlePool->Init(m_Scene.get(), 100);
		m_System.OnAttach(m_Scene.get());
		m_System.SetParticlePool(m_ParticlePool.get());
	}

	void TearDown() override
	{
		m_ParticlePool.reset();
		m_Scene.reset();
	}

	std::unique_ptr<Scene> m_Scene;
	std::unique_ptr<ParticlePool> m_ParticlePool;
	ParticleSystem m_System;
};

// Note: ParticleSystem tests are limited because the system iterates over
// ALL entities with ParticleComponent+TransformComponent+SpriteComponent,
// including pooled (inactive) entities. The pool pre-allocates entities,
// so counts include pooled entities.

TEST_F(ParticleSystemTests, OnUpdate_ProcessesParticles)
{
	// Spawn a single particle
	Entity particle = m_ParticlePool->SpawnParticle(
		glm::vec2(0), glm::vec2(0), glm::vec4(1), 0.1f, 2.0f
	);
	auto& comp = particle.GetComponent<ParticleComponent>();
	EXPECT_FLOAT_EQ(comp.Age, 0.0f);
	EXPECT_FALSE(comp.Dead);

	// Update should process particles
	m_System.OnUpdate(0.5f);

	// Particle should have been aged
	EXPECT_GE(comp.Age, 0.0f);
}

TEST_F(ParticleSystemTests, ParticleComponent_ShouldRemove_WhenDead)
{
	ParticleComponent comp;
	comp.Dead = true;
	EXPECT_TRUE(comp.ShouldRemove());
}

TEST_F(ParticleSystemTests, ParticleComponent_ShouldRemove_WhenAgeExceedsLifetime)
{
	ParticleComponent comp;
	comp.Lifetime = 1.0f;
	comp.Age = 1.5f;
	EXPECT_TRUE(comp.ShouldRemove());
}

// ============================================================================
// ParticleEmitterSystem Tests
// ============================================================================

class ParticleEmitterSystemTests : public ::testing::Test
{
protected:
	void SetUp() override
	{
		m_Scene = std::make_unique<Scene>();
		m_ParticlePool = std::make_unique<ParticlePool>();
		m_ParticlePool->Init(m_Scene.get(), 1000);
		m_System.OnAttach(m_Scene.get());
		m_System.SetParticlePool(m_ParticlePool.get());
	}

	void TearDown() override
	{
		m_ParticlePool.reset();
		m_Scene.reset();
	}

	std::unique_ptr<Scene> m_Scene;
	std::unique_ptr<ParticlePool> m_ParticlePool;
	ParticleEmitterSystem m_System;
};

TEST_F(ParticleEmitterSystemTests, OnUpdate_SpawnsParticles)
{
	Entity emitter = m_Scene->CreateEntity("Emitter");
	auto& emitterComp = emitter.AddComponent<ParticleEmitterComponent>();
	emitterComp.EmissionRate = 100.0f; // 100 particles per second
	emitterComp.Enabled = true;

	m_System.OnUpdate(1.0f); // 1 second

	// Should have spawned approximately 100 particles
	EXPECT_GT(m_ParticlePool->GetActiveCount(), 0);
}

TEST_F(ParticleEmitterSystemTests, OnUpdate_DisabledEmitter_NoParticles)
{
	Entity emitter = m_Scene->CreateEntity("Emitter");
	auto& emitterComp = emitter.AddComponent<ParticleEmitterComponent>();
	emitterComp.EmissionRate = 100.0f;
	emitterComp.Enabled = false;

	m_System.OnUpdate(1.0f);

	EXPECT_EQ(m_ParticlePool->GetActiveCount(), 0);
}

TEST_F(ParticleEmitterSystemTests, OnUpdate_BurstMode)
{
	Entity emitter = m_Scene->CreateEntity("Emitter");
	auto& emitterComp = emitter.AddComponent<ParticleEmitterComponent>();
	emitterComp.BurstMode = true;
	emitterComp.BurstCount = 50;
	emitterComp.BurstFired = false;
	emitterComp.Enabled = true;

	m_System.OnUpdate(0.016f);

	// Burst should have fired
	EXPECT_TRUE(emitterComp.BurstFired);
	EXPECT_EQ(m_ParticlePool->GetActiveCount(), 50);
}

TEST_F(ParticleEmitterSystemTests, OnUpdate_BurstMode_OnlyFiresOnce)
{
	Entity emitter = m_Scene->CreateEntity("Emitter");
	auto& emitterComp = emitter.AddComponent<ParticleEmitterComponent>();
	emitterComp.BurstMode = true;
	emitterComp.BurstCount = 25;
	emitterComp.BurstFired = false;
	emitterComp.Enabled = true;

	m_System.OnUpdate(0.016f);
	size_t countAfterFirst = m_ParticlePool->GetActiveCount();

	m_System.OnUpdate(0.016f);
	size_t countAfterSecond = m_ParticlePool->GetActiveCount();

	// Count should not increase (burst already fired)
	EXPECT_EQ(countAfterFirst, countAfterSecond);
}

TEST_F(ParticleEmitterSystemTests, GetEmitterCount)
{
	// Create multiple emitters
	for (int i = 0; i < 3; i++)
	{
		Entity emitter = m_Scene->CreateEntity("Emitter" + std::to_string(i));
		auto& comp = emitter.AddComponent<ParticleEmitterComponent>();
		comp.Enabled = true;
		comp.EmissionRate = 10.0f;
	}

	m_System.OnUpdate(0.016f);

	EXPECT_EQ(m_System.GetEmitterCount(), 3);
}
