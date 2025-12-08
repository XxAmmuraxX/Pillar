#pragma once

#include <glm/glm.hpp>
#include <string>

namespace Pillar {

	// Forward declarations
	struct ColorGradient;
	struct AnimationCurve;

	/**
	 * @brief Emission shape types for particle emitters
	 */
	enum class EmissionShape
	{
		Point,  // Single point emission
		Circle, // Emit in a circle/ring
		Box,    // Emit from rectangular area
		Cone    // Emit in a cone/spray
	};

	/**
	 * @brief Component for continuous particle emission
	 * 
	 * Phase 1-3 Features:
	 * - Emission rate control (particles/sec or burst mode)
	 * - Multiple emission shapes (point, circle, box, cone)
	 * - Position and velocity randomization
	 * - Lifetime, size, and color variance
	 * - Texture support (Phase 3)
	 * - Color gradients (Phase 3)
	 * - Animation curves (Phase 3)
	 */
	struct ParticleEmitterComponent
	{
		// === Emission Control ===
		bool Enabled = true;                   // Is emitter active?
		float EmissionRate = 10.0f;            // Particles per second
		float EmissionTimer = 0.0f;            // Internal timer for rate limiting
		bool BurstMode = false;                // One-shot emission?
		int BurstCount = 100;                  // Particles in burst
		bool BurstFired = false;               // Has burst been triggered?

		// === Emission Shape ===
		EmissionShape Shape = EmissionShape::Point;
		glm::vec2 ShapeSize = glm::vec2(1.0f); // Size for Box/Circle radius

		// === Direction & Speed ===
		glm::vec2 Direction = glm::vec2(0.0f, 1.0f); // Base emission direction
		float DirectionSpread = 30.0f;         // Angle variance (degrees)
		float Speed = 5.0f;                    // Base speed
		float SpeedVariance = 2.0f;            // Speed randomization

		// === Particle Properties ===
		float Lifetime = 2.0f;                 // Base lifetime
		float LifetimeVariance = 0.5f;         // Lifetime randomization
		float Size = 0.2f;                     // Base size
		float SizeVariance = 0.05f;            // Size randomization
		glm::vec4 StartColor = glm::vec4(1.0f);// Base color
		glm::vec4 ColorVariance = glm::vec4(0.1f, 0.1f, 0.1f, 0.0f); // Color randomization

		// === Visual Effects ===
		bool FadeOut = true;                   // Fade particles over lifetime?
		bool ScaleOverTime = false;            // Scale particles over lifetime?
		bool RotateOverTime = false;           // Rotate particles over lifetime?
		float EndScale = 0.5f;                 // End scale multiplier (if ScaleOverTime)
		float RotationSpeed = 180.0f;          // Rotation speed (degrees/sec)

		// === Gravity ===
		glm::vec2 Gravity = glm::vec2(0.0f, -2.0f); // Gravity acceleration

		// === Phase 3: Advanced Features ===
		std::string TexturePath;               // Texture for spawned particles
		bool UseColorGradient = false;         // Use gradient instead of single color
		ColorGradient* ColorGradientPtr = nullptr; // Color gradient data
		AnimationCurve* SizeCurvePtr = nullptr;    // Size animation curve
		AnimationCurve* RotationCurvePtr = nullptr; // Rotation animation curve
	};

} // namespace Pillar
