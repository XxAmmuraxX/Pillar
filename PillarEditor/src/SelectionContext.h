#pragma once

#include "Pillar/ECS/Entity.h"
#include <vector>
#include <functional>

namespace PillarEditor {

    class SelectionContext
    {
    public:
        using SelectionChangedCallback = std::function<void()>;

        void Select(Pillar::Entity entity);
        void Deselect();
        void AddToSelection(Pillar::Entity entity);
        void RemoveFromSelection(Pillar::Entity entity);
        void ClearSelection();

        Pillar::Entity GetPrimarySelection() const;
        const std::vector<Pillar::Entity>& GetSelection() const { return m_Selection; }
        bool IsSelected(Pillar::Entity entity) const;
        bool HasSelection() const { return !m_Selection.empty(); }
        size_t GetSelectionCount() const { return m_Selection.size(); }

        // Call this to remove any invalid entities from selection
        void ValidateSelection();

        void SetOnSelectionChanged(SelectionChangedCallback callback) { m_OnSelectionChanged = callback; }

    private:
        void NotifySelectionChanged();

    private:
        std::vector<Pillar::Entity> m_Selection;
        SelectionChangedCallback m_OnSelectionChanged;
    };

}
