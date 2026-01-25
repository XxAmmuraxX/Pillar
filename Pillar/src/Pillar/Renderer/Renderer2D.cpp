#include "Renderer2D.h"
#include "Pillar/Renderer/BatchRenderer2D.h"
#include "Pillar/Renderer/RenderCommand.h"
#include "Pillar/Renderer/TextureAtlas.h"
#include "Pillar/Logger.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Rendering/SpriteComponent.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <glm/gtc/constants.hpp>

namespace Pillar {

    // Internal state - single batch renderer (automatic memory management)
    static std::unique_ptr<IRenderer2D> s_BatchRenderer = nullptr;

    void Renderer2D::Init()
    {
        PIL_CORE_INFO("Initializing Renderer2D (Batch Renderer)");

        if (s_BatchRenderer)
        {
            PIL_CORE_WARN("Renderer2D::Init() called multiple times! Already initialized.");
            return;
        }

        s_BatchRenderer = BatchRenderer2D::Create();
        
        if (!s_BatchRenderer)
        {
            PIL_CORE_ERROR("Failed to create batch renderer! Renderer2D initialization failed.");
            return;
        }

        PIL_CORE_INFO("Renderer2D initialized successfully");
    }

    void Renderer2D::Shutdown()
    {
        PIL_CORE_INFO("Shutting down Renderer2D...");
        s_BatchRenderer.reset();  // Automatic cleanup
    }

    void Renderer2D::SetClearColor(const glm::vec4& color)
    {
        RenderCommand::SetClearColor(color);
    }

    void Renderer2D::Clear()
    {
        RenderCommand::Clear();
    }

    void Renderer2D::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    {
        RenderCommand::SetViewport(x, y, width, height);
    }

    void Renderer2D::BeginScene(const OrthographicCamera& camera)
    {
        if (!s_BatchRenderer)
        {
            PIL_CORE_ERROR("Renderer2D::BeginScene() called but Renderer2D not initialized! Call Renderer2D::Init() first.");
            return;
        }
        s_BatchRenderer->BeginScene(camera);
    }

    void Renderer2D::EndScene()
    {
        if (!s_BatchRenderer)
        {
            PIL_CORE_ERROR("Renderer2D::EndScene() called but Renderer2D not initialized! Call Renderer2D::Init() first.");
            return;
        }
        s_BatchRenderer->EndScene();
    }

    void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, 
                                    const glm::vec4& color)
    {
        if (s_BatchRenderer)
            s_BatchRenderer->DrawQuad(position, size, color);
    }

    void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, 
                                    const glm::vec4& color, const std::shared_ptr<Texture2D>& texture)
    {
        if (s_BatchRenderer)
            s_BatchRenderer->DrawQuad(position, size, color, texture.get());
    }

    void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size,
                                    const glm::vec4& color)
    {
        if (s_BatchRenderer)
            s_BatchRenderer->DrawQuad(position, size, color);
    }

    void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size,
                                    const std::shared_ptr<Texture2D>& texture)
    {
        if (s_BatchRenderer)
            s_BatchRenderer->DrawQuad(position, size, texture.get());
    }

    void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, 
                                    const glm::vec4& color, const std::shared_ptr<Texture2D>& texture,
                                    const glm::vec2& texCoordMin, const glm::vec2& texCoordMax,
                                    bool flipX, bool flipY)
    {
        if (s_BatchRenderer)
            s_BatchRenderer->DrawQuad(position, size, color, texture.get(), texCoordMin, texCoordMax, flipX, flipY);
    }

    void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                                           float rotation, const glm::vec4& color)
    {
        if (s_BatchRenderer)
            s_BatchRenderer->DrawRotatedQuad(position, size, rotation, color);
    }

    void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                                           float rotation, const glm::vec4& color, 
                                           const std::shared_ptr<Texture2D>& texture)
    {
        if (s_BatchRenderer)
            s_BatchRenderer->DrawRotatedQuad(position, size, rotation, color, texture.get());
    }

    void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size,
                                           float rotation, const glm::vec4& color)
    {
        if (s_BatchRenderer)
            s_BatchRenderer->DrawRotatedQuad(position, size, rotation, color);
    }

    void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size,
                                           float rotation, const glm::vec4& color, 
                                           const std::shared_ptr<Texture2D>& texture,
                                           const glm::vec2& texCoordMin, const glm::vec2& texCoordMax,
                                           bool flipX, bool flipY)
    {
        if (s_BatchRenderer)
            s_BatchRenderer->DrawRotatedQuad(position, size, rotation, color, texture.get(), texCoordMin, texCoordMax, flipX, flipY);
    }

    // ========================================================================
    // Texture Atlas Draw Methods
    // ========================================================================

    void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size,
                             const glm::vec4& color,
                             const std::shared_ptr<TextureAtlas>& atlas,
                             const std::string& spriteName)
    {
        DrawQuad(glm::vec3(position, 0.0f), size, color, atlas, spriteName);
    }

    void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size,
                             const glm::vec4& color,
                             const std::shared_ptr<TextureAtlas>& atlas,
                             const std::string& spriteName)
    {
        if (!atlas)
        {
            PIL_CORE_WARN("DrawQuad: Null texture atlas provided");
            return;
        }

        SubTexture subTex = atlas->GetSubTexture(spriteName);
        DrawQuad(position, size, color, atlas->GetTexture(), subTex, false, false);
    }

    void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                                     float rotation, const glm::vec4& color,
                                     const std::shared_ptr<TextureAtlas>& atlas,
                                     const std::string& spriteName)
    {
        DrawRotatedQuad(glm::vec3(position, 0.0f), size, rotation, color, atlas, spriteName);
    }

    void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size,
                                     float rotation, const glm::vec4& color,
                                     const std::shared_ptr<TextureAtlas>& atlas,
                                     const std::string& spriteName)
    {
        if (!atlas)
        {
            PIL_CORE_WARN("DrawRotatedQuad: Null texture atlas provided");
            return;
        }

        SubTexture subTex = atlas->GetSubTexture(spriteName);
        DrawRotatedQuad(position, size, rotation, color, atlas->GetTexture(), subTex, false, false);
    }

    void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size,
                             const glm::vec4& color,
                             const std::shared_ptr<Texture2D>& texture,
                             const SubTexture& subTexture,
                             bool flipX, bool flipY)
    {
        if (s_BatchRenderer)
            s_BatchRenderer->DrawQuad(position, size, color, texture.get(), subTexture.UVMin, subTexture.UVMax, flipX, flipY);
    }

    void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size,
                                     float rotation, const glm::vec4& color,
                                     const std::shared_ptr<Texture2D>& texture,
                                     const SubTexture& subTexture,
                                     bool flipX, bool flipY)
    {
        if (s_BatchRenderer)
            s_BatchRenderer->DrawRotatedQuad(position, size, rotation, color, texture.get(), subTexture.UVMin, subTexture.UVMax, flipX, flipY);
    }

    // ========================================================================
    // Debug Drawing Helpers
    // ========================================================================

    void Renderer2D::DrawLine(const glm::vec2& start, const glm::vec2& end, const glm::vec4& color, float thickness)
    {
        DrawLine(glm::vec3(start, 0.0f), glm::vec3(end, 0.0f), color, thickness);
    }

    void Renderer2D::DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color, float thickness)
    {
        if (!s_BatchRenderer || thickness <= 0.0f)
            return;

        glm::vec3 delta = end - start;
        float length = glm::length(glm::vec2(delta.x, delta.y));
        if (length <= std::numeric_limits<float>::epsilon())
            return;

        float angle = std::atan2(delta.y, delta.x);
        glm::vec3 midpoint = start + delta * 0.5f;
        s_BatchRenderer->DrawRotatedQuad(midpoint, { length, thickness }, angle, color);
    }

    void Renderer2D::DrawRect(const glm::vec2& center, const glm::vec2& size, const glm::vec4& color, float thickness)
    {
        DrawRect(glm::vec3(center, 0.0f), size, color, thickness);
    }

    void Renderer2D::DrawRect(const glm::vec3& center, const glm::vec2& size, const glm::vec4& color, float thickness)
    {
        glm::vec2 half = size * 0.5f;
        glm::vec3 bottomLeft = center + glm::vec3(-half.x, -half.y, 0.0f);
        glm::vec3 bottomRight = center + glm::vec3(half.x, -half.y, 0.0f);
        glm::vec3 topRight = center + glm::vec3(half.x, half.y, 0.0f);
        glm::vec3 topLeft = center + glm::vec3(-half.x, half.y, 0.0f);

        DrawLine(bottomLeft, bottomRight, color, thickness);
        DrawLine(bottomRight, topRight, color, thickness);
        DrawLine(topRight, topLeft, color, thickness);
        DrawLine(topLeft, bottomLeft, color, thickness);
    }

    void Renderer2D::DrawCircle(const glm::vec2& center, float radius, const glm::vec4& color, int segments, float thickness)
    {
        DrawCircle(glm::vec3(center, 0.0f), radius, color, segments, thickness);
    }

    void Renderer2D::DrawCircle(const glm::vec3& center, float radius, const glm::vec4& color, int segments, float thickness)
    {
        if (!s_BatchRenderer || radius <= 0.0f || thickness <= 0.0f)
            return;

        int clampedSegments = std::max(3, segments);
        float step = glm::two_pi<float>() / static_cast<float>(clampedSegments);

        glm::vec3 prevPoint = center + glm::vec3(radius, 0.0f, 0.0f);
        for (int i = 1; i <= clampedSegments; ++i)
        {
            float angle = step * static_cast<float>(i);
            glm::vec3 nextPoint = center + glm::vec3(std::cos(angle) * radius, std::sin(angle) * radius, 0.0f);
            DrawLine(prevPoint, nextPoint, color, thickness);
            prevPoint = nextPoint;
        }
    }

    void Renderer2D::DrawSprite(const TransformComponent& transform, const SpriteComponent& sprite)
    {
        glm::vec3 position(transform.Position, sprite.ZIndex);
        glm::vec2 size = sprite.Size * glm::vec2(transform.Scale.x, transform.Scale.y);
        bool hasTexture = sprite.Texture != nullptr;
        if (transform.Rotation != 0.0f)
        {
            if (hasTexture)
            {
                DrawRotatedQuad(
                    position,
                    size,
                    transform.Rotation,
                    sprite.Color,
                    sprite.Texture,
                    sprite.TexCoordMin,
                    sprite.TexCoordMax,
                    sprite.FlipX,
                    sprite.FlipY
                );
            }
            else
            {
                DrawRotatedQuad(
                    position,
                    size,
                    transform.Rotation,
                    sprite.Color
                );
            }
        }
        else
        {
            if (hasTexture)
            {
                DrawQuad(
                    position,
                    size,
                    sprite.Color,
                    sprite.Texture,
                    sprite.TexCoordMin,
                    sprite.TexCoordMax,
                    sprite.FlipX,
                    sprite.FlipY
                );
            }
            else
            {
                DrawQuad(position, size, sprite.Color);
            }
        }
    }

    uint32_t Renderer2D::GetDrawCallCount()
    {
        if (s_BatchRenderer)
            return s_BatchRenderer->GetDrawCallCount();
        return 0;
    }

    uint32_t Renderer2D::GetQuadCount()
    {
        if (s_BatchRenderer)
            return s_BatchRenderer->GetQuadCount();
        return 0;
    }

    Renderer2D::Renderer2DStats Renderer2D::GetStats()
    {
        Renderer2DStats stats;
        if (s_BatchRenderer)
        {
            // Cast to BatchRenderer2D to access full stats
            const auto* batchRenderer = dynamic_cast<const BatchRenderer2D*>(s_BatchRenderer.get());
            if (batchRenderer)
            {
                const auto& internalStats = batchRenderer->m_Stats;
                stats.DrawCalls = internalStats.DrawCalls;
                stats.QuadCount = internalStats.QuadCount;
                stats.VertexCount = internalStats.VertexCount;
                stats.BatchCount = internalStats.BatchCount;
                stats.TextureSwitches = internalStats.TextureSwitches;
                stats.FlushCount = internalStats.FlushCount;
                stats.BufferUploads = internalStats.BufferUploads;
                stats.TotalQuadsRendered = internalStats.TotalQuadsRendered;
                stats.TotalDrawCalls = internalStats.TotalDrawCalls;
                stats.PeakVertices = internalStats.PeakVertices;
                stats.PeakQuads = internalStats.PeakQuads;
            }
        }
        return stats;
    }

    void Renderer2D::ResetStats()
    {
        if (s_BatchRenderer)
            s_BatchRenderer->ResetStats();
    }

    Renderer2D::ScopedDepthState::ScopedDepthState(bool enableDepthTest, bool enableDepthWrite)
    {
        m_PreviousDepthTest = RenderCommand::GetDepthTest();
        m_PreviousDepthWrite = RenderCommand::GetDepthWrite();

        RenderCommand::SetDepthTest(enableDepthTest);
        RenderCommand::SetDepthWrite(enableDepthWrite);
    }

    Renderer2D::ScopedDepthState Renderer2D::ScopedDepthState::DepthWriteDisabled(bool keepDepthTest)
    {
        return ScopedDepthState(keepDepthTest, false);
    }

    Renderer2D::ScopedDepthState::~ScopedDepthState()
    {
        RenderCommand::SetDepthTest(m_PreviousDepthTest);
        RenderCommand::SetDepthWrite(m_PreviousDepthWrite);
    }

    Renderer2D::ScopedRenderState::ScopedRenderState(std::optional<bool> depthTestEnabled,
                                                             std::optional<bool> depthWriteEnabled,
                                                             std::optional<bool> blendingEnabled)
    {
        m_PreviousDepthTest = RenderCommand::GetDepthTest();
        m_PreviousDepthWrite = RenderCommand::GetDepthWrite();
        m_PreviousBlending = RenderCommand::GetBlending();

        if (depthTestEnabled.has_value())
        {
            m_ChangeDepthTest = true;
            RenderCommand::SetDepthTest(*depthTestEnabled);
        }

        if (depthWriteEnabled.has_value())
        {
            m_ChangeDepthWrite = true;
            RenderCommand::SetDepthWrite(*depthWriteEnabled);
        }

        if (blendingEnabled.has_value())
        {
            m_ChangeBlending = true;
            RenderCommand::SetBlending(*blendingEnabled);
        }
    }

    Renderer2D::ScopedRenderState Renderer2D::ScopedRenderState::SpritePass(bool keepDepthTest, bool enableBlending)
    {
        return ScopedRenderState(keepDepthTest, false, enableBlending);
    }

    Renderer2D::ScopedRenderState Renderer2D::ScopedRenderState::DepthOnly(bool enableDepthWrite)
    {
        return ScopedRenderState(true, enableDepthWrite, std::nullopt);
    }

    Renderer2D::ScopedRenderState::~ScopedRenderState()
    {
        if (m_ChangeDepthTest)
        {
            RenderCommand::SetDepthTest(m_PreviousDepthTest);
        }

        if (m_ChangeDepthWrite)
        {
            RenderCommand::SetDepthWrite(m_PreviousDepthWrite);
        }

        if (m_ChangeBlending)
        {
            RenderCommand::SetBlending(m_PreviousBlending);
        }
    }

} // namespace Pillar
