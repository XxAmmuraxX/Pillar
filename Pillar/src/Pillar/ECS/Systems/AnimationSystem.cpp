#include "AnimationSystem.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Components/Rendering/AnimationComponent.h"
#include "Pillar/ECS/Components/Rendering/SpriteComponent.h"
#include "Pillar/Utils/AnimationLoader.h"
#include "Pillar/Logger.h"
#include "Pillar/Renderer/Texture.h"

namespace Pillar {

	void AnimationSystem::OnAttach(Scene* scene)
	{
		System::OnAttach(scene);
		PIL_CORE_INFO("AnimationSystem attached");
	}

	void AnimationSystem::OnDetach()
	{
		PIL_CORE_INFO("AnimationSystem detached");
		ClearLibrary();
		System::OnDetach();
	}

	void AnimationSystem::OnUpdate(float dt)
	{
		if (!m_Scene)
			return;

		auto& registry = m_Scene->GetRegistry();

		// Query all entities with both AnimationComponent and SpriteComponent
		auto view = registry.view<AnimationComponent, SpriteComponent>();

		for (auto entity : view)
		{
			UpdateAnimation(entity, dt);
		}
	}

	bool AnimationSystem::LoadAnimationClip(const std::string& filePath)
	{
		AnimationClip clip = AnimationLoader::LoadFromJSON(filePath);
		
		if (!clip.IsValid())
		{
			PIL_CORE_ERROR("Failed to load animation clip from: {0}", filePath);
			return false;
		}

		RegisterClip(clip);
		PIL_CORE_INFO("Loaded animation clip: {0} ({1} frames)", clip.Name, clip.GetFrameCount());
		return true;
	}

	void AnimationSystem::RegisterClip(const AnimationClip& clip)
	{
		if (clip.Name.empty())
		{
			PIL_CORE_WARN("Attempting to register animation clip with empty name");
			return;
		}

		m_AnimationLibrary[clip.Name] = clip;
	}

	AnimationClip* AnimationSystem::GetClip(const std::string& name)
	{
		auto it = m_AnimationLibrary.find(name);
		if (it != m_AnimationLibrary.end())
		{
			return &it->second;
		}
		return nullptr;
	}

	bool AnimationSystem::HasClip(const std::string& name) const
	{
		return m_AnimationLibrary.find(name) != m_AnimationLibrary.end();
	}

	void AnimationSystem::ClearLibrary()
	{
		m_AnimationLibrary.clear();
	}

	void AnimationSystem::UpdateAnimation(entt::entity entity, float dt)
	{
		if (!m_Scene)
			return;

		auto& registry = m_Scene->GetRegistry();
		auto& anim = registry.get<AnimationComponent>(entity);

		// Get the animation clip (even if not playing, to set initial frame)
		AnimationClip* clip = GetClip(anim.CurrentClipName);
		if (!clip || !clip->IsValid())
		{
			// Only log once per invalid clip
			static std::unordered_map<std::string, bool> s_LoggedErrors;
			if (!s_LoggedErrors[anim.CurrentClipName])
			{
				PIL_CORE_WARN("Animation clip not found: {0}", anim.CurrentClipName);
				s_LoggedErrors[anim.CurrentClipName] = true;
			}
			return;
		}

		// Ensure frame index is valid
		if (anim.FrameIndex >= clip->GetFrameCount())
		{
			anim.FrameIndex = 0;
			anim.PlaybackTime = 0.0f;
		}

		// Get current frame
		const AnimationFrame& currentFrame = clip->Frames[anim.FrameIndex];

		// ALWAYS update sprite rendering (even when paused, to show correct frame)
		UpdateSpriteFromFrame(entity, currentFrame);

		// Skip time advancement if not playing
		if (!anim.IsPlaying())
			return;

		// Advance playback time
		anim.PlaybackTime += dt * anim.PlaybackSpeed * clip->PlaybackSpeed;

		// Check if we need to advance to next frame
		if (anim.PlaybackTime >= currentFrame.Duration)
		{
			int oldFrame = anim.FrameIndex;
			AdvanceFrame(anim, *clip, entity);
			FireAnimationEvents(anim, *clip, oldFrame, entity);
		}
	}

	void AnimationSystem::AdvanceFrame(AnimationComponent& anim, AnimationClip& clip, entt::entity entity)
	{
		// Reset playback time for next frame
		anim.PlaybackTime = 0.0f;

		// Advance to next frame
		anim.FrameIndex++;

		// Handle looping or stopping at end
		if (anim.FrameIndex >= clip.GetFrameCount())
		{
			if (clip.Loop)
			{
				anim.FrameIndex = 0;  // Loop back to start
			}
			else
			{
				anim.FrameIndex = clip.GetFrameCount() - 1;  // Stay on last frame
				anim.Playing = false;  // Stop playing
				
				// Fire completion callback
				if (anim.OnAnimationComplete)
				{
					anim.OnAnimationComplete(entity);
				}
			}
		}
	}

	void AnimationSystem::UpdateSpriteFromFrame(entt::entity entity, const AnimationFrame& frame)
	{
		if (!m_Scene)
			return;

		auto& registry = m_Scene->GetRegistry();
		auto& sprite = registry.get<SpriteComponent>(entity);

		// Update texture if needed
		if (!frame.TexturePath.empty())
		{
			// Cache textures to avoid reloading every frame
			static std::unordered_map<std::string, std::shared_ptr<Texture2D>> s_TextureCache;

			auto it = s_TextureCache.find(frame.TexturePath);
			if (it == s_TextureCache.end())
			{
				// Load and cache new texture
				auto texture = Texture2D::Create(frame.TexturePath);
				s_TextureCache[frame.TexturePath] = texture;
				sprite.Texture = texture;
			}
			else
			{
				sprite.Texture = it->second;
			}
		}

		// Update UV coordinates
		sprite.TexCoordMin = frame.UVMin;
		sprite.TexCoordMax = frame.UVMax;
	}

	void AnimationSystem::FireAnimationEvents(AnimationComponent& anim, AnimationClip& clip, 
		int oldFrame, entt::entity entity)
	{
		// Fire all events associated with the frame we just left
		for (const auto& event : clip.Events)
		{
			if (event.FrameIndex == oldFrame)
			{
				if (anim.OnAnimationEvent)
				{
					anim.OnAnimationEvent(event.EventName, entity);
				}
			}
		}
	}

} // namespace Pillar
