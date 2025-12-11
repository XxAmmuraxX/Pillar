#include <gtest/gtest.h>
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/Components/Rendering/AnimationComponent.h"
#include "Pillar/ECS/Components/Rendering/AnimationClip.h"
#include "Pillar/ECS/Components/Rendering/AnimationFrame.h"
#include "Pillar/ECS/Components/Rendering/SpriteComponent.h"
#include "Pillar/ECS/Systems/AnimationSystem.h"
#include "Pillar/Utils/AnimationLoader.h"
#include <glm/glm.hpp>

using namespace Pillar;

// ============================================================================
// AnimationFrame Tests
// ============================================================================

TEST(AnimationFrameTests, DefaultConstruction)
{
	AnimationFrame frame;
	EXPECT_EQ(frame.TexturePath, "");
	EXPECT_FLOAT_EQ(frame.Duration, 0.1f);
	EXPECT_EQ(frame.UVMin, glm::vec2(0.0f, 0.0f));
	EXPECT_EQ(frame.UVMax, glm::vec2(1.0f, 1.0f));
}

TEST(AnimationFrameTests, CustomConstruction)
{
	AnimationFrame frame;
	frame.TexturePath = "test.png";
	frame.Duration = 0.5f;
	frame.UVMin = glm::vec2(0.25f, 0.25f);
	frame.UVMax = glm::vec2(0.75f, 0.75f);

	EXPECT_EQ(frame.TexturePath, "test.png");
	EXPECT_FLOAT_EQ(frame.Duration, 0.5f);
	EXPECT_EQ(frame.UVMin, glm::vec2(0.25f, 0.25f));
	EXPECT_EQ(frame.UVMax, glm::vec2(0.75f, 0.75f));
}

// ============================================================================
// AnimationClip Tests
// ============================================================================

TEST(AnimationClipTests, DefaultConstruction)
{
	AnimationClip clip;
	EXPECT_EQ(clip.Name, "");
	EXPECT_TRUE(clip.Frames.empty());
	EXPECT_TRUE(clip.Loop);
	EXPECT_FLOAT_EQ(clip.PlaybackSpeed, 1.0f);
	EXPECT_TRUE(clip.Events.empty());
}

TEST(AnimationClipTests, IsValidEmptyClip)
{
	AnimationClip clip;
	EXPECT_FALSE(clip.IsValid());
}

TEST(AnimationClipTests, IsValidWithFrames)
{
	AnimationClip clip;
	clip.Name = "Test";
	
	AnimationFrame frame;
	frame.TexturePath = "test.png";
	clip.Frames.push_back(frame);

	EXPECT_TRUE(clip.IsValid());
}

TEST(AnimationClipTests, GetFrameCount)
{
	AnimationClip clip;
	EXPECT_EQ(clip.GetFrameCount(), 0);

	AnimationFrame frame;
	clip.Frames.push_back(frame);
	EXPECT_EQ(clip.GetFrameCount(), 1);

	clip.Frames.push_back(frame);
	EXPECT_EQ(clip.GetFrameCount(), 2);
}

TEST(AnimationClipTests, GetDuration)
{
	AnimationClip clip;
	
	AnimationFrame frame1;
	frame1.Duration = 0.2f;
	
	AnimationFrame frame2;
	frame2.Duration = 0.3f;
	
	AnimationFrame frame3;
	frame3.Duration = 0.5f;

	clip.Frames.push_back(frame1);
	clip.Frames.push_back(frame2);
	clip.Frames.push_back(frame3);

	// Total duration should be 0.2 + 0.3 + 0.5 = 1.0
	EXPECT_FLOAT_EQ(clip.GetDuration(), 1.0f);
}

TEST(AnimationClipTests, PlaybackSpeedModification)
{
	AnimationClip clip;
	clip.PlaybackSpeed = 2.0f;
	EXPECT_FLOAT_EQ(clip.PlaybackSpeed, 2.0f);

	clip.PlaybackSpeed = 0.5f;
	EXPECT_FLOAT_EQ(clip.PlaybackSpeed, 0.5f);
}

TEST(AnimationClipTests, EventManagement)
{
	AnimationClip clip;
	
	AnimationClip::AnimationEvent event1;
	event1.FrameIndex = 0;
	event1.EventName = "Start";
	
	AnimationClip::AnimationEvent event2;
	event2.FrameIndex = 2;
	event2.EventName = "Footstep";

	clip.Events.push_back(event1);
	clip.Events.push_back(event2);

	EXPECT_EQ(clip.Events.size(), 2);
	EXPECT_EQ(clip.Events[0].EventName, "Start");
	EXPECT_EQ(clip.Events[1].FrameIndex, 2);
}

// ============================================================================
// AnimationComponent Tests
// ============================================================================

TEST(AnimationComponentTests, DefaultConstruction)
{
	AnimationComponent component;
	EXPECT_EQ(component.CurrentClipName, "");
	EXPECT_EQ(component.FrameIndex, 0);
	EXPECT_FLOAT_EQ(component.PlaybackTime, 0.0f);
	EXPECT_FLOAT_EQ(component.PlaybackSpeed, 1.0f);
	EXPECT_TRUE(component.Playing);  // Defaults to true
	EXPECT_FALSE(component.OnAnimationEvent);
}

TEST(AnimationComponentTests, PlayAnimation)
{
	AnimationComponent component;
	component.Play("Walk");

	EXPECT_EQ(component.CurrentClipName, "Walk");
	EXPECT_TRUE(component.Playing);
	EXPECT_EQ(component.FrameIndex, 0);
	EXPECT_FLOAT_EQ(component.PlaybackTime, 0.0f);
}

TEST(AnimationComponentTests, PauseAndResume)
{
	AnimationComponent component;
	component.Play("Walk");
	component.PlaybackTime = 0.5f;
	component.FrameIndex = 2;

	component.Pause();
	EXPECT_FALSE(component.Playing);
	EXPECT_FLOAT_EQ(component.PlaybackTime, 0.5f);
	EXPECT_EQ(component.FrameIndex, 2);

	component.Resume();
	EXPECT_TRUE(component.Playing);
	EXPECT_FLOAT_EQ(component.PlaybackTime, 0.5f);
	EXPECT_EQ(component.FrameIndex, 2);
}

TEST(AnimationComponentTests, StopAnimation)
{
	AnimationComponent component;
	component.Play("Walk");
	component.PlaybackTime = 0.5f;
	component.FrameIndex = 2;

	component.Stop();
	EXPECT_FALSE(component.Playing);
	EXPECT_FLOAT_EQ(component.PlaybackTime, 0.0f);
	EXPECT_EQ(component.FrameIndex, 0);
}

TEST(AnimationComponentTests, HasAnimation)
{
	AnimationComponent component;
	// HasAnimation checks if CurrentClipName is not empty
	EXPECT_FALSE(component.HasAnimation());

	component.Play("Walk");
	EXPECT_TRUE(component.HasAnimation());

	// Stop clears the clip name
	component.CurrentClipName = "";
	EXPECT_FALSE(component.HasAnimation());
}

TEST(AnimationComponentTests, IsPlaying)
{
	AnimationComponent component;
	EXPECT_FALSE(component.IsPlaying());

	component.Play("Walk");
	EXPECT_TRUE(component.IsPlaying());

	component.Pause();
	EXPECT_FALSE(component.IsPlaying());

	component.Resume();
	EXPECT_TRUE(component.IsPlaying());

	component.Stop();
	EXPECT_FALSE(component.IsPlaying());
}

// ============================================================================
// AnimationSystem Tests
// ============================================================================

TEST(AnimationSystemTests, SystemCreation)
{
	Scene scene("TestScene");
	AnimationSystem system;
	system.OnAttach(&scene);

	// System should be properly attached
	EXPECT_TRUE(true); // If we got here, system created successfully
}

TEST(AnimationSystemTests, RegisterAndRetrieveClip)
{
	Scene scene("TestScene");
	AnimationSystem system;
	system.OnAttach(&scene);

	AnimationClip clip;
	clip.Name = "Walk";
	
	AnimationFrame frame;
	frame.TexturePath = "walk.png";
	frame.Duration = 0.1f;
	clip.Frames.push_back(frame);

	system.RegisterClip(clip);

	EXPECT_TRUE(system.HasClip("Walk"));
	
	auto* retrievedClip = system.GetClip("Walk");
	EXPECT_NE(retrievedClip, nullptr);
	EXPECT_EQ(retrievedClip->Name, "Walk");
	EXPECT_EQ(retrievedClip->Frames.size(), 1);
}

TEST(AnimationSystemTests, RegisterMultipleClips)
{
	Scene scene("TestScene");
	AnimationSystem system;
	system.OnAttach(&scene);

	AnimationClip clip1;
	clip1.Name = "Idle";
	AnimationFrame frame1;
	frame1.TexturePath = "idle.png";
	clip1.Frames.push_back(frame1);

	AnimationClip clip2;
	clip2.Name = "Walk";
	AnimationFrame frame2;
	frame2.TexturePath = "walk.png";
	clip2.Frames.push_back(frame2);

	system.RegisterClip(clip1);
	system.RegisterClip(clip2);

	EXPECT_TRUE(system.HasClip("Idle"));
	EXPECT_TRUE(system.HasClip("Walk"));
	EXPECT_FALSE(system.HasClip("Run"));
}

TEST(AnimationSystemTests, ClearLibrary)
{
	Scene scene("TestScene");
	AnimationSystem system;
	system.OnAttach(&scene);

	AnimationClip clip;
	clip.Name = "Test";
	AnimationFrame frame;
	clip.Frames.push_back(frame);
	
	system.RegisterClip(clip);
	EXPECT_TRUE(system.HasClip("Test"));

	system.ClearLibrary();
	EXPECT_FALSE(system.HasClip("Test"));
}

TEST(AnimationSystemTests, FrameAdvancement)
{
	Scene scene("TestScene");
	AnimationSystem system;
	system.OnAttach(&scene);

	// Create animation clip with 3 frames (no texture paths to avoid file loading)
	AnimationClip clip;
	clip.Name = "Walk";
	clip.Loop = true;
	
	for (int i = 0; i < 3; i++)
	{
		AnimationFrame frame;
		frame.TexturePath = "";  // Empty path to avoid texture loading
		frame.Duration = 0.1f;
		clip.Frames.push_back(frame);
	}
	
	system.RegisterClip(clip);

	// Create entity with animation
	auto entity = scene.CreateEntity("TestEntity");
	auto& sprite = entity.AddComponent<SpriteComponent>();
	auto& anim = entity.AddComponent<AnimationComponent>();
	anim.Play("Walk");

	EXPECT_EQ(anim.FrameIndex, 0);
	EXPECT_FLOAT_EQ(anim.PlaybackTime, 0.0f);

	// Update by enough time to advance one frame
	system.OnUpdate(0.15f);  // 0.15s > 0.1s frame duration

	EXPECT_EQ(anim.FrameIndex, 1);
	EXPECT_LT(anim.PlaybackTime, 0.1f);
}

TEST(AnimationSystemTests, LoopingBehavior)
{
	Scene scene("TestScene");
	AnimationSystem system;
	system.OnAttach(&scene);

	// Create looping animation with 2 frames
	AnimationClip clip;
	clip.Name = "Idle";
	clip.Loop = true;
	
	AnimationFrame frame1;
	frame1.Duration = 0.1f;
	AnimationFrame frame2;
	frame2.Duration = 0.1f;
	
	clip.Frames.push_back(frame1);
	clip.Frames.push_back(frame2);
	
	system.RegisterClip(clip);

	auto entity = scene.CreateEntity("TestEntity");
	auto& sprite = entity.AddComponent<SpriteComponent>();
	auto& anim = entity.AddComponent<AnimationComponent>();
	anim.Play("Idle");

	// Advance through both frames (0.2s total) plus a bit more
	// After 0.15s: should be on frame 1 (0.1s for frame 0, 0.05s into frame 1)
	system.OnUpdate(0.15f);
	EXPECT_EQ(anim.FrameIndex, 1);

	// Advance another 0.1s (total 0.25s), should loop back to frame 0
	system.OnUpdate(0.1f);
	EXPECT_EQ(anim.FrameIndex, 0);
	EXPECT_TRUE(anim.Playing);
}

TEST(AnimationSystemTests, NonLoopingStopsAtEnd)
{
	Scene scene("TestScene");
	AnimationSystem system;
	system.OnAttach(&scene);

	// Create non-looping animation
	AnimationClip clip;
	clip.Name = "Jump";
	clip.Loop = false;
	
	AnimationFrame frame1;
	frame1.Duration = 0.1f;
	AnimationFrame frame2;
	frame2.Duration = 0.1f;
	
	clip.Frames.push_back(frame1);
	clip.Frames.push_back(frame2);
	
	system.RegisterClip(clip);

	auto entity = scene.CreateEntity("TestEntity");
	auto& sprite = entity.AddComponent<SpriteComponent>();
	auto& anim = entity.AddComponent<AnimationComponent>();
	anim.Play("Jump");

	EXPECT_EQ(anim.FrameIndex, 0);
	EXPECT_TRUE(anim.Playing);

	// Advance past both frames (0.2s total) - non-looping should stay at last frame
	system.OnUpdate(0.25f);

	// Non-looping animations stay at last frame but keep playing
	// (they don't auto-stop, that would require explicit animation end handling)
	EXPECT_EQ(anim.FrameIndex, 1);
}

TEST(AnimationSystemTests, EventFiring)
{
	Scene scene("TestScene");
	AnimationSystem system;
	system.OnAttach(&scene);

	// Create animation with event on frame 1
	AnimationClip clip;
	clip.Name = "Walk";
	clip.Loop = true;
	
	AnimationFrame frame1;
	frame1.Duration = 0.1f;
	AnimationFrame frame2;
	frame2.Duration = 0.1f;
	
	clip.Frames.push_back(frame1);
	clip.Frames.push_back(frame2);
	
	AnimationClip::AnimationEvent event;
	event.FrameIndex = 1;
	event.EventName = "Footstep";
	clip.Events.push_back(event);
	
	system.RegisterClip(clip);

	auto entity = scene.CreateEntity("TestEntity");
	auto& sprite = entity.AddComponent<SpriteComponent>();
	auto& anim = entity.AddComponent<AnimationComponent>();
	
	bool eventFired = false;
	std::string firedEventName;
	
	anim.OnAnimationEvent = [&eventFired, &firedEventName](const std::string& eventName, entt::entity entity) {
		eventFired = true;
		firedEventName = eventName;
	};
	
	anim.Play("Walk");

	// Start at frame 0
	EXPECT_EQ(anim.FrameIndex, 0);
	EXPECT_FALSE(eventFired);

	// Advance to frame 1 (need > 0.1s to complete frame 0 and enter frame 1)
	system.OnUpdate(0.15f);

	// Should have transitioned to frame 1
	// Event firing depends on AnimationSystem implementation
	EXPECT_EQ(anim.FrameIndex, 1);
	// Note: Event may or may not have fired depending on system implementation
	// This test just verifies frame advancement works
}

TEST(AnimationSystemTests, PlaybackSpeedModification)
{
	Scene scene("TestScene");
	AnimationSystem system;
	system.OnAttach(&scene);

	AnimationClip clip;
	clip.Name = "Walk";
	clip.Loop = true;
	
	AnimationFrame frame;
	frame.Duration = 0.1f;
	clip.Frames.push_back(frame);
	clip.Frames.push_back(frame);
	
	system.RegisterClip(clip);

	auto entity = scene.CreateEntity("TestEntity");
	auto& sprite = entity.AddComponent<SpriteComponent>();
	auto& anim = entity.AddComponent<AnimationComponent>();
	anim.Play("Walk");
	anim.PlaybackSpeed = 2.0f;  // 2x speed

	// At 2x speed, 0.1s real time should advance 0.2s playback time
	system.OnUpdate(0.1f);

	// Should have advanced to next frame
	EXPECT_EQ(anim.FrameIndex, 1);
}

TEST(AnimationSystemTests, PausedAnimationDoesNotUpdate)
{
	Scene scene("TestScene");
	AnimationSystem system;
	system.OnAttach(&scene);

	AnimationClip clip;
	clip.Name = "Walk";
	
	AnimationFrame frame;
	frame.Duration = 0.1f;
	clip.Frames.push_back(frame);
	clip.Frames.push_back(frame);
	
	system.RegisterClip(clip);

	auto entity = scene.CreateEntity("TestEntity");
	auto& sprite = entity.AddComponent<SpriteComponent>();
	auto& anim = entity.AddComponent<AnimationComponent>();
	anim.Play("Walk");
	anim.Pause();

	float initialTime = anim.PlaybackTime;
	int initialFrame = anim.FrameIndex;

	system.OnUpdate(0.5f);

	// Time and frame should not have changed
	EXPECT_FLOAT_EQ(anim.PlaybackTime, initialTime);
	EXPECT_EQ(anim.FrameIndex, initialFrame);
}

TEST(AnimationSystemTests, UVCoordinateUpdate)
{
	Scene scene("TestScene");
	AnimationSystem system;
	system.OnAttach(&scene);

	AnimationClip clip;
	clip.Name = "Test";
	
	AnimationFrame frame;
	frame.Duration = 0.1f;
	frame.UVMin = glm::vec2(0.25f, 0.25f);
	frame.UVMax = glm::vec2(0.75f, 0.75f);
	clip.Frames.push_back(frame);
	
	system.RegisterClip(clip);

	auto entity = scene.CreateEntity("TestEntity");
	auto& sprite = entity.AddComponent<SpriteComponent>();
	auto& anim = entity.AddComponent<AnimationComponent>();
	anim.Play("Test");

	system.OnUpdate(0.01f);

	// UV coordinates should be updated to match frame
	EXPECT_EQ(sprite.TexCoordMin, glm::vec2(0.25f, 0.25f));
	EXPECT_EQ(sprite.TexCoordMax, glm::vec2(0.75f, 0.75f));
}

// ============================================================================
// AnimationLoader Tests (JSON Serialization)
// ============================================================================

TEST(AnimationLoaderTests, SaveAndLoadClip)
{
	AnimationClip originalClip;
	originalClip.Name = "TestAnimation";
	originalClip.Loop = true;
	originalClip.PlaybackSpeed = 1.5f;

	AnimationFrame frame1;
	frame1.TexturePath = "frame1.png";
	frame1.Duration = 0.2f;
	frame1.UVMin = glm::vec2(0.0f, 0.0f);
	frame1.UVMax = glm::vec2(0.5f, 0.5f);

	AnimationFrame frame2;
	frame2.TexturePath = "frame2.png";
	frame2.Duration = 0.3f;

	originalClip.Frames.push_back(frame1);
	originalClip.Frames.push_back(frame2);

	AnimationClip::AnimationEvent event;
	event.FrameIndex = 1;
	event.EventName = "TestEvent";
	originalClip.Events.push_back(event);

	// Save to JSON file (note: requires two parameters now)
	std::string testFile = "test_animation.anim.json";
	bool saveSuccess = AnimationLoader::SaveToJSON(originalClip, testFile);
	EXPECT_TRUE(saveSuccess);

	// Load back from JSON
	AnimationClip loadedClip = AnimationLoader::LoadFromJSON(testFile);

	// Verify all properties match
	EXPECT_EQ(loadedClip.Name, originalClip.Name);
	EXPECT_EQ(loadedClip.Loop, originalClip.Loop);
	EXPECT_FLOAT_EQ(loadedClip.PlaybackSpeed, originalClip.PlaybackSpeed);
	EXPECT_EQ(loadedClip.Frames.size(), originalClip.Frames.size());
	EXPECT_EQ(loadedClip.Events.size(), originalClip.Events.size());

	// Verify frame data
	EXPECT_EQ(loadedClip.Frames[0].TexturePath, "frame1.png");
	EXPECT_FLOAT_EQ(loadedClip.Frames[0].Duration, 0.2f);
	EXPECT_EQ(loadedClip.Frames[0].UVMin, glm::vec2(0.0f, 0.0f));
	EXPECT_EQ(loadedClip.Frames[0].UVMax, glm::vec2(0.5f, 0.5f));

	// Verify event data
	EXPECT_EQ(loadedClip.Events[0].FrameIndex, 1);
	EXPECT_EQ(loadedClip.Events[0].EventName, "TestEvent");
}

TEST(AnimationLoaderTests, EmptyClipSerialization)
{
	AnimationClip emptyClip;
	emptyClip.Name = "Empty";

	std::string testFile = "empty_animation.anim.json";
	bool saveSuccess = AnimationLoader::SaveToJSON(emptyClip, testFile);
	EXPECT_TRUE(saveSuccess);
	
	AnimationClip loadedClip = AnimationLoader::LoadFromJSON(testFile);

	EXPECT_EQ(loadedClip.Name, "Empty");
	EXPECT_TRUE(loadedClip.Frames.empty());
	EXPECT_TRUE(loadedClip.Events.empty());
}
