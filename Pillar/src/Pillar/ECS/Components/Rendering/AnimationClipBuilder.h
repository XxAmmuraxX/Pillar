#pragma once

#include "AnimationClip.h"
#include "AnimationFrame.h"
#include <string>
#include <vector>
#include <initializer_list>

namespace Pillar {

	/**
	 * @brief Fluent builder for creating animation clips in code
	 * 
	 * Provides a concise, readable API for defining animations programmatically
	 * without requiring JSON files.
	 * 
	 * Basic usage (individual frame textures):
	 * ```cpp
	 * auto walk = AnimationClipBuilder("Walk")
	 *     .AddFrame("walk_01.png", 0.1f)
	 *     .AddFrame("walk_02.png", 0.1f)
	 *     .AddFrame("walk_03.png", 0.1f)
	 *     .SetLooping(true)
	 *     .Build();
	 * 
	 * animSystem->RegisterClip(walk);
	 * ```
	 * 
	 * Sprite sheet usage (single texture, multiple UV regions):
	 * ```cpp
	 * auto walk = AnimationClipBuilder("Walk")
	 *     .FromSpriteSheet("character.png", 6, 1)  // 6 columns, 1 row
	 *     .SetFrameDuration(0.1f)
	 *     .SetLooping(true)
	 *     .Build();
	 * ```
	 * 
	 * With events:
	 * ```cpp
	 * auto attack = AnimationClipBuilder("Attack")
	 *     .FromSpriteSheet("attack.png", 8, 1)
	 *     .SetFrameDuration(0.05f)
	 *     .AddEvent(3, "hit")       // Fire "hit" event on frame 3
	 *     .AddEvent(7, "complete")
	 *     .SetLooping(false)
	 *     .Build();
	 * ```
	 */
	class AnimationClipBuilder
	{
	public:
		/**
		 * @brief Start building a new animation clip
		 * @param name Unique name for this animation (e.g., "player_walk")
		 */
		explicit AnimationClipBuilder(const std::string& name)
		{
			m_Clip.Name = name;
		}

		/**
		 * @brief Add a single frame with its own texture
		 * @param texturePath Path to the texture file
		 * @param duration Duration in seconds (default 0.1s = 10 FPS)
		 */
		AnimationClipBuilder& AddFrame(const std::string& texturePath, float duration = 0.1f)
		{
			m_Clip.Frames.emplace_back(texturePath, duration);
			return *this;
		}

		/**
		 * @brief Add a frame with custom UV coordinates (for manual sprite sheet regions)
		 * @param texturePath Path to the texture/sprite sheet
		 * @param duration Duration in seconds
		 * @param uvMin Bottom-left UV coordinate
		 * @param uvMax Top-right UV coordinate
		 */
		AnimationClipBuilder& AddFrame(const std::string& texturePath, float duration,
			const glm::vec2& uvMin, const glm::vec2& uvMax)
		{
			m_Clip.Frames.emplace_back(texturePath, duration, uvMin, uvMax);
			return *this;
		}

		/**
		 * @brief Generate frames from a sprite sheet grid
		 * 
		 * Automatically calculates UV coordinates for each cell in the grid.
		 * Frames are generated left-to-right, top-to-bottom.
		 * 
		 * @param texturePath Path to the sprite sheet texture
		 * @param columns Number of columns in the grid
		 * @param rows Number of rows in the grid
		 * @param frameCount Total frames to use (default: columns * rows)
		 * @param startFrame First frame index to use (default: 0)
		 * @return Reference to this builder for chaining
		 * 
		 * Example: A 4x2 sprite sheet with 8 frames:
		 * ```
		 * +---+---+---+---+
		 * | 0 | 1 | 2 | 3 |
		 * +---+---+---+---+
		 * | 4 | 5 | 6 | 7 |
		 * +---+---+---+---+
		 * ```
		 */
		AnimationClipBuilder& FromSpriteSheet(const std::string& texturePath,
			int columns, int rows, int frameCount = -1, int startFrame = 0)
		{
			if (columns <= 0 || rows <= 0)
			{
				return *this;
			}

			int totalFrames = columns * rows;
			int count = (frameCount < 0) ? totalFrames : std::min(frameCount, totalFrames - startFrame);

			float cellWidth = 1.0f / static_cast<float>(columns);
			float cellHeight = 1.0f / static_cast<float>(rows);

			for (int i = 0; i < count; ++i)
			{
				int frameIndex = startFrame + i;
				int col = frameIndex % columns;
				int row = frameIndex / columns;

				// UV coordinates: OpenGL has origin at bottom-left
				// Sprite sheets typically have origin at top-left, so we flip Y
				float uvMinX = col * cellWidth;
				float uvMaxX = (col + 1) * cellWidth;
				// Flip Y: row 0 is at top of texture (uvMax.y = 1.0)
				float uvMinY = 1.0f - (row + 1) * cellHeight;
				float uvMaxY = 1.0f - row * cellHeight;

				AnimationFrame frame;
				frame.TexturePath = texturePath;
				frame.Duration = m_DefaultFrameDuration;
				frame.UVMin = { uvMinX, uvMinY };
				frame.UVMax = { uvMaxX, uvMaxY };
				m_Clip.Frames.push_back(frame);
			}

			return *this;
		}

		/**
		 * @brief Set the duration for all frames (affects FromSpriteSheet and future AddFrame calls)
		 * @param duration Duration in seconds per frame
		 */
		AnimationClipBuilder& SetFrameDuration(float duration)
		{
			m_DefaultFrameDuration = duration;
			// Also update any existing frames
			for (auto& frame : m_Clip.Frames)
			{
				frame.Duration = duration;
			}
			return *this;
		}

		/**
		 * @brief Set whether the animation should loop
		 * @param loop True for looping animations (walk, idle), false for one-shot (attack, death)
		 */
		AnimationClipBuilder& SetLooping(bool loop)
		{
			m_Clip.Loop = loop;
			return *this;
		}

		/**
		 * @brief Set playback speed multiplier
		 * @param speed Speed multiplier (1.0 = normal, 2.0 = double speed, 0.5 = half speed)
		 */
		AnimationClipBuilder& SetPlaybackSpeed(float speed)
		{
			m_Clip.PlaybackSpeed = speed;
			return *this;
		}

		/**
		 * @brief Add an animation event that fires on a specific frame
		 * @param frameIndex The frame number (0-based) when event should fire
		 * @param eventName Event identifier (e.g., "footstep", "attack_hit", "spawn_particle")
		 */
		AnimationClipBuilder& AddEvent(int frameIndex, const std::string& eventName)
		{
			m_Clip.Events.emplace_back(frameIndex, eventName);
			return *this;
		}

		/**
		 * @brief Build and return the completed AnimationClip
		 * @return The constructed AnimationClip
		 */
		AnimationClip Build() const
		{
			return m_Clip;
		}

		/**
		 * @brief Implicit conversion to AnimationClip
		 * 
		 * Allows passing builder directly to RegisterClip:
		 * ```cpp
		 * animSystem->RegisterClip(AnimationClipBuilder("Walk")
		 *     .AddFrame("walk.png", 0.1f)
		 *     .Build());
		 * ```
		 */
		operator AnimationClip() const
		{
			return Build();
		}

	private:
		AnimationClip m_Clip;
		float m_DefaultFrameDuration = 0.1f;  // 10 FPS default
	};

	// ============================================================================
	// Convenience function for quick clip creation
	// ============================================================================

	/**
	 * @brief Create an animation clip from a sprite sheet with minimal code
	 * 
	 * Convenience function for the most common use case: a horizontal sprite strip.
	 * 
	 * @param name Animation name
	 * @param texturePath Path to sprite sheet
	 * @param frameCount Number of frames in horizontal strip
	 * @param frameDuration Duration per frame in seconds
	 * @param loop Whether animation should loop
	 * @return Configured AnimationClip
	 * 
	 * Example:
	 * ```cpp
	 * auto walk = CreateAnimationFromStrip("Walk", "walk.png", 6, 0.1f, true);
	 * animSystem->RegisterClip(walk);
	 * ```
	 */
	inline AnimationClip CreateAnimationFromStrip(
		const std::string& name,
		const std::string& texturePath,
		int frameCount,
		float frameDuration = 0.1f,
		bool loop = true)
	{
		return AnimationClipBuilder(name)
			.FromSpriteSheet(texturePath, frameCount, 1)
			.SetFrameDuration(frameDuration)
			.SetLooping(loop)
			.Build();
	}

} // namespace Pillar
