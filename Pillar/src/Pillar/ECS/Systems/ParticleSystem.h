#pragma once

#include "Pillar/Core.h"
#include "System.h"
#include <glm/glm.hpp>
#include <cstdint>

namespace Pillar {

	// Forward declarations
	class ParticlePool;
	struct ColorGradient;
	struct AnimationCurve;

	/**
	 * @brief System for updating particle lifetimes and visual effects
	 * 
	 * Phase 1-3 Features:
	 * - Age particles and mark dead when lifetime expires
	 * - Interpolate size, color, rotation over time (linear or curves)
	 * - Support color gradients (Phase 3)
	 * - Support animation curves (Phase 3)
	 * - Return particles to pool for recycling
	 */
	class PIL_API ParticleSystem : public System
	{
	public:
		ParticleSystem() = default;
		virtual ~ParticleSystem() = default;

		void OnUpdate(float dt) override;

		/**
		 * @brief Set the particle pool for recycling dead particles
		 * @param pool Pointer to the ParticlePool to return dead particles to
		 */
		void SetParticlePool(ParticlePool* pool) { m_ParticlePool = pool; }

		// Statistics
		uint32_t GetActiveParticleCount() const { return m_ActiveCount; }
		uint32_t GetDeadParticleCount() const { return m_DeadCount; }

	private:
		// Interpolation helpers
		float EvaluateCurve(const AnimationCurve* curve, float t) const;
		glm::vec4 EvaluateGradient(const ColorGradient* gradient, float t) const;

		ParticlePool* m_ParticlePool = nullptr;
		uint32_t m_ActiveCount = 0;
		uint32_t m_DeadCount = 0;
	};

} // namespace Pillar
