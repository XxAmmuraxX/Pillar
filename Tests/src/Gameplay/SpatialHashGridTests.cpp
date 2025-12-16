#include <gtest/gtest.h>
// SpatialHashGridTests: verifies insertion, querying, removal and performance
// characteristics of the spatial hash grid used for proximity queries.
#include "Pillar/ECS/Physics/SpatialHashGrid.h"
#include <glm/glm.hpp>
#include <chrono>

using namespace Pillar;

// ========================================
// SpatialHashGrid Tests
// ========================================

TEST(SpatialHashGridTests, Constructor_InitializesEmpty)
{
	SpatialHashGrid grid(2.0f);
	EXPECT_EQ(grid.GetEntityCount(), 0);
	EXPECT_EQ(grid.GetBucketCount(), 0);
}

TEST(SpatialHashGridTests, Insert_AddsEntity)
{
	SpatialHashGrid grid(2.0f);
	grid.Insert(1, glm::vec2(0, 0));
	
	EXPECT_EQ(grid.GetEntityCount(), 1);
	EXPECT_EQ(grid.GetBucketCount(), 1);
}

TEST(SpatialHashGridTests, Insert_MultipleEntities)
{
	SpatialHashGrid grid(2.0f);
	
	grid.Insert(1, glm::vec2(0, 0));
	grid.Insert(2, glm::vec2(1, 1));
	grid.Insert(3, glm::vec2(5, 5));
	
	EXPECT_EQ(grid.GetEntityCount(), 3);
	// Entities at (0,0) and (1,1) are in same cell (2.0 cell size)
	// Entity at (5,5) is in different cell
	EXPECT_GE(grid.GetBucketCount(), 2);
}

TEST(SpatialHashGridTests, Query_FindsNearbyEntities)
{
	SpatialHashGrid grid(2.0f);
	
	grid.Insert(1, glm::vec2(0, 0));
	grid.Insert(2, glm::vec2(1, 1));
	grid.Insert(3, glm::vec2(10, 10)); // Far away
	
	// Query near origin with radius 3.0
	auto results = grid.Query(glm::vec2(0, 0), 3.0f);
	
	// Should find entities 1 and 2 (in nearby cells)
	EXPECT_GE(results.size(), 2);
	
	// Should NOT find entity 3 (too far)
	// Note: Spatial grid returns all entities in cells, not distance-filtered
	// So we can't guarantee entity 3 is not in results (that's for the caller to check)
}

TEST(SpatialHashGridTests, QueryAABB_FindsEntitiesInBox)
{
	SpatialHashGrid grid(2.0f);
	
	grid.Insert(1, glm::vec2(0, 0));
	grid.Insert(2, glm::vec2(1, 1));
	grid.Insert(3, glm::vec2(5, 5));
	
	// Query AABB covering first two entities
	auto results = grid.QueryAABB(glm::vec2(-1, -1), glm::vec2(2, 2));
	
	EXPECT_GE(results.size(), 2);
}

TEST(SpatialHashGridTests, Remove_DeletesEntity)
{
	SpatialHashGrid grid(2.0f);
	
	grid.Insert(1, glm::vec2(0, 0));
	grid.Insert(2, glm::vec2(1, 1));
	
	EXPECT_EQ(grid.GetEntityCount(), 2);
	
	grid.Remove(1, glm::vec2(0, 0));
	
	EXPECT_EQ(grid.GetEntityCount(), 1);
}

TEST(SpatialHashGridTests, Clear_RemovesAllEntities)
{
	SpatialHashGrid grid(2.0f);
	
	for (uint32_t i = 0; i < 100; ++i)
	{
		grid.Insert(i, glm::vec2(i * 0.5f, i * 0.5f));
	}
	
	EXPECT_EQ(grid.GetEntityCount(), 100);
	
	grid.Clear();
	
	EXPECT_EQ(grid.GetEntityCount(), 0);
	EXPECT_EQ(grid.GetBucketCount(), 0);
}

TEST(SpatialHashGridTests, Performance_10000Entities)
{
	SpatialHashGrid grid(2.0f);
	
	// Insert 10,000 entities
	auto insertStart = std::chrono::high_resolution_clock::now();
	for (uint32_t i = 0; i < 10000; ++i)
	{
		float x = (i % 100) * 0.5f;
		float y = (i / 100) * 0.5f;
		grid.Insert(i, glm::vec2(x, y));
	}
	auto insertEnd = std::chrono::high_resolution_clock::now();
	
	EXPECT_EQ(grid.GetEntityCount(), 10000);
	
	// Query 1000 times
	auto queryStart = std::chrono::high_resolution_clock::now();
	size_t totalResults = 0;
	for (int i = 0; i < 1000; ++i)
	{
		auto results = grid.Query(glm::vec2(25, 25), 5.0f);
		totalResults += results.size();
	}
	auto queryEnd = std::chrono::high_resolution_clock::now();
	
	// Calculate times
	auto insertTime = std::chrono::duration_cast<std::chrono::milliseconds>(insertEnd - insertStart);
	auto queryTime = std::chrono::duration_cast<std::chrono::milliseconds>(queryEnd - queryStart);
	
	// Performance expectations
	EXPECT_LT(insertTime.count(), 100); // Insert 10k entities in < 100ms
	EXPECT_LT(queryTime.count(), 100);  // 1000 queries in < 100ms
	EXPECT_GT(totalResults, 0);         // Should find some results
	
	// Log performance metrics
	std::cout << "Insert 10,000 entities: " << insertTime.count() << "ms\n";
	std::cout << "1000 queries: " << queryTime.count() << "ms\n";
	std::cout << "Avg results per query: " << (totalResults / 1000.0f) << "\n";
}

TEST(SpatialHashGridTests, SameCell_MultipleEntities)
{
	SpatialHashGrid grid(10.0f); // Large cell size
	
	// All these entities should be in the same cell
	grid.Insert(1, glm::vec2(0, 0));
	grid.Insert(2, glm::vec2(1, 1));
	grid.Insert(3, glm::vec2(2, 2));
	grid.Insert(4, glm::vec2(3, 3));
	
	EXPECT_EQ(grid.GetEntityCount(), 4);
	EXPECT_EQ(grid.GetBucketCount(), 1); // All in one bucket
	
	// Query should find all entities
	auto results = grid.Query(glm::vec2(0, 0), 15.0f);
	EXPECT_EQ(results.size(), 4);
}

TEST(SpatialHashGridTests, NegativeCoordinates_Work)
{
	SpatialHashGrid grid(2.0f);
	
	grid.Insert(1, glm::vec2(-5, -5));
	grid.Insert(2, glm::vec2(-1, -1));
	grid.Insert(3, glm::vec2(5, 5));
	
	EXPECT_EQ(grid.GetEntityCount(), 3);
	
	// Query near negative coordinates
	auto results = grid.Query(glm::vec2(-3, -3), 5.0f);
	EXPECT_GE(results.size(), 2);
}
