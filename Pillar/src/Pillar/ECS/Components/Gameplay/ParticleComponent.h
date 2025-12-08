#pragma once

#include <glm/glm.hpp>
#include <string>

namespace Pillar {

	// Forward declarations
	struct ColorGradient;
	struct AnimationCurve;

	/**
	 * @brief Component for particle-specific data and behavior
	 * 
	 * Phase 1-3 Features:
	 * - Lifetime and age tracking
	 * - Size/scale curves over time
	 * - Color fading and gradient transitions
	 * - Rotation curves
	 * - Texture support (Phase 3)
	 * - Animation curves (Phase 3)
	 */
	struct ParticleComponent
	{
		// === Lifetime Management ===
		float Lifetime = 1.0f;      // Total lifetime (seconds)
		float Age = 0.0f;           // Current age (seconds)
		bool Dead = false;          // Mark for cleanup

		// === Visual Effects (Basic) ===
		glm::vec2 StartSize = { 0.1f, 0.1f };
		glm::vec2 EndSize = { 0.05f, 0.05f };

		glm::vec4 StartColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		glm::vec4 EndColor = { 1.0f, 1.0f, 1.0f, 0.0f };  // Fade out

		float StartRotation = 0.0f;
		float EndRotation = 0.0f;

		// === Behavior Flags ===
		bool FadeOut = true;         // Fade alpha over lifetime
		bool ScaleOverTime = false;  // Scale from Start to End
		bool RotateOverTime = false; // Rotate from Start to End

		// === Phase 3: Advanced Features ===
		std::string TexturePath;     // Texture to use (empty = default white square)
		bool UseColorGradient = false; // Use gradient instead of StartColor->EndColor
		ColorGradient* ColorGradientPtr = nullptr; // Gradient data (if UseColorGradient)
		AnimationCurve* SizeCurve = nullptr;       // Non-linear size animation
		AnimationCurve* RotationCurve = nullptr;   // Non-linear rotation animation

		ParticleComponent() = default;

		// Simple constructor
		ParticleComponent(float lifetime, const glm::vec4& startColor = { 1, 1, 1, 1 })
			: Lifetime(lifetime), StartColor(startColor), EndColor(startColor)
		{
			EndColor.a = 0.0f; // Fade to transparent
		}

		// Get normalized age (0-1)
		float GetNormalizedAge() const
		{
			return (Lifetime > 0.0f) ? (Age / Lifetime) : 1.0f;
		}

		// Check if particle should be removed
		bool ShouldRemove() const
		{
			return Dead || Age >= Lifetime;
		}
	};

} // namespace Pillar
