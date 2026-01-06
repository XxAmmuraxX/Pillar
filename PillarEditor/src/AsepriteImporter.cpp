#include "AsepriteImporter.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include "Pillar/Logger.h"

namespace Pillar
{
	bool AsepriteImporter::ParseFile(const std::string& jsonPath)
	{
		m_Frames.clear();
		m_AnimationTags.clear();
		m_Valid = false;
		m_ErrorMessage.clear();

		// Check if file exists
		if (!std::filesystem::exists(jsonPath))
		{
			m_ErrorMessage = "File not found: " + jsonPath;
			PIL_CORE_ERROR("AsepriteImporter: {0}", m_ErrorMessage);
			return false;
		}

		try
		{
			// Read JSON file
			std::ifstream file(jsonPath);
			if (!file.is_open())
			{
				m_ErrorMessage = "Failed to open file: " + jsonPath;
				PIL_CORE_ERROR("AsepriteImporter: {0}", m_ErrorMessage);
				return false;
			}

			nlohmann::json data = nlohmann::json::parse(file);

			// Parse metadata
			if (data.contains("meta"))
			{
				auto& meta = data["meta"];
				m_Metadata.ImagePath = meta.value("image", "");
				m_Metadata.AppVersion = meta.value("app", "");

				if (meta.contains("size"))
				{
					m_Metadata.TextureWidth = meta["size"].value("w", 0);
					m_Metadata.TextureHeight = meta["size"].value("h", 0);
				}

				// Parse animation tags if present
				if (meta.contains("frameTags") && meta["frameTags"].is_array())
				{
					for (auto& tagJson : meta["frameTags"])
					{
						AsepriteAnimationTag tag;
						tag.Name = tagJson.value("name", "");
						tag.FromFrame = tagJson.value("from", 0);
						tag.ToFrame = tagJson.value("to", 0);
						tag.Direction = tagJson.value("direction", "forward");
						m_AnimationTags.push_back(tag);
					}

					PIL_CORE_INFO("AsepriteImporter: Found {0} animation tags", m_AnimationTags.size());
				}
			}

			// Parse frames
			if (data.contains("frames"))
			{
				auto& framesJson = data["frames"];

				// Aseprite can export as object or array - handle both
				if (framesJson.is_object())
				{
					// Object format: { "sprite_0": {...}, "sprite_1": {...} }
					for (auto& [frameName, frameData] : framesJson.items())
					{
						AsepriteFrameData frame;
						frame.FrameName = frameName;

						// Parse frame rectangle
						if (frameData.contains("frame"))
						{
							auto& rect = frameData["frame"];
							frame.X = rect.value("x", 0);
							frame.Y = rect.value("y", 0);
							frame.Width = rect.value("w", 0);
							frame.Height = rect.value("h", 0);
						}

						// Parse duration (milliseconds)
						frame.DurationMs = frameData.value("duration", 100); // Default to 100ms

						// Calculate UV coordinates
						CalculateUVCoordinates(frame);

						m_Frames.push_back(frame);
					}
				}
				else if (framesJson.is_array())
				{
					// Array format: [ {...}, {...}, {...} ]
					int frameIndex = 0;
					for (auto& frameData : framesJson)
					{
						AsepriteFrameData frame;
						frame.FrameName = "frame_" + std::to_string(frameIndex++);

						if (frameData.contains("frame"))
						{
							auto& rect = frameData["frame"];
							frame.X = rect.value("x", 0);
							frame.Y = rect.value("y", 0);
							frame.Width = rect.value("w", 0);
							frame.Height = rect.value("h", 0);
						}

						frame.DurationMs = frameData.value("duration", 100);
						CalculateUVCoordinates(frame);
						m_Frames.push_back(frame);
					}
				}

				PIL_CORE_INFO("AsepriteImporter: Loaded {0} frames from {1}", m_Frames.size(), jsonPath);
			}

			m_Valid = true;
			return true;
		}
		catch (const std::exception& e)
		{
			m_ErrorMessage = std::string("Parse error: ") + e.what();
			PIL_CORE_ERROR("AsepriteImporter: {0}", m_ErrorMessage);
			return false;
		}
	}

	void AsepriteImporter::CalculateUVCoordinates(AsepriteFrameData& frame)
	{
		if (m_Metadata.TextureWidth == 0 || m_Metadata.TextureHeight == 0)
		{
			PIL_CORE_WARN("AsepriteImporter: Invalid texture dimensions, UVs may be incorrect");
			frame.UVMin = { 0.0f, 0.0f };
			frame.UVMax = { 1.0f, 1.0f };
			return;
		}

		// Convert pixel coordinates to normalized UV coordinates (0-1)
		float texWidth = static_cast<float>(m_Metadata.TextureWidth);
		float texHeight = static_cast<float>(m_Metadata.TextureHeight);

		// UV origin is top-left in Aseprite, but bottom-left in OpenGL
		// So we need to flip the Y coordinate
		frame.UVMin.x = static_cast<float>(frame.X) / texWidth;
		frame.UVMin.y = 1.0f - (static_cast<float>(frame.Y + frame.Height) / texHeight);

		frame.UVMax.x = static_cast<float>(frame.X + frame.Width) / texWidth;
		frame.UVMax.y = 1.0f - (static_cast<float>(frame.Y) / texHeight);
	}
}
