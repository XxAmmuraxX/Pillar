#pragma once

#include <glm/glm.hpp>

namespace Game {

    enum class PowerUpType
    {
        Health,         // Restore HP
        SpeedBoost,     // Temporary move speed increase
        FireRateUp,     // Temporary fire rate increase
        DamageUp,       // Temporary damage increase
        Shield,         // Temporary invulnerability
        Magnet          // Attract nearby power-ups
    };

    struct PowerUpComponent
    {
        PowerUpType Type = PowerUpType::Health;
        float Value = 25.0f;        // Amount of effect (HP restored, speed multiplier, etc.)
        float Duration = 5.0f;      // Duration for temporary effects (seconds)
        
        // Visual bobbing effect
        float BobSpeed = 3.0f;
        float BobAmplitude = 0.1f;
        float BobTimer = 0.0f;
        glm::vec2 OriginalPosition = { 0.0f, 0.0f };

        // Helper to get color based on type
        static glm::vec4 GetColorForType(PowerUpType type)
        {
            switch (type)
            {
                case PowerUpType::Health:     return { 0.2f, 1.0f, 0.2f, 1.0f }; // Green
                case PowerUpType::SpeedBoost: return { 1.0f, 1.0f, 0.2f, 1.0f }; // Yellow
                case PowerUpType::FireRateUp: return { 1.0f, 0.5f, 0.0f, 1.0f }; // Orange
                case PowerUpType::DamageUp:   return { 1.0f, 0.2f, 0.2f, 1.0f }; // Red
                case PowerUpType::Shield:     return { 0.2f, 0.5f, 1.0f, 1.0f }; // Blue
                case PowerUpType::Magnet:     return { 1.0f, 0.2f, 1.0f, 1.0f }; // Magenta
                default:                      return { 1.0f, 1.0f, 1.0f, 1.0f }; // White
            }
        }

        // Helper to get size based on type
        static float GetSizeForType(PowerUpType type)
        {
            switch (type)
            {
                case PowerUpType::Health:     return 0.5f;
                case PowerUpType::SpeedBoost: return 0.4f;
                case PowerUpType::FireRateUp: return 0.4f;
                case PowerUpType::DamageUp:   return 0.4f;
                case PowerUpType::Shield:     return 0.6f;
                case PowerUpType::Magnet:     return 0.5f;
                default:                      return 0.5f;
            }
        }
    };

    // Component to track active power-up effects on player
    struct ActivePowerUpEffect
    {
        PowerUpType Type;
        float RemainingDuration;
        float Value;
    };

    struct PlayerBuffsComponent
    {
        std::vector<ActivePowerUpEffect> ActiveEffects;

        float GetSpeedMultiplier() const
        {
            float multiplier = 1.0f;
            for (const auto& effect : ActiveEffects)
            {
                if (effect.Type == PowerUpType::SpeedBoost)
                    multiplier *= effect.Value;
            }
            return multiplier;
        }

        float GetFireRateMultiplier() const
        {
            float multiplier = 1.0f;
            for (const auto& effect : ActiveEffects)
            {
                if (effect.Type == PowerUpType::FireRateUp)
                    multiplier *= effect.Value;
            }
            return multiplier;
        }

        float GetDamageMultiplier() const
        {
            float multiplier = 1.0f;
            for (const auto& effect : ActiveEffects)
            {
                if (effect.Type == PowerUpType::DamageUp)
                    multiplier *= effect.Value;
            }
            return multiplier;
        }

        bool HasShield() const
        {
            for (const auto& effect : ActiveEffects)
            {
                if (effect.Type == PowerUpType::Shield)
                    return true;
            }
            return false;
        }

        void UpdateEffects(float dt)
        {
            // Update timers and remove expired effects
            ActiveEffects.erase(
                std::remove_if(ActiveEffects.begin(), ActiveEffects.end(),
                    [dt](ActivePowerUpEffect& effect) {
                        effect.RemainingDuration -= dt;
                        return effect.RemainingDuration <= 0.0f;
                    }),
                ActiveEffects.end()
            );
        }

        void AddEffect(PowerUpType type, float duration, float value)
        {
            // Check if effect already exists - refresh duration
            for (auto& effect : ActiveEffects)
            {
                if (effect.Type == type)
                {
                    effect.RemainingDuration = duration;
                    effect.Value = value;
                    return;
                }
            }

            // Add new effect
            ActiveEffects.push_back({ type, duration, value });
        }
    };

} // namespace Game
