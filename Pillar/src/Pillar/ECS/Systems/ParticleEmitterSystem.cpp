#include "ParticleEmitterSystem.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/SpecializedPools.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Gameplay/ParticleEmitterComponent.h"
#include "Pillar/Logger.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <cstdlib>
#include <cmath>

namespace Pillar {

	void ParticleEmitterSystem::OnUpdate(float dt)
	{
		if (!m_Scene || !m_ParticlePool)
			return;

		m_EmitterCount = 0;
		m_ParticlesSpawned = 0;

		// Process all emitters
		auto view = m_Scene->GetRegistry().view<ParticleEmitterComponent, TransformComponent>();

		for (auto entityHandle : view)
		{
			auto& emitter = view.get<ParticleEmitterComponent>(entityHandle);
			auto& transform = view.get<TransformComponent>(entityHandle);

			if (!emitter.Enabled)
				continue;

			m_EmitterCount++;

			// Handle burst mode
			if (emitter.BurstMode)
			{
				if (!emitter.BurstFired)
				{
					// Spawn all particles at once
					for (int i = 0; i < emitter.BurstCount; ++i)
					{
						glm::vec2 position = CalculateEmissionPosition(transform.Position, emitter);
						glm::vec2 velocity = CalculateEmissionVelocity(emitter);

						// Randomize lifetime
						float lifetime = emitter.Lifetime + RandomRange(-emitter.LifetimeVariance, emitter.LifetimeVariance);
						lifetime = glm::max(0.1f, lifetime);

						// Randomize size
						float size = emitter.Size + RandomRange(-emitter.SizeVariance, emitter.SizeVariance);
						size = glm::max(0.01f, size);

						// Randomize color
						glm::vec4 color = emitter.StartColor;
						color.r += RandomRange(-emitter.ColorVariance.r, emitter.ColorVariance.r);
						color.g += RandomRange(-emitter.ColorVariance.g, emitter.ColorVariance.g);
						color.b += RandomRange(-emitter.ColorVariance.b, emitter.ColorVariance.b);
						color.a += RandomRange(-emitter.ColorVariance.a, emitter.ColorVariance.a);
						color = glm::clamp(color, 0.0f, 1.0f);

						// Spawn particle
						m_ParticlePool->SpawnParticle(position, velocity, color, size, lifetime);
						m_ParticlesSpawned++;
					}

					emitter.BurstFired = true;
					PIL_CORE_TRACE("ParticleEmitterSystem: Burst fired ({} particles)", emitter.BurstCount);
				}
			}
			else
			{
				// Continuous emission
				emitter.EmissionTimer += dt;

				float emissionInterval = 1.0f / emitter.EmissionRate;
				int particlesToSpawn = 0;

				// Calculate how many particles to spawn this frame
				while (emitter.EmissionTimer >= emissionInterval)
				{
					particlesToSpawn++;
					emitter.EmissionTimer -= emissionInterval;
				}

				// Spawn particles
				for (int i = 0; i < particlesToSpawn; ++i)
				{
					glm::vec2 position = CalculateEmissionPosition(transform.Position, emitter);
					glm::vec2 velocity = CalculateEmissionVelocity(emitter);

					// Randomize lifetime
					float lifetime = emitter.Lifetime + RandomRange(-emitter.LifetimeVariance, emitter.LifetimeVariance);
					lifetime = glm::max(0.1f, lifetime);

					// Randomize size
					float size = emitter.Size + RandomRange(-emitter.SizeVariance, emitter.SizeVariance);
					size = glm::max(0.01f, size);

					// Randomize color
					glm::vec4 color = emitter.StartColor;
					color.r += RandomRange(-emitter.ColorVariance.r, emitter.ColorVariance.r);
					color.g += RandomRange(-emitter.ColorVariance.g, emitter.ColorVariance.g);
					color.b += RandomRange(-emitter.ColorVariance.b, emitter.ColorVariance.b);
					color.a += RandomRange(-emitter.ColorVariance.a, emitter.ColorVariance.a);
					color = glm::clamp(color, 0.0f, 1.0f);

					// Spawn particle
					m_ParticlePool->SpawnParticle(position, velocity, color, size, lifetime);
					m_ParticlesSpawned++;
				}
			}
		}
	}

	glm::vec2 ParticleEmitterSystem::CalculateEmissionPosition(const glm::vec2& basePos, const ParticleEmitterComponent& emitter)
	{
		switch (emitter.Shape)
		{
		case EmissionShape::Point:
			return basePos;

		case EmissionShape::Circle:
		{
			// Random angle and radius for circle emission
			float angle = Random01() * 2.0f * glm::pi<float>();
			float radius = Random01() * emitter.ShapeSize.x;
			return basePos + glm::vec2(cos(angle), sin(angle)) * radius;
		}

		case EmissionShape::Box:
		{
			// Random position within box
			float x = RandomRange(-emitter.ShapeSize.x * 0.5f, emitter.ShapeSize.x * 0.5f);
			float y = RandomRange(-emitter.ShapeSize.y * 0.5f, emitter.ShapeSize.y * 0.5f);
			return basePos + glm::vec2(x, y);
		}

		case EmissionShape::Cone:
		{
			// Emit from point but with cone-shaped velocity spread
			// (position is still the point, but velocity varies more)
			return basePos;
		}

		default:
			return basePos;
		}
	}

	glm::vec2 ParticleEmitterSystem::CalculateEmissionVelocity(const ParticleEmitterComponent& emitter)
	{
		// Base direction
		glm::vec2 baseDir = glm::normalize(emitter.Direction);

		// Add random spread
		float spreadRadians = glm::radians(emitter.DirectionSpread);
		float randomAngle = RandomRange(-spreadRadians, spreadRadians);

		// Rotate base direction by random angle
		float cosA = cos(randomAngle);
		float sinA = sin(randomAngle);
		glm::vec2 rotatedDir = glm::vec2(
			baseDir.x * cosA - baseDir.y * sinA,
			baseDir.x * sinA + baseDir.y * cosA
		);

		// Add random speed variance
		float speed = emitter.Speed + RandomRange(-emitter.SpeedVariance, emitter.SpeedVariance);
		speed = glm::max(0.1f, speed);

		return rotatedDir * speed;
	}

	float ParticleEmitterSystem::RandomRange(float min, float max)
	{
		float t = Random01();
		return min + t * (max - min);
	}

	float ParticleEmitterSystem::Random01()
	{
		return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
	}

} // namespace Pillar
