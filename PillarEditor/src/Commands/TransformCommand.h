#pragma once

#include "Command.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace PillarEditor {

    /**
     * @brief Command for entity transform changes
     * 
     * Stores old and new transform states for position, rotation, and scale.
     * Supports multiple entities being transformed together.
     */
    class TransformCommand : public Command
    {
    public:
        struct TransformState
        {
            entt::entity EntityID;
            glm::vec2 Position;
            float Rotation;
            glm::vec2 Scale;
        };

        TransformCommand(Pillar::Scene* scene, 
                        const std::vector<TransformState>& oldStates,
                        const std::vector<TransformState>& newStates,
                        const std::string& actionName = "Transform")
            : m_Scene(scene)
            , m_OldStates(oldStates)
            , m_NewStates(newStates)
            , m_ActionName(actionName)
        {
        }

        void Execute() override
        {
            ApplyStates(m_NewStates);
        }

        void Undo() override
        {
            ApplyStates(m_OldStates);
        }

        std::string GetName() const override
        {
            if (m_OldStates.size() == 1)
                return m_ActionName;
            return m_ActionName + " (" + std::to_string(m_OldStates.size()) + " entities)";
        }

        bool IsValid() const override
        {
            if (!m_Scene)
                return false;

            // Check if all entities still exist
            auto& registry = m_Scene->GetRegistry();
            for (const auto& state : m_OldStates)
            {
                if (!registry.valid(state.EntityID))
                    return false;
            }
            return true;
        }

    private:
        void ApplyStates(const std::vector<TransformState>& states)
        {
            if (!m_Scene)
                return;

            auto& registry = m_Scene->GetRegistry();
            
            for (const auto& state : states)
            {
                if (!registry.valid(state.EntityID))
                    continue;

                if (registry.all_of<Pillar::TransformComponent>(state.EntityID))
                {
                    auto& transform = registry.get<Pillar::TransformComponent>(state.EntityID);
                    transform.Position = state.Position;
                    transform.Rotation = state.Rotation;
                    transform.Scale = state.Scale;
                    transform.Dirty = true;
                }
            }
        }

        Pillar::Scene* m_Scene;
        std::vector<TransformState> m_OldStates;
        std::vector<TransformState> m_NewStates;
        std::string m_ActionName;
    };

} // namespace PillarEditor
