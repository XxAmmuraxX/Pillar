#include "BatchRenderer2D.h"
#include "Pillar/Renderer/RenderAPI.h"
#include "Platform/OpenGL/OpenGLBatchRenderer2D.h"
#include "Pillar/Logger.h"

namespace Pillar {

    BatchRenderer2D* BatchRenderer2D::Create()
    {
        switch (RenderAPI::GetAPI())
        {
            case RendererAPI::OpenGL:
            {
                PIL_CORE_INFO("Creating OpenGLBatchRenderer2D...");
                return new OpenGLBatchRenderer2D();  // Constructor calls Init()
            }
            case RendererAPI::None:
                PIL_CORE_ASSERT(false, "RendererAPI::None is not supported!");
                return nullptr;
        }

        PIL_CORE_ASSERT(false, "Unknown RendererAPI!");
        return nullptr;
    }

    void BatchRenderer2D::BeginScene(const OrthographicCamera& camera)
    {
        // Default implementation (subclass can override)
    }

    void BatchRenderer2D::EndScene()
    {
        // Default implementation (subclass can override)
    }

    void BatchRenderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, 
                                  const glm::vec4& color)
    {
        // Default implementation (subclass must override)
    }

    void BatchRenderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, 
                                  const glm::vec4& color, Texture2D* texture)
    {
        // Default implementation (subclass must override)
    }

	void BatchRenderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, 
							  const glm::vec4& color, Texture2D* texture,
							  const glm::vec2& texCoordMin, const glm::vec2& texCoordMax,
							  bool flipX, bool flipY)
	{
		// Default implementation (subclass must override)
	}    void BatchRenderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                                         float rotation, const glm::vec4& color)
    {
        // Default implementation (subclass must override)
    }

    void BatchRenderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                                         float rotation, const glm::vec4& color, Texture2D* texture)
    {
        // Default implementation (subclass must override)
    }

    void BatchRenderer2D::ResetStats()
    {
        m_Stats.DrawCalls = 0;
        m_Stats.QuadCount = 0;
        m_Stats.VertexCount = 0;
    }

} // namespace Pillar
