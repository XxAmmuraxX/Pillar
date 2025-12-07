#pragma once

#include "Pillar/Core.h"
#include <box2d/box2d.h>

namespace Pillar {

	class PIL_API Box2DContactListener : public b2ContactListener
	{
	public:
		Box2DContactListener() = default;
		~Box2DContactListener() override = default;

		// Called when two fixtures begin to touch
		void BeginContact(b2Contact* contact) override;

		// Called when two fixtures cease to touch
		void EndContact(b2Contact* contact) override;

		// Called after collision detection, before solving collisions
		void PreSolve(b2Contact* contact, const b2Manifold* oldManifold) override;

		// Called after solving collisions
		void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse) override;
	};

} // namespace Pillar
