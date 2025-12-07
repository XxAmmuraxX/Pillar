#include "SpriteRenderSystem.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Rendering/SpriteComponent.h"
#include "Pillar/Renderer/Renderer2DBackend.h"
#include "Pillar/Logger.h"

namespace Pillar {

	void SpriteRenderSystem::OnUpdate(float dt)
	{
		// Get all entities with sprite + transform
		auto view = m_Scene->GetRegistry().view<TransformComponent, SpriteComponent>();

		// Render each sprite
		// NOTE: This code is the SAME regardless of which renderer backend is active!
		// Team A implements BasicRenderer, Team B implements BatchRenderer,
		// but this system doesn't need to change.
		for (auto entity : view)
		{
			auto& transform = view.get<TransformComponent>(entity);
			auto& sprite = view.get<SpriteComponent>(entity);

			RenderSprite(transform, sprite);
		}
	}

	void SpriteRenderSystem::RenderSprite(const TransformComponent& transform,
	                                     const SpriteComponent& sprite)
	{
		// Handle rotation if needed
		if (transform.Rotation != 0.0f)
		{
			Renderer2DBackend::DrawRotatedQuad(
				transform.Position,
				sprite.Size * transform.Scale,
				transform.Rotation,
				sprite.Color,
				sprite.Texture.get()
			);
		}
		else
		{
			// Use texture coordinates for sprite sheet support
			Renderer2DBackend::DrawQuad(
				glm::vec3(transform.Position, sprite.ZIndex),
				sprite.Size * transform.Scale,
				sprite.Color,
				sprite.Texture.get(),
				sprite.TexCoordMin,
				sprite.TexCoordMax
			);
		}
	}

} // namespace Pillar
