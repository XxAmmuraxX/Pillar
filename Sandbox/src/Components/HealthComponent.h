#pragma once

namespace Pillar {

	/**
	 * @brief Generic health component for entities
	 * Part of Arena Protocol showcase
	 */
	struct HealthComponent
	{
		float Health = 100.0f;
		float MaxHealth = 100.0f;
		bool IsDead = false;
		float InvincibilityTimer = 0.0f;  // For i-frames after hit

		HealthComponent() = default;
		HealthComponent(float maxHealth) : Health(maxHealth), MaxHealth(maxHealth) {}
		HealthComponent(const HealthComponent&) = default;

		void TakeDamage(float damage)
		{
			if (InvincibilityTimer > 0.0f || IsDead) return;
			
			Health -= damage;
			if (Health <= 0.0f)
			{
				Health = 0.0f;
				IsDead = true;
			}
		}

		void Heal(float amount)
		{
			Health = std::min(Health + amount, MaxHealth);
		}

		float GetHealthPercent() const
		{
			return MaxHealth > 0.0f ? Health / MaxHealth : 0.0f;
		}
	};

} // namespace Pillar
