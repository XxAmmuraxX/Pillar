#include "Renderer2DBackend.h"
#include "Pillar/Renderer/BatchRenderer2D.h"
#include "Pillar/Logger.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Rendering/SpriteComponent.h"
#include <glad/gl.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <glm/gtc/constants.hpp>

namespace Pillar {

    // Internal state - single batch renderer
    static IRenderer2D* s_BatchRenderer = nullptr;

    void Renderer2DBackend::Init()
    {
        PIL_CORE_INFO("Initializing Renderer2DBackend (Batch Renderer)");

        if (!s_BatchRenderer)
        {
            s_BatchRenderer = BatchRenderer2D::Create();
        }

        PIL_CORE_INFO("Renderer2DBackend initialized successfully");
    }

    void Renderer2DBackend::Shutdown()
    {
        PIL_CORE_INFO("Shutting down Renderer2DBackend...");

        if (s_BatchRenderer)
        {
            delete s_BatchRenderer;
            s_BatchRenderer = nullptr;
        }
    }

    void Renderer2DBackend::BeginScene(const OrthographicCamera& camera)
    {
        if (s_BatchRenderer)
            s_BatchRenderer->BeginScene(camera);
    }

    void Renderer2DBackend::EndScene()
    {
        if (s_BatchRenderer)
            s_BatchRenderer->EndScene();
    }

    void Renderer2DBackend::DrawQuad(const glm::vec2& position, const glm::vec2& size, 
                                    const glm::vec4& color)
    {
        if (s_BatchRenderer)
            s_BatchRenderer->DrawQuad(position, size, color);
    }

    void Renderer2DBackend::DrawQuad(const glm::vec2& position, const glm::vec2& size, 
                                    const glm::vec4& color, const std::shared_ptr<Texture2D>& texture)
    {
        if (s_BatchRenderer)
            s_BatchRenderer->DrawQuad(position, size, color, texture.get());
    }

    void Renderer2DBackend::DrawQuad(const glm::vec3& position, const glm::vec2& size,
                                    const glm::vec4& color)
    {
        if (s_BatchRenderer)
            s_BatchRenderer->DrawQuad(position, size, color);
    }

    void Renderer2DBackend::DrawQuad(const glm::vec3& position, const glm::vec2& size,
                                    const std::shared_ptr<Texture2D>& texture)
    {
        if (s_BatchRenderer)
            s_BatchRenderer->DrawQuad(position, size, texture.get());
    }

    void Renderer2DBackend::DrawQuad(const glm::vec3& position, const glm::vec2& size, 
                                    const glm::vec4& color, const std::shared_ptr<Texture2D>& texture,
                                    const glm::vec2& texCoordMin, const glm::vec2& texCoordMax,
                                    bool flipX, bool flipY)
    {
        if (s_BatchRenderer)
            s_BatchRenderer->DrawQuad(position, size, color, texture.get(), texCoordMin, texCoordMax, flipX, flipY);
    }

    void Renderer2DBackend::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                                           float rotation, const glm::vec4& color)
    {
        if (s_BatchRenderer)
            s_BatchRenderer->DrawRotatedQuad(position, size, rotation, color);
    }

    void Renderer2DBackend::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                                           float rotation, const glm::vec4& color, 
                                           const std::shared_ptr<Texture2D>& texture)
    {
        if (s_BatchRenderer)
            s_BatchRenderer->DrawRotatedQuad(position, size, rotation, color, texture.get());
    }

    void Renderer2DBackend::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size,
                                           float rotation, const glm::vec4& color)
    {
        if (s_BatchRenderer)
            s_BatchRenderer->DrawRotatedQuad(position, size, rotation, color);
    }

    void Renderer2DBackend::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size,
                                           float rotation, const glm::vec4& color, 
                                           const std::shared_ptr<Texture2D>& texture,
                                           const glm::vec2& texCoordMin, const glm::vec2& texCoordMax,
                                           bool flipX, bool flipY)
    {
        if (s_BatchRenderer)
            s_BatchRenderer->DrawRotatedQuad(position, size, rotation, color, texture.get(), texCoordMin, texCoordMax, flipX, flipY);
    }

    void Renderer2DBackend::DrawLine(const glm::vec2& start, const glm::vec2& end, const glm::vec4& color, float thickness)
    {
        DrawLine(glm::vec3(start, 0.0f), glm::vec3(end, 0.0f), color, thickness);
    }

    void Renderer2DBackend::DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color, float thickness)
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

    void Renderer2DBackend::DrawRect(const glm::vec2& center, const glm::vec2& size, const glm::vec4& color, float thickness)
    {
        DrawRect(glm::vec3(center, 0.0f), size, color, thickness);
    }

    void Renderer2DBackend::DrawRect(const glm::vec3& center, const glm::vec2& size, const glm::vec4& color, float thickness)
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

    void Renderer2DBackend::DrawCircle(const glm::vec2& center, float radius, const glm::vec4& color, int segments, float thickness)
    {
        DrawCircle(glm::vec3(center, 0.0f), radius, color, segments, thickness);
    }

    void Renderer2DBackend::DrawCircle(const glm::vec3& center, float radius, const glm::vec4& color, int segments, float thickness)
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

    void Renderer2DBackend::DrawSprite(const TransformComponent& transform, const SpriteComponent& sprite)
    {
        glm::vec3 position(transform.Position, sprite.ZIndex);
        glm::vec2 size = sprite.Size * glm::vec2(transform.Scale.x, transform.Scale.y);
        bool hasTexture = sprite.Texture != nullptr;
        
        // Debug: Log UV coordinates for entities with locked UVs
        if (hasTexture && sprite.LockUV)
        {
            PIL_CORE_INFO("🎨 DrawSprite (LockUV=true) - Pos({}, {}) Size({}, {}) UV: ({}, {}) to ({}, {})", 
                          position.x, position.y,
                          size.x, size.y,
                          sprite.TexCoordMin.x, sprite.TexCoordMin.y,
                          sprite.TexCoordMax.x, sprite.TexCoordMax.y);
        }

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

    uint32_t Renderer2DBackend::GetDrawCallCount()
    {
        if (s_BatchRenderer)
            return s_BatchRenderer->GetDrawCallCount();
        return 0;
    }

    uint32_t Renderer2DBackend::GetQuadCount()
    {
        if (s_BatchRenderer)
            return s_BatchRenderer->GetQuadCount();
        return 0;
    }

    void Renderer2DBackend::ResetStats()
    {
        if (s_BatchRenderer)
            s_BatchRenderer->ResetStats();
    }

    Renderer2DBackend::ScopedDepthState::ScopedDepthState(bool enableDepthTest, bool enableDepthWrite)
    {
        m_PreviousDepthTest = glIsEnabled(GL_DEPTH_TEST) == GL_TRUE;
        GLboolean depthMask = GL_TRUE;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
        m_PreviousDepthWrite = depthMask == GL_TRUE;

        if (enableDepthTest)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);

        glDepthMask(enableDepthWrite ? GL_TRUE : GL_FALSE);
    }

    Renderer2DBackend::ScopedDepthState Renderer2DBackend::ScopedDepthState::DepthWriteDisabled(bool keepDepthTest)
    {
        return ScopedDepthState(keepDepthTest, false);
    }

    Renderer2DBackend::ScopedDepthState::~ScopedDepthState()
    {
        if (m_PreviousDepthTest)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);

        glDepthMask(m_PreviousDepthWrite ? GL_TRUE : GL_FALSE);
    }

    Renderer2DBackend::ScopedRenderState::ScopedRenderState(std::optional<bool> depthTestEnabled,
                                                             std::optional<bool> depthWriteEnabled,
                                                             std::optional<bool> blendingEnabled)
    {
        m_PreviousDepthTest = glIsEnabled(GL_DEPTH_TEST) == GL_TRUE;
        GLboolean depthMask = GL_TRUE;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
        m_PreviousDepthWrite = depthMask == GL_TRUE;
        m_PreviousBlending = glIsEnabled(GL_BLEND) == GL_TRUE;

        if (depthTestEnabled.has_value())
        {
            m_ChangeDepthTest = true;
            if (*depthTestEnabled)
                glEnable(GL_DEPTH_TEST);
            else
                glDisable(GL_DEPTH_TEST);
        }

        if (depthWriteEnabled.has_value())
        {
            m_ChangeDepthWrite = true;
            glDepthMask(*depthWriteEnabled ? GL_TRUE : GL_FALSE);
        }

        if (blendingEnabled.has_value())
        {
            m_ChangeBlending = true;
            if (*blendingEnabled)
                glEnable(GL_BLEND);
            else
                glDisable(GL_BLEND);
        }
    }

    Renderer2DBackend::ScopedRenderState Renderer2DBackend::ScopedRenderState::SpritePass(bool keepDepthTest, bool enableBlending)
    {
        return ScopedRenderState(keepDepthTest, false, enableBlending);
    }

    Renderer2DBackend::ScopedRenderState Renderer2DBackend::ScopedRenderState::DepthOnly(bool enableDepthWrite)
    {
        return ScopedRenderState(true, enableDepthWrite, std::nullopt);
    }

    Renderer2DBackend::ScopedRenderState::~ScopedRenderState()
    {
        if (m_ChangeDepthTest)
        {
            if (m_PreviousDepthTest)
                glEnable(GL_DEPTH_TEST);
            else
                glDisable(GL_DEPTH_TEST);
        }

        if (m_ChangeDepthWrite)
        {
            glDepthMask(m_PreviousDepthWrite ? GL_TRUE : GL_FALSE);
        }

        if (m_ChangeBlending)
        {
            if (m_PreviousBlending)
                glEnable(GL_BLEND);
            else
                glDisable(GL_BLEND);
        }
    }

} // namespace Pillar
