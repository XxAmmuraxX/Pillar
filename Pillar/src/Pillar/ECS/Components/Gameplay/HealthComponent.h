#pragma once

#include <algorithm>
#include <cstdint>

namespace Pillar {

    /**
     * @brief Component for entity health management
     * 
     * Provides common health functionality including damage, healing,
     * death detection, and temporary invulnerability.
     * 
     * Usage:
     * @code
     * auto& health = entity.AddComponent<HealthComponent>(100.0f, 100.0f);
     * health.TakeDamage(25.0f);  // 75 HP remaining
     * health.Heal(10.0f);         // 85 HP remaining
     * if (health.IsDead) { ... }
     * @endcode
     */
    struct HealthComponent
    {
        float CurrentHealth = 100.0f;
        float MaxHealth = 100.0f;
        bool IsDead = false;
        
        /// When true, TakeDamage() has no effect
        bool IsInvulnerable = false;
        
        /// Countdown timer for temporary invulnerability (in seconds)
        /// Set this > 0 to enable timed invulnerability
        float InvulnerabilityTimer = 0.0f;
        
        /// If true, entity should be destroyed when IsDead becomes true
        /// Set to false if you want to handle death manually (e.g., play death animation)
        bool DestroyOnDeath = false;

        HealthComponent() = default;
        HealthComponent(const HealthComponent&) = default;
        
        /**
         * @brief Construct with specific max health
         * @param maxHealth Maximum health (also sets current health)
         */
        explicit HealthComponent(float maxHealth)
            : CurrentHealth(maxHealth), MaxHealth(maxHealth) {}
        
        /**
         * @brief Construct with specific current and max health
         * @param current Current health
         * @param max Maximum health
         */
        HealthComponent(float current, float max)
            : CurrentHealth(current), MaxHealth(max) {}

        /**
         * @brief Apply damage to the entity
         * 
         * Respects invulnerability. Sets IsDead = true when health reaches 0.
         * 
         * @param amount Amount of damage to apply (positive value)
         * @return Actual damage dealt (may be 0 if invulnerable or already dead)
         */
        float TakeDamage(float amount)
        {
            if (IsInvulnerable || IsDead || amount <= 0.0f)
                return 0.0f;
            
            float actualDamage = std::min(amount, CurrentHealth);
            CurrentHealth -= actualDamage;
            
            if (CurrentHealth <= 0.0f)
            {
                CurrentHealth = 0.0f;
                IsDead = true;
            }
            
            return actualDamage;
        }
        
        /**
         * @brief Heal the entity
         * 
         * Cannot heal above MaxHealth. Cannot heal if dead.
         * 
         * @param amount Amount to heal (positive value)
         * @return Actual amount healed
         */
        float Heal(float amount)
        {
            if (IsDead || amount <= 0.0f)
                return 0.0f;
            
            float oldHealth = CurrentHealth;
            CurrentHealth = std::min(MaxHealth, CurrentHealth + amount);
            return CurrentHealth - oldHealth;
        }
        
        /**
         * @brief Get health as a percentage (0.0 to 1.0)
         * @return Health percentage, or 0 if MaxHealth is 0
         */
        float GetHealthPercent() const
        {
            return MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f;
        }
        
        /**
         * @brief Reset health to maximum and clear death state
         * 
         * Use for respawning or reviving entities.
         */
        void Revive()
        {
            CurrentHealth = MaxHealth;
            IsDead = false;
            IsInvulnerable = false;
            InvulnerabilityTimer = 0.0f;
        }
        
        /**
         * @brief Reset to full health without clearing death state
         * 
         * Use for healing to full. Will not revive a dead entity.
         */
        void RestoreToFull()
        {
            if (!IsDead)
            {
                CurrentHealth = MaxHealth;
            }
        }
        
        /**
         * @brief Set temporary invulnerability
         * 
         * @param duration How long to remain invulnerable (seconds)
         */
        void SetTemporaryInvulnerability(float duration)
        {
            IsInvulnerable = true;
            InvulnerabilityTimer = duration;
        }
        
        /**
         * @brief Update invulnerability timer (call from update loop)
         * 
         * @param deltaTime Time since last frame (seconds)
         */
        void UpdateInvulnerability(float deltaTime)
        {
            if (InvulnerabilityTimer > 0.0f)
            {
                InvulnerabilityTimer -= deltaTime;
                if (InvulnerabilityTimer <= 0.0f)
                {
                    InvulnerabilityTimer = 0.0f;
                    IsInvulnerable = false;
                }
            }
        }
        
        /**
         * @brief Check if entity is alive
         * @return true if CurrentHealth > 0 and !IsDead
         */
        bool IsAlive() const { return !IsDead && CurrentHealth > 0.0f; }
        
        /**
         * @brief Get missing health (MaxHealth - CurrentHealth)
         */
        float GetMissingHealth() const { return MaxHealth - CurrentHealth; }
    };

} // namespace Pillar
