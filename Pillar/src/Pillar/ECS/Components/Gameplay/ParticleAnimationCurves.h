#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace Pillar {

	/**
	 * @brief Interpolation curve types for particle animations
	 */
	enum class CurveType
	{
		Linear,      // Linear interpolation (default)
		EaseIn,      // Slow start, fast end (t^2)
		EaseOut,     // Fast start, slow end (1 - (1-t)^2)
		EaseInOut,   // Smooth S-curve
		Bounce       // Bouncing effect
	};

	/**
	 * @brief Color gradient for smooth color transitions
	 * 
	 * Defines color stops at specific lifetime points (0-1).
	 * Supports 2-5 color stops for smooth gradients.
	 */
	struct ColorGradient
	{
		struct ColorStop
		{
			float Time;        // Normalized time (0-1)
			glm::vec4 Color;   // RGBA color at this time
		};

		std::vector<ColorStop> Stops;

		ColorGradient() = default;

		// Two-color gradient (start -> end)
		ColorGradient(const glm::vec4& start, const glm::vec4& end)
		{
			Stops.push_back({ 0.0f, start });
			Stops.push_back({ 1.0f, end });
		}

		// Three-color gradient
		ColorGradient(const glm::vec4& start, const glm::vec4& mid, const glm::vec4& end)
		{
			Stops.push_back({ 0.0f, start });
			Stops.push_back({ 0.5f, mid });
			Stops.push_back({ 1.0f, end });
		}

		// Evaluate gradient at normalized time t (0-1)
		glm::vec4 Evaluate(float t) const
		{
			if (Stops.empty())
				return glm::vec4(1.0f);

			if (Stops.size() == 1)
				return Stops[0].Color;

			// Find surrounding stops
			for (size_t i = 0; i < Stops.size() - 1; ++i)
			{
				if (t >= Stops[i].Time && t <= Stops[i + 1].Time)
				{
					// Interpolate between stops
					float localT = (t - Stops[i].Time) / (Stops[i + 1].Time - Stops[i].Time);
					return glm::mix(Stops[i].Color, Stops[i + 1].Color, localT);
				}
			}

			// Beyond last stop
			return Stops.back().Color;
		}

		// Check if gradient is valid
		bool IsValid() const
		{
			return Stops.size() >= 2;
		}
	};

	/**
	 * @brief Animation curve for non-linear interpolation
	 */
	struct AnimationCurve
	{
		CurveType Type = CurveType::Linear;
		float Strength = 1.0f; // Curve intensity (for ease curves)

		AnimationCurve() = default;
		AnimationCurve(CurveType type, float strength = 1.0f)
			: Type(type), Strength(strength)
		{
		}

		// Evaluate curve at time t (0-1), returns modified t
		float Evaluate(float t) const
		{
			t = glm::clamp(t, 0.0f, 1.0f);

			switch (Type)
			{
			case CurveType::Linear:
				return t;

			case CurveType::EaseIn:
				return t * t * Strength + t * (1.0f - Strength);

			case CurveType::EaseOut:
				return 1.0f - (1.0f - t) * (1.0f - t) * Strength - (1.0f - t) * (1.0f - Strength);

			case CurveType::EaseInOut:
			{
				float t2 = t * t;
				float t3 = t2 * t;
				return (3.0f * t2 - 2.0f * t3) * Strength + t * (1.0f - Strength);
			}

			case CurveType::Bounce:
			{
				if (t < 0.5f)
					return 2.0f * t * t;
				else
					return 1.0f - 2.0f * (1.0f - t) * (1.0f - t);
			}

			default:
				return t;
			}
		}
	};

} // namespace Pillar
