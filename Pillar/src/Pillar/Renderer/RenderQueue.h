#pragma once

#include "Pillar/Core.h"
#include "Pillar/Renderer/Texture.h"
#include <glm/glm.hpp>
#include <vector>
#include <functional>
#include <cstdint>

namespace Pillar {

    /**
     * @brief Render queue for sorting and batching draw commands
     * 
     * The render queue collects draw commands during scene submission
     * and sorts them for optimal rendering (minimize state changes).
     * 
     * Sorting criteria (in order of priority):
     * 1. Render layer (background, game, UI, debug)
     * 2. Depth/Z-index (for transparency sorting)
     * 3. Texture ID (for batching)
     * 4. Shader ID (rarely changes in 2D)
     */
    class PIL_API RenderQueue
    {
    public:
        /**
         * @brief Render layer enum for coarse sorting
         */
        enum class RenderLayer : uint8_t
        {
            Background = 0,  // Skybox, far background
            World = 1,       // Main game world objects
            Particles = 2,   // Particle effects
            UI = 3,          // UI elements
            Debug = 4        // Debug visualization
        };

        /**
         * @brief Sorting mode for transparent objects
         */
        enum class SortMode
        {
            None,           // No sorting (render in submission order)
            BackToFront,    // Sort by depth (far to near) - for transparency
            FrontToBack,    // Sort by depth (near to far) - for opaque with Z-test
            Texture         // Sort by texture to minimize texture switches
        };

        /**
         * @brief Render command key for sorting
         * 
         * Packed into 64-bit integer for fast comparison:
         * - Bits 63-56: Render layer (8 bits)
         * - Bits 55-32: Depth (24 bits, signed)
         * - Bits 31-0:  Texture ID (32 bits)
         */
        struct SortKey
        {
            union
            {
                struct
                {
                    uint32_t TextureID;       // Bits 0-31
                    int32_t  Depth : 24;      // Bits 32-55 (signed for negative Z)
                    uint8_t  Layer : 8;       // Bits 56-63
                };
                uint64_t Value;
            };

            SortKey() : Value(0) {}

            SortKey(RenderLayer layer, float depth, uint32_t textureID)
            {
                Layer = static_cast<uint8_t>(layer);
                // Convert float depth to fixed-point 24-bit signed int
                // Range: -8388.608 to +8388.607 (good for most 2D games)
                Depth = static_cast<int32_t>(depth * 1000.0f);
                TextureID = textureID;
            }

            // Comparison operator for sorting
            bool operator<(const SortKey& other) const
            {
                return Value < other.Value;
            }
        };

        /**
         * @brief Render command data
         */
        struct RenderCommand
        {
            SortKey Key;
            
            // Command data (could be index into vertex buffer, or direct data)
            // For now, store a function callback to execute the draw
            std::function<void()> Execute;
        };

        /**
         * @brief Add a render command to the queue
         */
        void Submit(RenderLayer layer, float depth, uint32_t textureID, std::function<void()> drawFunc);

        /**
         * @brief Sort all queued commands by their sort keys
         */
        void Sort();

        /**
         * @brief Execute all queued commands in sorted order
         */
        void Flush();

        /**
         * @brief Clear all queued commands without executing
         */
        void Clear();

        /**
         * @brief Get number of queued commands
         */
        size_t GetCommandCount() const { return m_Commands.size(); }

        /**
         * @brief Set sorting mode (for transparent vs opaque passes)
         */
        void SetSortMode(SortMode mode) { m_SortMode = mode; }

        /**
         * @brief Get current sort mode
         */
        SortMode GetSortMode() const { return m_SortMode; }

        /**
         * @brief Enable/disable automatic sorting (default: enabled)
         */
        void SetAutoSort(bool enable) { m_AutoSort = enable; }

    private:
        std::vector<RenderCommand> m_Commands;
        SortMode m_SortMode = SortMode::Texture;
        bool m_AutoSort = true;
    };

    /**
     * @brief Helper to convert Z-index to render layer enum
     */
    inline RenderQueue::RenderLayer ZIndexToLayer(float zIndex)
    {
        if (zIndex < -1000.0f)
            return RenderQueue::RenderLayer::Background;
        else if (zIndex < 0.0f)
            return RenderQueue::RenderLayer::World;
        else if (zIndex < 100.0f)
            return RenderQueue::RenderLayer::Particles;
        else if (zIndex < 1000.0f)
            return RenderQueue::RenderLayer::UI;
        else
            return RenderQueue::RenderLayer::Debug;
    }

}
