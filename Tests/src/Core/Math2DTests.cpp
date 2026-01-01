#include <gtest/gtest.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include "Pillar/Utils/Math2D.h"

using namespace Pillar::Math2D;

TEST(Math2DTests, SafeNormalizeUsesFallbackForZero)
{
    glm::vec2 v(0.0f);
    glm::vec2 fallback(0.0f, 1.0f);
    glm::vec2 result = SafeNormalize(v, kEpsilon, fallback);
    EXPECT_FLOAT_EQ(result.x, fallback.x);
    EXPECT_FLOAT_EQ(result.y, fallback.y);
}

TEST(Math2DTests, NormalizeOrZeroReturnsZeroForTiny)
{
    glm::vec2 v(1e-7f, 0.0f);
    glm::vec2 result = NormalizeOrZero(v);
    EXPECT_FLOAT_EQ(result.x, 0.0f);
    EXPECT_FLOAT_EQ(result.y, 0.0f);
}

TEST(Math2DTests, ClampLengthCapsMagnitude)
{
    glm::vec2 v(3.0f, 4.0f); // length 5
    glm::vec2 clamped = ClampLength(v, 2.0f);
    EXPECT_NEAR(glm::length(clamped), 2.0f, 1e-4f);
}

TEST(Math2DTests, ClampLengthMinMaxKeepsWithinBounds)
{
    glm::vec2 v(0.5f, 0.0f);
    glm::vec2 clamped = ClampLengthRange(v, 1.0f, 2.0f);
    EXPECT_NEAR(glm::length(clamped), 1.0f, 1e-4f);

    glm::vec2 v2(5.0f, 0.0f);
    glm::vec2 clamped2 = ClampLengthRange(v2, 1.0f, 2.0f);
    EXPECT_NEAR(glm::length(clamped2), 2.0f, 1e-4f);
}

TEST(Math2DTests, MoveTowardsDoesNotOvershoot)
{
    glm::vec2 a(0.0f, 0.0f);
    glm::vec2 b(3.0f, 0.0f);
    glm::vec2 moved = MoveTowards(a, b, 1.0f);
    EXPECT_NEAR(moved.x, 1.0f, 1e-5f);
    EXPECT_NEAR(moved.y, 0.0f, 1e-5f);

    glm::vec2 reached = MoveTowards(a, b, 4.0f);
    EXPECT_NEAR(reached.x, 3.0f, 1e-5f);
}

TEST(Math2DTests, DistanceHelpersAgree)
{
    glm::vec2 a(0.0f, 0.0f);
    glm::vec2 b(3.0f, 4.0f);
    EXPECT_FLOAT_EQ(Distance(a, b), 5.0f);
    EXPECT_FLOAT_EQ(DistanceSq(a, b), 25.0f);
}

TEST(Math2DTests, PerpendicularsRotateCorrectly)
{
    glm::vec2 v(2.0f, 0.0f);
    glm::vec2 left = PerpLeft(v);
    glm::vec2 right = PerpRight(v);
    EXPECT_FLOAT_EQ(left.x, 0.0f);
    EXPECT_FLOAT_EQ(left.y, 2.0f);
    EXPECT_FLOAT_EQ(right.x, 0.0f);
    EXPECT_FLOAT_EQ(right.y, -2.0f);
}

TEST(Math2DTests, RotateQuarterTurn)
{
    glm::vec2 v(1.0f, 0.0f);
    glm::vec2 rotated = Rotate(v, glm::half_pi<float>());
    EXPECT_NEAR(rotated.x, 0.0f, 1e-5f);
    EXPECT_NEAR(rotated.y, 1.0f, 1e-5f);
}

TEST(Math2DTests, ProjectHandlesDegenerate)
{
    glm::vec2 a(1.0f, 1.0f);
    glm::vec2 b(0.0f, 0.0f);
    glm::vec2 proj = Project(a, b);
    EXPECT_FLOAT_EQ(proj.x, 0.0f);
    EXPECT_FLOAT_EQ(proj.y, 0.0f);
}

TEST(Math2DTests, ReflectAcrossNormal)
{
    glm::vec2 incident(1.0f, -1.0f);
    glm::vec2 normal(0.0f, 1.0f);
    glm::vec2 reflected = Reflect(incident, normal);
    EXPECT_NEAR(reflected.x, 1.0f, 1e-5f);
    EXPECT_NEAR(reflected.y, 1.0f, 1e-5f);
}

TEST(Math2DTests, AngleBetweenAndSignedAngle)
{
    glm::vec2 a(1.0f, 0.0f);
    glm::vec2 b(0.0f, 1.0f);
    EXPECT_NEAR(AngleBetween(a, b), glm::half_pi<float>(), 1e-5f);
    EXPECT_NEAR(SignedAngle(a, b), glm::half_pi<float>(), 1e-5f);
    EXPECT_NEAR(SignedAngle(b, a), -glm::half_pi<float>(), 1e-5f);
}

TEST(Math2DTests, ComponentOperations)
{
    glm::vec2 v(2.0f, -4.0f);
    glm::vec2 clampRes = Clamp(v, glm::vec2(0.0f, -2.0f), glm::vec2(1.0f, -1.0f));
    EXPECT_FLOAT_EQ(clampRes.x, 1.0f);
    EXPECT_FLOAT_EQ(clampRes.y, -2.0f);

    glm::vec2 mulRes = Mul(glm::vec2(2.0f, 3.0f), glm::vec2(-1.0f, 0.5f));
    EXPECT_FLOAT_EQ(mulRes.x, -2.0f);
    EXPECT_FLOAT_EQ(mulRes.y, 1.5f);

    glm::vec2 divRes = DivSafe(glm::vec2(4.0f, 6.0f), glm::vec2(2.0f, 0.0f));
    EXPECT_FLOAT_EQ(divRes.x, 2.0f);
    EXPECT_FLOAT_EQ(divRes.y, 0.0f); // guarded divide
}
