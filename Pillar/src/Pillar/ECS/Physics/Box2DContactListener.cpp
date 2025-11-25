#include "Box2DContactListener.h"
#include "Pillar/Logger.h"

namespace Pillar {

	void Box2DContactListener::BeginContact(b2Contact* contact)
	{
		PIL_CORE_TRACE("Collision Begin");
	}

	void Box2DContactListener::EndContact(b2Contact* contact)
	{
		PIL_CORE_TRACE("Collision End");
	}

	void Box2DContactListener::PreSolve(b2Contact* contact, const b2Manifold* oldManifold)
	{
		// Can modify contact here (e.g., disable collision)
	}

	void Box2DContactListener::PostSolve(b2Contact* contact, const b2ContactImpulse* impulse)
	{
		// Can read collision impulses here (e.g., for sound effects)
	}

} // namespace Pillar
