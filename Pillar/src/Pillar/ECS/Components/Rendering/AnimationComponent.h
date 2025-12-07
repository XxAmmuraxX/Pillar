#pragma once

#include "Pillar/Core.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace Pillar {

	/**
	 * @brief Defines a sprite animation sequence
	 */
	struct AnimationClip
	{
		std::string Name;
		int StartFrame = 0;
		int FrameCount = 1;
		float FrameDuration = 0.1f; // seconds per frame
		bool Loop = true;
	};

	/**
	 * @brief Component for sprite animation
	 * 
	 * Works with SpriteComponent to animate texture coordinates.
	 * Managed by AnimationSystem.
	 */
	struct PIL_API AnimationComponent
	{
		std::vector<AnimationClip> Clips;
		int CurrentClipIndex = 0;
		float AnimationTime = 0.0f;
		bool Playing = true;

		// Sprite sheet layout
		int FrameWidth = 32;
		int FrameHeight = 32;
		int SheetColumns = 8;
		int SheetRows = 8;

		AnimationComponent() = default;
		AnimationComponent(const AnimationComponent&) = default;

		void Play(const std::string& clipName);
		void Stop();
		void Pause();
		AnimationClip* GetCurrentClip();
	};

} // namespace Pillar
