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

        // Helper to get color based on type - SCRAPYARD SALVATION gritty palette
        static glm::vec4 GetColorForType(PowerUpType type)
        {
            switch (type)
            {
                case PowerUpType::Health:     return { 0.2f, 0.8f, 0.3f, 1.0f }; // MED-STIM: Muted toxic green
                case PowerUpType::SpeedBoost: return { 1.0f, 0.85f, 0.0f, 1.0f }; // ADRENALINE: Electric yellow
                case PowerUpType::FireRateUp: return { 1.0f, 0.4f, 0.0f, 1.0f }; // OVERCLOCKED: Hazard orange
                case PowerUpType::DamageUp:   return { 0.55f, 0.0f, 0.0f, 1.0f }; // HOLLOW POINTS: Dried crimson
                case PowerUpType::Shield:     return { 0.53f, 0.81f, 0.92f, 1.0f }; // SCRAP BARRIER: Welding blue
                case PowerUpType::Magnet:     return { 0.6f, 0.2f, 0.8f, 1.0f }; // SALVAGE BEACON: Toxic purple
                default:                      return { 1.0f, 1.0f, 1.0f, 1.0f }; // White
            }
        }

        // Helper to get size based on type (increased for visibility)
        static float GetSizeForType(PowerUpType type)
        {
            switch (type)
            {
                case PowerUpType::Health:     return 1.0f;   // Increased from 0.5
                case PowerUpType::SpeedBoost: return 0.9f;   // Increased from 0.4
                case PowerUpType::FireRateUp: return 0.9f;   // Increased from 0.4
                case PowerUpType::DamageUp:   return 0.9f;   // Increased from 0.4
                case PowerUpType::Shield:     return 1.1f;   // Increased from 0.6
                case PowerUpType::Magnet:     return 1.0f;   // Increased from 0.5
                default:                      return 1.0f;   // Increased from 0.5
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

        // Returns magnet range if active, 0.0f otherwise
        float GetMagnetRange() const
        {
            for (const auto& effect : ActiveEffects)
            {
                if (effect.Type == PowerUpType::Magnet)
                    return effect.Value;
            }
            return 0.0f;
        }

        bool HasMagnet() const
        {
            return GetMagnetRange() > 0.0f;
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
