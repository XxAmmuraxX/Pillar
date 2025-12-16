#include <gtest/gtest.h>
// ComponentTests: unit tests for core ECS components (Transform, Hierarchy,
// Velocity, Collider, Sprite, Camera) verifying defaults and basic behavior.
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Core/TagComponent.h"
#include "Pillar/ECS/Components/Core/HierarchyComponent.h"
#include "Pillar/ECS/Components/Core/UUIDComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"
#include "Pillar/ECS/Components/Physics/ColliderComponent.h"
#include "Pillar/ECS/Components/Rendering/SpriteComponent.h"
#include "Pillar/ECS/Components/Rendering/CameraComponent.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace Pillar;

// ============================================================================
// TransformComponent Tests
// ============================================================================

TEST(TransformComponentTests, DefaultConstruction)
{
	TransformComponent transform;

	EXPECT_EQ(transform.Position, glm::vec2(0.0f, 0.0f));
	EXPECT_FLOAT_EQ(transform.Rotation, 0.0f);
	EXPECT_EQ(transform.Scale, glm::vec2(1.0f, 1.0f));
	EXPECT_TRUE(transform.Dirty);
}

TEST(TransformComponentTests, PositionConstruction)
{
	TransformComponent transform(glm::vec2(10.0f, 20.0f));

	EXPECT_EQ(transform.Position, glm::vec2(10.0f, 20.0f));
	EXPECT_FLOAT_EQ(transform.Rotation, 0.0f);
	EXPECT_EQ(transform.Scale, glm::vec2(1.0f, 1.0f));
}

TEST(TransformComponentTests, GetTransform_ReturnsMatrix)
{
	TransformComponent transform;
	transform.Position = glm::vec2(5.0f, 10.0f);
	transform.Rotation = 0.0f;
	transform.Scale = glm::vec2(2.0f, 2.0f);

	glm::mat4 matrix = transform.GetTransform();

	// Check translation
	EXPECT_FLOAT_EQ(matrix[3][0], 5.0f);
	EXPECT_FLOAT_EQ(matrix[3][1], 10.0f);
	EXPECT_FLOAT_EQ(matrix[3][2], 0.0f);

	// Check scale (no rotation)
	EXPECT_FLOAT_EQ(matrix[0][0], 2.0f);
	EXPECT_FLOAT_EQ(matrix[1][1], 2.0f);
}

TEST(TransformComponentTests, GetTransform_CachesResult)
{
	TransformComponent transform;
	transform.Position = glm::vec2(1.0f, 2.0f);
	transform.Dirty = true;

	glm::mat4 first = transform.GetTransform();
	EXPECT_FALSE(transform.Dirty);

	glm::mat4 second = transform.GetTransform();
	EXPECT_EQ(first, second);
}

TEST(TransformComponentTests, DirtyFlag_SetOnChange)
{
	TransformComponent transform;
	transform.GetTransform(); // Clear dirty flag
	EXPECT_FALSE(transform.Dirty);

	transform.Position = glm::vec2(5.0f, 5.0f);
	transform.Dirty = true; // Manual set (in real use, would be done automatically)
	
	EXPECT_TRUE(transform.Dirty);
}

TEST(TransformComponentTests, GetTransform_WithRotation)
{
	TransformComponent transform;
	transform.Position = glm::vec2(0.0f, 0.0f);
	transform.Rotation = glm::radians(90.0f); // 90 degrees
	transform.Scale = glm::vec2(1.0f, 1.0f);

	glm::mat4 matrix = transform.GetTransform();

	// After 90 degree rotation around Z, X axis should point to Y
	EXPECT_NEAR(matrix[0][0], 0.0f, 0.001f);
	EXPECT_NEAR(matrix[0][1], 1.0f, 0.001f);
	EXPECT_NEAR(matrix[1][0], -1.0f, 0.001f);
	EXPECT_NEAR(matrix[1][1], 0.0f, 0.001f);
}

// ============================================================================
// HierarchyComponent Tests
// ============================================================================

TEST(HierarchyComponentTests, DefaultConstruction)
{
	HierarchyComponent hierarchy;

	EXPECT_EQ(hierarchy.ParentUUID, 0);
}

TEST(HierarchyComponentTests, ConstructionWithParent)
{
	HierarchyComponent hierarchy(12345);

	EXPECT_EQ(hierarchy.ParentUUID, 12345);
}

TEST(HierarchyComponentTests, CopyConstruction)
{
	HierarchyComponent original(98765);
	HierarchyComponent copy = original;

	EXPECT_EQ(copy.ParentUUID, 98765);
}

// ============================================================================
// VelocityComponent Tests
// ============================================================================

TEST(VelocityComponentTests, DefaultConstruction)
{
	VelocityComponent velocity;

	EXPECT_EQ(velocity.Velocity, glm::vec2(0.0f, 0.0f));
	EXPECT_EQ(velocity.Acceleration, glm::vec2(0.0f, 0.0f));
	EXPECT_FLOAT_EQ(velocity.Drag, 0.0f);
	EXPECT_FLOAT_EQ(velocity.MaxSpeed, 1000.0f);
}

TEST(VelocityComponentTests, VelocityConstruction)
{
	VelocityComponent velocity(glm::vec2(100.0f, 50.0f));

	EXPECT_EQ(velocity.Velocity, glm::vec2(100.0f, 50.0f));
}

TEST(VelocityComponentTests, CopyConstruction)
{
	VelocityComponent original;
	original.Velocity = glm::vec2(10.0f, 20.0f);
	original.Acceleration = glm::vec2(0.0f, -9.8f);
	original.Drag = 0.5f;
	original.MaxSpeed = 500.0f;

	VelocityComponent copy = original;

	EXPECT_EQ(copy.Velocity, glm::vec2(10.0f, 20.0f));
	EXPECT_EQ(copy.Acceleration, glm::vec2(0.0f, -9.8f));
	EXPECT_FLOAT_EQ(copy.Drag, 0.5f);
	EXPECT_FLOAT_EQ(copy.MaxSpeed, 500.0f);
}

// ============================================================================
// ColliderComponent Tests
// ============================================================================

TEST(ColliderComponentTests, DefaultConstruction)
{
	ColliderComponent collider;

	EXPECT_EQ(collider.Type, ColliderType::Circle);
	EXPECT_FLOAT_EQ(collider.Radius, 0.5f);
	EXPECT_EQ(collider.Offset, glm::vec2(0.0f, 0.0f));
	EXPECT_FALSE(collider.IsSensor);
}

TEST(ColliderComponentTests, CircleFactory)
{
	ColliderComponent collider = ColliderComponent::Circle(2.0f);

	EXPECT_EQ(collider.Type, ColliderType::Circle);
	EXPECT_FLOAT_EQ(collider.Radius, 2.0f);
}

TEST(ColliderComponentTests, BoxFactory)
{
	ColliderComponent collider = ColliderComponent::Box(glm::vec2(1.0f, 2.0f));

	EXPECT_EQ(collider.Type, ColliderType::Box);
	EXPECT_EQ(collider.HalfExtents, glm::vec2(1.0f, 2.0f));
}

TEST(ColliderComponentTests, MaterialProperties)
{
	ColliderComponent collider;
	collider.Density = 2.0f;
	collider.Friction = 0.8f;
	collider.Restitution = 0.5f;

	EXPECT_FLOAT_EQ(collider.Density, 2.0f);
	EXPECT_FLOAT_EQ(collider.Friction, 0.8f);
	EXPECT_FLOAT_EQ(collider.Restitution, 0.5f);
}

TEST(ColliderComponentTests, CollisionFiltering)
{
	ColliderComponent collider;
	collider.CategoryBits = 0x0002;
	collider.MaskBits = 0x0004;
	collider.GroupIndex = -1;

	EXPECT_EQ(collider.CategoryBits, 0x0002);
	EXPECT_EQ(collider.MaskBits, 0x0004);
	EXPECT_EQ(collider.GroupIndex, -1);
}

TEST(ColliderComponentTests, SensorFlag)
{
	ColliderComponent collider;
	collider.IsSensor = true;

	EXPECT_TRUE(collider.IsSensor);
}

// ============================================================================
// SpriteComponent Tests
// ============================================================================

TEST(SpriteComponentTests, DefaultConstruction)
{
	SpriteComponent sprite;

	EXPECT_EQ(sprite.Texture, nullptr);
	EXPECT_EQ(sprite.Color, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	EXPECT_EQ(sprite.Size, glm::vec2(1.0f, 1.0f));
	EXPECT_EQ(sprite.TexCoordMin, glm::vec2(0.0f, 0.0f));
	EXPECT_EQ(sprite.TexCoordMax, glm::vec2(1.0f, 1.0f));
	EXPECT_FLOAT_EQ(sprite.ZIndex, 0.0f);
	EXPECT_FALSE(sprite.FlipX);
	EXPECT_FALSE(sprite.FlipY);
}

TEST(SpriteComponentTests, ColorConstruction)
{
	SpriteComponent sprite(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

	EXPECT_EQ(sprite.Color, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	EXPECT_EQ(sprite.Texture, nullptr);
}

TEST(SpriteComponentTests, FlipSettings)
{
	SpriteComponent sprite;
	sprite.FlipX = true;
	sprite.FlipY = true;

	EXPECT_TRUE(sprite.FlipX);
	EXPECT_TRUE(sprite.FlipY);
}

TEST(SpriteComponentTests, TextureCoordinates)
{
	SpriteComponent sprite;
	// Sprite sheet: second frame in a 4x4 grid
	sprite.TexCoordMin = glm::vec2(0.25f, 0.0f);
	sprite.TexCoordMax = glm::vec2(0.5f, 0.25f);

	EXPECT_EQ(sprite.TexCoordMin, glm::vec2(0.25f, 0.0f));
	EXPECT_EQ(sprite.TexCoordMax, glm::vec2(0.5f, 0.25f));
}

TEST(SpriteComponentTests, ZIndexSorting)
{
	SpriteComponent background;
	background.ZIndex = -10.0f;

	SpriteComponent foreground;
	foreground.ZIndex = 10.0f;

	EXPECT_LT(background.ZIndex, foreground.ZIndex);
}

// ============================================================================
// CameraComponent Tests
// ============================================================================

TEST(CameraComponentTests, DefaultConstruction)
{
	CameraComponent camera;

	EXPECT_FLOAT_EQ(camera.OrthographicSize, 10.0f);
	EXPECT_FLOAT_EQ(camera.NearClip, -1.0f);
	EXPECT_FLOAT_EQ(camera.FarClip, 1.0f);
	EXPECT_TRUE(camera.Primary);
	EXPECT_FALSE(camera.FixedAspectRatio);
}

TEST(CameraComponentTests, SizeConstruction)
{
	CameraComponent camera(20.0f);

	EXPECT_FLOAT_EQ(camera.OrthographicSize, 20.0f);
}

TEST(CameraComponentTests, GetProjectionMatrix)
{
	CameraComponent camera;
	camera.OrthographicSize = 10.0f;
	float aspectRatio = 16.0f / 9.0f;

	glm::mat4 projection = camera.GetProjectionMatrix(aspectRatio);

	// Verify it's a valid orthographic projection
	EXPECT_NE(projection, glm::mat4(1.0f));
	// Check that projection scales correctly
	float expectedHalfWidth = 10.0f * aspectRatio * 0.5f;
	float expectedHalfHeight = 10.0f * 0.5f;
	
	// For orthographic projection: [0][0] = 2 / (right - left)
	EXPECT_NEAR(projection[0][0], 2.0f / (2.0f * expectedHalfWidth), 0.001f);
	// [1][1] = 2 / (top - bottom)
	EXPECT_NEAR(projection[1][1], 2.0f / (2.0f * expectedHalfHeight), 0.001f);
}

TEST(CameraComponentTests, GetViewMatrix)
{
	CameraComponent camera;
	glm::vec2 position(10.0f, 5.0f);
	float rotation = 0.0f;

	glm::mat4 view = camera.GetViewMatrix(position, rotation);

	// View matrix is inverse of transform
	// Camera at (10, 5) means world should be translated by (-10, -5)
	EXPECT_NEAR(view[3][0], -10.0f, 0.001f);
	EXPECT_NEAR(view[3][1], -5.0f, 0.001f);
}

TEST(CameraComponentTests, GetViewMatrix_WithRotation)
{
	CameraComponent camera;
	glm::vec2 position(0.0f, 0.0f);
	float rotation = glm::radians(90.0f);

	glm::mat4 view = camera.GetViewMatrix(position, rotation);

	// With 90 degree rotation, the view matrix should rotate world -90 degrees
	EXPECT_NE(view, glm::mat4(1.0f));
}

TEST(CameraComponentTests, PrimaryFlag)
{
	CameraComponent camera1;
	camera1.Primary = true;

	CameraComponent camera2;
	camera2.Primary = false;

	EXPECT_TRUE(camera1.Primary);
	EXPECT_FALSE(camera2.Primary);
}
