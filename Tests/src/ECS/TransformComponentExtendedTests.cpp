// Additional TransformComponent ergonomics and utility tests
#include <gtest/gtest.h>
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>

using namespace Pillar;

TEST(TransformComponentExtendedTests, TranslateAndRotateHelpers)
{
	TransformComponent t;
	t.SetPosition(1.0f, 2.0f);
	t.Translate(3.0f, -1.0f);
	EXPECT_EQ(t.Position, glm::vec2(4.0f, 1.0f));

	float startRot = t.Rotation;
	t.Rotate(glm::half_pi<float>());
	EXPECT_FLOAT_EQ(t.Rotation, startRot + glm::half_pi<float>());

	t.RotateDegrees(90.0f);
	EXPECT_NEAR(t.Rotation, startRot + glm::half_pi<float>() + glm::half_pi<float>(), 0.0001f);
}

TEST(TransformComponentExtendedTests, ScaleHelpers)
{
	TransformComponent t;
	t.SetScale(2.0f, 3.0f);
	EXPECT_EQ(t.Scale, glm::vec2(2.0f, 3.0f));

	t.ScaleBy(0.5f);
	EXPECT_NEAR(t.Scale.x, 1.0f, 0.0001f);
	EXPECT_NEAR(t.Scale.y, 1.5f, 0.0001f);

	t.ScaleBy(glm::vec2(2.0f, 0.0f));
	EXPECT_NEAR(t.Scale.x, 2.0f, 0.0001f);
	EXPECT_NEAR(t.Scale.y, 0.0f, 0.0001f);
}

TEST(TransformComponentExtendedTests, TRSSetterAndReset)
{
	TransformComponent t;
	t.SetTRS(glm::vec2(5.0f, 6.0f), glm::quarter_pi<float>(), glm::vec2(2.0f, 2.0f));
	EXPECT_EQ(t.Position, glm::vec2(5.0f, 6.0f));
	EXPECT_NEAR(t.Rotation, glm::quarter_pi<float>(), 0.0001f);
	EXPECT_EQ(t.Scale, glm::vec2(2.0f, 2.0f));

	t.Reset();
	EXPECT_EQ(t.Position, glm::vec2(0.0f, 0.0f));
	EXPECT_FLOAT_EQ(t.Rotation, 0.0f);
	EXPECT_EQ(t.Scale, glm::vec2(1.0f, 1.0f));
}

TEST(TransformComponentExtendedTests, TransformPointAndDirection)
{
	TransformComponent t;
	t.SetTRS(glm::vec2(1.0f, 2.0f), glm::quarter_pi<float>(), glm::vec2(2.0f, 1.0f));

	glm::vec2 worldPoint = t.TransformPoint(glm::vec2(1.0f, 0.0f));
	EXPECT_NEAR(worldPoint.x, 1.0f + 2.0f * std::cos(glm::quarter_pi<float>()), 0.0001f);
	EXPECT_NEAR(worldPoint.y, 2.0f + 2.0f * std::sin(glm::quarter_pi<float>()), 0.0001f);

	glm::vec2 worldDir = t.TransformDirection(glm::vec2(1.0f, 0.0f));
	EXPECT_NEAR(worldDir.x, 2.0f * std::cos(glm::quarter_pi<float>()), 0.0001f);
	EXPECT_NEAR(worldDir.y, 2.0f * std::sin(glm::quarter_pi<float>()), 0.0001f);
}