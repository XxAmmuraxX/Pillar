#pragma once

#include "System.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Rendering/SpriteComponent.h"

namespace Pillar {

	/**
	 * @brief System for rendering sprites with batch optimization
	 * 
	 * Features:
	 * - Sorts sprites by texture and Z-order for optimal batching
	 * - Supports rotation, scaling, and texture coordinates
	 * - Uses Renderer2DBackend (batch or basic renderer)
	 */
	class SpriteRenderSystem : public System
	{
	public:
		SpriteRenderSystem() = default;

		void OnUpdate(float dt) override;

	private:
		void RenderSprite(const TransformComponent& transform, const SpriteComponent& sprite);
	};

} // namespace Pillar
