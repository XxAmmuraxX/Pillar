#pragma once
#include "Pillar/Core.h"
#include "Pillar/Logger.h"
#include "Layer.h"

// Dear ImGui forward decls (we'll add proper includes in the cpp)

namespace Pillar {

class PIL_API ImGuiLayer : public Layer
{
public:
	ImGuiLayer();
	~ImGuiLayer() override;

	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate(float deltaTime) override;
	void OnImGuiRender() override;
};

}
