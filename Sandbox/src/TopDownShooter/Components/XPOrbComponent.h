#pragma once

#include <glm/glm.hpp>

namespace Game {

    /**
     * XPOrbComponent - Dropped by enemies, collected for experience
     */
    struct XPOrbComponent
    {
        int XPValue = 10;
        float PickupRadius = 1.5f;      // Base radius, modified by player's magnet perk
        float MagnetRadius = 5.0f;      // Radius at which orb starts moving toward player
        float MoveSpeed = 8.0f;         // Speed when attracted to player
        
        // Visual effects
        float BobTimer = 0.0f;
        float BobSpeed = 4.0f;
        float BobAmplitude = 0.1f;
        glm::vec2 OriginalPosition = { 0.0f, 0.0f };
        
        // Lifetime (orbs disappear after a while)
        float Lifetime = 30.0f;
        
        // Helper to get size based on XP value
        static float GetSizeForValue(int value)
        {
            if (value >= 50) return 0.5f;      // Large orb
            if (value >= 25) return 0.4f;      // Medium orb  
            return 0.3f;                        // Small orb
        }
        
        // Helper to get color based on XP value
        static glm::vec4 GetColorForValue(int value)
        {
            if (value >= 50) return { 1.0f, 0.8f, 0.2f, 1.0f };   // Gold
            if (value >= 25) return { 0.2f, 0.8f, 1.0f, 1.0f };   // Cyan
            return { 0.4f, 1.0f, 0.4f, 1.0f };                     // Green
        }
    };

} // namespace Game
