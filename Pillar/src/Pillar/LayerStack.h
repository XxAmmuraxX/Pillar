#pragma once

#include <vector>
#include <algorithm>
#include <string>

#include "Pillar/Core.h"
#include "Pillar/Logger.h"
#include "Layer.h"

namespace Pillar {

	class PIL_API LayerStack
	{
	public:
		LayerStack();
		~LayerStack();

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);
		bool PopLayer(Layer* layer);
		bool PopOverlay(Layer* overlay);

		std::vector<Layer*>::iterator begin() { return m_Layers.begin(); }
		std::vector<Layer*>::iterator end() { return m_Layers.end(); }
		std::vector<Layer*>::reverse_iterator rbegin() { return m_Layers.rbegin(); }
		std::vector<Layer*>::reverse_iterator rend() { return m_Layers.rend(); }

		void DebugList() const;
		inline size_t Size() const { return m_Layers.size(); }

	private:
		std::vector<Layer*> m_Layers;
		unsigned int m_LayerInsertIndex = 0; // overlays are always after this index
	};
}
