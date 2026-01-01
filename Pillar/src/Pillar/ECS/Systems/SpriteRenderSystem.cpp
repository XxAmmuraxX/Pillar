#include "SpriteRenderSystem.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/Renderer/Renderer2DBackend.h"
#include "Pillar/Logger.h"
#include <algorithm>

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

				// Then by Z-order
				return spriteA.ZIndex < spriteB.ZIndex;
			});

		// Render each sprite (batch renderer accumulates internally)
		for (auto entity : sortedEntities)
		{
			auto& transform = view.get<TransformComponent>(entity);
			auto& sprite = view.get<SpriteComponent>(entity);

			RenderSprite(transform, sprite);
		}
	}

	void SpriteRenderSystem::RenderSprite(const TransformComponent& transform,
		const SpriteComponent& sprite)
	{
		Renderer2DBackend::DrawSprite(transform, sprite);
	}

} // namespace Pillar
