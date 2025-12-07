#pragma once

#include "Pillar/Core.h"
#include "Pillar/ECS/System.h"

namespace Pillar {

	/**
	 * @brief System that renders all entities with SpriteComponent
	 * 
	 * Works with both BasicRenderer2D and BatchRenderer2D through
	 * the Renderer2DBackend interface. No changes needed when switching
	 * between implementations.
	 */
	class PIL_API SpriteRenderSystem : public System
	{
	public:
		SpriteRenderSystem() = default;
		~SpriteRenderSystem() override = default;

		void OnUpdate(float dt) override;

	private:
		void RenderSprite(const class TransformComponent& transform,
		                 const class SpriteComponent& sprite);
	};

} // namespace Pillar
