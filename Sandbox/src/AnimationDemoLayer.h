#pragma once

#include "Pillar.h"
#include "Pillar/Renderer/Renderer2DBackend.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/Systems/AnimationSystem.h"
#include "Pillar/ECS/Components/Rendering/SpriteComponent.h"
#include "Pillar/ECS/Components/Rendering/AnimationComponent.h"
#include "Pillar/ECS/Components/Rendering/AnimationClip.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>
#include <string>

class AnimationDemoLayer : public Pillar::Layer
{
public:
	AnimationDemoLayer() 
		: Layer("AnimationDemoLayer"), 
		  m_CameraController(16.0f / 9.0f, true) // Aspect ratio, enable rotation
	{
	}

	void OnAttach() override 
	{ 
		Layer::OnAttach();
		PIL_INFO("AnimationDemoLayer attached - Testing Animation System");
		
		// Create scene and animation system
		m_Scene = std::make_shared<Pillar::Scene>();
		m_AnimSystem = std::make_shared<Pillar::AnimationSystem>();
		m_AnimSystem->OnAttach(m_Scene.get());
		
		// Create sample animation clips programmatically
		CreateSampleAnimations();
		
		// Log available animations
		PIL_INFO("Created {0} sample animations", m_AnimSystem->GetClipCount());
		
		PIL_INFO("AnimationDemoLayer initialized!");
		PIL_INFO("Controls:");
		PIL_INFO("  - Left Click: Spawn animated entity");
		PIL_INFO("  - 1/2/3: Switch active animation");
		PIL_INFO("  - SPACE: Toggle global pause");
		PIL_INFO("  - WASD: Move camera, Q/E: Rotate, Mouse Wheel: Zoom");
	}
	
	void OnDetach() override 
	{ 
		if (m_AnimSystem)
		{
			m_AnimSystem->OnDetach();
		}
		m_AnimatedEntities.clear();
		m_Scene.reset();
		Layer::OnDetach();
	}
	
	void OnUpdate(float dt) override
	{
		// Update camera controller
		m_CameraController.OnUpdate(dt);

		// Apply global pause
		float adjustedDt = m_GlobalPaused ? 0.0f : dt * m_GlobalSpeed;

		// Update animation system
		if (m_AnimSystem)
		{
			m_AnimSystem->OnUpdate(adjustedDt);
		}

		// Clear screen
		Pillar::Renderer::SetClearColor({ 0.15f, 0.15f, 0.2f, 1.0f });
		Pillar::Renderer::Clear();

		// Begin scene
		Pillar::Renderer2DBackend::BeginScene(m_CameraController.GetCamera());

		// Render all animated entities
		auto& registry = m_Scene->GetRegistry();
		auto view = registry.view<Pillar::TransformComponent, Pillar::SpriteComponent>();
		
		for (auto entity : view)
		{
			auto& transform = view.get<Pillar::TransformComponent>(entity);
			auto& sprite = view.get<Pillar::SpriteComponent>(entity);
			
			// Only render if has valid texture
			if (sprite.Texture)
			{
			// Draw textured quad with UV coordinates from sprite
			Pillar::Renderer2DBackend::DrawQuad(
				glm::vec3(transform.Position, 0.0f),  // Convert vec2 to vec3
				transform.Scale,
				sprite.Color,
				sprite.Texture,
				sprite.TexCoordMin,  // Pass UV min
				sprite.TexCoordMax,  // Pass UV max
				sprite.FlipX,        // Horizontal flip
				sprite.FlipY         // Vertical flip
			);
			}
		}

		Pillar::Renderer2DBackend::EndScene();
	}
	
	void OnEvent(Pillar::Event& event) override
	{
		// Pass events to camera controller
		m_CameraController.OnEvent(event);

		// Handle mouse clicks for spawning entities
		if (event.GetEventType() == Pillar::EventType::MouseButtonPressed)
		{
			Pillar::MouseButtonPressedEvent& mouseEvent = static_cast<Pillar::MouseButtonPressedEvent&>(event);
			if (mouseEvent.GetMouseButton() == PIL_MOUSE_BUTTON_LEFT)
			{
				SpawnAnimatedEntity();
				event.Handled = true;
			}
		}

		// Handle keyboard input
		if (event.GetEventType() == Pillar::EventType::KeyPressed)
		{
			Pillar::KeyPressedEvent& keyEvent = static_cast<Pillar::KeyPressedEvent&>(event);
			if (keyEvent.GetRepeatCount() == 0)  // Only on first press
			{
				const char* animNames[] = { "Idle", "Walk", "Jump" };
				auto& registry = m_Scene->GetRegistry();
				
				switch (keyEvent.GetKeyCode())
				{
					case PIL_KEY_1:
						m_CurrentAnimIndex = 0;
						// Update all existing entities
						for (auto entityID : m_AnimatedEntities)
						{
							if (registry.valid(entityID) && registry.all_of<Pillar::AnimationComponent>(entityID))
							{
								auto& anim = registry.get<Pillar::AnimationComponent>(entityID);
								anim.Play(animNames[m_CurrentAnimIndex]);
							}
						}
						PIL_INFO("Switched all entities to animation: Idle");
						break;
					case PIL_KEY_2:
						m_CurrentAnimIndex = 1;
						// Update all existing entities
						for (auto entityID : m_AnimatedEntities)
						{
							if (registry.valid(entityID) && registry.all_of<Pillar::AnimationComponent>(entityID))
							{
								auto& anim = registry.get<Pillar::AnimationComponent>(entityID);
								anim.Play(animNames[m_CurrentAnimIndex]);
							}
						}
						PIL_INFO("Switched all entities to animation: Walk");
						break;
					case PIL_KEY_3:
						m_CurrentAnimIndex = 2;
						// Update all existing entities
						for (auto entityID : m_AnimatedEntities)
						{
							if (registry.valid(entityID) && registry.all_of<Pillar::AnimationComponent>(entityID))
							{
								auto& anim = registry.get<Pillar::AnimationComponent>(entityID);
								anim.Play(animNames[m_CurrentAnimIndex]);
							}
						}
						PIL_INFO("Switched all entities to animation: Jump");
						break;
					case PIL_KEY_SPACE:
						m_GlobalPaused = !m_GlobalPaused;
						PIL_INFO("Global animation pause: {0}", m_GlobalPaused ? "ON" : "OFF");
						break;
				}
			}
		}
	}

	void OnImGuiRender() override
	{
		ImGui::Begin("Animation Demo Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

		ImGui::Text("Animation System Test");
		ImGui::Separator();

		// Stats
		ImGui::Text("Animated Entities: %d", (int)m_AnimatedEntities.size());
		ImGui::Text("Available Clips: %d", (int)m_AnimSystem->GetClipCount());
		
		ImGui::Separator();

		// Global controls
		if (ImGui::Button(m_GlobalPaused ? "Resume All" : "Pause All"))
		{
			m_GlobalPaused = !m_GlobalPaused;
		}
		
		ImGui::SliderFloat("Global Speed", &m_GlobalSpeed, 0.1f, 5.0f, "%.1fx");
		
		ImGui::Separator();

		// Animation selection
		ImGui::Text("Current Animation:");
		const char* animNames[] = { "Idle", "Walk", "Jump" };
		if (ImGui::Combo("##AnimSelection", &m_CurrentAnimIndex, animNames, 3))
		{
			PIL_INFO("Switched to animation: {0}", animNames[m_CurrentAnimIndex]);
		}

		ImGui::Separator();

		// Spawn controls
		if (ImGui::Button("Spawn Animated Entity", ImVec2(200, 30)))
		{
			SpawnAnimatedEntity();
		}
		ImGui::Text("(or Left Click in scene)");

		if (ImGui::Button("Clear All Entities", ImVec2(200, 30)))
		{
			ClearAllEntities();
		}

		ImGui::Separator();

		// Event log
		ImGui::Text("Animation Events:");
		ImGui::BeginChild("EventLog", ImVec2(300, 100), true);
		for (const auto& logEntry : m_EventLog)
		{
			ImGui::TextWrapped("%s", logEntry.c_str());
		}
		// Auto-scroll to bottom
		if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
			ImGui::SetScrollHereY(1.0f);
		ImGui::EndChild();

		if (ImGui::Button("Clear Log"))
		{
			m_EventLog.clear();
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Controls:");
		ImGui::BulletText("1/2/3: Switch animation type");
		ImGui::BulletText("SPACE: Toggle pause");
		ImGui::BulletText("Left Click: Spawn entity");

		ImGui::End();

		// Individual entity controls (if any entities exist)
		if (!m_AnimatedEntities.empty())
		{
			ImGui::Begin("Entity Inspector", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

			ImGui::Text("Inspecting: Entity %d", m_SelectedEntityIndex);
			if (ImGui::Button("< Prev"))
			{
				m_SelectedEntityIndex = (m_SelectedEntityIndex - 1 + m_AnimatedEntities.size()) % m_AnimatedEntities.size();
			}
			ImGui::SameLine();
			if (ImGui::Button("Next >"))
			{
				m_SelectedEntityIndex = (m_SelectedEntityIndex + 1) % m_AnimatedEntities.size();
			}

			if (m_SelectedEntityIndex < m_AnimatedEntities.size())
			{
				auto entityID = m_AnimatedEntities[m_SelectedEntityIndex];
				auto& registry = m_Scene->GetRegistry();
				
				if (registry.valid(entityID))
				{
					auto& anim = registry.get<Pillar::AnimationComponent>(entityID);
					auto& transform = registry.get<Pillar::TransformComponent>(entityID);
					
					ImGui::Separator();
					ImGui::Text("Animation: %s", anim.CurrentClipName.c_str());
					ImGui::Text("Frame: %d", anim.FrameIndex);
					ImGui::Text("Time: %.2f", anim.PlaybackTime);
					ImGui::Text("Playing: %s", anim.IsPlaying() ? "Yes" : "No");
					
					ImGui::Separator();
					if (ImGui::Button(anim.Playing ? "Pause" : "Play"))
					{
						if (anim.Playing)
							anim.Pause();
						else
							anim.Resume();
					}
					ImGui::SameLine();
					if (ImGui::Button("Stop"))
					{
						anim.Stop();
					}
					
					ImGui::SliderFloat("Speed", &anim.PlaybackSpeed, 0.1f, 3.0f, "%.1fx");
					
					ImGui::Separator();
					ImGui::Text("Sprite Flipping:");
					auto& sprite = registry.get<Pillar::SpriteComponent>(entityID);
					ImGui::Checkbox("Flip X (Horizontal)", &sprite.FlipX);
					ImGui::Checkbox("Flip Y (Vertical)", &sprite.FlipY);
					
					ImGui::Separator();
					ImGui::Text("Transform:");
					float pos[2] = { transform.Position.x, transform.Position.y };
					if (ImGui::DragFloat2("Position", pos, 0.1f))
					{
						transform.Position = glm::vec2(pos[0], pos[1]);
					}
				}
			}

			ImGui::End();
		}
	}

private:
	void CreateSampleAnimations()
	{
		// Note: In a real game, these would be loaded from JSON files
		// For this demo, we create them programmatically

		// Animation 1: Idle (first frame of walk cycle, looping)
		{
			Pillar::AnimationClip idle;
			idle.Name = "Idle";
			idle.Loop = true;
			idle.PlaybackSpeed = 1.0f;
			
			// Use first frame of walk cycle for idle
			Pillar::AnimationFrame frame;
			frame.TexturePath = "character_walk_cycle.png";
			frame.Duration = 1.0f;
			// First frame (leftmost): 0.0 to 1/6 = 0.1667
			frame.UVMin = { 0.0f, 0.0f };
			frame.UVMax = { 0.1667f, 1.0f };
			idle.Frames.push_back(frame);
			
			// Event: idle cycle complete
			Pillar::AnimationClip::AnimationEvent idleEvent;
			idleEvent.FrameIndex = 0;
			idleEvent.EventName = "IdleCycle";
			idle.Events.push_back(idleEvent);
			
			m_AnimSystem->RegisterClip(idle);
		}

		// Animation 2: Walk (6 frames from sprite sheet, looping)
		{
			Pillar::AnimationClip walk;
			walk.Name = "Walk";
			walk.Loop = true;
			walk.PlaybackSpeed = 1.0f;
			
			// 6 frames in horizontal strip (96x96 sprite, 576x96 total)
			// Each frame is 1/6 of the texture width = 0.1667
			for (int i = 0; i < 6; i++)
			{
				Pillar::AnimationFrame frame;
				frame.TexturePath = "character_walk_cycle.png";
				frame.Duration = 0.1f;  // 100ms per frame = 10 FPS
				
				// Calculate UV coordinates for this frame
				float frameWidth = 1.0f / 6.0f;  // 0.1667
				frame.UVMin = { i * frameWidth, 0.0f };
				frame.UVMax = { (i + 1) * frameWidth, 1.0f };
				
				walk.Frames.push_back(frame);
			}
			
			// Event: footstep on frame 2 and 5 (typical walk cycle)
			Pillar::AnimationClip::AnimationEvent footstep1;
			footstep1.FrameIndex = 2;
			footstep1.EventName = "Footstep";
			walk.Events.push_back(footstep1);
			
			Pillar::AnimationClip::AnimationEvent footstep2;
			footstep2.FrameIndex = 5;
			footstep2.EventName = "Footstep";
			walk.Events.push_back(footstep2);
			
			m_AnimSystem->RegisterClip(walk);
		}

		// Animation 3: Jump (using frames 1, 3, 5 from walk cycle, non-looping)
		{
			Pillar::AnimationClip jump;
			jump.Name = "Jump";
			jump.Loop = false;  // One-shot animation
			jump.PlaybackSpeed = 1.5f;  // Faster for snappier jump
			
			float frameWidth = 1.0f / 6.0f;  // 0.1667
			
			// Frame 1: windup (use frame 1 of walk cycle)
			Pillar::AnimationFrame frame1;
			frame1.TexturePath = "character_walk_cycle.png";
			frame1.Duration = 0.1f;
			frame1.UVMin = { 1 * frameWidth, 0.0f };
			frame1.UVMax = { 2 * frameWidth, 1.0f };
			jump.Frames.push_back(frame1);
			
			// Frame 2: jump (use frame 3 of walk cycle)
			Pillar::AnimationFrame frame2;
			frame2.TexturePath = "character_walk_cycle.png";
			frame2.Duration = 0.2f;
			frame2.UVMin = { 3 * frameWidth, 0.0f };
			frame2.UVMax = { 4 * frameWidth, 1.0f };
			jump.Frames.push_back(frame2);
			
			// Frame 3: land (use frame 5 of walk cycle)
			Pillar::AnimationFrame frame3;
			frame3.TexturePath = "character_walk_cycle.png";
			frame3.Duration = 0.1f;
			frame3.UVMin = { 5 * frameWidth, 0.0f };
			frame3.UVMax = { 6 * frameWidth, 1.0f };
			jump.Frames.push_back(frame3);
			
			// Event: jump start
			Pillar::AnimationClip::AnimationEvent jumpStart;
			jumpStart.FrameIndex = 1;
			jumpStart.EventName = "JumpStart";
			jump.Events.push_back(jumpStart);
			
			// Event: land
			Pillar::AnimationClip::AnimationEvent land;
			land.FrameIndex = 2;
			land.EventName = "Land";
			jump.Events.push_back(land);
			
			m_AnimSystem->RegisterClip(jump);
		}
	}

	void SpawnAnimatedEntity()
	{
		// Create entity in scene (already has Transform, Tag, UUID)
		auto entity = m_Scene->CreateEntity("AnimatedEntity");
		entt::entity entityID = static_cast<entt::entity>(entity);
		
		// Configure transform at random position
		float x = ((rand() % 200) - 100) / 50.0f;  // -2 to +2
		float y = ((rand() % 200) - 100) / 50.0f;  // -2 to +2
		auto& transform = entity.GetComponent<Pillar::TransformComponent>();
		transform.Position = glm::vec2(x, y);
		transform.Scale = glm::vec2(0.5f, 0.5f);
		
		// Add sprite component (texture and UVs will be set by AnimationSystem)
		auto& sprite = entity.AddComponent<Pillar::SpriteComponent>();
		sprite.Color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		
		// Add animation component
		auto& anim = entity.AddComponent<Pillar::AnimationComponent>();
		
	// Set up animation event callback
	anim.OnAnimationEvent = [this, entityID](const std::string& eventName, entt::entity entity) {
		std::string logMsg = "Entity " + std::to_string(static_cast<uint32_t>(entityID)) + " - Event: " + eventName;
		m_EventLog.push_back(logMsg);
		PIL_INFO("{0}", logMsg);
		
		// Keep log size manageable
		if (m_EventLog.size() > 20)
		{
			m_EventLog.erase(m_EventLog.begin());
		}
	};
	
	// Set up animation completion callback (fires when non-looping animation finishes)
	anim.OnAnimationComplete = [this, entityID, &anim](entt::entity entity) {
		std::string logMsg = "Entity " + std::to_string(static_cast<uint32_t>(entityID)) + " - Animation COMPLETE: " + anim.CurrentClipName;
		m_EventLog.push_back(logMsg);
		PIL_INFO("{0}", logMsg);
		
		// Auto-transition: Jump -> Idle (non-looping -> looping)
		if (anim.CurrentClipName == "Jump")
		{
			anim.Play("Idle");
			PIL_INFO("Entity auto-transitioned from Jump to Idle");
		}
	};
	
	// Play current selected animation
	const char* animNames[] = { "Idle", "Walk", "Jump" };
	anim.Play(animNames[m_CurrentAnimIndex]);		m_AnimatedEntities.push_back(entityID);
		
		PIL_INFO("Spawned animated entity at ({0}, {1}) with animation: {2}", x, y, animNames[m_CurrentAnimIndex]);
	}

	void ClearAllEntities()
	{
		for (auto entityID : m_AnimatedEntities)
		{
			if (m_Scene->GetRegistry().valid(entityID))
			{
				m_Scene->DestroyEntity(Pillar::Entity(entityID, m_Scene.get()));
			}
		}
		m_AnimatedEntities.clear();
		m_SelectedEntityIndex = 0;
		PIL_INFO("Cleared all animated entities");
	}

private:
	Pillar::OrthographicCameraController m_CameraController;
	
	std::shared_ptr<Pillar::Scene> m_Scene;
	std::shared_ptr<Pillar::AnimationSystem> m_AnimSystem;
	
	std::vector<entt::entity> m_AnimatedEntities;
	std::vector<std::string> m_EventLog;
	
	int m_CurrentAnimIndex = 0;  // 0=Idle, 1=Walk, 2=Jump
	int m_SelectedEntityIndex = 0;
	
	bool m_GlobalPaused = false;
	float m_GlobalSpeed = 1.0f;
};
