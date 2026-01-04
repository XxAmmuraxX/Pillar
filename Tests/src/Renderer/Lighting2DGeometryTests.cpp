#include <gtest/gtest.h>

#include <cmath>

#include "Pillar/Renderer/Lighting2D.h"
#include "Pillar/Renderer/Lighting2DGeometry.h"
#include "Pillar/Renderer/OrthographicCamera.h"

using namespace Pillar;

TEST(Lighting2DGeometry, OneSidedSquareFacesSingleEdge)
{
    Lighting2DGeometry::Light2D light;
    light.Position = { -2.0f, 0.0f };
    light.Radius = 5.0f;

    Lighting2DGeometry::ShadowCaster2D caster;
    caster.Closed = true;
    caster.TwoSided = false;
    caster.WorldPoints = {
        { -1.0f, -1.0f },
        {  1.0f, -1.0f },
        {  1.0f,  1.0f },
        { -1.0f,  1.0f },
    };

    std::vector<glm::vec2> tris;
    Lighting2DGeometry::BuildShadowVolumeTriangles(light, caster, tris);

    // With light on the left, only the left edge should face it for a CCW quad.
    EXPECT_EQ(tris.size(), 6u); // 2 triangles * 3 vertices

    // Ensure we extrude a little beyond the light radius to reduce popping.
    // For this one-edge case, tris[1] is p1 and tris[2] is p1e.
    const float dx = tris[2].x - tris[1].x;
    const float dy = tris[2].y - tris[1].y;
    const float extrudeLen = std::sqrt(dx * dx + dy * dy);
    EXPECT_NEAR(extrudeLen, light.Radius * 1.1f, 1e-4f);
}

TEST(Lighting2DGeometry, OneSidedSquareCWAlsoFacesSingleEdge)
{
    Lighting2DGeometry::Light2D light;
    light.Position = { -2.0f, 0.0f };
    light.Radius = 5.0f;

    Lighting2DGeometry::ShadowCaster2D caster;
    caster.Closed = true;
    caster.TwoSided = false;
    // Same square but with CW winding.
    caster.WorldPoints = {
        { -1.0f, -1.0f },
        { -1.0f,  1.0f },
        {  1.0f,  1.0f },
        {  1.0f, -1.0f },
    };

    std::vector<glm::vec2> tris;
    Lighting2DGeometry::BuildShadowVolumeTriangles(light, caster, tris);

    // Mixed winding should not break one-sided casting.
    EXPECT_EQ(tris.size(), 6u);
}

TEST(Lighting2DGeometry, TwoSidedSquareGeneratesAllEdges)
{
    Lighting2DGeometry::Light2D light;
    light.Position = { -2.0f, 0.0f };
    light.Radius = 5.0f;

    Lighting2DGeometry::ShadowCaster2D caster;
    caster.Closed = true;
    caster.TwoSided = true;
    caster.WorldPoints = {
        { -1.0f, -1.0f },
        {  1.0f, -1.0f },
        {  1.0f,  1.0f },
        { -1.0f,  1.0f },
    };

    std::vector<glm::vec2> tris;
    Lighting2DGeometry::BuildShadowVolumeTriangles(light, caster, tris);

    // 4 edges * 2 triangles per edge * 3 vertices.
    EXPECT_EQ(tris.size(), 24u);
}

TEST(Lighting2DGeometry, IsCasterInRangeRejectsFarCaster)
{
    Lighting2DGeometry::Light2D light;
    light.Position = { 0.0f, 0.0f };
    light.Radius = 1.0f;

    Lighting2DGeometry::ShadowCaster2D caster;
    caster.Closed = true;
    caster.WorldPoints = { { 10.0f, 10.0f }, { 11.0f, 10.0f }, { 11.0f, 11.0f } };

    EXPECT_FALSE(Lighting2DGeometry::IsCasterInRange(light, caster));
}

TEST(Lighting2D, ComputeScissorRectConservativeAndClamped)
{
    OrthographicCamera cam(-10.0f, 10.0f, -10.0f, 10.0f);

    auto rect = Lighting2D::ComputeScissorRect(
        cam.GetViewProjectionMatrix(),
        { 0.0f, 0.0f },
        5.0f,
        100,
        100
    );

    ASSERT_TRUE(rect.Valid);
    EXPECT_GE(rect.X, 0);
    EXPECT_GE(rect.Y, 0);
    EXPECT_LE(rect.X + rect.Width, 100);
    EXPECT_LE(rect.Y + rect.Height, 100);

    // For this camera/viewport, world [-5..5] maps to pixel [25..75]
    EXPECT_NEAR((float)rect.X, 25.0f, 2.0f);
    EXPECT_NEAR((float)(rect.X + rect.Width), 75.0f, 2.0f);
}
