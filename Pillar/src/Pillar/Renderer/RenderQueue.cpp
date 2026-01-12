#include "RenderQueue.h"
#include "Pillar/Logger.h"
#include <algorithm>

namespace Pillar {

    void RenderQueue::Submit(RenderLayer layer, float depth, uint32_t textureID, std::function<void()> drawFunc)
    {
        RenderCommand cmd;
        cmd.Key = SortKey(layer, depth, textureID);
        cmd.Execute = std::move(drawFunc);
        m_Commands.push_back(std::move(cmd));
    }

    void RenderQueue::Sort()
    {
        switch (m_SortMode)
        {
            case SortMode::None:
                // No sorting - render in submission order
                break;

            case SortMode::BackToFront:
                // Sort by layer first, then depth (descending for back-to-front)
                std::sort(m_Commands.begin(), m_Commands.end(), [](const RenderCommand& a, const RenderCommand& b) {
                    if (a.Key.Layer != b.Key.Layer)
                        return a.Key.Layer < b.Key.Layer;
                    return a.Key.Depth > b.Key.Depth; // Descending depth (far first)
                });
                break;

            case SortMode::FrontToBack:
                // Sort by layer first, then depth (ascending for front-to-back)
                std::sort(m_Commands.begin(), m_Commands.end(), [](const RenderCommand& a, const RenderCommand& b) {
                    if (a.Key.Layer != b.Key.Layer)
                        return a.Key.Layer < b.Key.Layer;
                    return a.Key.Depth < b.Key.Depth; // Ascending depth (near first)
                });
                break;

            case SortMode::Texture:
                // Sort by layer, then texture ID for batching
                std::sort(m_Commands.begin(), m_Commands.end(), [](const RenderCommand& a, const RenderCommand& b) {
                    if (a.Key.Layer != b.Key.Layer)
                        return a.Key.Layer < b.Key.Layer;
                    if (a.Key.TextureID != b.Key.TextureID)
                        return a.Key.TextureID < b.Key.TextureID;
                    return a.Key.Depth < b.Key.Depth; // Secondary sort by depth
                });
                break;
        }
    }

    void RenderQueue::Flush()
    {
        if (m_AutoSort && !m_Commands.empty())
        {
            Sort();
        }

        // Execute all commands in sorted order
        for (auto& cmd : m_Commands)
        {
            if (cmd.Execute)
            {
                cmd.Execute();
            }
        }

        // Clear commands after execution
        m_Commands.clear();
    }

    void RenderQueue::Clear()
    {
        m_Commands.clear();
    }

}
