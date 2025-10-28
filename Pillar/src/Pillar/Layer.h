#pragma once

#include "Pillar/Core.h"
#include "Pillar/Events/Event.h"
#include "Pillar/Events/KeyEvent.h"
#include "Pillar/Events/MouseEvent.h"
#include "Pillar/Logger.h"

#include <string>

namespace Pillar {

    class PIL_API Layer
    {
    public:
        Layer(const std::string& name = "Layer");
        virtual ~Layer();

        // Lifecycle
        virtual void OnAttach();
        virtual void OnDetach();
        virtual void OnUpdate(float deltaTime); // deltaTime in seconds
        virtual void OnImGuiRender() {}
        virtual void OnEvent(Event& event) {}

        inline const std::string& GetName() const { return m_DebugName; }
    protected:
        std::string m_DebugName;
    };

}


