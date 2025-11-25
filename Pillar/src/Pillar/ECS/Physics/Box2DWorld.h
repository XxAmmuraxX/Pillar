#pragma once

#include "Pillar/Core.h"
#include <box2d/box2d.h>
#include <glm/glm.hpp>
#include <memory>

namespace Pillar {

	class PIL_API Box2DWorld
	{
	public:
		Box2DWorld(const glm::vec2& gravity = { 0.0f, -9.81f });
		~Box2DWorld();

		// No copy/move
		Box2DWorld(const Box2DWorld&) = delete;
		Box2DWorld& operator=(const Box2DWorld&) = delete;

		void Step(float timeStep, int32_t velocityIterations = 8, int32_t positionIterations = 3);

		b2World* GetWorld() { return m_World.get(); }
		const b2World* GetWorld() const { return m_World.get(); }

		void SetGravity(const glm::vec2& gravity);
		glm::vec2 GetGravity() const;

	private:
		std::unique_ptr<b2World> m_World;
	};

} // namespace Pillar
