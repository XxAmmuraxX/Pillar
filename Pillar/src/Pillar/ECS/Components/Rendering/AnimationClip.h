#pragma once

#include "AnimationFrame.h"
#include <vector>
#include <string>

namespace Pillar {

	/**
	 * @brief Represents a complete animation sequence
	 * 
	 * An animation clip contains:
	 * - A sequence of frames with timing information
	 * - Playback settings (loop, speed)
	 * - Animation events that fire on specific frames
	 */
	struct AnimationClip
	{
		/**
		 * @brief Event that fires when animation reaches a specific frame
		 * 
		 * Animation events allow triggering actions synchronized with animation:
		 * - Play sound effects (footsteps, weapon swings)
		 * - Spawn particles (dust, sparks, magic effects)
		 * - Deal damage (attack hit frames)
		 * - Change game state (animation complete callbacks)
		 */
		struct AnimationEvent
		{
			int FrameIndex = 0;              // Frame number to fire event on
			std::string EventName = "";      // Event identifier (e.g., "footstep", "attack_hit")

			AnimationEvent() = default;
			AnimationEvent(int frameIndex, const std::string& eventName)
				: FrameIndex(frameIndex), EventName(eventName) {}
		};

		std::string Name = "";                   // Animation identifier (e.g., "player_walk")
		std::vector<AnimationFrame> Frames;      // Frame sequence
		bool Loop = true;                        // Should animation loop?
		float PlaybackSpeed = 1.0f;             // Speed multiplier (1.0 = normal)
		std::vector<AnimationEvent> Events;      // Events triggered during playback

		AnimationClip() = default;
		AnimationClip(const std::string& name) : Name(name) {}

		/**
		 * @brief Get total duration of the animation clip
		 * @return Total duration in seconds
		 */
		float GetDuration() const
		{
			float total = 0.0f;
			for (const auto& frame : Frames)
			{
				total += frame.Duration;
			}
			return total;
		}

		/**
		 * @brief Get number of frames in animation
		 */
		int GetFrameCount() const
		{
			return static_cast<int>(Frames.size());
		}

		/**
		 * @brief Check if animation is valid (has at least one frame)
		 */
		bool IsValid() const
		{
			return !Frames.empty();
		}
	};

} // namespace Pillar
