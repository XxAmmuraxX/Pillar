#include "SelectionContext.h"
#include <algorithm>

namespace PillarEditor {

    void SelectionContext::Select(Pillar::Entity entity)
    {
        m_Selection.clear();
        if (entity && entity.IsValid())
        {
            m_Selection.push_back(entity);
        }
        NotifySelectionChanged();
    }

    void SelectionContext::Deselect()
    {
        if (!m_Selection.empty())
        {
            m_Selection.clear();
            NotifySelectionChanged();
        }
    }

    void SelectionContext::AddToSelection(Pillar::Entity entity)
    {
        if (entity && entity.IsValid() && !IsSelected(entity))
        {
            m_Selection.push_back(entity);
            NotifySelectionChanged();
        }
    }

    void SelectionContext::RemoveFromSelection(Pillar::Entity entity)
    {
        auto it = std::find(m_Selection.begin(), m_Selection.end(), entity);
        if (it != m_Selection.end())
        {
            m_Selection.erase(it);
            NotifySelectionChanged();
        }
    }

    void SelectionContext::ClearSelection()
    {
        if (!m_Selection.empty())
        {
            m_Selection.clear();
            NotifySelectionChanged();
        }
    }

    Pillar::Entity SelectionContext::GetPrimarySelection() const
    {
        if (m_Selection.empty())
            return Pillar::Entity();
        
        // Validate the entity is still valid
        const auto& entity = m_Selection[0];
        if (entity && entity.IsValid())
            return entity;
        
        return Pillar::Entity();
    }

    bool SelectionContext::IsSelected(Pillar::Entity entity) const
    {
        if (!entity)
            return false;
        return std::find(m_Selection.begin(), m_Selection.end(), entity) != m_Selection.end();
    }

    void SelectionContext::ValidateSelection()
    {
        // Remove any invalid entities from selection
        auto it = std::remove_if(m_Selection.begin(), m_Selection.end(),
            [](const Pillar::Entity& e) { return !e || !e.IsValid(); });
        
        if (it != m_Selection.end())
        {
            m_Selection.erase(it, m_Selection.end());
            NotifySelectionChanged();
        }
    }

    void SelectionContext::NotifySelectionChanged()
    {
        if (m_OnSelectionChanged)
        {
            m_OnSelectionChanged();
        }
    }

}
