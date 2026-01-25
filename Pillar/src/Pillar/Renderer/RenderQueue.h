#pragma once

#include "Pillar/Core.h"
#include "Pillar/Renderer/Texture.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <type_traits>
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
            uint32_t TextureID = 0;       // Bits 0-31
            int32_t  Depth = 0;           // Bits 32-55 (signed, 24-bit range used)
            uint8_t  Layer = 0;           // Bits 56-63

            SortKey() = default;

            SortKey(RenderLayer layer, float depth, uint32_t textureID)
                : TextureID(textureID)
                , Depth(static_cast<int32_t>(depth * 1000.0f))  // Range: -2147483.648 to +2147483.647
                , Layer(static_cast<std::underlying_type_t<RenderLayer>>(layer))
            {
            }

            /**
             * @brief Get packed 64-bit value for fast comparison
             * Layout: Layer (8 bits) | Depth (24 bits) | TextureID (32 bits)
             */
            uint64_t GetPackedValue() const
            {
                // Clamp depth to 24-bit signed range for packing
                int32_t clampedDepth = std::max(-8388608, std::min(8388607, Depth));
                uint64_t value = static_cast<uint64_t>(TextureID);
                value |= (static_cast<uint64_t>(clampedDepth & 0x00FFFFFF) << 32);
                value |= (static_cast<uint64_t>(Layer) << 56);
                return value;
            }

            // Comparison operator for sorting
            bool operator<(const SortKey& other) const
            {
                return GetPackedValue() < other.GetPackedValue();
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
