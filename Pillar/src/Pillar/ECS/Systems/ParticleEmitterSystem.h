#pragma once

#include "Pillar/Core.h"
#include "System.h"
#include <glm/glm.hpp>
#include <cstdint>

namespace Pillar {

	class ParticlePool; // Forward declaration

	/**
	 * @brief System that manages particle emitters and spawns particles
	 * 
	 * This system handles:
	 * - Continuous emission at specified rates
	 * - Burst emission (one-shot spawning)
	 * - Emission shape calculations (point, circle, box, cone)
	 * - Randomization of particle properties
	 * - Spawning particles via ParticlePool
	 * 
	 * Phase 2 Implementation
	 */
	class PIL_API ParticleEmitterSystem : public System
	{
	public:
		void OnUpdate(float dt) override;

		/**
		 * @brief Set the particle pool for spawning particles
		 * @param pool Pointer to the ParticlePool to spawn from
		 */
		void SetParticlePool(ParticlePool* pool) { m_ParticlePool = pool; }

		/**
		 * @brief Get number of emitters processed this frame
		 */
		uint32_t GetEmitterCount() const { return m_EmitterCount; }

		/**
		 * @brief Get number of particles spawned this frame
		 */
		uint32_t GetParticlesSpawnedThisFrame() const { return m_ParticlesSpawned; }

	private:
		/**
		 * @brief Calculate emission position based on shape
		 */
		glm::vec2 CalculateEmissionPosition(const glm::vec2& basePos, const class ParticleEmitterComponent& emitter);

		/**
		 * @brief Calculate emission velocity with randomization
		 */
		glm::vec2 CalculateEmissionVelocity(const class ParticleEmitterComponent& emitter);

		/**
		 * @brief Get random value in range [-1, 1]
		 */
		float RandomRange(float min, float max);

		/**
		 * @brief Get random float in [0, 1]
		 */
		float Random01();

	private:
		ParticlePool* m_ParticlePool = nullptr;
		uint32_t m_EmitterCount = 0;
		uint32_t m_ParticlesSpawned = 0;
	};

} // namespace Pillar
