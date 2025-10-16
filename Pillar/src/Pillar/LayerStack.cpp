#include "LayerStack.h"

namespace Pillar {

LayerStack::LayerStack()
	: m_LayerInsertIndex(0)
{
}

LayerStack::~LayerStack()
{
	for (Layer* layer : m_Layers)
	{
		if (layer)
		{
			layer->OnDetach();
			delete layer;
		}
	}
	m_Layers.clear();
}

void LayerStack::PushLayer(Layer* layer)
{
	PIL_CORE_INFO("PushLayer: {0}", layer ? layer->GetName() : std::string("<null>"));
	m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, layer);
	m_LayerInsertIndex++;
	if (layer) layer->OnAttach();
}

void LayerStack::PushOverlay(Layer* overlay)
{
	PIL_CORE_INFO("PushOverlay: {0}", overlay ? overlay->GetName() : std::string("<null>"));
	m_Layers.emplace_back(overlay);
	if (overlay) overlay->OnAttach();
}

bool LayerStack::PopLayer(Layer* layer)
{
	auto it = std::find(m_Layers.begin(), m_Layers.begin() + m_LayerInsertIndex, layer);
	if (it != m_Layers.begin() + m_LayerInsertIndex)
	{
		m_Layers.erase(it);
		m_LayerInsertIndex--;
		return true;
	}
	return false;
}

bool LayerStack::PopOverlay(Layer* overlay)
{
	auto it = std::find(m_Layers.begin() + m_LayerInsertIndex, m_Layers.end(), overlay);
	if (it != m_Layers.end())
	{
		m_Layers.erase(it);
		return true;
	}
	return false;
}

void LayerStack::DebugList() const
{
	PIL_CORE_INFO("-- LayerStack ({0} total, split={1}) --", m_Layers.size(), m_LayerInsertIndex);
	for (size_t i = 0; i < m_Layers.size(); ++i)
	{
		const Layer* l = m_Layers[i];
		const char* kind = (i < m_LayerInsertIndex) ? "Layer" : "Overlay";
		PIL_CORE_INFO("  [%02llu] %s: %s", static_cast<unsigned long long>(i), kind, l ? l->GetName().c_str() : "<null>");
	}
}

}
