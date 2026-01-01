#pragma once

#include <glm/glm.hpp>

namespace Pillar {

	/**
	 * @brief Weapon types for player
	 */
	enum class WeaponType
	{
		Primary,    // Single shot
		Spread,     // 3-shot spread
		Rapid,      // Fast fire rate
		Rocket      // Slow, high damage
	};

	/**
	 * @brief Weapon component for entities that can shoot
	 * Part of Arena Protocol showcase
	 */
	struct WeaponComponent
	{
		WeaponType Type = WeaponType::Primary;
		
		// Firing stats
		float FireRate = 10.0f;          // Shots per second
		float FireTimer = 0.0f;          // Cooldown timer
		float Damage = 10.0f;
		float BulletSpeed = 25.0f;
		float BulletLifetime = 3.0f;
		
		// Spread weapon specifics
		int SpreadCount = 3;             // Number of bullets
		float SpreadAngle = 15.0f;       // Angle between bullets
		
		// Ammo (optional, -1 = infinite)
		int Ammo = -1;
		int MaxAmmo = -1;
		
		// Visual offset from entity center
		glm::vec2 MuzzleOffset = glm::vec2(0.5f, 0.0f);

		WeaponComponent() = default;
		WeaponComponent(const WeaponComponent&) = default;
		WeaponComponent(WeaponType type) : Type(type)
		{
			switch (type)
			{
			case WeaponType::Primary:
				FireRate = 10.0f;
				Damage = 10.0f;
				BulletSpeed = 25.0f;
				break;
			case WeaponType::Spread:
				FireRate = 3.0f;
				Damage = 8.0f;
				SpreadCount = 5;
				SpreadAngle = 10.0f;
				break;
			case WeaponType::Rapid:
				FireRate = 20.0f;
				Damage = 5.0f;
				BulletSpeed = 30.0f;
				break;
			case WeaponType::Rocket:
				FireRate = 0.5f;
				Damage = 50.0f;
				BulletSpeed = 15.0f;
				BulletLifetime = 5.0f;
				break;
			}
		}
	};

} // namespace Pillar
