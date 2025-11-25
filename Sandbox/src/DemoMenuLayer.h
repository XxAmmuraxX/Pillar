#pragma once

#include "Pillar.h"
#include <imgui.h>

class DemoMenuLayer : public Pillar::Layer
{
public:
	DemoMenuLayer(std::function<void(int)> switchCallback)
		: Layer("DemoMenuLayer"),
		  m_SwitchCallback(switchCallback)
	{
	}

	void OnImGuiRender() override
	{
		ImGui::Begin("Demo Selector", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

		ImGui::Text("Pillar Engine - ECS/Physics Demos");
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

		ImGui::End();
	}

private:
	std::function<void(int)> m_SwitchCallback;
};
