#include "BatchRenderer2D.h"
#include "Pillar/Renderer/RenderAPI.h"
#include "Platform/OpenGL/OpenGLBatchRenderer2D.h"
#include "Pillar/Logger.h"

namespace Pillar {

    std::unique_ptr<BatchRenderer2D> BatchRenderer2D::Create()
    {
        switch (RenderAPI::GetAPI())
        {
            case RendererAPI::OpenGL:
            {
                PIL_CORE_INFO("Creating OpenGLBatchRenderer2D...");
                return std::make_unique<OpenGLBatchRenderer2D>();  // Constructor calls Init()
            }
            case RendererAPI::None:
                PIL_CORE_ASSERT(false, "RendererAPI::None is not supported!");
                return nullptr;
        }

        PIL_CORE_ASSERT(false, "Unknown RendererAPI!");
        return nullptr;
    }

    void BatchRenderer2D::ResetStats()
    {
        m_Stats.DrawCalls = 0;
        m_Stats.QuadCount = 0;
        m_Stats.VertexCount = 0;
    }

} // namespace Pillar
