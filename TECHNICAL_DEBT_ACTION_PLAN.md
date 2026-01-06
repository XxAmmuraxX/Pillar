# Technical Debt Action Plan - Pillar Editor
**Date:** January 1, 2026  
**Priority:** High - Clean up codebase before adding new features

---

## Overview

This document addresses the technical debt identified in the editor codebase. These improvements will make the code more maintainable, performant, and less bug-prone.

---

## Technical Debt Items

### 1. Magic Numbers (Code Quality) 🔴
**Priority:** P1 - High  
**Effort:** 2-3 hours  
**Impact:** High - Improves maintainability and consistency

#### Current Problem:
Hardcoded values scattered throughout UI code:
- `100.0f` - Column widths repeated ~15+ times
- `0.1f` - Drag speeds inconsistent
- `1.0f` - Default reset values
- Color values repeated (button colors, outline colors)
- Size values (button sizes, padding, etc.)

#### Examples Found:
```cpp
// InspectorPanel.cpp
ImGui::SetColumnWidth(0, 100.0f);  // Repeated 15+ times
ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
```

#### Solution:
Create centralized constants file.

**Implementation:**

1. **Create EditorConstants.h**
```cpp
// PillarEditor/src/EditorConstants.h
#pragma once

#include <glm/glm.hpp>
#include <imgui.h>

namespace PillarEditor::Constants {

    // Inspector Panel
    namespace Inspector {
        constexpr float COLUMN_WIDTH_LABEL = 100.0f;
        constexpr float DRAG_SPEED_DEFAULT = 0.1f;
        constexpr float DRAG_SPEED_FAST = 1.0f;
        constexpr float DRAG_SPEED_PRECISE = 0.01f;
        
        constexpr float RESET_VALUE_ZERO = 0.0f;
        constexpr float RESET_VALUE_ONE = 1.0f;
        
        // Component colors
        namespace Colors {
            constexpr ImVec4 BUTTON_X_NORMAL  = ImVec4(0.6f, 0.2f, 0.2f, 1.0f);  // Red
            constexpr ImVec4 BUTTON_X_HOVERED = ImVec4(0.7f, 0.3f, 0.3f, 1.0f);
            constexpr ImVec4 BUTTON_X_ACTIVE  = ImVec4(0.6f, 0.2f, 0.2f, 1.0f);
            
            constexpr ImVec4 BUTTON_Y_NORMAL  = ImVec4(0.2f, 0.6f, 0.2f, 1.0f);  // Green
            constexpr ImVec4 BUTTON_Y_HOVERED = ImVec4(0.3f, 0.7f, 0.3f, 1.0f);
            constexpr ImVec4 BUTTON_Y_ACTIVE  = ImVec4(0.2f, 0.6f, 0.2f, 1.0f);
        }
    }
    
    // Viewport Panel
    namespace Viewport {
        constexpr float GRID_LINE_THICKNESS = 1.0f;
        constexpr float GRID_MAJOR_SPACING = 1.0f;
        constexpr float GRID_MINOR_SPACING = 0.5f;
        
        constexpr ImVec4 GRID_COLOR = ImVec4(0.3f, 0.3f, 0.3f, 0.5f);
        constexpr ImVec4 GRID_AXIS_X_COLOR = ImVec4(0.8f, 0.2f, 0.2f, 0.8f);  // Red
        constexpr ImVec4 GRID_AXIS_Y_COLOR = ImVec4(0.2f, 0.8f, 0.2f, 0.8f);  // Green
        
        constexpr ImVec4 SELECTION_COLOR = ImVec4(1.0f, 0.7f, 0.0f, 1.0f);    // Orange
        constexpr float SELECTION_LINE_THICKNESS = 2.0f;
        
        constexpr float GIZMO_BUTTON_SIZE = 35.0f;
        constexpr float GIZMO_TOOLBAR_PADDING = 10.0f;
    }
    
    // Scene Hierarchy
    namespace Hierarchy {
        constexpr float TREE_NODE_INDENT = 20.0f;
        constexpr float ENTITY_BUTTON_SIZE = 20.0f;
    }
    
    // Content Browser
    namespace ContentBrowser {
        constexpr float THUMBNAIL_SIZE = 80.0f;
        constexpr float PADDING = 8.0f;
    }
    
    // Auto-save
    namespace AutoSave {
        constexpr float DEFAULT_INTERVAL = 300.0f;  // 5 minutes in seconds
        constexpr float MIN_INTERVAL = 60.0f;       // 1 minute
        constexpr float MAX_INTERVAL = 1800.0f;     // 30 minutes
    }
    
    // Editor Performance
    namespace Performance {
        constexpr size_t MAX_UNDO_HISTORY = 100;
        constexpr float FRAMEBUFFER_RESIZE_THRESHOLD = 1.0f;  // Minimum pixel difference
    }

}  // namespace PillarEditor::Constants
```

2. **Update InspectorPanel.cpp**
Replace all magic numbers with constants:
```cpp
#include "EditorConstants.h"

using namespace PillarEditor::Constants;

// Before:
ImGui::SetColumnWidth(0, 100.0f);

// After:
ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);

// Before:
ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");

// After:
ImGui::DragFloat("##X", &values.x, Inspector::DRAG_SPEED_DEFAULT, 0.0f, 0.0f, "%.2f");

// Before:
ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));

// After:
ImGui::PushStyleColor(ImGuiCol_Button, Inspector::Colors::BUTTON_X_NORMAL);
```

3. **Update ViewportPanel.cpp**
```cpp
#include "EditorConstants.h"

using namespace PillarEditor::Constants;

// Selection outline color
glm::vec4 outlineColor = { Viewport::SELECTION_COLOR.x, Viewport::SELECTION_COLOR.y, 
                           Viewport::SELECTION_COLOR.z, Viewport::SELECTION_COLOR.w };
```

4. **Update CommandHistory.h**
```cpp
#include "EditorConstants.h"

CommandHistory(size_t maxHistorySize = Constants::Performance::MAX_UNDO_HISTORY)
```

#### Files to Update:
- [ ] Create `EditorConstants.h`
- [ ] Update `InspectorPanel.cpp` (~30 replacements)
- [ ] Update `ViewportPanel.cpp` (~10 replacements)
- [ ] Update `CommandHistory.h` (1 replacement)
- [ ] Update `EditorLayer.cpp` (auto-save constants)

---

### 2. ImGui ID Conflicts (Code Quality) 🟠
**Priority:** P1 - High  
**Effort:** 2 hours  
**Impact:** Medium - Prevents UI bugs

#### Current Problem:
Using `##` for unique IDs is good, but some patterns could cause conflicts:
- Generic IDs like `##Tag`, `##X`, `##Y` used in multiple places
- IDs not scoped to component/entity
- Missing `ImGui::PushID()`/`PopID()` in some loops

#### Current Good Practices:
```cpp
// Good: Using PushID/PopID in DrawVec2Control
ImGui::PushID(label);
ImGui::DragFloat("##X", &values.x, ...);
ImGui::PopID();
```

#### Potential Issues:
```cpp
// Potential conflict if multiple components have velocity
ImGui::DragFloat("##MaxSpeed", &velocity.MaxSpeed, ...);
```

#### Solution:

1. **Add PushID/PopID scoping to all component drawing**
```cpp
void InspectorPanel::DrawVelocityComponent(Pillar::Entity entity)
{
    ImGui::PushID("VelocityComponent");  // Scope all IDs to this component
    
    // ... existing code ...
    
    ImGui::PopID();
}
```

2. **Use entity ID for unique scoping**
```cpp
void InspectorPanel::DrawComponents(Pillar::Entity entity)
{
    // Push entity ID to make all widgets unique per entity
    ImGui::PushID((int)entity.GetHandle());
    
    // Draw all components...
    
    ImGui::PopID();
}
```

3. **Audit all ImGui widgets without explicit IDs**

**Implementation Steps:**
- [ ] Add `ImGui::PushID()/PopID()` to all `Draw*Component()` methods
- [ ] Add entity-based `PushID()` in `DrawComponents()`
- [ ] Verify no ID conflicts in multi-select scenarios
- [ ] Test with multiple panels open

#### Files to Update:
- [ ] `InspectorPanel.cpp` - Add PushID/PopID to all component methods
- [ ] `SceneHierarchyPanel.cpp` - Verify entity node IDs
- [ ] `AnimationManagerPanel.cpp` - Check animation list IDs

---

### 3. Resource Management - Framebuffer Resize (Performance) 🟡
**Priority:** P2 - Medium  
**Effort:** 1 hour  
**Impact:** Medium - Improves performance

#### Current Problem:
Framebuffer resizes every frame if viewport size changes even slightly.
```cpp
// ViewportPanel.cpp line 385
if (viewportPanelSize.x > 0 && viewportPanelSize.y > 0 &&
    (m_Framebuffer->GetSpecification().Width != viewportPanelSize.x ||
     m_Framebuffer->GetSpecification().Height != viewportPanelSize.y))
{
    m_Framebuffer->Resize((uint32_t)viewportPanelSize.x, (uint32_t)viewportPanelSize.y);
}
```

Issue: Floating-point comparisons can trigger resize on sub-pixel changes.

#### Solution:

1. **Add resize threshold**
```cpp
// In ViewportPanel.h
float m_LastFramebufferWidth = 0.0f;
float m_LastFramebufferHeight = 0.0f;

// In ViewportPanel.cpp
bool NeedsResize(float newWidth, float newHeight)
{
    constexpr float THRESHOLD = Constants::Performance::FRAMEBUFFER_RESIZE_THRESHOLD;
    
    float widthDiff = std::abs(newWidth - m_LastFramebufferWidth);
    float heightDiff = std::abs(newHeight - m_LastFramebufferHeight);
    
    return widthDiff >= THRESHOLD || heightDiff >= THRESHOLD;
}

// In OnImGuiRender()
if (viewportPanelSize.x > 0 && viewportPanelSize.y > 0 &&
    NeedsResize(viewportPanelSize.x, viewportPanelSize.y))
{
    m_Framebuffer->Resize((uint32_t)viewportPanelSize.x, (uint32_t)viewportPanelSize.y);
    m_LastFramebufferWidth = viewportPanelSize.x;
    m_LastFramebufferHeight = viewportPanelSize.y;
}
```

2. **Alternative: Integer rounding comparison**
```cpp
uint32_t newWidth = (uint32_t)viewportPanelSize.x;
uint32_t newHeight = (uint32_t)viewportPanelSize.y;

if (newWidth > 0 && newHeight > 0 &&
    (m_Framebuffer->GetSpecification().Width != newWidth ||
     m_Framebuffer->GetSpecification().Height != newHeight))
{
    m_Framebuffer->Resize(newWidth, newHeight);
}
```

#### Files to Update:
- [ ] `ViewportPanel.h` - Add last size tracking
- [ ] `ViewportPanel.cpp` - Implement resize threshold

---

### 4. Event System - Event Consumption (Bug Fix) 🔴
**Priority:** P1 - High  
**Effort:** 2 hours  
**Impact:** High - Prevents event handling bugs

#### Current Problem:
Some events may not be properly marked as handled, causing:
- Viewport camera moving while typing in text fields
- Keyboard shortcuts triggering during text input
- Mouse clicks falling through panels

#### Current State:
Good: ViewportPanel checks `m_ViewportFocused` and `m_ViewportHovered`  
Issue: Not checking `ImGui::GetIO().WantCaptureKeyboard` and `WantCaptureMouse` everywhere

#### Solution:

1. **Add ImGui input capture checks**
```cpp
// In ViewportPanel::OnEvent()
bool ViewportPanel::OnMouseButtonPressed(Pillar::MouseButtonPressedEvent& e)
{
    // Don't handle if ImGui wants the mouse
    if (ImGui::GetIO().WantCaptureMouse)
        return false;
    
    // Only handle if viewport is hovered
    if (!m_ViewportHovered)
        return false;
    
    // ... rest of handling ...
    
    return true;  // Event handled
}
```

2. **Add keyboard event filtering**
```cpp
// In EditorLayer::OnKeyPressed()
bool EditorLayer::OnKeyPressed(Pillar::KeyPressedEvent& e)
{
    // Don't handle shortcuts if typing in text field
    if (ImGui::GetIO().WantCaptureKeyboard)
        return false;
    
    // ... rest of handling ...
}
```

3. **Properly consume events**
```cpp
// Mark event as handled when consumed
if (handleShortcut)
{
    e.Handled = true;
    return true;
}
```

#### Files to Update:
- [ ] `EditorLayer.cpp` - Add `WantCaptureKeyboard` check in `OnKeyPressed()`
- [ ] `ViewportPanel.cpp` - Add `WantCaptureMouse` check in mouse events
- [ ] `SceneHierarchyPanel.cpp` - Verify event handling
- [ ] Test: Type in text field while viewport focused (camera shouldn't move)
- [ ] Test: Press shortcut while typing (shouldn't trigger)

---

### 5. Selection Validation (Bug Fix) 🔴
**Priority:** P0 - Critical  
**Effort:** 1 hour  
**Impact:** High - Prevents crashes from invalid entity references

#### Current Problem:
Dead entities can remain in selection after deletion, causing:
- Crashes when accessing deleted entities
- Invalid entity references in inspector
- Undo/redo operating on non-existent entities

#### Current State:
Good: `SelectionContext::ValidateSelection()` exists  
Issue: **Never called!** (grep search found 0 calls to ValidateSelection)

#### Solution:

1. **Call ValidateSelection() periodically**
```cpp
// In EditorLayer::OnUpdate()
void EditorLayer::OnUpdate(float deltaTime)
{
    // Validate selection every frame (cheap operation)
    m_SelectionContext.ValidateSelection();
    
    // ... rest of update ...
}
```

2. **Call after entity deletion**
```cpp
// In SceneHierarchyPanel::DeleteEntity()
void SceneHierarchyPanel::DeleteEntity(Pillar::Entity entity)
{
    m_Scene->DestroyEntity(entity);
    m_SelectionContext->ValidateSelection();  // Clean up selection
}
```

3. **Call after scene load**
```cpp
// In EditorLayer::OpenScene()
void EditorLayer::OpenScene(const std::string& filepath)
{
    // ... load scene ...
    
    m_SelectionContext.ClearSelection();  // Clear old scene's selection
}
```

4. **Improve ValidateSelection() implementation**
```cpp
// In SelectionContext.cpp
void SelectionContext::ValidateSelection()
{
    // Remove invalid entities
    auto it = std::remove_if(m_Selection.begin(), m_Selection.end(),
        [](Pillar::Entity entity) {
            return !entity || !entity.IsValid();
        });
    
    if (it != m_Selection.end())
    {
        m_Selection.erase(it, m_Selection.end());
        NotifySelectionChanged();  // Notify if selection changed
    }
}
```

#### Files to Update:
- [ ] `SelectionContext.cpp` - Improve `ValidateSelection()` with notification
- [ ] `EditorLayer.cpp` - Call `ValidateSelection()` in `OnUpdate()`
- [ ] `SceneHierarchyPanel.cpp` - Call after entity deletion
- [ ] `EditorLayer.cpp` - Clear selection on scene load/new

---

### 6. Panel System - Generic Registration (Architecture) 🟡
**Priority:** P3 - Low  
**Effort:** 3-4 hours  
**Impact:** Medium - Better extensibility

#### Current Problem:
Panels are manually instantiated and managed in EditorLayer:
```cpp
m_HierarchyPanel = std::make_shared<SceneHierarchyPanel>();
m_InspectorPanel = std::make_shared<InspectorPanel>();
// ... etc
```

Adding new panel requires modifying EditorLayer.

#### Solution:

1. **Create PanelManager**
```cpp
// PanelManager.h
class PanelManager
{
public:
    template<typename T>
    std::shared_ptr<T> AddPanel(const std::string& name);
    
    template<typename T>
    std::shared_ptr<T> GetPanel();
    
    void OnImGuiRender();
    void OnUpdate(float deltaTime);
    
private:
    std::vector<std::shared_ptr<EditorPanel>> m_Panels;
    std::unordered_map<std::string, std::shared_ptr<EditorPanel>> m_PanelMap;
};
```

2. **Register panels declaratively**
```cpp
// In EditorLayer::OnAttach()
m_PanelManager.AddPanel<ViewportPanel>("Viewport");
m_PanelManager.AddPanel<SceneHierarchyPanel>("Hierarchy");
m_PanelManager.AddPanel<InspectorPanel>("Inspector");
// ... etc
```

**Note:** This is nice-to-have but lower priority. Current manual approach works fine for now.

---

### 7. Asset System - UUID Database (Architecture) 🟢
**Priority:** P4 - Future  
**Effort:** 8+ hours  
**Impact:** High - But can be deferred

#### Current Problem:
Assets referenced by filepath strings:
- Moving/renaming assets breaks references
- No way to track asset dependencies
- Difficult to implement asset hot-reload

#### Solution (Future):
- Create AssetDatabase with UUID → Asset mapping
- Store UUIDs in components instead of paths
- Track asset dependencies

**Defer this until after QOL features are complete.**

---

### 8. Settings Persistence (Architecture) 🟡
**Priority:** P2 - Medium  
**Effort:** 2-3 hours  
**Impact:** Medium - Quality of life

#### Current Problem:
No editor settings persistence:
- Editor layout not saved
- Auto-save interval not configurable
- Recent files not persisted
- Camera preferences not saved

#### Solution:

1. **Create EditorSettings.json**
```json
{
    "autoSave": {
        "enabled": true,
        "interval": 300
    },
    "viewport": {
        "showGrid": true,
        "gridSize": 1.0,
        "cameraSpeed": 5.0
    },
    "recentFiles": [
        "C:/Projects/Game/Scenes/Level1.scene.json",
        "C:/Projects/Game/Scenes/Level2.scene.json"
    ],
    "layout": "default"
}
```

2. **Create EditorSettings class**
```cpp
class EditorSettings
{
public:
    static EditorSettings& Get();
    
    void Load(const std::string& filepath = "EditorSettings.json");
    void Save(const std::string& filepath = "EditorSettings.json");
    
    // Auto-save settings
    bool AutoSaveEnabled = true;
    float AutoSaveInterval = 300.0f;
    
    // Viewport settings
    bool ShowGrid = true;
    float GridSize = 1.0f;
    float CameraSpeed = 5.0f;
    
    // Recent files
    std::vector<std::string> RecentFiles;
    void AddRecentFile(const std::string& filepath);
    
private:
    EditorSettings() = default;
};
```

3. **Load on startup, save on exit**
```cpp
// In EditorLayer::OnAttach()
EditorSettings::Get().Load();

// In EditorLayer::OnDetach()
EditorSettings::Get().Save();
```

#### Files to Create:
- [ ] `EditorSettings.h/cpp` - Settings management
- [ ] `EditorSettings.json` - Default settings file

---

## Implementation Priority Order

### Phase 1: Critical Fixes (Day 1 - 4 hours)
**Must fix before adding features**

1. ✅ **Selection Validation** (1 hour) - Prevents crashes
2. ✅ **Event Consumption** (2 hours) - Fixes input bugs
3. ✅ **ImGui ID Conflicts** (1 hour) - Prevents UI bugs

### Phase 2: Code Quality (Day 2 - 5 hours)
**Makes codebase maintainable**

4. ✅ **Magic Numbers** (3 hours) - Creates EditorConstants.h
5. ✅ **Settings Persistence** (2 hours) - Saves preferences

### Phase 3: Performance (Day 3 - 1 hour)
**Optional optimizations**

6. ✅ **Framebuffer Resize** (1 hour) - Reduces unnecessary work

### Phase 4: Architecture (Future)
**Nice-to-have improvements**

7. ⏸️ **Panel System** (3-4 hours) - Better extensibility (defer)
8. ⏸️ **Asset System** (8+ hours) - UUID database (defer)

---

## Testing Checklist

After each fix:
- [ ] Editor starts without errors
- [ ] Scene can be saved/loaded
- [ ] Entities can be selected
- [ ] Components can be edited
- [ ] Undo/redo still works
- [ ] Keyboard shortcuts work correctly
- [ ] No input falling through panels
- [ ] No crashes when deleting entities
- [ ] Selection updates correctly

---

## Success Metrics

**Before Technical Debt Cleanup:**
- Magic numbers: ~50+ hardcoded values
- Potential ID conflicts: 5-10 locations
- Event bugs: Input sometimes falls through
- Selection bugs: Crashes possible on entity deletion
- Settings: Lost on editor restart

**After Technical Debt Cleanup:**
- Magic numbers: 0 (all in EditorConstants.h)
- ID conflicts: 0 (proper scoping everywhere)
- Event bugs: 0 (proper capture checks)
- Selection bugs: 0 (validation every frame)
- Settings: Persisted across sessions

**Code Maintainability:** ⭐⭐⭐⭐⭐ (excellent)  
**Bug Risk:** ⭐⭐⭐⭐⭐ (very low)  
**Performance:** ⭐⭐⭐⭐⭐ (optimized)

---

## Next Steps

1. ✅ Review this technical debt plan
2. ✅ Start with Phase 1 (Critical Fixes)
3. ✅ Test thoroughly after each fix
4. ✅ Move to Phase 2 (Code Quality)
5. ✅ Then proceed to QOL features (Sprint 1)

**After technical debt cleanup, the codebase will be solid and ready for new features!** 🎯
