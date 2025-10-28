#include "Layer.h"
#include "Pillar/Logger.h"

namespace Pillar {

Layer::Layer(const std::string& name)
	: m_DebugName(name)
{
}

Layer::~Layer()
{
}

void Layer::OnAttach()
{
	PIL_CORE_INFO("Layer OnAttach: {0}", m_DebugName);
}

void Layer::OnDetach()
{
	PIL_CORE_INFO("Layer OnDetach: {0}", m_DebugName);
}

void Layer::OnUpdate(float deltaTime)
{
	PIL_CORE_TRACE("Layer OnUpdate: {0} dt={1}", m_DebugName, deltaTime);
}

}
