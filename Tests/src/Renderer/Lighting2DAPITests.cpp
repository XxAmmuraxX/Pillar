#include <gtest/gtest.h>
#include <type_traits>

#include "Pillar/Renderer/Lighting2D.h"

using namespace Pillar;

TEST(Lighting2DAPI, HasBeginSceneViewportOverload)
{
    using Fn = void(*)(const OrthographicCamera&, uint32_t, uint32_t, const Lighting2DSettings&);
    (void)static_cast<Fn>(&Lighting2D::BeginScene);
    SUCCEED();
}

TEST(Lighting2DAPI, HasBeginSceneFramebufferOverload)
{
    using Fn = void(*)(const OrthographicCamera&, const std::shared_ptr<Framebuffer>&, const Lighting2DSettings&);
    (void)static_cast<Fn>(&Lighting2D::BeginScene);
    SUCCEED();
}
