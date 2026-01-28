#pragma once

#include <glm/glm.hpp>

namespace Game {

    /**
     * Environmental Hazard Types
     */
    enum class HazardType
    {
        SpikeTrap,      // Damages anything that touches it periodically
        ExplosiveBarrel, // Explodes when shot, dealing area damage
        SlowField,      // Slows movement speed
        DamageZone,     // Constant damage over time
        PoisonPool      // Damages over time with lingering effect
    };

    /**
     * HazardComponent - Environmental hazards that affect gameplay
     */
    struct HazardComponent
    {
        HazardType Type = HazardType::SpikeTrap;
        
        // Damage settings
        float Damage = 10.0f;               // Damage per tick or on trigger
        float DamageInterval = 0.5f;        // Time between damage ticks
        float DamageTimer = 0.0f;           // Current timer
        float DamageRadius = 1.0f;          // Area of effect radius
        
        // Effect settings (for slow fields, etc.)
        float EffectStrength = 0.5f;        // e.g., 50% slow
        float EffectDuration = 2.0f;        // How long effect lingers
        
        // Explosive barrel settings
        float ExplosionRadius = 3.0f;
        float ExplosionDamage = 50.0f;
        bool HasExploded = false;
        float Health = 30.0f;               // Barrel can be shot
        
        // Visual
        glm::vec4 HazardColor = { 1.0f, 0.3f, 0.3f, 1.0f };
        float PulseTimer = 0.0f;            // For visual pulsing effect
        
        // State
        bool IsActive = true;
        float Lifetime = -1.0f;             // -1 = permanent
        
        HazardComponent() = default;
        
        explicit HazardComponent(HazardType type) : Type(type)
        {
            ConfigureByType();
        }
        
        void ConfigureByType()
        {
            switch (Type)
            {
                case HazardType::SpikeTrap:
                    Damage = 15.0f;
                    DamageInterval = 1.0f;
                    DamageRadius = 0.8f;
                    HazardColor = { 0.6f, 0.6f, 0.6f, 1.0f };  // Gray metal
                    break;
                    
                case HazardType::ExplosiveBarrel:
                    Health = 30.0f;
                    ExplosionRadius = 3.5f;
                    ExplosionDamage = 60.0f;
                    HazardColor = { 0.8f, 0.2f, 0.1f, 1.0f };  // Red
                    break;
                    
                case HazardType::SlowField:
                    EffectStrength = 0.4f;  // 60% slow
                    DamageRadius = 2.0f;
                    HazardColor = { 0.3f, 0.3f, 0.8f, 0.5f };  // Blue translucent
                    Damage = 0.0f;
                    break;
                    
                case HazardType::DamageZone:
                    Damage = 5.0f;
                    DamageInterval = 0.25f;
                    DamageRadius = 1.5f;
                    HazardColor = { 1.0f, 0.5f, 0.0f, 0.7f };  // Orange
                    break;
                    
                case HazardType::PoisonPool:
                    Damage = 3.0f;
                    DamageInterval = 0.5f;
                    DamageRadius = 1.2f;
                    EffectDuration = 3.0f;  // Poison lingers
                    HazardColor = { 0.2f, 0.8f, 0.2f, 0.6f };  // Green
                    break;
            }
        }
        
        bool CanDamage() const
        {
            return DamageTimer <= 0.0f && Damage > 0.0f && IsActive;
        }
        
        void ResetDamageTimer()
        {
            DamageTimer = DamageInterval;
        }
        
        void Update(float dt)
        {
            if (DamageTimer > 0.0f)
                DamageTimer -= dt;
            
            PulseTimer += dt * 3.0f;  // For visual effect
            
            if (Lifetime > 0.0f)
            {
                Lifetime -= dt;
                if (Lifetime <= 0.0f)
                    IsActive = false;
            }
        }
        
        // Get pulse factor for visual effects (0-1)
        float GetPulseFactor() const
        {
            return (std::sin(PulseTimer) + 1.0f) * 0.5f;
        }
    };

    /**
     * StatusEffectComponent - Applied to entities affected by hazards
     */
    struct StatusEffectComponent
    {
        bool IsSlowed = false;
        float SlowAmount = 0.0f;
        float SlowTimer = 0.0f;
        
        bool IsPoisoned = false;
        float PoisonDamage = 0.0f;
        float PoisonTimer = 0.0f;
        float PoisonTickTimer = 0.0f;
        
        void ApplySlow(float amount, float duration)
        {
            IsSlowed = true;
            SlowAmount = std::max(SlowAmount, amount);
            SlowTimer = std::max(SlowTimer, duration);
        }
        
        void ApplyPoison(float damagePerTick, float duration)
        {
            IsPoisoned = true;
            PoisonDamage = damagePerTick;
            PoisonTimer = std::max(PoisonTimer, duration);
        }
        
        void Update(float dt)
        {
            if (SlowTimer > 0.0f)
            {
                SlowTimer -= dt;
                if (SlowTimer <= 0.0f)
                {
                    IsSlowed = false;
                    SlowAmount = 0.0f;
                }
            }
            
            if (PoisonTimer > 0.0f)
            {
                PoisonTimer -= dt;
                PoisonTickTimer -= dt;
                
                if (PoisonTimer <= 0.0f)
                {
                    IsPoisoned = false;
                    PoisonDamage = 0.0f;
                }
            }
        }
        
        bool ShouldApplyPoisonDamage()
        {
            if (IsPoisoned && PoisonTickTimer <= 0.0f)
            {
                PoisonTickTimer = 0.5f;  // Poison ticks every 0.5s
                return true;
            }
            return false;
        }
        
        float GetSpeedMultiplier() const
        {
            return IsSlowed ? (1.0f - SlowAmount) : 1.0f;
        }
    };

} // namespace Game
