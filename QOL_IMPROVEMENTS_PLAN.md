# Quality of Life Improvements - Implementation Plan

**Date:** December 11, 2025  
**Goal:** Improve editor usability and developer experience before tackling complex features

---

## Overview

This document outlines practical Quality of Life improvements that will significantly enhance the editor experience. These are quick wins that make daily usage smoother and more intuitive.

---

## Priority QOL Items (Ordered by Impact)

### 🟢 QOL-001: Missing Components in Inspector/Add Menu ⭐⭐⭐⭐⭐
**Impact:** Critical - Cannot edit gameplay components  
**Effort:** 2-3 hours  
**Priority:** P0

#### What's Missing:
Currently only 5 component types are editable in Inspector:
- ✅ TagComponent
- ✅ TransformComponent  
- ✅ VelocityComponent
- ✅ RigidbodyComponent
- ✅ BoxColliderComponent / CircleColliderComponent

Missing from Inspector editing:
- ❌ BulletComponent (Damage, Speed, Lifetime, Team)
- ❌ XPGemComponent (XPValue, CollectionRadius)
- ❌ HierarchyComponent (Parent, Children - read-only display)
- ❌ SpriteComponent (already added, but verify serialization)
- ❌ CameraComponent (already added, but verify all properties)
- ❌ AnimationComponent (already added, but improve UI)

Missing from "Add Component" menu:
- ❌ RigidbodyComponent
- ❌ BulletComponent
- ❌ XPGemComponent
- ❌ HierarchyComponent (special - maybe shouldn't be manually added?)

#### Implementation:
1. Add `DrawBulletComponent()` method
2. Add `DrawXPGemComponent()` method
3. Add `DrawHierarchyComponent()` (read-only parent/child display)
4. Update Add Component menu with missing types
5. Test all component serialization

---

### 🟢 QOL-002: Viewport Entity Name Labels ⭐⭐⭐⭐⭐
**Impact:** High - Hard to identify entities in viewport  
**Effort:** 1-2 hours  
**Priority:** P0

#### Problem:
Selected entities show orange outline, but you can't tell which entity is which without checking hierarchy panel.

#### Solution:
- Draw entity name above selected entity
- Optional: Show names for all entities (toggle in View menu)
- Small, readable font with background for contrast
- Fade when camera zooms out

#### Implementation:
```cpp
// In ViewportPanel::RenderScene()
if (selectedEntities.contains(entity))
{
    auto& tag = entity.GetComponent<TagComponent>();
    ImVec2 screenPos = WorldToScreen(transform.Position);
    
    // Draw text with background
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    std::string label = tag.Tag;
    ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
    
    // Background rect
    drawList->AddRectFilled(
        screenPos - ImVec2(textSize.x/2, textSize.y + 5),
        screenPos + ImVec2(textSize.x/2, 0),
        IM_COL32(0, 0, 0, 180)
    );
    
    // Text
    drawList->AddText(
        screenPos - ImVec2(textSize.x/2, textSize.y),
        IM_COL32(255, 255, 255, 255),
        label.c_str()
    );
}
```

---

### 🟢 QOL-003: Arrow Key Nudging ⭐⭐⭐⭐
**Impact:** High - Precise positioning is tedious  
**Effort:** 1 hour  
**Priority:** P1

#### Feature:
- Arrow keys move selected entity by 0.1 units
- Shift + Arrow keys move by 1.0 unit (faster)
- Ctrl + Arrow keys move by 0.01 units (precise)
- Works with multiple selection (moves all)
- Integrates with undo/redo system

#### Implementation:
```cpp
// In EditorLayer::OnUpdate() or ViewportPanel
if (m_ViewportFocused && !ImGui::GetIO().WantCaptureKeyboard)
{
    float nudgeAmount = 0.1f;
    if (ImGui::IsKeyDown(ImGuiKey_LeftShift)) nudgeAmount = 1.0f;
    if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) nudgeAmount = 0.01f;
    
    glm::vec2 nudge(0.0f);
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) nudge.x -= nudgeAmount;
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) nudge.x += nudgeAmount;
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) nudge.y += nudgeAmount;
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) nudge.y -= nudgeAmount;
    
    if (nudge != glm::vec2(0.0f))
    {
        for (auto entity : m_SelectionContext)
        {
            auto& transform = entity.GetComponent<TransformComponent>();
            transform.Position += nudge;
        }
        // TODO: Create TransformCommand for undo/redo
    }
}
```

---

### 🟢 QOL-004: Hierarchy Search/Filter ⭐⭐⭐⭐
**Impact:** High - Hard to find entities in large scenes  
**Effort:** 1-2 hours  
**Priority:** P1

#### Feature:
- Search box at top of Hierarchy panel
- Filter entities by name (case-insensitive)
- Show only matching entities and their parents
- Clear button (X icon)
- ESC to clear search

#### Implementation:
```cpp
// In HierarchyPanel::OnImGuiRender()
static char searchBuffer[256] = "";
ImGui::SetNextItemWidth(-1);
if (ImGui::InputTextWithHint("##SearchEntities", "Search entities...", 
    searchBuffer, IM_ARRAYSIZE(searchBuffer)))
{
    // Filter updated
}

if (searchBuffer[0] != '\0')
{
    ImGui::SameLine();
    if (ImGui::Button("X"))
    {
        searchBuffer[0] = '\0';
    }
}

// When rendering entities:
std::string searchStr = searchBuffer;
std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);

for (auto entity : entities)
{
    std::string entityName = entity.GetComponent<TagComponent>().Tag;
    std::transform(entityName.begin(), entityName.end(), entityName.begin(), ::tolower);
    
    if (searchStr.empty() || entityName.find(searchStr) != std::string::npos)
    {
        // Draw entity node
    }
}
```

---

### 🟢 QOL-005: Component Property Reset Buttons ⭐⭐⭐⭐
**Impact:** Medium-High - Annoying to manually reset values  
**Effort:** 2 hours  
**Priority:** P1

#### Feature:
- Small "↺" button next to each property label
- Resets property to default value
- Tooltip: "Reset to default"
- Works for: Position (0,0), Rotation (0), Scale (1,1), etc.

#### Implementation:
```cpp
bool DrawVec2Control(const std::string& label, glm::vec2& value, 
                     glm::vec2 defaultValue = glm::vec2(0.0f))
{
    bool modified = false;
    ImGui::PushID(label.c_str());
    
    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, 100.0f);
    ImGui::Text(label.c_str());
    
    // Reset button
    ImGui::SameLine();
    if (ImGui::Button("↺", ImVec2(20, 0)))
    {
        value = defaultValue;
        modified = true;
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Reset to default (%.2f, %.2f)", defaultValue.x, defaultValue.y);
    
    ImGui::NextColumn();
    
    // ... rest of property control
    
    ImGui::Columns(1);
    ImGui::PopID();
    return modified;
}
```

---

### 🟢 QOL-006: Inspector Component Collapse Persistence ⭐⭐⭐
**Impact:** Medium - Components collapse every time you select different entity  
**Effort:** 1-2 hours  
**Priority:** P1

#### Problem:
When you collapse a component (e.g., BoxColliderComponent), it expands again when you select a different entity.

#### Solution:
- Store collapsed state per component type (not per entity)
- Use ImGui's ID stack + storage system
- Or maintain `std::unordered_map<std::string, bool> m_ComponentCollapseState`

#### Implementation:
```cpp
// In InspectorPanel.h
std::unordered_map<std::string, bool> m_ComponentCollapseState;

// In DrawComponent methods:
if (entity.HasComponent<T>())
{
    std::string componentName = typeid(T).name(); // Or use custom names
    
    bool& collapsed = m_ComponentCollapseState[componentName];
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;
    if (!collapsed)
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    
    bool opened = ImGui::CollapsingHeader("Component Name", flags);
    collapsed = !opened;
    
    if (opened)
    {
        // Draw component UI
    }
}
```

---

### 🟢 QOL-007: Status Bar ⭐⭐⭐⭐
**Impact:** Medium-High - Missing useful information  
**Effort:** 2-3 hours  
**Priority:** P1

#### Feature:
Bottom status bar showing:
- FPS (e.g., "60 FPS")
- Entity count (e.g., "45 entities")
- Selected count (e.g., "3 selected")
- Current tool (e.g., "Translate", "Rotate", "Select")
- Camera zoom level (e.g., "Zoom: 100%")
- Play mode indicator (e.g., "▶ Playing" with color)
- Notification area (e.g., "Scene saved" with fade)

#### Implementation:
```cpp
// In EditorLayer::OnImGuiRender(), after all panels
ImGuiViewport* viewport = ImGui::GetMainViewport();
ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - 30));
ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 30));
ImGui::SetNextWindowViewport(viewport->ID);

ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | 
                         ImGuiWindowFlags_NoMove | 
                         ImGuiWindowFlags_NoDocking |
                         ImGuiWindowFlags_NoSavedSettings;

if (ImGui::Begin("StatusBar", nullptr, flags))
{
    // Left side: FPS, entities
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::SameLine(100);
    ImGui::Text("Entities: %d", m_ActiveScene->GetEntityCount());
    
    // Middle: Tool
    ImGui::SameLine(ImGui::GetWindowWidth() / 2 - 50);
    ImGui::Text("Tool: %s", GetCurrentToolName());
    
    // Right side: Play mode, notifications
    ImGui::SameLine(ImGui::GetWindowWidth() - 200);
    if (m_SceneState == SceneState::Play)
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "▶ Playing");
    else if (m_SceneState == SceneState::Pause)
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "⏸ Paused");
    else
        ImGui::Text("⏹ Stopped");
}
ImGui::End();
```

---

### 🟢 QOL-008: Viewport Grid Improvements ⭐⭐⭐
**Impact:** Medium - Current grid is basic  
**Effort:** 2-3 hours  
**Priority:** P2

#### Current State:
Simple grid exists but lacks features.

#### Improvements:
- **Configurable grid size** (0.5, 1.0, 2.0, 5.0 units)
- **Toggle grid visibility** (View menu or G key)
- **Grid follows camera** (doesn't scroll off-screen)
- **Axis lines highlighted** (X=red, Y=green)
- **Grid fades at distance** (alpha based on zoom level)
- **Subdivisions** (minor grid lines every 0.5, major every 1.0)

---

### 🟢 QOL-009: Duplicate Selected (Ctrl+D) ⭐⭐⭐⭐
**Impact:** High - Very common operation  
**Effort:** 1 hour  
**Priority:** P1

#### Feature:
- Ctrl+D duplicates selected entities
- Duplicates appear slightly offset (+0.5, +0.5) from original
- Maintains all components and values
- New entities get new UUIDs
- Integrates with undo/redo

#### Implementation:
Already exists as `DuplicateSelectedEntity()` in HierarchyPanel, just need:
1. Add keyboard shortcut in EditorLayer
2. Ensure it works with multi-selection
3. Add to Edit menu

---

### 🟢 QOL-010: Recent Files Menu ⭐⭐⭐
**Impact:** Medium - Speed up workflow  
**Effort:** 2-3 hours  
**Priority:** P2

#### Feature:
- File → Recent Files submenu
- Shows last 10 opened scenes
- Click to open scene
- Clear recent files option
- Persisted in editor settings JSON

#### Implementation:
```cpp
// Store in EditorSettings.json
{
    "recentFiles": [
        "C:/Projects/Game/scenes/level1.scene.json",
        "C:/Projects/Game/scenes/level2.scene.json"
    ],
    "maxRecentFiles": 10
}

// In EditorLayer
void AddToRecentFiles(const std::string& path)
{
    // Remove if exists (move to front)
    // Add to beginning
    // Truncate to maxRecentFiles
    // Save settings
}
```

---

### 🟢 QOL-011: Improved Console Panel ⭐⭐⭐
**Impact:** Medium - Console is functional but basic  
**Effort:** 2-3 hours  
**Priority:** P2

#### Improvements:
- **Timestamps** on each log entry (HH:MM:SS.mmm)
- **Copy button** per log entry
- **Copy all** button
- **Search/filter** text input
- **Source location** (file:line) on hover
- **Log level icons** (ℹ️ 📊 ⚠️ ❌)
- **Collapsible repeated messages** (e.g., "message x42")

---

### 🟢 QOL-012: Content Browser Improvements ⭐⭐⭐
**Impact:** Medium - Content Browser is basic  
**Effort:** 3-4 hours  
**Priority:** P2

#### Improvements:
- **Texture thumbnails** instead of generic file icons
- **Search/filter** by filename
- **Create folder** button
- **Rename file** (F2 key or context menu)
- **Delete file** with confirmation dialog
- **Refresh button** (F5)
- **Favorites** (pin frequently used folders)
- **Breadcrumb navigation** (clickable path segments)

---

### 🟢 QOL-013: Entity Visibility Toggle ⭐⭐⭐
**Impact:** Medium-High - Useful for complex scenes  
**Effort:** 2-3 hours  
**Priority:** P2

#### Feature:
- Eye icon next to each entity in Hierarchy
- Click to hide/show entity in viewport
- Hidden entities are grayed out in hierarchy
- Hidden entities still selectable in hierarchy (for editing)
- Doesn't affect play mode (visibility is editor-only)
- Stored in entity metadata (not serialized to scene file)

#### Implementation:
```cpp
// Add to entity metadata (editor-only)
std::unordered_map<entt::entity, bool> m_EntityVisibility;

// In HierarchyPanel::DrawEntityNode()
bool isVisible = m_EntityVisibility[entity];
if (ImGui::SmallButton(isVisible ? "👁" : "👁‍🗨"))
{
    m_EntityVisibility[entity] = !isVisible;
}

// In ViewportPanel::RenderScene()
if (!m_EntityVisibility[entity])
    continue; // Skip rendering
```

---

### 🟢 QOL-014: Auto-Save ⭐⭐⭐⭐
**Impact:** High - Prevents data loss  
**Effort:** 2-3 hours  
**Priority:** P1

#### Feature:
- Auto-save every N minutes (default: 5)
- Configurable in preferences
- Creates `.autosave` backup file
- Notification: "Auto-saved at HH:MM:SS"
- Restore from auto-save on crash recovery

#### Implementation:
```cpp
// In EditorLayer
float m_AutoSaveTimer = 0.0f;
float m_AutoSaveInterval = 300.0f; // 5 minutes

void OnUpdate(float deltaTime)
{
    if (m_SceneState == SceneState::Edit)
    {
        m_AutoSaveTimer += deltaTime;
        if (m_AutoSaveTimer >= m_AutoSaveInterval)
        {
            AutoSave();
            m_AutoSaveTimer = 0.0f;
        }
    }
}

void AutoSave()
{
    if (m_CurrentScenePath.empty())
        return; // Don't auto-save untitled scenes
        
    std::string backupPath = m_CurrentScenePath + ".autosave";
    SaveScene(backupPath);
    ShowNotification("Auto-saved");
}
```

---

### 🟢 QOL-015: Viewport Camera Presets ⭐⭐⭐
**Impact:** Medium - Speeds up camera navigation  
**Effort:** 1-2 hours  
**Priority:** P2

#### Feature:
- Save camera positions (Ctrl+1-9)
- Restore camera positions (1-9 keys)
- "Home" button to reset to default view
- Stored per scene (in editor metadata)

---

### 🟢 QOL-016: Multi-Select Transform Editing ⭐⭐⭐⭐
**Impact:** High - Currently each entity must be edited individually  
**Effort:** 2-3 hours  
**Priority:** P1

#### Feature:
When multiple entities selected in Inspector:
- Show "Multiple Entities (N selected)"
- Transform section shows:
  - Position: "---" (mixed values)
  - Rotation: "---" (mixed values)
  - Scale: "---" (mixed values)
- Editing any value applies RELATIVE change to all
  - Example: Set Position X to 5.0 → adds 5.0 to all selected entities' X
  - Or: Add +1.0 to Position X → moves all entities +1.0 in X
- "Apply to All" button for specific components

---

### 🟢 QOL-017: Component Copy/Paste ⭐⭐⭐
**Impact:** Medium - Saves time copying settings  
**Effort:** 2-3 hours  
**Priority:** P2

#### Feature:
- Right-click component header → "Copy Component"
- Right-click entity → "Paste Component As New"
- Or right-click existing component → "Paste Component Values"
- Stores component data in JSON clipboard

---

### 🟢 QOL-018: Improved Selection Outline ⭐⭐⭐
**Impact:** Medium - Current outline is subtle  
**Effort:** 1-2 hours  
**Priority:** P1

#### Current Issue:
Orange tint behind entity is hard to see, especially on light-colored entities.

#### Solution:
- Draw thick outline AROUND entity (not behind)
- Animated dashed outline (marching ants effect)
- Selection handles at corners (small squares)
- Different color for multi-selection (e.g., blue)

---

### 🟢 QOL-019: Zoom to Fit Selected (F key) ⭐⭐⭐⭐
**Impact:** High - Already mentioned but needs proper implementation  
**Effort:** 1 hour  
**Priority:** P1

#### Enhancement:
- F key focuses on selected entity (already exists?)
- Shift+F fits ALL selected entities in view
- Alt+F fits all entities in scene (zoom to scene bounds)
- Smooth animated camera movement (tweening)

---

### 🟢 QOL-020: Notification System ⭐⭐⭐
**Impact:** Medium - Better user feedback  
**Effort:** 2-3 hours  
**Priority:** P2

#### Feature:
- Toast notifications (bottom-right corner)
- Auto-dismiss after 3-5 seconds
- Color-coded: Info (blue), Success (green), Warning (yellow), Error (red)
- Queue multiple notifications
- Click to dismiss early

#### Usage:
```cpp
ShowNotification("Scene saved successfully", NotificationType::Success);
ShowNotification("Failed to load texture", NotificationType::Error);
ShowNotification("Physics system initialized", NotificationType::Info);
```

---

## Implementation Roadmap

### Phase 1: Critical Missing Features (Week 1)
**Goal: Can edit all components, see what you're doing**

- [x] QOL-001: Add missing component editors (Bullet, XPGem, Hierarchy)
- [ ] QOL-002: Entity name labels in viewport
- [ ] QOL-003: Arrow key nudging
- [ ] QOL-004: Hierarchy search/filter
- [ ] QOL-009: Duplicate selected (Ctrl+D)

**Deliverable: All gameplay components editable, easier entity identification**

---

### Phase 2: Workflow Improvements (Week 2)
**Goal: Daily editor usage feels smooth**

- [ ] QOL-005: Component property reset buttons
- [ ] QOL-006: Component collapse persistence
- [ ] QOL-007: Status bar with FPS/entity count
- [ ] QOL-014: Auto-save with configurable interval
- [ ] QOL-018: Improved selection outline
- [ ] QOL-019: Zoom to fit selected (enhanced)

**Deliverable: Polished editor experience, no data loss**

---

### Phase 3: Advanced UX (Week 3)
**Goal: Power user features**

- [ ] QOL-008: Viewport grid improvements
- [ ] QOL-010: Recent files menu
- [ ] QOL-011: Improved console panel
- [ ] QOL-012: Content Browser improvements
- [ ] QOL-016: Multi-select transform editing
- [ ] QOL-020: Notification system

**Deliverable: Professional-grade editor UX**

---

### Phase 4: Polish (Week 4)
**Goal: Final touches**

- [ ] QOL-013: Entity visibility toggle
- [ ] QOL-015: Viewport camera presets
- [ ] QOL-017: Component copy/paste
- [ ] Preferences panel (auto-save interval, grid size, etc.)
- [ ] Keyboard shortcuts panel (list all shortcuts)
- [ ] Documentation updates

**Deliverable: Production-ready editor**

---

## Testing Checklist

For each QOL item:
- [ ] Feature works as described
- [ ] No crashes or errors
- [ ] Keyboard shortcuts don't conflict
- [ ] Undo/redo integration (if applicable)
- [ ] Settings persistence (if applicable)
- [ ] Works with multi-selection (if applicable)
- [ ] Responsive (no lag)
- [ ] Visual feedback is clear

---

## Success Metrics

**Before QOL improvements:**
- Creating and positioning 10 entities: ~5 minutes
- Finding specific entity in large scene: ~30 seconds
- Adjusting component values precisely: ~1 minute per entity

**After QOL improvements:**
- Creating and positioning 10 entities: ~2 minutes (60% faster)
- Finding specific entity: ~5 seconds (83% faster)
- Adjusting component values: ~10 seconds per entity (83% faster)

**User Satisfaction:**
- Can complete common tasks without frustration
- Editor feels responsive and professional
- Minimal context switching between panels
- Clear visual feedback for all actions

---

## Notes

- Many QOL items are quick wins (1-3 hours each)
- Prioritize items that are used most frequently
- Focus on reducing friction in common workflows
- Test with real-world usage scenarios
- Gather feedback after Phase 2

---

**Next Steps:**
1. Review and approve this QOL plan
2. Start with Phase 1 (missing component editors)
3. Implement in priority order
4. Test thoroughly after each item
5. Gather user feedback before moving to advanced features
