#include "AnimationLoader.h"
#include "Pillar/Logger.h"
#include "Pillar/Utils/AssetManager.h"

#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

namespace Pillar {

	AnimationClip AnimationLoader::LoadFromJSON(const std::string& filePath)
	{
		AnimationClip clip;

		try
		{
			// Resolve asset path
			std::string resolvedPath = AssetManager::GetAssetPath(filePath);
			
			// Open and parse JSON file
			std::ifstream file(resolvedPath);
			if (!file.is_open())
			{
				PIL_CORE_ERROR("Failed to open animation file: {0}", resolvedPath);
				return clip;
			}

			json j;
			file >> j;

			// Parse animation metadata
			if (j.contains("name"))
				clip.Name = j["name"].get<std::string>();

			if (j.contains("loop"))
				clip.Loop = j["loop"].get<bool>();

			if (j.contains("playbackSpeed"))
				clip.PlaybackSpeed = j["playbackSpeed"].get<float>();

			// Parse frames
			if (j.contains("frames") && j["frames"].is_array())
			{
				for (const auto& frameJson : j["frames"])
				{
					AnimationFrame frame;

					if (frameJson.contains("texturePath"))
						frame.TexturePath = frameJson["texturePath"].get<std::string>();

					if (frameJson.contains("duration"))
						frame.Duration = frameJson["duration"].get<float>();

					// Parse UV coordinates (optional, defaults to full texture)
					if (frameJson.contains("uvMin") && frameJson["uvMin"].is_array() && frameJson["uvMin"].size() >= 2)
					{
						frame.UVMin.x = frameJson["uvMin"][0].get<float>();
						frame.UVMin.y = frameJson["uvMin"][1].get<float>();
					}

					if (frameJson.contains("uvMax") && frameJson["uvMax"].is_array() && frameJson["uvMax"].size() >= 2)
					{
						frame.UVMax.x = frameJson["uvMax"][0].get<float>();
						frame.UVMax.y = frameJson["uvMax"][1].get<float>();
					}

					clip.Frames.push_back(frame);
				}
			}

			// Parse animation events
			if (j.contains("events") && j["events"].is_array())
			{
				for (const auto& eventJson : j["events"])
				{
					AnimationClip::AnimationEvent event;

					if (eventJson.contains("frameIndex"))
						event.FrameIndex = eventJson["frameIndex"].get<int>();

					if (eventJson.contains("eventName"))
						event.EventName = eventJson["eventName"].get<std::string>();

					clip.Events.push_back(event);
				}
			}

			PIL_CORE_INFO("Loaded animation: {0} with {1} frames", clip.Name, clip.GetFrameCount());
		}
		catch (const json::exception& e)
		{
			PIL_CORE_ERROR("JSON parsing error in {0}: {1}", filePath, e.what());
		}
		catch (const std::exception& e)
		{
			PIL_CORE_ERROR("Error loading animation from {0}: {1}", filePath, e.what());
		}

		return clip;
	}

	bool AnimationLoader::SaveToJSON(const AnimationClip& clip, const std::string& filePath)
	{
		try
		{
			json j;

			// Serialize animation metadata
			j["name"] = clip.Name;
			j["loop"] = clip.Loop;
			j["playbackSpeed"] = clip.PlaybackSpeed;

			// Serialize frames
			json framesArray = json::array();
			for (const auto& frame : clip.Frames)
			{
				json frameJson;
				frameJson["texturePath"] = frame.TexturePath;
				frameJson["duration"] = frame.Duration;
				frameJson["uvMin"] = { frame.UVMin.x, frame.UVMin.y };
				frameJson["uvMax"] = { frame.UVMax.x, frame.UVMax.y };
				framesArray.push_back(frameJson);
			}
			j["frames"] = framesArray;

			// Serialize events
			json eventsArray = json::array();
			for (const auto& event : clip.Events)
			{
				json eventJson;
				eventJson["frameIndex"] = event.FrameIndex;
				eventJson["eventName"] = event.EventName;
				eventsArray.push_back(eventJson);
			}
			j["events"] = eventsArray;

			// Write to file with pretty formatting
			std::ofstream file(filePath);
			if (!file.is_open())
			{
				PIL_CORE_ERROR("Failed to create animation file: {0}", filePath);
				return false;
			}

			file << j.dump(2);  // Indent with 2 spaces
			file.close();

			PIL_CORE_INFO("Saved animation: {0} to {1}", clip.Name, filePath);
			return true;
		}
		catch (const json::exception& e)
		{
			PIL_CORE_ERROR("JSON serialization error: {0}", e.what());
			return false;
		}
		catch (const std::exception& e)
		{
			PIL_CORE_ERROR("Error saving animation to {0}: {1}", filePath, e.what());
			return false;
		}
	}

} // namespace Pillar
