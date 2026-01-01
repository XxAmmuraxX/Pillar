#pragma once

namespace Pillar {

	/**
	 * @brief Custom component for player statistics
	 * Part of Arena Protocol showcase - demonstrates extending engine with gameplay-specific components
	 */
	struct PlayerStatsComponent
	{
		float Health = 100.0f;
		float MaxHealth = 100.0f;
		int XP = 0;
		int Level = 1;
		int XPToNextLevel = 100;

		float DashCooldown = 2.0f;
		float DashCooldownTimer = 0.0f;
		bool CanDash = true;

		PlayerStatsComponent() = default;
		PlayerStatsComponent(const PlayerStatsComponent&) = default;
	};

} // namespace Pillar
