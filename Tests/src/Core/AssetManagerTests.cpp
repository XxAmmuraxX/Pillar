#include <gtest/gtest.h>
// AssetManagerTests: verifies asset path resolution, subdirectory lookup and
// behavior when directories or files are missing or absolute/relative paths.
#include "Pillar/Utils/AssetManager.h"
#include <filesystem>
#include <fstream>

using namespace Pillar;

// ============================================================================
// AssetManager Tests
// ============================================================================

class AssetManagerTests : public ::testing::Test
{
protected:
	void SetUp() override
	{
		// Store original directory
		m_OriginalDir = AssetManager::GetAssetsDirectory();
	}

	void TearDown() override
	{
		// Restore original directory if it was set
		if (!m_OriginalDir.empty())
		{
			AssetManager::SetAssetsDirectory(m_OriginalDir);
		}
	}

	std::string m_OriginalDir;
};

TEST_F(AssetManagerTests, GetExecutableDirectory_ReturnsValidPath)
{
	std::filesystem::path exeDir = AssetManager::GetExecutableDirectory();

	EXPECT_FALSE(exeDir.empty());
	EXPECT_TRUE(std::filesystem::exists(exeDir));
	EXPECT_TRUE(std::filesystem::is_directory(exeDir));
}

TEST_F(AssetManagerTests, SetAssetsDirectory_UpdatesDirectory)
{
	std::string testPath = "C:/TestAssets";
	AssetManager::SetAssetsDirectory(testPath);

	std::string result = AssetManager::GetAssetsDirectory();
	EXPECT_EQ(result, testPath);
}

TEST_F(AssetManagerTests, GetAssetPath_ReturnsOriginalIfNotFound)
{
	// Set a non-existent assets directory
	AssetManager::SetAssetsDirectory("C:/NonExistentPath/Assets");

	std::string result = AssetManager::GetAssetPath("nonexistent_file.png");

	// Should return original path when file not found
	EXPECT_EQ(result, "nonexistent_file.png");
}

TEST_F(AssetManagerTests, GetTexturePath_ReturnsOriginalIfNotFound)
{
	AssetManager::SetAssetsDirectory("C:/NonExistentPath/Assets");

	std::string result = AssetManager::GetTexturePath("missing_texture.png");

	// Should return original path when texture not found
	EXPECT_EQ(result, "missing_texture.png");
}

TEST_F(AssetManagerTests, GetAudioPath_ReturnsOriginalIfNotFound)
{
	AssetManager::SetAssetsDirectory("C:/NonExistentPath/Assets");

	std::string result = AssetManager::GetAudioPath("missing_audio.wav");

	EXPECT_EQ(result, "missing_audio.wav");
}

TEST_F(AssetManagerTests, GetSFXPath_ReturnsOriginalIfNotFound)
{
	AssetManager::SetAssetsDirectory("C:/NonExistentPath/Assets");

	std::string result = AssetManager::GetSFXPath("missing_sfx.wav");

	EXPECT_EQ(result, "missing_sfx.wav");
}

TEST_F(AssetManagerTests, GetMusicPath_ReturnsOriginalIfNotFound)
{
	AssetManager::SetAssetsDirectory("C:/NonExistentPath/Assets");

	std::string result = AssetManager::GetMusicPath("missing_music.wav");

	EXPECT_EQ(result, "missing_music.wav");
}

TEST_F(AssetManagerTests, GetAssetPath_HandlesAbsolutePath)
{
	// Create a temporary file for testing
	std::filesystem::path tempDir = std::filesystem::temp_directory_path();
	std::filesystem::path testFile = tempDir / "test_asset_manager_file.txt";
	
	// Create the file
	std::ofstream file(testFile);
	file << "test";
	file.close();

	std::string result = AssetManager::GetAssetPath(testFile.string());

	// Should return the absolute path if file exists
	EXPECT_EQ(result, testFile.string());

	// Cleanup
	std::filesystem::remove(testFile);
}

TEST_F(AssetManagerTests, GetAssetPath_HandlesRelativePath)
{
	// Test with a relative path that doesn't exist
	std::string result = AssetManager::GetAssetPath("relative/path/to/asset.png");

	// Should return original path when not found
	EXPECT_EQ(result, "relative/path/to/asset.png");
}

TEST_F(AssetManagerTests, GetAssetsDirectory_ReturnsSetDirectory)
{
	std::string customPath = "D:/CustomAssets";
	AssetManager::SetAssetsDirectory(customPath);

	std::string result = AssetManager::GetAssetsDirectory();

	EXPECT_EQ(result, customPath);
}

TEST_F(AssetManagerTests, PathResolution_TexturesSubdirectory)
{
	// Create a temp directory structure
	std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "pillar_test_assets";
	std::filesystem::path texturesDir = tempDir / "textures";
	std::filesystem::create_directories(texturesDir);

	std::filesystem::path testTexture = texturesDir / "test_texture.png";
	std::ofstream file(testTexture);
	file << "test";
	file.close();

	AssetManager::SetAssetsDirectory(tempDir.string());

	// Should find texture in textures subdirectory
	std::string result = AssetManager::GetTexturePath("test_texture.png");
	EXPECT_TRUE(std::filesystem::exists(result));

	// Cleanup
	std::filesystem::remove_all(tempDir);
}

TEST_F(AssetManagerTests, PathResolution_AudioSubdirectory)
{
	// Create a temp directory structure
	std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "pillar_test_assets";
	std::filesystem::path audioDir = tempDir / "audio";
	std::filesystem::create_directories(audioDir);

	std::filesystem::path testAudio = audioDir / "test_audio.wav";
	std::ofstream file(testAudio);
	file << "test";
	file.close();

	AssetManager::SetAssetsDirectory(tempDir.string());

	// Should find audio in audio subdirectory
	std::string result = AssetManager::GetAudioPath("test_audio.wav");
	EXPECT_TRUE(std::filesystem::exists(result));

	// Cleanup
	std::filesystem::remove_all(tempDir);
}

TEST_F(AssetManagerTests, PathResolution_SFXSubdirectory)
{
	// Create a temp directory structure
	std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "pillar_test_assets";
	std::filesystem::path sfxDir = tempDir / "audio" / "sfx";
	std::filesystem::create_directories(sfxDir);

	std::filesystem::path testSfx = sfxDir / "test_sfx.wav";
	std::ofstream file(testSfx);
	file << "test";
	file.close();

	AssetManager::SetAssetsDirectory(tempDir.string());

	// Should find SFX in audio/sfx subdirectory
	std::string result = AssetManager::GetSFXPath("test_sfx.wav");
	EXPECT_TRUE(std::filesystem::exists(result));

	// Cleanup
	std::filesystem::remove_all(tempDir);
}

TEST_F(AssetManagerTests, PathResolution_MusicSubdirectory)
{
	// Create a temp directory structure
	std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "pillar_test_assets";
	std::filesystem::path musicDir = tempDir / "audio" / "music";
	std::filesystem::create_directories(musicDir);

	std::filesystem::path testMusic = musicDir / "test_music.wav";
	std::ofstream file(testMusic);
	file << "test";
	file.close();

	AssetManager::SetAssetsDirectory(tempDir.string());

	// Should find music in audio/music subdirectory
	std::string result = AssetManager::GetMusicPath("test_music.wav");
	EXPECT_TRUE(std::filesystem::exists(result));

	// Cleanup
	std::filesystem::remove_all(tempDir);
}
