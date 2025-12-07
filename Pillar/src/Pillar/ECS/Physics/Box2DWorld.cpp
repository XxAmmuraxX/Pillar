#include "Box2DWorld.h"

namespace Pillar {

	Box2DWorld::Box2DWorld(const glm::vec2& gravity)
		: m_World(std::make_unique<b2World>(b2Vec2(gravity.x, gravity.y)))
	{
	}

	Box2DWorld::~Box2DWorld()
	{
		// b2World destructor automatically cleans up all bodies, fixtures, and joints
	}

	void Box2DWorld::Step(float timeStep, int32_t velocityIterations, int32_t positionIterations)
	{
		m_World->Step(timeStep, velocityIterations, positionIterations);
	}

	void Box2DWorld::SetGravity(const glm::vec2& gravity)
	{
		m_World->SetGravity(b2Vec2(gravity.x, gravity.y));
	}

	glm::vec2 Box2DWorld::GetGravity() const
	{
		b2Vec2 gravity = m_World->GetGravity();
		return glm::vec2(gravity.x, gravity.y);
	}

} // namespace Pillar
