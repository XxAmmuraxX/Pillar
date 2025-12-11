#pragma once

#include "Command.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/SceneSerializer.h"
#include <nlohmann/json.hpp>
#include <string>

namespace PillarEditor {

    /**
     * @brief Command for creating an entity
     * 
     * Stores the serialized entity data to allow deletion/recreation.
     */
    class CreateEntityCommand : public Command
    {
    public:
        CreateEntityCommand(Pillar::Scene* scene, const std::string& name = "New Entity")
            : m_Scene(scene)
            , m_EntityName(name)
            , m_EntityID(entt::null)
        {
        }

        void Execute() override
        {
            if (!m_Scene)
                return;

            // Create the entity
            Pillar::Entity entity = m_Scene->CreateEntity(m_EntityName);
            m_EntityID = entity.GetHandle();

            // If we have serialized data (from redo), restore it
            if (!m_SerializedData.empty())
            {
                // TODO: Deserialize component data when needed
            }
        }

        void Undo() override
        {
            if (!m_Scene || m_EntityID == entt::null)
                return;

            auto& registry = m_Scene->GetRegistry();
            if (registry.valid(m_EntityID))
            {
                // Serialize entity before destroying (for redo)
                // TODO: Serialize component data when needed
                
                // Destroy the entity
                m_Scene->DestroyEntity(Pillar::Entity(m_EntityID, m_Scene));
            }
        }

        std::string GetName() const override
        {
            return "Create Entity";
        }

        bool IsValid() const override
        {
            return m_Scene != nullptr;
        }

        entt::entity GetEntityID() const { return m_EntityID; }

    private:
        Pillar::Scene* m_Scene;
        std::string m_EntityName;
        entt::entity m_EntityID;
        std::string m_SerializedData;  // JSON serialized entity data
    };

    /**
     * @brief Command for deleting an entity
     * 
     * Stores all entity data to allow recreation.
     */
    class DeleteEntityCommand : public Command
    {
    public:
        DeleteEntityCommand(Pillar::Scene* scene, Pillar::Entity entity)
            : m_Scene(scene)
            , m_EntityID(entity.GetHandle())
        {
            // Store entity data before deletion
            if (entity.HasComponent<Pillar::TagComponent>())
            {
                m_EntityName = entity.GetComponent<Pillar::TagComponent>().Tag;
            }

            // Store transform
            if (entity.HasComponent<Pillar::TransformComponent>())
            {
                auto& tc = entity.GetComponent<Pillar::TransformComponent>();
                m_HasTransform = true;
                m_Position = tc.Position;
                m_Rotation = tc.Rotation;
                m_Scale = tc.Scale;
            }

            // TODO: Store other components when needed
        }

        void Execute() override
        {
            if (!m_Scene)
                return;

            auto& registry = m_Scene->GetRegistry();
            if (registry.valid(m_EntityID))
            {
                m_Scene->DestroyEntity(Pillar::Entity(m_EntityID, m_Scene));
            }
        }

        void Undo() override
        {
            if (!m_Scene)
                return;

            // Recreate the entity
            Pillar::Entity entity = m_Scene->CreateEntity(m_EntityName);
            m_EntityID = entity.GetHandle();

            // Restore transform
            if (m_HasTransform && entity.HasComponent<Pillar::TransformComponent>())
            {
                auto& tc = entity.GetComponent<Pillar::TransformComponent>();
                tc.Position = m_Position;
                tc.Rotation = m_Rotation;
                tc.Scale = m_Scale;
                tc.Dirty = true;
            }

            // TODO: Restore other components when needed
        }

        std::string GetName() const override
        {
            return "Delete Entity";
        }

        bool IsValid() const override
        {
            return m_Scene != nullptr;
        }

    private:
        Pillar::Scene* m_Scene;
        entt::entity m_EntityID;
        std::string m_EntityName;
        
        // Transform data
        bool m_HasTransform = false;
        glm::vec2 m_Position;
        float m_Rotation;
        glm::vec2 m_Scale;

        // TODO: Add storage for other component types
    };

} // namespace PillarEditor
