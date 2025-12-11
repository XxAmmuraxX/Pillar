#pragma once

#include <entt/entt.hpp>
#include <string>
#include <functional>

namespace Pillar {

	/**
	 * @brief Component for controlling animation playback on entities
	 * 
	 * This component stores the current animation state for an entity.
	 * The AnimationSystem reads this state and updates the entity's
	 * SpriteComponent to display the correct frame.
	 * 
	 * Usage:
	 * ```cpp
	 * auto entity = scene.CreateEntity("AnimatedCharacter");
	 * auto& anim = entity.AddComponent<AnimationComponent>();
	 * anim.CurrentClipName = "player_walk";
	 * anim.OnAnimationEvent = [](const std::string& event, entt::entity e) {
	 *     if (event == "footstep") PlaySound("step.wav");
	 * };
	 * ```
	 */
	struct AnimationComponent
	{
	using EventCallback = std::function<void(const std::string& eventName, entt::entity entity)>;
	using CompletionCallback = std::function<void(entt::entity entity)>;

	std::string CurrentClipName = "";    // Currently playing animation clip
	int FrameIndex = 0;                  // Current frame in the clip
	float PlaybackTime = 0.0f;          // Time elapsed in current frame
	float PlaybackSpeed = 1.0f;         // Speed multiplier (1.0 = normal)
	bool Playing = true;                 // Is animation playing or paused?
	
	EventCallback OnAnimationEvent;      // Callback for animation events
	CompletionCallback OnAnimationComplete;  // Callback when animation finishes (non-looping only)

	AnimationComponent() = default;
	AnimationComponent(const AnimationComponent&) = default;

	/**
	 * @brief Start playing an animation clip
		 * @param clipName Name of the animation to play
		 * @param restart If true, restart animation from frame 0 even if already playing this clip
		 */
		void Play(const std::string& clipName, bool restart = false)
		{
			if (CurrentClipName != clipName || restart)
			{
				CurrentClipName = clipName;
				FrameIndex = 0;
				PlaybackTime = 0.0f;
				Playing = true;
			}
		}

		/**
		 * @brief Pause the current animation
		 */
		void Pause()
		{
			Playing = false;
		}

		/**
		 * @brief Resume the paused animation
		 */
		void Resume()
		{
			Playing = true;
		}

		/**
		 * @brief Stop the animation and reset to frame 0
		 */
		void Stop()
		{
			FrameIndex = 0;
			PlaybackTime = 0.0f;
			Playing = false;
		}

		/**
		 * @brief Check if this component has an active animation
		 */
		bool HasAnimation() const
		{
			return !CurrentClipName.empty();
		}

		/**
		 * @brief Check if animation is playing (not paused)
		 */
		bool IsPlaying() const
		{
			return Playing && HasAnimation();
		}
	};

} // namespace Pillar
