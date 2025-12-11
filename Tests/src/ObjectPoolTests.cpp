#include <gtest/gtest.h>
#include "Pillar/ECS/ObjectPool.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"

using namespace Pillar;

// ============================================================================
// ObjectPool Tests
// ============================================================================

class ObjectPoolTests : public ::testing::Test
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

TEST_F(ObjectPoolTests, Init_PreallocatesEntities)
{
	ObjectPool pool;
	pool.Init(m_Scene.get(), 50);

	EXPECT_EQ(pool.GetAvailableCount(), 50);
	EXPECT_EQ(pool.GetTotalCount(), 50);
	EXPECT_EQ(pool.GetActiveCount(), 0);
}

TEST_F(ObjectPoolTests, Init_ZeroCapacity)
{
	ObjectPool pool;
	pool.Init(m_Scene.get(), 0);

	EXPECT_EQ(pool.GetAvailableCount(), 0);
	EXPECT_EQ(pool.GetTotalCount(), 0);
}

TEST_F(ObjectPoolTests, Acquire_ReturnsValidEntity)
{
	ObjectPool pool;
	pool.Init(m_Scene.get(), 10);

	Entity entity = pool.Acquire();

	EXPECT_TRUE(entity);
	EXPECT_EQ(pool.GetAvailableCount(), 9);
	EXPECT_EQ(pool.GetActiveCount(), 1);
}

TEST_F(ObjectPoolTests, Acquire_MultipleEntities)
{
	ObjectPool pool;
	pool.Init(m_Scene.get(), 5);

	Entity e1 = pool.Acquire();
	Entity e2 = pool.Acquire();
	Entity e3 = pool.Acquire();

	EXPECT_TRUE(e1);
	EXPECT_TRUE(e2);
	EXPECT_TRUE(e3);
	EXPECT_NE(static_cast<entt::entity>(e1), static_cast<entt::entity>(e2));
	EXPECT_NE(static_cast<entt::entity>(e2), static_cast<entt::entity>(e3));
	EXPECT_EQ(pool.GetAvailableCount(), 2);
	EXPECT_EQ(pool.GetActiveCount(), 3);
}

TEST_F(ObjectPoolTests, Acquire_ExhaustsPool_CreatesNew)
{
	ObjectPool pool;
	pool.Init(m_Scene.get(), 2);

	pool.Acquire();
	pool.Acquire();
	Entity entity = pool.Acquire(); // Should create new

	EXPECT_TRUE(entity);
	EXPECT_EQ(pool.GetAvailableCount(), 0);
	EXPECT_EQ(pool.GetTotalCount(), 3);
	EXPECT_EQ(pool.GetActiveCount(), 3);
}

TEST_F(ObjectPoolTests, Release_ReturnsEntityToPool)
{
	ObjectPool pool;
	pool.Init(m_Scene.get(), 5);

	Entity entity = pool.Acquire();
	EXPECT_EQ(pool.GetAvailableCount(), 4);

	pool.Release(entity);
	EXPECT_EQ(pool.GetAvailableCount(), 5);
	EXPECT_EQ(pool.GetActiveCount(), 0);
}

TEST_F(ObjectPoolTests, Release_EntityCanBeReused)
{
	ObjectPool pool;
	pool.Init(m_Scene.get(), 1);

	Entity first = pool.Acquire();
	pool.Release(first);
	Entity second = pool.Acquire();

	// Should get the same entity back
	EXPECT_EQ(static_cast<entt::entity>(first), static_cast<entt::entity>(second));
}

TEST_F(ObjectPoolTests, IsInPool_ReturnsTrueForPooledEntity)
{
	ObjectPool pool;
	pool.Init(m_Scene.get(), 5);

	Entity entity = pool.Acquire();
	EXPECT_FALSE(pool.IsInPool(entity));

	pool.Release(entity);
	EXPECT_TRUE(pool.IsInPool(entity));
}

TEST_F(ObjectPoolTests, Clear_RemovesAllEntities)
{
	ObjectPool pool;
	pool.Init(m_Scene.get(), 10);

	pool.Acquire();
	pool.Acquire();
	pool.Clear();

	EXPECT_EQ(pool.GetAvailableCount(), 0);
	EXPECT_EQ(pool.GetTotalCount(), 0);
}

TEST_F(ObjectPoolTests, InitCallback_CalledOnNewEntities)
{
	ObjectPool pool;
	int callCount = 0;

	pool.SetInitCallback([&callCount](Entity e) {
		e.AddComponent<VelocityComponent>();
		callCount++;
	});

	pool.Init(m_Scene.get(), 5);

	EXPECT_EQ(callCount, 5);

	// Verify components were added
	Entity entity = pool.Acquire();
	EXPECT_TRUE(entity.HasComponent<VelocityComponent>());
}

TEST_F(ObjectPoolTests, ResetCallback_CalledOnRelease)
{
	ObjectPool pool;
	int resetCallCount = 0;

	pool.SetInitCallback([](Entity e) {
		e.AddComponent<VelocityComponent>(glm::vec2(100.0f, 100.0f));
	});

	pool.SetResetCallback([&resetCallCount](Entity e) {
		auto& vel = e.GetComponent<VelocityComponent>();
		vel.Velocity = glm::vec2(0.0f);
		resetCallCount++;
	});

	pool.Init(m_Scene.get(), 3);

	Entity entity = pool.Acquire();
	entity.GetComponent<VelocityComponent>().Velocity = glm::vec2(500.0f, 500.0f);

	pool.Release(entity);

	EXPECT_EQ(resetCallCount, 1);
	EXPECT_EQ(entity.GetComponent<VelocityComponent>().Velocity, glm::vec2(0.0f));
}

TEST_F(ObjectPoolTests, GetStatistics_Accurate)
{
	ObjectPool pool;
	pool.Init(m_Scene.get(), 100);

	for (int i = 0; i < 30; i++)
	{
		pool.Acquire();
	}

	EXPECT_EQ(pool.GetTotalCount(), 100);
	EXPECT_EQ(pool.GetAvailableCount(), 70);
	EXPECT_EQ(pool.GetActiveCount(), 30);
}
