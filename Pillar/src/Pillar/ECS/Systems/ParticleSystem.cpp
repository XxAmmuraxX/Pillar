#include "ParticleSystem.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/SpecializedPools.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Gameplay/ParticleComponent.h"
#include "Pillar/ECS/Components/Gameplay/ParticleAnimationCurves.h"
#include "Pillar/ECS/Components/Rendering/SpriteComponent.h"
#include "Pillar/Logger.h"
#include <glm/glm.hpp>

namespace Pillar {

	void ParticleSystem::OnUpdate(float dt)
	{
		if (!m_Scene)
			return;

		m_ActiveCount = 0;
		m_DeadCount = 0;

		// Update all particles
		auto view = m_Scene->GetRegistry().view<ParticleComponent, TransformComponent, SpriteComponent>();

		std::vector<Entity> deadParticles;
		deadParticles.reserve(100); // Pre-allocate for performance

	for (auto entityHandle : view)
	{
		Entity entity(entityHandle, m_Scene);
		auto& particle = view.get<ParticleComponent>(entityHandle);

		// Skip particles that are already dead (waiting to be returned to pool)
		if (particle.Dead)
		{
			m_DeadCount++;
			continue;
		}

		// Age the particle
		particle.Age += dt;

		// Check if particle has expired
		if (particle.Age >= particle.Lifetime)
		{
			particle.Dead = true;
			deadParticles.push_back(entity);
			m_DeadCount++;
			continue;
		}

		m_ActiveCount++;			// Update visual effects
			float t = particle.GetNormalizedAge(); // 0 to 1

			auto& transform = view.get<TransformComponent>(entityHandle);
			auto& sprite = view.get<SpriteComponent>(entityHandle);

			// === Size Interpolation ===
			if (particle.ScaleOverTime)
			{
				// Apply animation curve if available
				float curveT = t;
				if (particle.SizeCurve)
				{
					curveT = EvaluateCurve(particle.SizeCurve, t);
				}

				glm::vec2 size = glm::mix(particle.StartSize, particle.EndSize, curveT);
				transform.Scale = glm::vec3(size.x, size.y, 1.0f);
				sprite.Size = size;
				transform.Dirty = true;
			}

			// === Color Interpolation ===
			if (particle.FadeOut)
			{
				if (particle.UseColorGradient && particle.ColorGradientPtr)
				{
					// Use color gradient
					sprite.Color = EvaluateGradient(particle.ColorGradientPtr, t);
				}
				else
				{
					// Simple start->end lerp
					sprite.Color = glm::mix(particle.StartColor, particle.EndColor, t);
				}
			}

			// === Rotation Interpolation ===
			if (particle.RotateOverTime)
			{
				// Apply animation curve if available
				float curveT = t;
				if (particle.RotationCurve)
				{
					curveT = EvaluateCurve(particle.RotationCurve, t);
				}

				transform.Rotation = glm::mix(particle.StartRotation, particle.EndRotation, curveT);
				transform.Dirty = true;
			}
		}

		// Cleanup dead particles - return to pool if available, otherwise destroy
		for (auto entity : deadParticles)
		{
			if (m_ParticlePool)
			{
				m_ParticlePool->ReturnParticle(entity);
			}
			else
			{
				// Fallback: destroy if no pool is set (shouldn't happen in production)
				m_Scene->DestroyEntity(entity);
				PIL_CORE_WARN("ParticleSystem: No pool set, destroying particle entity!");
			}
		}
	}

	float ParticleSystem::EvaluateCurve(const AnimationCurve* curve, float t) const
	{
		if (!curve)
			return t; // Linear fallback

		return curve->Evaluate(t);
	}

	glm::vec4 ParticleSystem::EvaluateGradient(const ColorGradient* gradient, float t) const
	{
		if (!gradient || !gradient->IsValid())
			return glm::vec4(1.0f); // White fallback

		return gradient->Evaluate(t);
	}

} // namespace Pillar
