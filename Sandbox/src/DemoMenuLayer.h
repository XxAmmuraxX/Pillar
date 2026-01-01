#pragma once

#include "Pillar.h"
#include "Pillar/Renderer/Renderer2DBackend.h"
#include <imgui.h>

class DemoMenuLayer : public Pillar::Layer
{
public:
	DemoMenuLayer(std::function<void(int)> switchCallback = nullptr)
		: Layer("DemoMenuLayer"),
		  m_SwitchCallback(switchCallback)
	{
	}

	void OnImGuiRender() override
	{
		ImGui::Begin("Demo Selector", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

		ImGui::Text("Pillar Engine - ECS/Physics Demos");
		ImGui::Separator();

		// Renderer Info
		ImGui::Text("Renderer Backend:");
		ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.3f, 1.0f), "* Batch Renderer (GPU-Optimized)");
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "  Up to 10,000 quads per draw call");

		// Only show demo selection if callback is provided
		if (m_SwitchCallback)
		{
			ImGui::Separator();

			if (ImGui::Button("Physics Demo (Gameplay)", ImVec2(250, 40)))
			{
				m_SwitchCallback(0);
			}
			ImGui::TextWrapped("Interactive demo with player, enemies, bullets, and XP gems. Use arrow keys to move.");

			ImGui::Separator();

			if (ImGui::Button("Light Entity Performance", ImVec2(250, 40)))
			{
				m_SwitchCallback(1);
			}
			ImGui::TextWrapped("Stress test for pure ECS light entities. Spawn thousands of particles with velocity integration.");

			ImGui::Separator();

			if (ImGui::Button("Heavy Entity Performance", ImVec2(250, 40)))
			{
				m_SwitchCallback(2);
			}
			ImGui::TextWrapped("Stress test for Box2D physics. Spawn hundreds of physics bodies with collision detection.");
		}
		else
		{
			ImGui::Separator();
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "To use demos, uncomment them in Source.cpp");
		}

		ImGui::End();
	}

private:
	std::function<void(int)> m_SwitchCallback;
};
