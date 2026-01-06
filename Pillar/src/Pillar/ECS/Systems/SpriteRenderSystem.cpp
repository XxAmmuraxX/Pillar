#include "SpriteRenderSystem.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/Renderer/Renderer2DBackend.h"
#include "Pillar/Logger.h"
#include <algorithm>

// Forward declare LayerManager to check visibility (editor only)
#ifdef PIL_EDITOR
namespace PillarEditor {
	class LayerManager;
}
#endif

namespace Pillar {

	void SpriteRenderSystem::OnUpdate(float dt)
	{
		// Collect all entities with sprite + transform
		auto view = m_Scene->GetRegistry().view<TransformComponent, SpriteComponent>();

		// Sort by (Texture, ZIndex) for optimal batching
		std::vector<entt::entity> sortedEntities(view.begin(), view.end());
		std::sort(sortedEntities.begin(), sortedEntities.end(),
			[&view](entt::entity a, entt::entity b) {
				const auto& spriteA = view.get<SpriteComponent>(a);
				const auto& spriteB = view.get<SpriteComponent>(b);

				// Sort by texture first (minimize texture swaps)
				// Null textures go first
				if (!spriteA.Texture && spriteB.Texture)
					return true;
				if (spriteA.Texture && !spriteB.Texture)
					return false;
				if (spriteA.Texture && spriteB.Texture && spriteA.Texture.get() != spriteB.Texture.get())
					return spriteA.Texture.get() < spriteB.Texture.get();

				// Then by Z-order (use computed final Z-index from layer system)
				return spriteA.GetFinalZIndex() < spriteB.GetFinalZIndex();
			});

		// Render each sprite (batch renderer accumulates internally)
		for (auto entity : sortedEntities)
		{
			auto& transform = view.get<TransformComponent>(entity);
			auto& sprite = view.get<SpriteComponent>(entity);

			// Skip invisible sprites
			if (!sprite.Visible)
				continue;

			RenderSprite(transform, sprite);
		}
	}

	void SpriteRenderSystem::RenderSprite(const TransformComponent& transform,
		const SpriteComponent& sprite)
	{
		// Debug: Log sprites with locked UVs to track coordinate flow
		if (sprite.LockUV && sprite.Texture)
		{
			PIL_CORE_INFO("📐 SpriteRenderSystem: Rendering sprite with LockUV=true, UV: ({}, {}) to ({}, {})",
				sprite.TexCoordMin.x, sprite.TexCoordMin.y,
				sprite.TexCoordMax.x, sprite.TexCoordMax.y);
		}
		Renderer2DBackend::DrawSprite(transform, sprite);
	}

} // namespace Pillar
