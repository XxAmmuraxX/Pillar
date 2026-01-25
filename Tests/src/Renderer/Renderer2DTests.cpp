#include <gtest/gtest.h>
#include <type_traits>

#include "Pillar/Renderer/Renderer2D.h"

// These tests are compile-time signature checks to ensure the new overloads
// remain available without pulling a graphics context. They validate overload
// presence for vec3 positions, default textures, and UV-capable rotated draws.

namespace Pillar {

TEST(Renderer2DOverloads, SupportsVec3ColoredQuadWithoutTexture)
{
    using Fn = void(*)(const glm::vec3&, const glm::vec2&, const glm::vec4&);
    (void)static_cast<Fn>(&Renderer2D::DrawQuad);
    SUCCEED();
}

TEST(Renderer2DOverloads, SupportsVec3TexturedQuadWithDefaultTint)
{
    using Fn = void(*)(const glm::vec3&, const glm::vec2&, const std::shared_ptr<Texture2D>&);
    (void)static_cast<Fn>(&Renderer2D::DrawQuad);
    SUCCEED();
}

TEST(Renderer2DOverloads, SupportsVec3RotatedQuadWithUVAndFlip)
{
    using Fn = void(*)(const glm::vec3&, const glm::vec2&, float, const glm::vec4&, const std::shared_ptr<Texture2D>&, const glm::vec2&, const glm::vec2&, bool, bool);
    (void)static_cast<Fn>(&Renderer2D::DrawRotatedQuad);
    SUCCEED();
}

TEST(Renderer2DScopedDepthState, ReturnsScopedType)
{
    using Scoped = Renderer2D::ScopedDepthState;
    static_assert(std::is_same_v<Scoped, decltype(Scoped::DepthWriteDisabled())>, "DepthWriteDisabled should return ScopedDepthState");
    SUCCEED();
}

} // namespace Pillar
