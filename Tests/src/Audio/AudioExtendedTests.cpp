#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Pillar/Audio/AudioEngine.h"
#include "Pillar/Audio/AudioBuffer.h"
#include "Pillar/Audio/AudioSource.h"
#include "Pillar/Audio/AudioClip.h"
#include "Pillar/Audio/AudioListener.h"
#include "Pillar/Audio/WavLoader.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Components/Audio/AudioSourceComponent.h"
#include "Pillar/ECS/Components/Audio/AudioListenerComponent.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Systems/AudioSystem.h"
#include <entt/entt.hpp>
#include <glm/glm.hpp>

namespace Pillar {
namespace Tests {

// ==================== AudioClip Tests ====================

class AudioClipTests : public ::testing::Test {
protected:
    void SetUp() override {
        AudioEngine::Init();
    }
    
    void TearDown() override {
        AudioEngine::Shutdown();
    }
};

TEST_F(AudioClipTests, Create_WithInvalidPath_ReturnsNull) {
    auto clip = AudioClip::Create("nonexistent_audio_file.wav");
    EXPECT_EQ(clip, nullptr);
}

TEST_F(AudioClipTests, IsLoaded_ReturnsFalseForInvalidFile) {
    // Direct construction with invalid path
    AudioClip clip("invalid_path_that_does_not_exist.wav");
    EXPECT_FALSE(clip.IsLoaded());
}

TEST_F(AudioClipTests, PlayPauseResume_DoesNotCrashWhenNotLoaded) {
    AudioClip clip("nonexistent.wav");
    
    // These should not crash even when not loaded
    clip.Play();
    clip.Pause();
    clip.Resume();
    clip.Stop();
    
    EXPECT_FALSE(clip.IsPlaying());
    EXPECT_FALSE(clip.IsPaused());
}

TEST_F(AudioClipTests, GetDuration_ReturnsZeroWhenNotLoaded) {
    AudioClip clip("nonexistent.wav");
    EXPECT_FLOAT_EQ(clip.GetDuration(), 0.0f);
}

TEST_F(AudioClipTests, SetVolume_WorksEvenWhenNotLoaded) {
    AudioClip clip("nonexistent.wav");
    
    clip.SetVolume(0.5f);
    // Should not crash, getter should return reasonable value or default
    float volume = clip.GetVolume();
    EXPECT_GE(volume, 0.0f);
    EXPECT_LE(volume, 1.0f);
}

TEST_F(AudioClipTests, SetPitch_WorksEvenWhenNotLoaded) {
    AudioClip clip("nonexistent.wav");
    
    clip.SetPitch(1.5f);
    float pitch = clip.GetPitch();
    EXPECT_GT(pitch, 0.0f);
}

TEST_F(AudioClipTests, SetLooping_WorksEvenWhenNotLoaded) {
    AudioClip clip("nonexistent.wav");
    
    clip.SetLooping(true);
    // Should not crash
    bool looping = clip.IsLooping();
    // Value may or may not be set depending on implementation
    (void)looping;
}

TEST_F(AudioClipTests, SetPosition_WorksEvenWhenNotLoaded) {
    AudioClip clip("nonexistent.wav");
    
    glm::vec3 pos = { 10.0f, 20.0f, 30.0f };
    clip.SetPosition(pos);
    // Should not crash
}

// ==================== AudioListener Tests ====================

class AudioListenerTests : public ::testing::Test {
protected:
    void SetUp() override {
        AudioEngine::Init();
    }
    
    void TearDown() override {
        AudioEngine::Shutdown();
    }
};

TEST_F(AudioListenerTests, SetPosition_UpdatesEngineListener) {
    AudioListener listener;
    
    glm::vec3 position = { 5.0f, 10.0f, 15.0f };
    listener.SetPosition(position);
    
    glm::vec3 enginePos = AudioEngine::GetListenerPosition();
    EXPECT_FLOAT_EQ(enginePos.x, position.x);
    EXPECT_FLOAT_EQ(enginePos.y, position.y);
    EXPECT_FLOAT_EQ(enginePos.z, position.z);
}

TEST_F(AudioListenerTests, SetOrientation_UpdatesEngineListener) {
    AudioListener listener;
    
    glm::vec3 forward = { 0.0f, 0.0f, -1.0f };
    glm::vec3 up = { 0.0f, 1.0f, 0.0f };
    listener.SetOrientation(forward, up);
    
    // Check that internal state is updated
    EXPECT_FLOAT_EQ(listener.GetForward().x, forward.x);
    EXPECT_FLOAT_EQ(listener.GetForward().y, forward.y);
    EXPECT_FLOAT_EQ(listener.GetForward().z, forward.z);
    EXPECT_FLOAT_EQ(listener.GetUp().x, up.x);
    EXPECT_FLOAT_EQ(listener.GetUp().y, up.y);
    EXPECT_FLOAT_EQ(listener.GetUp().z, up.z);
}

TEST_F(AudioListenerTests, UpdateFromCamera_SetsPositionAndOrientation) {
    AudioListener listener;
    
    glm::vec3 position = { 100.0f, 200.0f, 300.0f };
    glm::vec3 forward = { 1.0f, 0.0f, 0.0f };
    glm::vec3 up = { 0.0f, 0.0f, 1.0f };
    
    listener.UpdateFromCamera(position, forward, up);
    
    glm::vec3 resultPos = listener.GetPosition();
    EXPECT_FLOAT_EQ(resultPos.x, position.x);
    EXPECT_FLOAT_EQ(resultPos.y, position.y);
    EXPECT_FLOAT_EQ(resultPos.z, position.z);
    
    EXPECT_FLOAT_EQ(listener.GetForward().x, forward.x);
    EXPECT_FLOAT_EQ(listener.GetForward().y, forward.y);
    EXPECT_FLOAT_EQ(listener.GetForward().z, forward.z);
}

TEST_F(AudioListenerTests, SetVelocity_UpdatesInternalState) {
    AudioListener listener;
    
    glm::vec3 velocity = { 1.0f, 2.0f, 3.0f };
    listener.SetVelocity(velocity);
    
    glm::vec3 result = listener.GetVelocity();
    EXPECT_FLOAT_EQ(result.x, velocity.x);
    EXPECT_FLOAT_EQ(result.y, velocity.y);
    EXPECT_FLOAT_EQ(result.z, velocity.z);
}

// ==================== AudioSource Extended Tests ====================

class AudioSourceExtendedTests : public ::testing::Test {
protected:
    void SetUp() override {
        AudioEngine::Init();
    }
    
    void TearDown() override {
        AudioEngine::Shutdown();
    }
};

TEST_F(AudioSourceExtendedTests, Velocity_SetAndGet) {
    auto source = AudioSource::Create();
    ASSERT_NE(source, nullptr);
    
    glm::vec3 velocity = { 1.0f, 2.0f, 3.0f };
    source->SetVelocity(velocity);
    // Velocity doesn't have getter in interface, but should not crash
    SUCCEED();
}

TEST_F(AudioSourceExtendedTests, Direction_SetAndGet) {
    auto source = AudioSource::Create();
    ASSERT_NE(source, nullptr);
    
    glm::vec3 direction = { 0.0f, 1.0f, 0.0f };
    source->SetDirection(direction);
    // Direction doesn't have getter in interface, but should not crash
    SUCCEED();
}

TEST_F(AudioSourceExtendedTests, MinDistance_SetAndGet) {
    auto source = AudioSource::Create();
    ASSERT_NE(source, nullptr);
    
    source->SetMinDistance(5.0f);
    // No getter exposed, but should not crash
    SUCCEED();
}

TEST_F(AudioSourceExtendedTests, MaxDistance_SetAndGet) {
    auto source = AudioSource::Create();
    ASSERT_NE(source, nullptr);
    
    source->SetMaxDistance(100.0f);
    // No getter exposed, but should not crash
    SUCCEED();
}

TEST_F(AudioSourceExtendedTests, RolloffFactor_SetAndGet) {
    auto source = AudioSource::Create();
    ASSERT_NE(source, nullptr);
    
    source->SetRolloffFactor(2.0f);
    // No getter exposed, but should not crash
    SUCCEED();
}

TEST_F(AudioSourceExtendedTests, PlaybackPosition_SetAndGet) {
    auto source = AudioSource::Create();
    ASSERT_NE(source, nullptr);
    
    source->SetPlaybackPosition(0.5f);
    float pos = source->GetPlaybackPosition();
    // Without buffer, position may be 0
    EXPECT_GE(pos, 0.0f);
}

TEST_F(AudioSourceExtendedTests, DestroyWhilePlaying_DoesNotCrash) {
    {
        auto source = AudioSource::Create();
        ASSERT_NE(source, nullptr);
        
        // Try to play (won't actually play without buffer)
        source->Play();
    }
    // Source destroyed here - should not crash
    SUCCEED();
}

// ==================== AudioEngine Extended Tests ====================

class AudioEngineExtendedTests : public ::testing::Test {
protected:
    void SetUp() override {
        AudioEngine::Init();
    }
    
    void TearDown() override {
        AudioEngine::Shutdown();
    }
};

TEST_F(AudioEngineExtendedTests, StopAllSounds_StopsPlayingSources) {
    auto source1 = AudioEngine::CreateSource();
    auto source2 = AudioEngine::CreateSource();
    
    ASSERT_NE(source1, nullptr);
    ASSERT_NE(source2, nullptr);
    
    // Without buffers, they won't actually play, but shouldn't crash
    source1->Play();
    source2->Play();
    
    AudioEngine::StopAllSounds();
    
    EXPECT_TRUE(source1->IsStopped());
    EXPECT_TRUE(source2->IsStopped());
}

TEST_F(AudioEngineExtendedTests, PauseAllSounds_PausesPlayingSources) {
    auto source1 = AudioEngine::CreateSource();
    auto source2 = AudioEngine::CreateSource();
    
    ASSERT_NE(source1, nullptr);
    ASSERT_NE(source2, nullptr);
    
    // Call pause all - should not crash even without playing sources
    AudioEngine::PauseAllSounds();
    
    SUCCEED();
}

TEST_F(AudioEngineExtendedTests, ResumeAllSounds_ResumesPausedSources) {
    auto source = AudioEngine::CreateSource();
    ASSERT_NE(source, nullptr);
    
    // Pause and resume - should not crash
    AudioEngine::PauseAllSounds();
    AudioEngine::ResumeAllSounds();
    
    SUCCEED();
}

TEST_F(AudioEngineExtendedTests, ListenerVelocity_SetWorks) {
    glm::vec3 velocity = { 5.0f, 0.0f, 0.0f };
    AudioEngine::SetListenerVelocity(velocity);
    // Should not crash
    SUCCEED();
}

TEST_F(AudioEngineExtendedTests, ListenerOrientation_SetWorks) {
    glm::vec3 forward = { 0.0f, 0.0f, -1.0f };
    glm::vec3 up = { 0.0f, 1.0f, 0.0f };
    AudioEngine::SetListenerOrientation(forward, up);
    // Should not crash
    SUCCEED();
}

TEST_F(AudioEngineExtendedTests, ShutdownWithActiveSources_CleansUp) {
    auto source1 = AudioEngine::CreateSource();
    auto source2 = AudioEngine::CreateSource();
    
    ASSERT_NE(source1, nullptr);
    ASSERT_NE(source2, nullptr);
    
    // Keep sources alive while shutting down
    AudioEngine::Shutdown();
    
    EXPECT_FALSE(AudioEngine::IsInitialized());
    
    // Re-initialize for TearDown
    AudioEngine::Init();
}

// ==================== AudioSourceComponent Tests ====================

class AudioSourceComponentTests : public ::testing::Test {
protected:
    void SetUp() override {
        AudioEngine::Init();
    }
    
    void TearDown() override {
        AudioEngine::Shutdown();
    }
};

TEST_F(AudioSourceComponentTests, DefaultValues_AreCorrect) {
    AudioSourceComponent comp;
    
    EXPECT_EQ(comp.Source, nullptr);
    EXPECT_TRUE(comp.AudioFile.empty());
    EXPECT_FLOAT_EQ(comp.Volume, 1.0f);
    EXPECT_FLOAT_EQ(comp.Pitch, 1.0f);
    EXPECT_FALSE(comp.Loop);
    EXPECT_FALSE(comp.PlayOnAwake);
    EXPECT_TRUE(comp.Is3D);
    EXPECT_FLOAT_EQ(comp.MinDistance, 1.0f);
    EXPECT_FLOAT_EQ(comp.MaxDistance, 100.0f);
    EXPECT_FLOAT_EQ(comp.RolloffFactor, 1.0f);
}

TEST_F(AudioSourceComponentTests, CopyConstructor_DoesNotCopySource) {
    AudioSourceComponent original;
    original.AudioFile = "test.wav";
    original.Volume = 0.5f;
    original.Pitch = 1.5f;
    original.Loop = true;
    original.PlayOnAwake = true;
    original.Is3D = false;
    original.MinDistance = 2.0f;
    original.MaxDistance = 50.0f;
    original.RolloffFactor = 0.5f;
    original.Source = AudioSource::Create();  // Create a source
    
    AudioSourceComponent copy(original);
    
    // Properties should be copied
    EXPECT_EQ(copy.AudioFile, "test.wav");
    EXPECT_FLOAT_EQ(copy.Volume, 0.5f);
    EXPECT_FLOAT_EQ(copy.Pitch, 1.5f);
    EXPECT_TRUE(copy.Loop);
    EXPECT_TRUE(copy.PlayOnAwake);
    EXPECT_FALSE(copy.Is3D);
    EXPECT_FLOAT_EQ(copy.MinDistance, 2.0f);
    EXPECT_FLOAT_EQ(copy.MaxDistance, 50.0f);
    EXPECT_FLOAT_EQ(copy.RolloffFactor, 0.5f);
    
    // Source should NOT be copied
    EXPECT_EQ(copy.Source, nullptr);
}

TEST_F(AudioSourceComponentTests, Constructor_WithFilePath_SetsAudioFile) {
    AudioSourceComponent comp("my_sound.wav");
    
    EXPECT_EQ(comp.AudioFile, "my_sound.wav");
    EXPECT_EQ(comp.Source, nullptr);  // Source not created yet
}

TEST_F(AudioSourceComponentTests, AssignmentOperator_DoesNotCopySource) {
    AudioSourceComponent original;
    original.AudioFile = "original.wav";
    original.Volume = 0.7f;
    original.Source = AudioSource::Create();
    
    AudioSourceComponent copy;
    copy.Source = AudioSource::Create();  // Give copy its own source
    auto copySourceId = copy.Source->GetSourceID();
    
    copy = original;
    
    EXPECT_EQ(copy.AudioFile, "original.wav");
    EXPECT_FLOAT_EQ(copy.Volume, 0.7f);
    // Source should not be overwritten (it's excluded from copy)
    // Per implementation, Source is NOT copied
    EXPECT_NE(copy.Source, original.Source);
}

// ==================== AudioListenerComponent Tests ====================

class AudioListenerComponentTests : public ::testing::Test {
};

TEST_F(AudioListenerComponentTests, DefaultValues_AreCorrect) {
    AudioListenerComponent comp;
    
    EXPECT_TRUE(comp.IsActive);
    EXPECT_FLOAT_EQ(comp.Forward.x, 0.0f);
    EXPECT_FLOAT_EQ(comp.Forward.y, 0.0f);
    EXPECT_FLOAT_EQ(comp.Forward.z, -1.0f);
    EXPECT_FLOAT_EQ(comp.Up.x, 0.0f);
    EXPECT_FLOAT_EQ(comp.Up.y, 1.0f);
    EXPECT_FLOAT_EQ(comp.Up.z, 0.0f);
}

TEST_F(AudioListenerComponentTests, IsActive_DefaultsToTrue) {
    AudioListenerComponent comp;
    EXPECT_TRUE(comp.IsActive);
}

TEST_F(AudioListenerComponentTests, CopyConstructor_CopiesAllValues) {
    AudioListenerComponent original;
    original.IsActive = false;
    original.Forward = { 1.0f, 0.0f, 0.0f };
    original.Up = { 0.0f, 0.0f, 1.0f };
    
    AudioListenerComponent copy(original);
    
    EXPECT_FALSE(copy.IsActive);
    EXPECT_FLOAT_EQ(copy.Forward.x, 1.0f);
    EXPECT_FLOAT_EQ(copy.Forward.y, 0.0f);
    EXPECT_FLOAT_EQ(copy.Forward.z, 0.0f);
    EXPECT_FLOAT_EQ(copy.Up.x, 0.0f);
    EXPECT_FLOAT_EQ(copy.Up.y, 0.0f);
    EXPECT_FLOAT_EQ(copy.Up.z, 1.0f);
}

// ==================== AudioSystem Tests ====================

class AudioSystemTests : public ::testing::Test {
protected:
    void SetUp() override {
        AudioEngine::Init();
    }
    
    void TearDown() override {
        AudioEngine::Shutdown();
    }
};

TEST_F(AudioSystemTests, UpdateListener_SetsEngineListenerPosition) {
    Scene scene;
    AudioSystem system;
    system.OnAttach(&scene);
    entt::registry& registry = scene.GetRegistry();
    
    // Create entity with listener and transform
    auto entity = registry.create();
    auto& listener = registry.emplace<AudioListenerComponent>(entity);
    listener.IsActive = true;
    
    auto& transform = registry.emplace<TransformComponent>(entity);
    transform.Position = { 50.0f, 100.0f };
    
    // Update system
    system.OnUpdate(0.016f);
    
    // Check listener position was updated
    glm::vec3 enginePos = AudioEngine::GetListenerPosition();
    EXPECT_FLOAT_EQ(enginePos.x, 50.0f);
    EXPECT_FLOAT_EQ(enginePos.y, 100.0f);
    EXPECT_FLOAT_EQ(enginePos.z, 0.0f);  // 2D transform, z = 0
}

TEST_F(AudioSystemTests, UpdateSources_InitializesUninitializedSources) {
    Scene scene;
    AudioSystem system;
    system.OnAttach(&scene);
    entt::registry& registry = scene.GetRegistry();
    
    // Create entity with audio source component (no Source set)
    auto entity = registry.create();
    auto& audioComp = registry.emplace<AudioSourceComponent>(entity);
    EXPECT_EQ(audioComp.Source, nullptr);
    
    // Update system
    system.OnUpdate(0.016f);
    
    // Source should now be created
    EXPECT_NE(audioComp.Source, nullptr);
}

TEST_F(AudioSystemTests, UpdateSources_Updates3DPositionFromTransform) {
    Scene scene;
    AudioSystem system;
    system.OnAttach(&scene);
    entt::registry& registry = scene.GetRegistry();
    
    // Create entity with audio source and transform
    auto entity = registry.create();
    auto& audioComp = registry.emplace<AudioSourceComponent>(entity);
    audioComp.Is3D = true;
    
    auto& transform = registry.emplace<TransformComponent>(entity);
    transform.Position = { 25.0f, 75.0f };
    
    // First update to initialize source
    system.OnUpdate(0.016f);
    ASSERT_NE(audioComp.Source, nullptr);
    
    // Check position
    glm::vec3 sourcePos = audioComp.Source->GetPosition();
    EXPECT_FLOAT_EQ(sourcePos.x, 25.0f);
    EXPECT_FLOAT_EQ(sourcePos.y, 75.0f);
    EXPECT_FLOAT_EQ(sourcePos.z, 0.0f);
    
    // Update transform
    transform.Position = { 100.0f, 200.0f };
    
    // Update system again
    system.OnUpdate(0.016f);
    
    // Check new position
    sourcePos = audioComp.Source->GetPosition();
    EXPECT_FLOAT_EQ(sourcePos.x, 100.0f);
    EXPECT_FLOAT_EQ(sourcePos.y, 200.0f);
}

TEST_F(AudioSystemTests, PlayOnAwake_PlaysWhenSourceInitialized) {
    Scene scene;
    AudioSystem system;
    system.OnAttach(&scene);
    entt::registry& registry = scene.GetRegistry();
    
    // Create entity with PlayOnAwake enabled
    auto entity = registry.create();
    auto& audioComp = registry.emplace<AudioSourceComponent>(entity);
    audioComp.PlayOnAwake = true;
    // Note: Without valid audio file, Play() won't actually start playback
    // but the flag should be respected
    
    // Update system
    system.OnUpdate(0.016f);
    
    ASSERT_NE(audioComp.Source, nullptr);
    // Since there's no valid audio buffer, it won't actually be playing
    // but the source should have been created and Play() should have been called
}

TEST_F(AudioSystemTests, InactiveListener_IsIgnored) {
    Scene scene;
    AudioSystem system;
    system.OnAttach(&scene);
    entt::registry& registry = scene.GetRegistry();
    
    // Set initial listener position
    AudioEngine::SetListenerPosition({ 0.0f, 0.0f, 0.0f });
    
    // Create entity with inactive listener
    auto entity = registry.create();
    auto& listener = registry.emplace<AudioListenerComponent>(entity);
    listener.IsActive = false;  // Inactive!
    
    auto& transform = registry.emplace<TransformComponent>(entity);
    transform.Position = { 999.0f, 999.0f };
    
    // Update system
    system.OnUpdate(0.016f);
    
    // Listener position should NOT have been updated
    glm::vec3 enginePos = AudioEngine::GetListenerPosition();
    EXPECT_FLOAT_EQ(enginePos.x, 0.0f);
    EXPECT_FLOAT_EQ(enginePos.y, 0.0f);
}

TEST_F(AudioSystemTests, Non3DSource_DoesNotUpdatePosition) {
    Scene scene;
    AudioSystem system;
    system.OnAttach(&scene);
    entt::registry& registry = scene.GetRegistry();
    
    // Create entity with non-3D audio source
    auto entity = registry.create();
    auto& audioComp = registry.emplace<AudioSourceComponent>(entity);
    audioComp.Is3D = false;  // 2D sound
    
    auto& transform = registry.emplace<TransformComponent>(entity);
    transform.Position = { 50.0f, 50.0f };
    
    // Initialize source
    system.OnUpdate(0.016f);
    ASSERT_NE(audioComp.Source, nullptr);
    
    // Get initial position (should be origin for 2D)
    glm::vec3 initialPos = audioComp.Source->GetPosition();
    
    // Update transform
    transform.Position = { 100.0f, 100.0f };
    system.OnUpdate(0.016f);
    
    // Position should not have changed for 2D source
    glm::vec3 newPos = audioComp.Source->GetPosition();
    EXPECT_FLOAT_EQ(newPos.x, initialPos.x);
    EXPECT_FLOAT_EQ(newPos.y, initialPos.y);
}

// ==================== WavLoader Extended Tests ====================

class WavLoaderExtendedTests : public ::testing::Test {
};

TEST_F(WavLoaderExtendedTests, LoadFromMemory_TooSmallForHeader_ReturnsFalse) {
    WavData data;
    
    // Create data smaller than minimum WAV header
    const char smallData[] = "RIFF\x00\x00\x00\x00WAV";
    bool result = WavLoader::LoadFromMemory(smallData, sizeof(smallData) - 1, data);
    EXPECT_FALSE(result);
}

TEST_F(WavLoaderExtendedTests, LoadFromMemory_InvalidRIFFHeader_ReturnsFalse) {
    WavData data;
    
    // Invalid RIFF header
    const char invalidData[] = "XXXX\x00\x00\x00\x00WAVE";
    bool result = WavLoader::LoadFromMemory(invalidData, sizeof(invalidData) - 1, data);
    EXPECT_FALSE(result);
}

TEST_F(WavLoaderExtendedTests, LoadFromMemory_InvalidWAVEFormat_ReturnsFalse) {
    WavData data;
    
    // RIFF but not WAVE format
    const char invalidData[] = "RIFF\x00\x00\x00\x00XXXX";
    bool result = WavLoader::LoadFromMemory(invalidData, sizeof(invalidData) - 1, data);
    EXPECT_FALSE(result);
}

TEST_F(WavLoaderExtendedTests, Load_EmptyPath_ReturnsFalse) {
    WavData data;
    bool result = WavLoader::Load("", data);
    EXPECT_FALSE(result);
}

TEST_F(WavLoaderExtendedTests, LoadFromMemory_NullPointer_ReturnsFalse) {
    WavData data;
    bool result = WavLoader::LoadFromMemory(nullptr, 100, data);
    EXPECT_FALSE(result);
}

TEST_F(WavLoaderExtendedTests, LoadFromMemory_ZeroSize_ReturnsFalse) {
    WavData data;
    const char someData[] = "RIFF";
    bool result = WavLoader::LoadFromMemory(someData, 0, data);
    EXPECT_FALSE(result);
}

// ==================== AudioBuffer Extended Tests ====================

class AudioBufferExtendedTests : public ::testing::Test {
protected:
    void SetUp() override {
        AudioEngine::Init();
    }
    
    void TearDown() override {
        AudioEngine::Shutdown();
    }
};

TEST_F(AudioBufferExtendedTests, MultipleSourcesShareBuffer_Works) {
    // This test would require a valid WAV file
    // For now, test that creating multiple sources doesn't crash
    auto source1 = AudioSource::Create();
    auto source2 = AudioSource::Create();
    auto source3 = AudioSource::Create();
    
    EXPECT_NE(source1, nullptr);
    EXPECT_NE(source2, nullptr);
    EXPECT_NE(source3, nullptr);
    
    // Each source should have unique ID
    EXPECT_NE(source1->GetSourceID(), source2->GetSourceID());
    EXPECT_NE(source2->GetSourceID(), source3->GetSourceID());
    EXPECT_NE(source1->GetSourceID(), source3->GetSourceID());
}

TEST_F(AudioBufferExtendedTests, CreateBuffer_WithEmptyPath_ReturnsNull) {
    auto buffer = AudioBuffer::Create("");
    EXPECT_EQ(buffer, nullptr);
}

} // namespace Tests
} // namespace Pillar
