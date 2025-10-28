#pragma once
#include "Pillar/Logger.h"
#include "Layer.h"

namespace Pillar {

class ExampleLayer : public Layer
{
public:
	ExampleLayer() : Layer("ExampleLayer") {}
	void OnAttach() override { Layer::OnAttach(); }
	void OnDetach() override { Layer::OnDetach(); }
	void OnUpdate(float dt) override { //PIL_INFO("ExampleLayer update dt={0}", dt);
	}
	void OnImGuiRender() override {}
};

}
