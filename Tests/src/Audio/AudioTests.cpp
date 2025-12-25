#include <gtest/gtest.h>
// AudioTests: unit tests for AudioEngine, AudioSource, AudioBuffer, WavLoader
// and factory functions to validate initialization, properties and error cases.
#include <gmock/gmock.h>
#include "Pillar/Audio/AudioEngine.h"
#include "Pillar/Audio/AudioBuffer.h"
#include "Pillar/Audio/AudioSource.h"
#include "Pillar/Audio/AudioClip.h"
#include "Pillar/Audio/WavLoader.h"
#include <glm/glm.hpp>

namespace Pillar {
namespace Tests {

// ==================== AudioEngine Tests ====================

class AudioEngineTests : public ::testing::Test {
protected:
    void SetUp() override {
        // Audio engine will be initialized per-test if needed
    }
    
    void TearDown() override {
        // Ensure shutdown after each test
        if (AudioEngine::IsInitialized())
        {
            AudioEngine::Shutdown();
        }
    }
};

TEST_F(AudioEngineTests, Initialize_Succeeds) {
    AudioEngine::Init();
    EXPECT_TRUE(AudioEngine::IsInitialized());
}

TEST_F(AudioEngineTests, Shutdown_Succeeds) {
    AudioEngine::Init();
    EXPECT_TRUE(AudioEngine::IsInitialized());
    
    AudioEngine::Shutdown();
    EXPECT_FALSE(AudioEngine::IsInitialized());
}

TEST_F(AudioEngineTests, DoubleInit_DoesNotCrash) {
    AudioEngine::Init();
    AudioEngine::Init(); // Should not crash
    EXPECT_TRUE(AudioEngine::IsInitialized());
}

TEST_F(AudioEngineTests, MasterVolume_DefaultsToOne) {
    AudioEngine::Init();
    EXPECT_FLOAT_EQ(AudioEngine::GetMasterVolume(), 1.0f);
}

TEST_F(AudioEngineTests, MasterVolume_SetAndGet) {
    AudioEngine::Init();
    
    AudioEngine::SetMasterVolume(0.5f);
    EXPECT_FLOAT_EQ(AudioEngine::GetMasterVolume(), 0.5f);
    
    AudioEngine::SetMasterVolume(0.0f);
    EXPECT_FLOAT_EQ(AudioEngine::GetMasterVolume(), 0.0f);
    
    AudioEngine::SetMasterVolume(1.0f);
    EXPECT_FLOAT_EQ(AudioEngine::GetMasterVolume(), 1.0f);
}

TEST_F(AudioEngineTests, MasterVolume_ClampsToBounds) {
    AudioEngine::Init();
    
    AudioEngine::SetMasterVolume(1.5f);
    EXPECT_LE(AudioEngine::GetMasterVolume(), 1.0f);
    
    AudioEngine::SetMasterVolume(-0.5f);
    EXPECT_GE(AudioEngine::GetMasterVolume(), 0.0f);
}

TEST_F(AudioEngineTests, PlayOneShot_InvalidPath_ReturnsNull) {
    AudioEngine::Init();

    auto source = AudioEngine::PlayOneShot("nonexistent.wav");
    EXPECT_EQ(source, nullptr);
}

TEST_F(AudioEngineTests, BusVolume_AppliesToTrackedSource) {
    AudioEngine::Init();

    auto source = AudioEngine::CreateSource();
    ASSERT_NE(source, nullptr);

    AudioEngine::SetSourceBus(source, AudioEngine::AudioBus::Music);
    AudioEngine::SetSourceVolume(source, 0.8f);
    AudioEngine::SetBusVolume(AudioEngine::AudioBus::Music, 0.5f);

    EXPECT_NEAR(source->GetVolume(), 0.4f, 1e-3f);
}

TEST_F(AudioEngineTests, BusMute_ForcesZeroGain) {
    AudioEngine::Init();

    auto source = AudioEngine::CreateSource();
    ASSERT_NE(source, nullptr);

    AudioEngine::SetSourceVolume(source, 0.7f);
    AudioEngine::MuteBus(AudioEngine::AudioBus::SFX);

    EXPECT_NEAR(source->GetVolume(), 0.0f, 1e-4f);
}

TEST_F(AudioEngineTests, BusFade_ReachesTarget) {
    AudioEngine::Init();

    auto source = AudioEngine::CreateSource();
    ASSERT_NE(source, nullptr);

    AudioEngine::SetSourceVolume(source, 1.0f);
    AudioEngine::FadeBusTo(AudioEngine::AudioBus::SFX, 0.0f, 1.0f);

    AudioEngine::Update(0.5f);
    float midVolume = source->GetVolume();
    EXPECT_LT(midVolume, 1.0f);
    EXPECT_GT(midVolume, 0.0f);

    AudioEngine::Update(1.0f);
    EXPECT_NEAR(source->GetVolume(), 0.0f, 1e-3f);
}

TEST_F(AudioEngineTests, ListenerPosition_SetAndGet) {
    AudioEngine::Init();
    
    glm::vec3 position = { 10.0f, 5.0f, 3.0f };
    AudioEngine::SetListenerPosition(position);
    
    glm::vec3 result = AudioEngine::GetListenerPosition();
    EXPECT_FLOAT_EQ(result.x, position.x);
    EXPECT_FLOAT_EQ(result.y, position.y);
    EXPECT_FLOAT_EQ(result.z, position.z);
}

TEST_F(AudioEngineTests, ListenerPosition_DefaultsToOrigin) {
    AudioEngine::Init();
    
    glm::vec3 result = AudioEngine::GetListenerPosition();
    EXPECT_FLOAT_EQ(result.x, 0.0f);
    EXPECT_FLOAT_EQ(result.y, 0.0f);
    EXPECT_FLOAT_EQ(result.z, 0.0f);
}

// ==================== AudioSource Tests ====================

class AudioSourceTests : public ::testing::Test {
protected:
    void SetUp() override {
        AudioEngine::Init();
    }
    
    void TearDown() override {
        AudioEngine::Shutdown();
    }
};

TEST_F(AudioSourceTests, Create_ReturnsValidSource) {
    auto source = AudioSource::Create();
    ASSERT_NE(source, nullptr);
    EXPECT_GT(source->GetSourceID(), 0u);
}

TEST_F(AudioSourceTests, DefaultState_IsStopped) {
    auto source = AudioSource::Create();
    ASSERT_NE(source, nullptr);
    
    EXPECT_TRUE(source->IsStopped());
    EXPECT_FALSE(source->IsPlaying());
    EXPECT_FALSE(source->IsPaused());
    EXPECT_EQ(source->GetState(), AudioState::Stopped);
}

TEST_F(AudioSourceTests, DefaultVolume_IsOne) {
    auto source = AudioSource::Create();
    ASSERT_NE(source, nullptr);
    
    EXPECT_FLOAT_EQ(source->GetVolume(), 1.0f);
}

TEST_F(AudioSourceTests, Volume_SetAndGet) {
    auto source = AudioSource::Create();
    ASSERT_NE(source, nullptr);
    
    source->SetVolume(0.5f);
    EXPECT_FLOAT_EQ(source->GetVolume(), 0.5f);
}

TEST_F(AudioSourceTests, Volume_ClampsToBounds) {
    auto source = AudioSource::Create();
    ASSERT_NE(source, nullptr);
    
    source->SetVolume(1.5f);
    EXPECT_LE(source->GetVolume(), 1.0f);
    
    source->SetVolume(-0.5f);
    EXPECT_GE(source->GetVolume(), 0.0f);
}

TEST_F(AudioSourceTests, DefaultPitch_IsOne) {
    auto source = AudioSource::Create();
    ASSERT_NE(source, nullptr);
    
    EXPECT_FLOAT_EQ(source->GetPitch(), 1.0f);
}

TEST_F(AudioSourceTests, Pitch_SetAndGet) {
    auto source = AudioSource::Create();
    ASSERT_NE(source, nullptr);
    
    source->SetPitch(1.5f);
    EXPECT_FLOAT_EQ(source->GetPitch(), 1.5f);
}

TEST_F(AudioSourceTests, DefaultLooping_IsFalse) {
    auto source = AudioSource::Create();
    ASSERT_NE(source, nullptr);
    
    EXPECT_FALSE(source->IsLooping());
}

TEST_F(AudioSourceTests, Looping_SetAndGet) {
    auto source = AudioSource::Create();
    ASSERT_NE(source, nullptr);
    
    source->SetLooping(true);
    EXPECT_TRUE(source->IsLooping());
    
    source->SetLooping(false);
    EXPECT_FALSE(source->IsLooping());
}

TEST_F(AudioSourceTests, DefaultPosition_IsOrigin) {
    auto source = AudioSource::Create();
    ASSERT_NE(source, nullptr);
    
    glm::vec3 pos = source->GetPosition();
    EXPECT_FLOAT_EQ(pos.x, 0.0f);
    EXPECT_FLOAT_EQ(pos.y, 0.0f);
    EXPECT_FLOAT_EQ(pos.z, 0.0f);
}

TEST_F(AudioSourceTests, Position_SetAndGet) {
    auto source = AudioSource::Create();
    ASSERT_NE(source, nullptr);
    
    glm::vec3 newPos = { 5.0f, 10.0f, 15.0f };
    source->SetPosition(newPos);
    
    glm::vec3 pos = source->GetPosition();
    EXPECT_FLOAT_EQ(pos.x, newPos.x);
    EXPECT_FLOAT_EQ(pos.y, newPos.y);
    EXPECT_FLOAT_EQ(pos.z, newPos.z);
}

TEST_F(AudioSourceTests, NoBuffer_PlayDoesNotCrash) {
    auto source = AudioSource::Create();
    ASSERT_NE(source, nullptr);
    
    // Should not crash when playing without a buffer
    source->Play();
    // State should still be stopped since there's nothing to play
    EXPECT_TRUE(source->IsStopped());
}

// ==================== WavLoader Tests ====================

class WavLoaderTests : public ::testing::Test {
protected:
    void SetUp() override {
        // No setup needed
    }
};

TEST_F(WavLoaderTests, LoadInvalidPath_ReturnsFalse) {
    WavData data;
    bool result = WavLoader::Load("nonexistent_file.wav", data);
    EXPECT_FALSE(result);
}

TEST_F(WavLoaderTests, LoadFromMemory_InvalidData_ReturnsFalse) {
    WavData data;
    const char* invalidData = "not a wav file";
    bool result = WavLoader::LoadFromMemory(invalidData, 14, data);
    EXPECT_FALSE(result);
}

TEST_F(WavLoaderTests, LoadFromMemory_TooSmall_ReturnsFalse) {
    WavData data;
    const char* smallData = "RIFF";
    bool result = WavLoader::LoadFromMemory(smallData, 4, data);
    EXPECT_FALSE(result);
}

// ==================== AudioBuffer Tests ====================

class AudioBufferTests : public ::testing::Test {
protected:
    void SetUp() override {
        AudioEngine::Init();
    }
    
    void TearDown() override {
        AudioEngine::Shutdown();
    }
};

TEST_F(AudioBufferTests, CreateWithInvalidPath_ReturnsNull) {
    auto buffer = AudioBuffer::Create("nonexistent.wav");
    EXPECT_EQ(buffer, nullptr);
}

TEST_F(AudioBufferTests, CreateWithoutInit_ReturnsNull) {
    AudioEngine::Shutdown();
    
    auto buffer = AudioBuffer::Create("test.wav");
    EXPECT_EQ(buffer, nullptr);
}

// ==================== Factory Pattern Tests ====================

class AudioFactoryTests : public ::testing::Test {
protected:
    void SetUp() override {
        AudioEngine::Init();
    }
    
    void TearDown() override {
        AudioEngine::Shutdown();
    }
};

TEST_F(AudioFactoryTests, AudioEngineCreateSource_ReturnsValidSource) {
    auto source = AudioEngine::CreateSource();
    ASSERT_NE(source, nullptr);
}

TEST_F(AudioFactoryTests, MultipleSourcesCreated_AreIndependent) {
    auto source1 = AudioEngine::CreateSource();
    auto source2 = AudioEngine::CreateSource();
    
    ASSERT_NE(source1, nullptr);
    ASSERT_NE(source2, nullptr);
    
    // Different source IDs
    EXPECT_NE(source1->GetSourceID(), source2->GetSourceID());
    
    // Independent properties
    source1->SetVolume(0.5f);
    source2->SetVolume(0.8f);
    
    EXPECT_FLOAT_EQ(source1->GetVolume(), 0.5f);
    EXPECT_FLOAT_EQ(source2->GetVolume(), 0.8f);
}

} // namespace Tests
} // namespace Pillar
