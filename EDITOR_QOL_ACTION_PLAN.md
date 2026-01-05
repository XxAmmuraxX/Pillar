# Pillar Editor - QOL Implementation Action Plan
**Date:** January 1, 2026  
**Goal:** Implement high-impact QOL features to polish the editor

---

## Sprint 1: Immediate QOL Wins
**Duration:** 1-2 days  
**Goal:** Fix the most annoying daily-use issues

---

### Task 1: Entity Name Labels in Viewport ⭐⭐⭐⭐⭐
**Priority:** P0 - Critical  
**Effort:** 2 hours  
**Impact:** MASSIVE - Users can finally see what they're selecting!

#### Current Problem:
- Selected entities show orange outline but no label
- Can't distinguish between multiple similar entities
- Must check hierarchy panel to confirm selection

#### Implementation Steps:

1. **Add helper method to ViewportPanel**
```cpp
// In ViewportPanel.cpp
void ViewportPanel::DrawEntityNameLabel(const glm::vec2& worldPos, const std::string& name)
{
    // Convert world position to screen coordinates
    ImVec2 screenPos = WorldToScreenImGui(worldPos);
    
    // Get ImGui draw list
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    // Calculate text size
    ImVec2 textSize = ImGui::CalcTextSize(name.c_str());
    
    // Position text above entity (offset by 20 pixels)
    ImVec2 textPos = ImVec2(screenPos.x - textSize.x * 0.5f, screenPos.y - textSize.y - 20);
    
    // Draw background rectangle
    ImVec2 bgMin = ImVec2(textPos.x - 4, textPos.y - 2);
    ImVec2 bgMax = ImVec2(textPos.x + textSize.x + 4, textPos.y + textSize.y + 2);
    drawList->AddRectFilled(bgMin, bgMax, IM_COL32(0, 0, 0, 180), 3.0f);
    
    // Draw text
    drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), name.c_str());
}
```

2. **Add WorldToScreenImGui helper**
```cpp
ImVec2 ViewportPanel::WorldToScreenImGui(const glm::vec2& worldPos)
{
    // Get camera matrices
    glm::mat4 viewProj = m_EditorCamera.GetViewProjectionMatrix();
    
    // Convert world to NDC
    glm::vec4 clipSpace = viewProj * glm::vec4(worldPos.x, worldPos.y, 0.0f, 1.0f);
    glm::vec3 ndc = glm::vec3(clipSpace) / clipSpace.w;
    
    // Convert NDC to screen space
    float screenX = (ndc.x + 1.0f) * 0.5f * m_ViewportSize.x;
    float screenY = (1.0f - ndc.y) * 0.5f * m_ViewportSize.y;
    
    // Add viewport bounds offset
    return ImVec2(m_ViewportBounds[0].x + screenX, m_ViewportBounds[0].y + screenY);
}
```

3. **Call in RenderScene() after drawing entities**
```cpp
// In ViewportPanel::RenderScene()
// After drawing all entities and selection highlights
if (m_SelectionContext)
{
    auto& selected = m_SelectionContext->GetSelection();
    for (auto e : selected)
    {
        Pillar::Entity entity{ e, m_Scene.get() };
        if (entity && entity.HasComponent<Pillar::TagComponent>() && 
            entity.HasComponent<Pillar::TransformComponent>())
        {
            auto& tag = entity.GetComponent<Pillar::TagComponent>();
            auto& transform = entity.GetComponent<Pillar::TransformComponent>();
            
            DrawEntityNameLabel(transform.Position, tag.Tag);
        }
    }
}
```

4. **Add toggle option**
```cpp
// In ViewportPanel.h
bool m_ShowEntityLabels = true;

// In DrawGizmoToolbar() or new View menu
if (ImGui::MenuItem("Show Entity Labels", nullptr, &m_ShowEntityLabels))
{
    // Toggle visibility
}
```

#### Testing:
- [ ] Labels appear above selected entities
- [ ] Labels are readable (white text, dark background)
- [ ] Labels follow entity when camera moves
- [ ] Labels don't overlap (if multiple selected, acceptable)
- [ ] Toggle on/off works

---

### Task 2: Hierarchy Search/Filter ⭐⭐⭐⭐⭐
**Priority:** P0 - Critical  
**Effort:** 2 hours  
**Impact:** MASSIVE - Find entities instantly

#### Current Problem:
- With 50+ entities, finding specific entity takes 20-30 seconds
- Must manually scroll and read names
- No keyboard shortcut to jump to entity

#### Implementation Steps:

1. **Add search buffer to SceneHierarchyPanel**
```cpp
// In SceneHierarchyPanel.h
char m_SearchBuffer[256] = "";
bool m_IsSearching = false;
```

2. **Add search UI in OnImGuiRender()**
```cpp
// At the top of hierarchy panel, before entity tree
ImGui::PushItemWidth(-1);
if (ImGui::InputTextWithHint("##EntitySearch", ICON_FA_SEARCH " Search entities...", 
    m_SearchBuffer, IM_ARRAYSIZE(m_SearchBuffer)))
{
    // Filter updated
}
ImGui::PopItemWidth();

// Clear button if searching
if (m_SearchBuffer[0] != '\0')
{
    ImGui::SameLine();
    if (ImGui::Button("X") || ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
        m_SearchBuffer[0] = '\0';
    }
    m_IsSearching = true;
}
else
{
    m_IsSearching = false;
}

ImGui::Separator();
```

3. **Add filter logic in DrawEntityNode()**
```cpp
void SceneHierarchyPanel::DrawEntityNode(Pillar::Entity entity)
{
    // If searching, filter entities
    if (m_IsSearching)
    {
        auto& tag = entity.GetComponent<Pillar::TagComponent>();
        std::string tagLower = tag.Tag;
        std::string searchLower = m_SearchBuffer;
        
        // Convert to lowercase for case-insensitive search
        std::transform(tagLower.begin(), tagLower.end(), tagLower.begin(), ::tolower);
        std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);
        
        // Skip if not matching
        if (tagLower.find(searchLower) == std::string::npos)
            return;
    }
    
    // ... rest of existing DrawEntityNode code
}
```

4. **Add keyboard shortcuts**
```cpp
// Ctrl+F to focus search
if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_F) && 
    ImGui::GetIO().KeyCtrl)
{
    ImGui::SetKeyboardFocusHere(-1);  // Focus previous item (search box)
}
```

#### Testing:
- [ ] Search box appears at top of hierarchy
- [ ] Typing filters entities instantly
- [ ] Case-insensitive search works
- [ ] ESC clears search
- [ ] X button clears search
- [ ] Ctrl+F focuses search box
- [ ] Clear search shows all entities again

---

### Task 3: Arrow Key Nudging ⭐⭐⭐⭐
**Priority:** P0 - Critical  
**Effort:** 1 hour  
**Impact:** HIGH - 80% faster precise positioning

#### Current Problem:
- Must use gizmo or type values to move entities
- No quick way to nudge entity by small amounts
- Keyboard shortcuts would be much faster

#### Implementation Steps:

1. **Add nudging logic to ViewportPanel::OnUpdate()**
```cpp
void ViewportPanel::OnUpdate(float deltaTime)
{
    // Only handle nudging when viewport is focused and not using gizmo
    if (m_ViewportFocused && !m_GizmoInUse && 
        !ImGui::GetIO().WantCaptureKeyboard)
    {
        // Determine nudge amount based on modifiers
        float nudgeAmount = 0.1f;  // Default: 0.1 units
        if (ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift))
            nudgeAmount = 1.0f;  // Shift: 1.0 unit (faster)
        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl))
            nudgeAmount = 0.01f;  // Ctrl: 0.01 unit (precise)
        
        // Check arrow key presses
        glm::vec2 nudge(0.0f);
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
            nudge.x -= nudgeAmount;
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
            nudge.x += nudgeAmount;
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
            nudge.y += nudgeAmount;
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
            nudge.y -= nudgeAmount;
        
        // Apply nudge to all selected entities
        if (nudge != glm::vec2(0.0f) && m_SelectionContext && 
            !m_SelectionContext->IsEmpty())
        {
            ApplyNudge(nudge);
        }
    }
}
```

2. **Add ApplyNudge helper method**
```cpp
void ViewportPanel::ApplyNudge(const glm::vec2& nudge)
{
    auto& selected = m_SelectionContext->GetSelection();
    
    // Store old transforms for undo/redo
    std::vector<TransformCommand::TransformState> oldStates;
    std::vector<TransformCommand::TransformState> newStates;
    
    for (auto e : selected)
    {
        Pillar::Entity entity{ e, m_Scene.get() };
        if (entity && entity.HasComponent<Pillar::TransformComponent>())
        {
            auto& transform = entity.GetComponent<Pillar::TransformComponent>();
            
            // Store old state
            TransformCommand::TransformState oldState;
            oldState.Position = transform.Position;
            oldState.Rotation = transform.Rotation;
            oldState.Scale = transform.Scale;
            oldStates.push_back(oldState);
            
            // Apply nudge
            transform.Position += nudge;
            
            // Store new state
            TransformCommand::TransformState newState;
            newState.Position = transform.Position;
            newState.Rotation = transform.Rotation;
            newState.Scale = transform.Scale;
            newStates.push_back(newState);
        }
    }
    
    // Create undo command (if command history is accessible)
    if (m_EditorLayer && !oldStates.empty())
    {
        auto command = std::make_unique<TransformCommand>(
            selected, oldStates, newStates, "Nudge Entity"
        );
        m_EditorLayer->GetCommandHistory().ExecuteCommand(std::move(command));
    }
}
```

3. **Add to ViewportPanel.h**
```cpp
private:
    void ApplyNudge(const glm::vec2& nudge);
```

#### Testing:
- [ ] Arrow keys move selected entity by 0.1 units
- [ ] Shift+Arrow moves by 1.0 unit
- [ ] Ctrl+Arrow moves by 0.01 unit
- [ ] Works with multiple selected entities
- [ ] Integrates with undo/redo (Ctrl+Z undoes nudge)
- [ ] Only works when viewport is focused
- [ ] Doesn't interfere with text input fields

---

### Task 4: Auto-Save ⭐⭐⭐⭐
**Priority:** P0 - Critical  
**Effort:** 3 hours  
**Impact:** HIGH - Prevents all data loss

#### Current Problem:
- No auto-save functionality
- If editor crashes or user forgets to save, work is lost
- No backup system

#### Implementation Steps:

1. **Add auto-save state to EditorLayer**
```cpp
// In EditorLayer.h
float m_AutoSaveTimer = 0.0f;
float m_AutoSaveInterval = 300.0f;  // 5 minutes (configurable)
bool m_AutoSaveEnabled = true;
std::string m_AutoSavePath;
```

2. **Add auto-save logic to OnUpdate()**
```cpp
// In EditorLayer::OnUpdate()
if (m_SceneState == SceneState::Edit && m_AutoSaveEnabled && 
    !m_CurrentScenePath.empty())
{
    m_AutoSaveTimer += deltaTime;
    
    if (m_AutoSaveTimer >= m_AutoSaveInterval)
    {
        m_AutoSaveTimer = 0.0f;
        AutoSave();
    }
}
```

3. **Implement AutoSave()**
```cpp
void EditorLayer::AutoSave()
{
    if (m_CurrentScenePath.empty())
    {
        ConsolePanel::Log("Auto-save skipped: No scene file open", LogLevel::Trace);
        return;
    }
    
    // Create auto-save path: "scene.scene.json" -> "scene.scene.autosave.json"
    std::filesystem::path scenePath(m_CurrentScenePath);
    std::string autoSavePath = scenePath.parent_path().string() + "/" + 
                               scenePath.stem().string() + ".autosave" + 
                               scenePath.extension().string();
    
    // Save scene to auto-save file
    Pillar::SceneSerializer serializer(m_ActiveScene);
    if (serializer.Serialize(autoSavePath))
    {
        // Get current time
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm = *std::localtime(&now_time_t);
        
        char timeStr[32];
        std::strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &tm);
        
        std::string message = "Auto-saved at " + std::string(timeStr);
        ConsolePanel::Log(message, LogLevel::Info);
        
        // TODO: Show notification toast (Task 15 - Notification System)
    }
    else
    {
        ConsolePanel::Log("Auto-save failed!", LogLevel::Error);
    }
}
```

4. **Add preferences for auto-save**
```cpp
// In future Preferences panel or menu
if (ImGui::Checkbox("Auto-save enabled", &m_AutoSaveEnabled))
{
    // Save preference
}

ImGui::Text("Auto-save interval (seconds):");
if (ImGui::SliderFloat("##AutoSaveInterval", &m_AutoSaveInterval, 60.0f, 1800.0f, "%.0f"))
{
    // Interval changed
}
```

5. **Add crash recovery (optional)**
```cpp
// On editor startup, check for .autosave files
void EditorLayer::OnAttach()
{
    // ... existing code ...
    
    // Check for auto-save files
    CheckForAutoSaveRecovery();
}

void EditorLayer::CheckForAutoSaveRecovery()
{
    // Scan for .autosave.json files in common directories
    // If found, show dialog: "Recover unsaved changes from [timestamp]?"
    // If Yes: Load auto-save file and mark as modified
}
```

#### Testing:
- [ ] Auto-save triggers after 5 minutes of editing
- [ ] Auto-save creates `.autosave.json` file
- [ ] Auto-save doesn't interrupt editing
- [ ] Console shows "Auto-saved at HH:MM:SS"
- [ ] Auto-save only happens in Edit mode (not Play mode)
- [ ] Auto-save skipped if no scene is open
- [ ] Timer resets after manual save
- [ ] Interval is configurable

---

### Task 5: Status Bar ⭐⭐⭐⭐
**Priority:** P0 - Critical  
**Effort:** 3 hours  
**Impact:** HIGH - Professional appearance + useful info

#### Current Problem:
- No FPS display
- Don't know how many entities in scene
- Don't know current tool mode
- Don't know play mode status

#### Implementation Steps:

1. **Create StatusBar panel or integrate into EditorLayer**
```cpp
// Option 1: Add to EditorLayer::OnImGuiRender() at the end
void EditorLayer::OnImGuiRender()
{
    // ... existing panels ...
    
    DrawStatusBar();
}
```

2. **Implement DrawStatusBar()**
```cpp
void EditorLayer::DrawStatusBar()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    
    // Position at bottom of editor
    ImVec2 statusBarPos = ImVec2(viewport->Pos.x, 
                                  viewport->Pos.y + viewport->Size.y - 30);
    ImVec2 statusBarSize = ImVec2(viewport->Size.x, 30);
    
    ImGui::SetNextWindowPos(statusBarPos);
    ImGui::SetNextWindowSize(statusBarSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | 
                             ImGuiWindowFlags_NoMove | 
                             ImGuiWindowFlags_NoDocking |
                             ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoFocusOnAppearing |
                             ImGuiWindowFlags_NoNav;
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 6));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(16, 0));
    
    if (ImGui::Begin("##StatusBar", nullptr, flags))
    {
        // Left side: FPS
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        
        ImGui::SameLine();
        ImGui::Separator();
        
        // Entity count
        ImGui::SameLine();
        auto& registry = m_ActiveScene->GetRegistry();
        size_t entityCount = registry.size();
        ImGui::Text("Entities: %zu", entityCount);
        
        // Selected count
        if (m_HierarchyPanel && m_HierarchyPanel->GetSelectionContext())
        {
            auto& selection = m_HierarchyPanel->GetSelectionContext()->GetSelection();
            if (!selection.empty())
            {
                ImGui::SameLine();
                ImGui::Text("Selected: %zu", selection.size());
            }
        }
        
        ImGui::SameLine();
        ImGui::Separator();
        
        // Current tool
        ImGui::SameLine();
        std::string toolName = "Select";
        if (m_ViewportPanel)
        {
            switch (m_ViewportPanel->GetGizmoMode())
            {
                case GizmoMode::Translate: toolName = "Translate"; break;
                case GizmoMode::Rotate: toolName = "Rotate"; break;
                case GizmoMode::Scale: toolName = "Scale"; break;
                default: toolName = "Select"; break;
            }
        }
        ImGui::Text("Tool: %s", toolName.c_str());
        
        ImGui::SameLine();
        ImGui::Separator();
        
        // Camera zoom
        ImGui::SameLine();
        if (m_ViewportPanel)
        {
            float zoom = m_ViewportPanel->GetCamera().GetZoomLevel();
            ImGui::Text("Zoom: %.0f%%", zoom * 100.0f);
        }
        
        // Right side: Play mode indicator
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 120);
        
        if (m_SceneState == SceneState::Play)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
            ImGui::Text("▶ Playing");
            ImGui::PopStyleColor();
        }
        else if (m_SceneState == SceneState::Pause)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.0f, 1.0f));
            ImGui::Text("⏸ Paused");
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            ImGui::Text("⏹ Stopped");
            ImGui::PopStyleColor();
        }
    }
    ImGui::End();
    
    ImGui::PopStyleVar(2);
}
```

3. **Add to EditorLayer.h**
```cpp
private:
    void DrawStatusBar();
```

#### Testing:
- [ ] Status bar appears at bottom of editor
- [ ] FPS updates in real-time
- [ ] Entity count is accurate
- [ ] Selected count updates when selection changes
- [ ] Current tool shows correct mode
- [ ] Zoom level updates when zooming
- [ ] Play mode indicator shows correct state with color
- [ ] Status bar doesn't overlap panels
- [ ] Status bar is not dockable

---

## Sprint 1 Summary

**Total Effort:** ~11 hours (1-2 days)  
**Total Impact:** 🚀 **MASSIVE**

After completing Sprint 1, the editor will feel **10x more polished**:
- ✅ Users can see entity names in viewport
- ✅ Users can find entities instantly with search
- ✅ Users can position entities precisely with arrow keys
- ✅ Users never lose work (auto-save)
- ✅ Users have full awareness of editor state (status bar)

**Next:** Move to Sprint 2 (Inspector & Transform Polish) after Sprint 1 is tested and working.

---

## Notes

- Each task has detailed implementation steps
- All tasks integrate with existing undo/redo system
- All tasks respect existing architecture patterns
- Testing checklist for each task
- Estimated times are conservative (may be faster)

**Ready to start implementation!** 🎯
