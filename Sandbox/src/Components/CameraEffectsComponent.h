#pragma once

#include <glm/glm.hpp>

namespace Pillar {

	/**
	 * @brief Camera effects component for screenshake and other effects
	 * Part of Arena Protocol showcase
	 */
	struct CameraEffectsComponent
	{
		// Screenshake
		bool ShakeActive = false;
		float ShakeIntensity = 0.0f;     // Current intensity
		float ShakeMaxIntensity = 0.3f;  // Max offset
		float ShakeDuration = 0.0f;      // Remaining time
		float ShakeTimer = 0.0f;
		glm::vec2 ShakeOffset = glm::vec2(0.0f);
		
		// Zoom effect
		float TargetZoom = 1.0f;
		float CurrentZoom = 1.0f;
		float ZoomSpeed = 5.0f;
		
		// Flash effect
		bool FlashActive = false;
		float FlashDuration = 0.0f;
		float FlashTimer = 0.0f;
		glm::vec4 FlashColor = glm::vec4(1.0f);

		CameraEffectsComponent() = default;
		CameraEffectsComponent(const CameraEffectsComponent&) = default;

		void TriggerShake(float intensity, float duration)
		{
			ShakeActive = true;
			ShakeMaxIntensity = intensity;
			ShakeDuration = duration;
			ShakeTimer = duration;
		}

		void TriggerFlash(const glm::vec4& color, float duration)
		{
			FlashActive = true;
			FlashColor = color;
			FlashDuration = duration;
			FlashTimer = duration;
		}
	};

} // namespace Pillar
