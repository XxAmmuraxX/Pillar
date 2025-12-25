#include <gtest/gtest.h>
#include <type_traits>

#include "Pillar/Renderer/Renderer2DBackend.h"
#include "Pillar/Renderer/BatchRenderer2D.h"
#include "Platform/OpenGL/OpenGLBatchRenderer2D.h"

// These tests are compile-time signature checks to ensure the new overloads
// remain available without pulling a graphics context. They validate overload
// presence for vec3 positions, default textures, and UV-capable rotated draws.

namespace Pillar {

TEST(Renderer2DBackendOverloads, SupportsVec3ColoredQuadWithoutTexture)
{
    using Fn = void(*)(const glm::vec3&, const glm::vec2&, const glm::vec4&);
    (void)static_cast<Fn>(&Renderer2DBackend::DrawQuad);
    SUCCEED();
}

TEST(Renderer2DBackendOverloads, SupportsVec3TexturedQuadWithDefaultTint)
{
    using Fn = void(*)(const glm::vec3&, const glm::vec2&, const std::shared_ptr<Texture2D>&);
    (void)static_cast<Fn>(&Renderer2DBackend::DrawQuad);
    SUCCEED();
}

TEST(Renderer2DBackendOverloads, SupportsVec3RotatedQuadWithUVAndFlip)
{
    using Fn = void(*)(const glm::vec3&, const glm::vec2&, float, const glm::vec4&, const std::shared_ptr<Texture2D>&, const glm::vec2&, const glm::vec2&, bool, bool);
    (void)static_cast<Fn>(&Renderer2DBackend::DrawRotatedQuad);
    SUCCEED();
}

TEST(BatchRenderer2DOverloads, SupportsVec3TexturedQuadWithUVAndFlip)
{
    using Fn = void (BatchRenderer2D::*)(const glm::vec3&, const glm::vec2&, const glm::vec4&, Texture2D*, const glm::vec2&, const glm::vec2&, bool, bool);
    (void)static_cast<Fn>(&BatchRenderer2D::DrawQuad);
    SUCCEED();
}

TEST(BatchRenderer2DOverloads, SupportsVec3RotatedQuadWithUVAndFlip)
{
    using Fn = void (BatchRenderer2D::*)(const glm::vec3&, const glm::vec2&, float, const glm::vec4&, Texture2D*, const glm::vec2&, const glm::vec2&, bool, bool);
    (void)static_cast<Fn>(&BatchRenderer2D::DrawRotatedQuad);
    SUCCEED();
}

TEST(OpenGLBatchRenderer2DOverloads, SupportsVec3TexturedQuadWithUVAndFlip)
{
    using Fn = void (OpenGLBatchRenderer2D::*)(const glm::vec3&, const glm::vec2&, const glm::vec4&, Texture2D*, const glm::vec2&, const glm::vec2&, bool, bool);
    (void)static_cast<Fn>(&OpenGLBatchRenderer2D::DrawQuad);
    SUCCEED();
}

TEST(OpenGLBatchRenderer2DOverloads, SupportsVec3RotatedQuadWithUVAndFlip)
{
    using Fn = void (OpenGLBatchRenderer2D::*)(const glm::vec3&, const glm::vec2&, float, const glm::vec4&, Texture2D*, const glm::vec2&, const glm::vec2&, bool, bool);
    (void)static_cast<Fn>(&OpenGLBatchRenderer2D::DrawRotatedQuad);
    SUCCEED();
}

TEST(Renderer2DBackendScopedDepthState, ReturnsScopedType)
{
    using Scoped = Renderer2DBackend::ScopedDepthState;
    static_assert(std::is_same<Scoped, decltype(Scoped::DepthWriteDisabled())>::value, "DepthWriteDisabled should return ScopedDepthState");
    SUCCEED();
}

} // namespace Pillar
