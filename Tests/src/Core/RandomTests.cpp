#include <gtest/gtest.h>
#include "Pillar/Utils/Random.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

using namespace Pillar::Random;

TEST(RandomTests, Float01WithinRange)
{
    for (int i = 0; i < 50; ++i)
    {
        float v = Float01();
        EXPECT_GE(v, 0.0f);
        EXPECT_LE(v, 1.0f);
    }
}

TEST(RandomTests, FloatRangeWithinBounds)
{
    for (int i = 0; i < 50; ++i)
    {
        float v = Float(-2.5f, 3.5f);
        EXPECT_GE(v, -2.5f);
        EXPECT_LE(v, 3.5f);
    }
}

TEST(RandomTests, SeedingIsDeterministic)
{
    Seed(42u);
    float a1 = Float01();
    float a2 = Float01();

    Seed(42u);
    float b1 = Float01();
    float b2 = Float01();

    EXPECT_FLOAT_EQ(a1, b1);
    EXPECT_FLOAT_EQ(a2, b2);
}

TEST(RandomTests, AngleRangesAreValid)
{
    for (int i = 0; i < 20; ++i)
    {
        float r = AngleRadians();
        EXPECT_GE(r, 0.0f);
        EXPECT_LT(r, glm::two_pi<float>());

        float d = AngleDegrees();
        EXPECT_GE(d, 0.0f);
        EXPECT_LT(d, 360.0f);
    }
}

TEST(RandomTests, Direction2DIsUnitLength)
{
    for (int i = 0; i < 20; ++i)
    {
        glm::vec2 dir = Direction2D();
        float len = glm::length(dir);
        EXPECT_NEAR(len, 1.0f, 1e-3f);
    }
}
