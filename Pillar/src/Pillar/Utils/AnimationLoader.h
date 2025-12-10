#pragma once

#include "Pillar/ECS/Components/Rendering/AnimationClip.h"
#include <string>

namespace Pillar {

	/**
	 * @brief Utility class for loading and saving animation clips from/to JSON files
	 * 
	 * Supports loading animation definitions from JSON files, including:
	 * - Frame sequences with texture paths and durations
	 * - UV coordinates for sprite sheets (optional)
	 * - Animation events with frame triggers
	 * - Playback settings (loop, speed)
	 * 
	 * Example JSON format:
	 * ```json
	 * {
	 *   "name": "player_walk",
	 *   "loop": true,
	 *   "playbackSpeed": 1.0,
	 *   "frames": [
	 *     {
	 *       "texturePath": "player_walk_00.png",
	 *       "duration": 0.1,
	 *       "uvMin": [0.0, 0.0],
	 *       "uvMax": [1.0, 1.0]
	 *     }
	 *   ],
	 *   "events": [
	 *     {
	 *       "frameIndex": 1,
	 *       "eventName": "footstep"
	 *     }
	 *   ]
	 * }
	 * ```
	 */
	class AnimationLoader
	{
	public:
		/**
		 * @brief Load an animation clip from a JSON file
		 * @param filePath Path to the .anim.json file
		 * @return AnimationClip object (empty if load failed)
		 */
		static AnimationClip LoadFromJSON(const std::string& filePath);

		/**
		 * @brief Save an animation clip to a JSON file
		 * @param clip The animation clip to save
		 * @param filePath Path where the JSON file should be saved
		 * @return True if saved successfully, false otherwise
		 */
		static bool SaveToJSON(const AnimationClip& clip, const std::string& filePath);
	};

} // namespace Pillar
