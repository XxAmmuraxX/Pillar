#pragma once

#include "Pillar/Core.h"
#include "System.h"

namespace Pillar {

	// CRITICAL SYSTEM: Reads b2Body positions and writes to TransformComponent
	// MUST run AFTER PhysicsSystem and BEFORE rendering
	// This is the "Source of Truth" sync: Box2D -> ECS (one-way)
	class PIL_API PhysicsSyncSystem : public System
	{
	public:
		void OnUpdate(float deltaTime) override;

	private:
		void SyncTransformsFromBox2D();
	};

} // namespace Pillar
