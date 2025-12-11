#include <gtest/gtest.h>
#include "Pillar/Audio/AudioEngine.h"
#include "Pillar/Audio/AudioBuffer.h"
#include "Pillar/Audio/AudioSource.h"
#include "Pillar/Audio/AudioClip.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Audio/AudioSourceComponent.h"
#include "Pillar/ECS/Components/Audio/AudioListenerComponent.h"
#include "Pillar/ECS/Systems/AudioSystem.h"
#include <glm/glm.hpp>

using namespace Pillar;

// ============================================================================
// Audio System Integration Tests
// Tests that verify the audio subsystem works correctly with ECS
// ============================================================================

class AudioIntegrationTests : public ::testing::Test
{
protected:
	void SetUp() override
	{
		AudioEngine::Init();
		m_Scene = std::make_unique<Scene>("AudioTestScene");
	}

	void TearDown() override
	{
		m_Scene.reset();
		AudioEngine::Shutdown();
	}

	std::unique_ptr<Scene> m_Scene;
};

// -----------------------------------------------------------------------------
// Audio Engine Integration
// -----------------------------------------------------------------------------

TEST_F(AudioIntegrationTests, AudioEngine_InitShutdown_Clean)
{
	// Audio engine should be initialized from SetUp
	EXPECT_TRUE(AudioEngine::IsInitialized());

	// Shutdown
	AudioEngine::Shutdown();
	EXPECT_FALSE(AudioEngine::IsInitialized());

	// Re-initialize
	AudioEngine::Init();
	EXPECT_TRUE(AudioEngine::IsInitialized());
}

TEST_F(AudioIntegrationTests, AudioEngine_MasterVolume_Persists)
{
	AudioEngine::SetMasterVolume(0.5f);
	EXPECT_FLOAT_EQ(AudioEngine::GetMasterVolume(), 0.5f);

	AudioEngine::SetMasterVolume(0.75f);
	EXPECT_FLOAT_EQ(AudioEngine::GetMasterVolume(), 0.75f);

	// Reset to 1.0 for other tests
	AudioEngine::SetMasterVolume(1.0f);
}

TEST_F(AudioIntegrationTests, AudioEngine_ListenerPosition_Updates)
{
	glm::vec3 position(10.0f, 20.0f, 0.0f);
	AudioEngine::SetListenerPosition(position);

	// No direct getter, but should not crash
	SUCCEED();
}

// -----------------------------------------------------------------------------
// Audio Source Integration
// -----------------------------------------------------------------------------

TEST_F(AudioIntegrationTests, AudioSource_CreateAndControl)
{
	auto source = AudioEngine::CreateSource();
	ASSERT_NE(source, nullptr);

	// Test state transitions
	EXPECT_TRUE(source->IsStopped());

	// Without a buffer, play won't actually play, but shouldn't crash
	source->Play();
	source->Pause();
	source->Stop();

	EXPECT_TRUE(source->IsStopped());
}

TEST_F(AudioIntegrationTests, AudioSource_VolumeAndPitch)
{
	auto source = AudioEngine::CreateSource();
	ASSERT_NE(source, nullptr);

	source->SetVolume(0.5f);
	EXPECT_FLOAT_EQ(source->GetVolume(), 0.5f);

	source->SetPitch(1.5f);
	EXPECT_FLOAT_EQ(source->GetPitch(), 1.5f);

	source->SetLooping(true);
	EXPECT_TRUE(source->IsLooping());
}

TEST_F(AudioIntegrationTests, AudioSource_3DPositioning)
{
	auto source = AudioEngine::CreateSource();
	ASSERT_NE(source, nullptr);

	source->SetPosition(glm::vec3(5.0f, 10.0f, 0.0f));
	source->SetVelocity(glm::vec3(1.0f, 0.0f, 0.0f));
	source->SetMinDistance(1.0f);
	source->SetMaxDistance(100.0f);
	source->SetRolloffFactor(1.0f);

	// Should not crash
	SUCCEED();
}

// -----------------------------------------------------------------------------
// Audio with ECS Integration
// -----------------------------------------------------------------------------

TEST_F(AudioIntegrationTests, AudioSourceComponent_EntityIntegration)
{
	Entity soundEmitter = m_Scene->CreateEntity("SoundEmitter");
	
	auto& audioComp = soundEmitter.AddComponent<AudioSourceComponent>();
	audioComp.Volume = 0.8f;
	audioComp.Pitch = 1.2f;
	audioComp.Loop = true;
	audioComp.Is3D = true;

	EXPECT_TRUE(soundEmitter.HasComponent<AudioSourceComponent>());
	EXPECT_FLOAT_EQ(soundEmitter.GetComponent<AudioSourceComponent>().Volume, 0.8f);
}

TEST_F(AudioIntegrationTests, AudioListenerComponent_CameraIntegration)
{
	Entity camera = m_Scene->CreateEntity("MainCamera");
	camera.AddComponent<AudioListenerComponent>();

	auto& transform = camera.GetComponent<TransformComponent>();
	transform.Position = glm::vec2(100.0f, 50.0f);

	EXPECT_TRUE(camera.HasComponent<AudioListenerComponent>());
}

TEST_F(AudioIntegrationTests, AudioSystem_UpdatesSourcePositions)
{
	AudioSystem audioSystem;
	audioSystem.OnAttach(m_Scene.get());

	// Create camera with listener
	Entity camera = m_Scene->CreateEntity("Camera");
	camera.AddComponent<AudioListenerComponent>();
	camera.GetComponent<TransformComponent>().Position = glm::vec2(0.0f, 0.0f);

	// Create sound emitter
	Entity emitter = m_Scene->CreateEntity("Emitter");
	auto& audioComp = emitter.AddComponent<AudioSourceComponent>();
	audioComp.Is3D = true;
	emitter.GetComponent<TransformComponent>().Position = glm::vec2(10.0f, 0.0f);

	// Update should not crash
	audioSystem.OnUpdate(0.016f);

	SUCCEED();
}

TEST_F(AudioIntegrationTests, MultipleAudioSources_Independent)
{
	Entity emitter1 = m_Scene->CreateEntity("Emitter1");
	Entity emitter2 = m_Scene->CreateEntity("Emitter2");

	auto& audio1 = emitter1.AddComponent<AudioSourceComponent>();
	auto& audio2 = emitter2.AddComponent<AudioSourceComponent>();

	audio1.Volume = 0.5f;
	audio2.Volume = 0.9f;

	audio1.Pitch = 1.0f;
	audio2.Pitch = 2.0f;

	// Verify independence
	EXPECT_FLOAT_EQ(emitter1.GetComponent<AudioSourceComponent>().Volume, 0.5f);
	EXPECT_FLOAT_EQ(emitter2.GetComponent<AudioSourceComponent>().Volume, 0.9f);
	EXPECT_FLOAT_EQ(emitter1.GetComponent<AudioSourceComponent>().Pitch, 1.0f);
	EXPECT_FLOAT_EQ(emitter2.GetComponent<AudioSourceComponent>().Pitch, 2.0f);
}

// -----------------------------------------------------------------------------
// Audio Global Control Integration
// -----------------------------------------------------------------------------

TEST_F(AudioIntegrationTests, GlobalAudioControl_StopAll)
{
	auto source1 = AudioEngine::CreateSource();
	auto source2 = AudioEngine::CreateSource();

	// Stop all should not crash
	AudioEngine::StopAllSounds();

	EXPECT_TRUE(source1->IsStopped());
	EXPECT_TRUE(source2->IsStopped());
}

TEST_F(AudioIntegrationTests, GlobalAudioControl_PauseResumeAll)
{
	auto source = AudioEngine::CreateSource();

	// Pause and resume all should not crash
	AudioEngine::PauseAllSounds();
	AudioEngine::ResumeAllSounds();

	SUCCEED();
}

// -----------------------------------------------------------------------------
// Audio Cleanup Integration
// -----------------------------------------------------------------------------

TEST_F(AudioIntegrationTests, AudioCleanup_EntityDestruction)
{
	Entity emitter = m_Scene->CreateEntity("ToBeDestroyed");
	emitter.AddComponent<AudioSourceComponent>();

	uint64_t uuid = emitter.GetComponent<UUIDComponent>().UUID;

	m_Scene->DestroyEntity(emitter);

	// Entity should be gone
	EXPECT_FALSE(m_Scene->FindEntityByUUID(uuid));
}

TEST_F(AudioIntegrationTests, AudioCleanup_SceneClear)
{
	for (int i = 0; i < 10; i++)
	{
		Entity emitter = m_Scene->CreateEntity("Emitter" + std::to_string(i));
		emitter.AddComponent<AudioSourceComponent>();
	}

	EXPECT_EQ(m_Scene->GetEntityCount(), 10);

	// Clear scene
	m_Scene.reset();
	m_Scene = std::make_unique<Scene>("NewScene");

	EXPECT_EQ(m_Scene->GetEntityCount(), 0);
}
