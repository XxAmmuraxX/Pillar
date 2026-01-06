#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace Pillar
{
	// Represents a single frame from an Aseprite sprite sheet
	struct AsepriteFrameData
	{
		std::string FrameName;        // Original frame identifier (e.g., "sprite_0")
		int X = 0;                    // Position in sprite sheet (pixels)
		int Y = 0;
		int Width = 0;                // Frame dimensions (pixels)
		int Height = 0;
		int DurationMs = 100;         // Frame duration in milliseconds (for animation timing)
		glm::vec2 UVMin{ 0.0f, 0.0f };  // Texture coordinates (0-1 range)
		glm::vec2 UVMax{ 1.0f, 1.0f };
	};

	// Represents an animation tag (sequence of frames)
	struct AsepriteAnimationTag
	{
		std::string Name;             // Animation name (e.g., "walk", "idle")
		int FromFrame = 0;            // Starting frame index
		int ToFrame = 0;              // Ending frame index
		std::string Direction = "forward";  // Playback direction: "forward", "reverse", "pingpong"
	};

	// Metadata about the sprite sheet
	struct AsepriteMetadata
	{
		std::string ImagePath;        // Path to sprite sheet texture
		int TextureWidth = 0;         // Total texture width
		int TextureHeight = 0;        // Total texture height
		std::string AppVersion;       // Aseprite version used
	};

	// Importer for Aseprite JSON exports
	class AsepriteImporter
	{
	public:
		AsepriteImporter() = default;
		~AsepriteImporter() = default;

		// Parse an Aseprite JSON file
		bool ParseFile(const std::string& jsonPath);

		// Get all frame data (with timing info)
		const std::vector<AsepriteFrameData>& GetFrames() const { return m_Frames; }

		// Get animation tags (named sequences)
		const std::vector<AsepriteAnimationTag>& GetAnimationTags() const { return m_AnimationTags; }

		// Get metadata
		const AsepriteMetadata& GetMetadata() const { return m_Metadata; }

		// Check if parsing was successful
		bool IsValid() const { return m_Valid; }

		// Get error message if parsing failed
		const std::string& GetErrorMessage() const { return m_ErrorMessage; }

	private:
		// Calculate UV coordinates from pixel coordinates
		void CalculateUVCoordinates(AsepriteFrameData& frame);

		std::vector<AsepriteFrameData> m_Frames;
		std::vector<AsepriteAnimationTag> m_AnimationTags;
		AsepriteMetadata m_Metadata;
		bool m_Valid = false;
		std::string m_ErrorMessage;
	};
}
