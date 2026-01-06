# Animation System - Comprehensive Analysis & Editor Integration Plan

**Date:** January 6, 2026  
**Status:** � Phase 1 Complete - Ready for Integration & Testing  
**Component:** Animation System + Sprite System + Editor Integration  
**Priority:** HIGH (Critical for 2D games)  

---

## Executive Summary

The Pillar Engine has a **functionally complete animation system** at the engine level, with `AnimationComponent`, `AnimationSystem`, `AnimationClip`, and JSON-based animation data. However, the **editor integration is minimal**, requiring manual JSON editing and lacking visual workflows. This document analyzes the current state and proposes a comprehensive plan to create a professional, artist-friendly animation workflow.

**Current State:** ⭐⭐⭐☆☆ (3/5) - Working engine system, poor editor UX  
**Target State:** ⭐⭐⭐⭐⭐ (5/5) - Unity/Godot-level visual workflow

**Key Strengths:**
- ✅ Solid engine architecture (ECS-based)
- ✅ Frame-based animation with UV coordinates
- ✅ JSON serialization working
- ✅ Playback controls (play/pause/stop)
- ✅ Animation events system
- ✅ Sprite sheet integration
- ✅ TexturePacker/Aseprite import

**Critical Gaps:**
- ❌ No visual animation editor
- ❌ No timeline/scrubber UI
- ❌ No frame preview in editor
- ❌ No animation clip library panel
- ❌ No clip management (create/delete/rename)
- ❌ Manual frame library → animation conversion
- ❌ No event editing in UI

---

## Table of Contents

1. [System Architecture](#system-architecture)
2. [Current Implementation](#current-implementation)
3. [Workflow Analysis](#workflow-analysis)
4. [Integration Opportunities](#integration-opportunities)
5. [Missing Components](#missing-components)
6. [Implementation Plan](#implementation-plan)
7. [Visual Mockups](#visual-mockups)

---

## System Architecture

### **Component Diagram**

```
┌─────────────────────────────────────────────────────────────────┐
│                    ANIMATION SYSTEM ARCHITECTURE                 │
└─────────────────────────────────────────────────────────────────┘

ENGINE LAYER (Pillar/)
┌────────────────────┐     ┌────────────────────┐
│ AnimationComponent │────▶│  AnimationSystem   │
│  (ECS Component)   │     │  (System Update)   │
└────────────────────┘     └────────────────────┘
         │                          │
         │                          ▼
         │                 ┌────────────────────┐
         │                 │  AnimationClip     │
         │                 │  (Data Container)  │
         │                 └────────────────────┘
         │                          │
         ▼                          ▼
┌────────────────────┐     ┌────────────────────┐
│ SpriteComponent    │     │ AnimationFrame     │
│ (Rendering Target) │     │ (Frame Data)       │
└────────────────────┘     └────────────────────┘
         │                          │
         ▼                          ▼
┌────────────────────┐     ┌────────────────────┐
│  Renderer2D        │     │  Texture2D + UV    │
└────────────────────┘     └────────────────────┘

EDITOR LAYER (PillarEditor/)
┌────────────────────┐     ┌────────────────────┐
│ InspectorPanel     │     │SpriteSheetEditor   │
│ (AnimComponent UI) │     │ (Frame Selection)  │
└────────────────────┘     └────────────────────┘
         │                          │
         ▼                          ▼
┌────────────────────────────────────────────────┐
│        MISSING: AnimationEditorPanel           │
│   (Timeline, Preview, Clip Management)         │
└────────────────────────────────────────────────┘
```

### **Data Flow**

#### **1. Animation Authoring (Current - Manual):**
```
TexturePacker/Aseprite
         │
         ▼ (Export JSON)
SpriteSheetEditor (Frame Library)
         │
         ▼ (Manual Export)
.anim.json file (Created manually)
         │
         ▼ (Manual Load in Code)
AnimationSystem (LoadAnimationClip)
         │
         ▼ (Runtime Assignment)
AnimationComponent.CurrentClipName = "walk"
```

**Issues:**
- ❌ 5-step manual process
- ❌ No visual feedback until play mode
- ❌ JSON editing required for frame durations/events

#### **2. Animation Authoring (Proposed - Visual):**
```
TexturePacker/Aseprite
         │
         ▼ (Import Button)
SpriteSheetEditor (Auto-import frames)
         │
         ▼ (Drag-Drop or Button)
AnimationEditorPanel (Timeline UI)
         │
         ▼ (Visual Editing)
- Add/remove frames
- Adjust durations (timeline scrubber)
- Preview animation
- Add events (markers)
         │
         ▼ (Auto-Save)
.anim.json file (Created automatically)
         │
         ▼ (Auto-Load)
AnimationSystem (Auto-detects clips)
         │
         ▼ (Dropdown Selection)
InspectorPanel (Clip Selector)
```

**Benefits:**
- ✅ Streamlined workflow
- ✅ Visual preview before play mode
- ✅ No manual JSON editing

---

## Current Implementation

### **Engine Components**

#### **AnimationComponent** (`Pillar/src/Pillar/ECS/Components/Rendering/AnimationComponent.h`)

**Purpose:** Stores animation state for an entity

**Fields:**
```cpp
struct AnimationComponent
{
    std::string CurrentClipName = "";    // Active animation clip
    int FrameIndex = 0;                  // Current frame (0-based)
    float PlaybackTime = 0.0f;          // Time in current frame (seconds)
    float PlaybackSpeed = 1.0f;         // Speed multiplier
    bool Playing = true;                 // Playback state
    
    EventCallback OnAnimationEvent;      // Callback for events
    CompletionCallback OnAnimationComplete; // Non-looping completion
};
```

**Methods:**
- `Play(clipName, restart)` - Start/switch animation
- `Pause()` - Pause playback
- `Resume()` - Resume from pause
- `Stop()` - Reset to frame 0
- `HasAnimation()` - Check if clip assigned
- `IsPlaying()` - Check playback state

**Strengths:**
- ✅ Simple, data-oriented design
- ✅ Callbacks for extensibility
- ✅ Clean API for game code

**Gaps:**
- No reference to loaded clips (stored in system)
- No frame metadata (duration, events) - stored in clip

---

#### **AnimationSystem** (`Pillar/src/Pillar/ECS/Systems/AnimationSystem.h`)

**Purpose:** Updates all animation components, manages clip library

**Key Methods:**
```cpp
class AnimationSystem : public System
{
public:
    void OnUpdate(float deltaTime) override;
    
    // Clip management
    bool LoadAnimationClip(const std::string& filePath);
    void RegisterClip(const AnimationClip& clip);
    AnimationClip* GetClip(const std::string& name);
    bool HasClip(const std::string& name) const;
    void ClearLibrary();
    
private:
    std::unordered_map<std::string, AnimationClip> m_AnimationLibrary;
    
    void UpdateAnimation(entt::entity entity, float dt);
    void AdvanceFrame(AnimationComponent& anim, AnimationClip& clip, entt::entity entity);
    void UpdateSpriteFromFrame(entt::entity entity, const AnimationFrame& frame);
    void FireAnimationEvents(...);
};
```

**Update Flow:**
1. Query all entities with `AnimationComponent` + `SpriteComponent`
2. For each entity:
   - Get animation clip from library by name
   - Update sprite UV coordinates from current frame
   - If playing: advance playback time
   - If frame duration exceeded: advance to next frame
   - Fire animation events if crossing event frame
   - Handle looping or stop at end

**Strengths:**
- ✅ Centralized clip storage (shared between entities)
- ✅ Efficient ECS query (only animated sprites)
- ✅ Updates sprite component directly (no coupling)

**Gaps:**
- Clips loaded manually via `LoadAnimationClip(path)`
- No auto-discovery of animation files
- No clip hot-reloading

---

#### **AnimationClip** (`Pillar/src/Pillar/ECS/Components/Rendering/AnimationClip.h`)

**Purpose:** Container for animation sequence data

**Structure:**
```cpp
struct AnimationClip
{
    struct AnimationEvent {
        int FrameIndex = 0;              // Frame to fire on
        std::string EventName = "";      // Event identifier
    };
    
    std::string Name = "";               // Clip identifier
    std::vector<AnimationFrame> Frames;  // Frame sequence
    bool Loop = true;                    // Looping behavior
    float PlaybackSpeed = 1.0f;         // Speed multiplier
    std::vector<AnimationEvent> Events;  // Event markers
};
```

**Methods:**
- `GetDuration()` - Total clip duration (sum of frame durations)
- `GetFrameCount()` - Number of frames
- `IsValid()` - Has at least one frame

**Strengths:**
- ✅ Self-contained data structure
- ✅ Animation events integrated
- ✅ Simple to serialize/deserialize

---

#### **AnimationFrame** (`Pillar/src/Pillar/ECS/Components/Rendering/AnimationFrame.h`)

**Purpose:** Single frame in animation sequence

**Structure:**
```cpp
struct AnimationFrame
{
    std::string TexturePath = "";        // Texture filename
    float Duration = 0.1f;               // Frame duration (seconds)
    glm::vec2 UVMin = { 0.0f, 0.0f };   // UV coordinates
    glm::vec2 UVMax = { 1.0f, 1.0f };
};
```

**Notes:**
- Each frame can have different texture (for multi-file animations)
- UV coordinates support sprite sheets
- Duration allows variable-length frames

---

#### **AnimationLoader** (`Pillar/src/Pillar/Utils/AnimationLoader.h`)

**Purpose:** Load/save animation clips from JSON

**JSON Format:**
```json
{
  "name": "player_walk",
  "loop": true,
  "playbackSpeed": 1.0,
  "frames": [
    {
      "texturePath": "player_walk_cycle.png",
      "duration": 0.1,
      "uvMin": [0.0, 0.0],
      "uvMax": [0.1667, 1.0]
    }
  ],
  "events": [
    {
      "frameIndex": 1,
      "eventName": "footstep"
    }
  ]
}
```

**Strengths:**
- ✅ Human-readable JSON
- ✅ Supports all clip features
- ✅ Error handling with logging

**Gaps:**
- No schema validation
- No version number for format upgrades

---

### **Editor Components**

#### **InspectorPanel - AnimationComponent UI** (`PillarEditor/src/Panels/InspectorPanel.cpp:1401`)

**Current Features:**

**1. Clip Selection:**
- Text input for `CurrentClipName` (manual entry)
- ⚠️ No dropdown of available clips
- ⚠️ No validation if clip exists

**2. Playback Status:**
- Playing checkbox with visual indicator (green = playing)
- Read-only frame index display
- Read-only playback time display

**3. Playback Controls:**
- Play/Pause/Stop/Reset buttons
- Playback speed slider (0.0 - 5.0x)
- Speed presets (0.5x, 1.0x, 1.5x, 2.0x)

**4. Info Box:**
- Tips about animation system
- Links to Animation Manager panel (doesn't exist yet!)

**What's Missing:**
- ❌ No clip dropdown (must type name manually)
- ❌ No frame preview (can't see current frame)
- ❌ No timeline scrubber (can't seek to frame)
- ❌ No frame-by-frame stepping
- ❌ No event visualization
- ❌ No clip duration display
- ❌ No "Create New Clip" button

---

#### **SpriteSheetEditorPanel** (`PillarEditor/src/Panels/SpriteSheetEditorPanel.cpp`)

**Relevant Features:**

**1. Frame Library:**
- Select frames from sprite sheet grid
- Add to frame library (ordered list)
- Preview frames (64x64 thumbnails)
- Clear library button

**2. Export to Animation:**
- `ExportToAnimationClip()` method (lines 935-990)
- Creates `.anim.json` file with frame library
- **Issue:** Very basic export - no duration editing, no events

**Export Format:**
```json
{
  "name": "SpriteSheetAnimation",
  "texture": "sprite_sheet.png",
  "frameCount": 8,
  "frameDuration": 0.1,  // Fixed duration for all frames
  "frames": [
    {
      "index": 0,
      "name": "Frame 0",
      "uvMin": [0.0, 0.0],
      "uvMax": [0.125, 1.0],
      "gridPos": [0, 0]
    }
  ]
}
```

**Issues with Current Export:**
- ❌ Fixed frame duration (0.1s for all frames)
- ❌ No way to edit individual frame timings
- ❌ No animation events
- ❌ No looping setting
- ❌ No playback speed setting
- ❌ Format doesn't match `AnimationLoader` expected format!

**Format Mismatch:**
```json
// SpriteSheetEditor exports:
{
  "frameDuration": 0.1,  // Single value
  "frames": [
    { "uvMin": [...], "uvMax": [...] }
  ]
}

// AnimationLoader expects:
{
  "playbackSpeed": 1.0,
  "frames": [
    {
      "texturePath": "...",  // Required!
      "duration": 0.1,       // Per-frame
      "uvMin": [...],
      "uvMax": [...]
    }
  ]
}
```

**Critical Bug:** Exported animations won't load! 🔴

---

#### **TexturePacker/Aseprite Import** (`SpriteSheetEditorPanel.cpp:995+`)

**TexturePacker Import:**
- Looks for `.json` next to texture
- Parses TexturePacker JSON format
- Imports frames into frame library
- Preserves frame names
- Handles rotated/trimmed sprites

**Aseprite Import:**
- Looks for `.json` next to texture
- Parses Aseprite JSON format
- Imports frames + animation tags
- **Auto-creates .anim.json files per tag** ✅
- Reads frame durations from Aseprite
- Supports forward/reverse/ping-pong directions

**Aseprite Auto-Animation:**
```cpp
void CreateAnimationClipsFromTags(
    const std::vector<AsepriteAnimationTag>& tags,
    const std::vector<AsepriteFrameData>& frames)
{
    for (const auto& tag : tags)
    {
        // Create one .anim.json per animation tag
        AnimationClip clip;
        clip.Name = tag.name;
        clip.Loop = (tag.direction == "forward" || tag.direction == "pingpong");
        
        // Add frames from tag range
        for (int i = tag.from; i <= tag.to; ++i)
        {
            AnimationFrame frame;
            frame.TexturePath = m_TexturePath;
            frame.Duration = frames[i].duration / 1000.0f; // ms to seconds
            frame.UVMin = frames[i].UVMin;
            frame.UVMax = frames[i].UVMax;
            clip.Frames.push_back(frame);
        }
        
        // Save to file
        AnimationLoader::SaveToJSON(clip, tag.name + ".anim.json");
    }
}
```

**Strengths:**
- ✅ Fully automated Aseprite → Animation pipeline
- ✅ Preserves timing from Aseprite
- ✅ One-click workflow

**This is the gold standard!** We need this level of automation for manual workflows.

---

## Workflow Analysis

### **Current Animation Workflow (Manual)**

**Step 1: Create Sprite Sheet**
- Artist creates sprite sheet in external tool (Aseprite, Photoshop, etc.)
- Export as PNG to `assets/textures/`

**Step 2: Define Frames (Sprite Sheet Editor)**
1. Open Sprite Sheet Editor panel
2. Load texture
3. Configure grid (columns, rows, cell size)
4. Click cells to add to frame library
5. Click "Export to Animation Clip"
6. ❌ **Bug:** Exported JSON incompatible with AnimationLoader!

**Step 3: Fix JSON Manually**
1. Open exported `.anim.json` in text editor
2. Fix format to match AnimationLoader expectations:
   - Add `"texturePath"` to each frame
   - Rename `"frameDuration"` to per-frame `"duration"`
   - Add `"playbackSpeed"` at clip level
3. Save file

**Step 4: Load Animation (Code)**
```cpp
// In Sandbox or game code:
m_AnimSystem->LoadAnimationClip("player_walk.anim.json");
```

**Step 5: Assign to Entity (Inspector)**
1. Select entity in scene
2. Add AnimationComponent
3. Type clip name manually (e.g., "player_walk")
4. Enter play mode to test

**Issues:**
- ⏱️ 5-10 minutes per animation
- 🐛 Manual JSON editing required (error-prone)
- 🔁 No iteration loop (can't preview in editor)
- 📝 No clip management UI

---

### **Improved Workflow (Proposed)**

**Step 1: Import Assets**
- Drag sprite sheet to Content Browser
- (Aseprite users: Automatically create animations from tags)

**Step 2: Create Animation (Visual Editor)**
1. Open **Animation Editor Panel**
2. Click "New Animation Clip"
3. Name clip (e.g., "player_walk")
4. Drag frames from Sprite Sheet Editor or Content Browser to timeline
5. Adjust frame durations by dragging timeline handles
6. Add event markers (footsteps, hit frames)
7. Preview animation in real-time

**Step 3: Assign to Entity (Inspector)**
1. Add AnimationComponent to entity
2. Select clip from **dropdown** (shows all clips)
3. Preview animation in viewport (edit mode)
4. Test in play mode

**Timeline:**
- ⏱️ 1-2 minutes per animation
- ✅ No manual JSON editing
- ✅ Visual preview before play mode
- ✅ Clip library managed automatically

---

## Integration Opportunities

### **1. Sprite Sheet Editor ↔ Animation Editor**

**Current Gap:** Frames selected in Sprite Sheet Editor export to JSON, but no visual way to edit timing/events.

**Proposed Integration:**
```
SpriteSheetEditorPanel
         │
         ├─ Frame Library (existing)
         │      │
         │      ▼ (Button: "Create Animation from Frames")
         │      │
         ▼      ▼
AnimationEditorPanel (NEW!)
         │
         ├─ Import frame library as initial clip
         ├─ Visual timeline for duration editing
         ├─ Event marker tools
         └─ Save as .anim.json (correct format)
```

**Implementation:**
```cpp
// SpriteSheetEditorPanel - Add button
if (ImGui::Button("Create Animation from Frames"))
{
    if (m_FrameLibrary.empty())
    {
        ConsolePanel::Log("No frames to animate", LogLevel::Warn);
        return;
    }
    
    // Open Animation Editor with these frames
    m_AnimationEditor->CreateFromFrames(
        m_FrameLibrary,
        m_Texture,
        m_TexturePath
    );
    m_AnimationEditor->SetVisible(true);
}
```

---

### **2. Inspector ↔ Animation Editor**

**Current Gap:** Inspector shows animation state, but no way to edit clip.

**Proposed Integration:**
```
InspectorPanel
    │
    ├─ AnimationComponent
    │      │
    │      ├─ Clip Dropdown (shows all loaded clips)
    │      │      │
    │      │      └─ (On select: auto-assign to component)
    │      │
    │      ├─ Preview Controls
    │      │      │
    │      │      ├─ Play/Pause (edit mode preview)
    │      │      └─ Timeline Scrubber
    │      │
    │      └─ "Edit Clip" Button
    │             │
    │             ▼
    │      AnimationEditorPanel (opens with this clip)
```

**Implementation:**
```cpp
// InspectorPanel - Animation Component UI

// Clip dropdown
if (ImGui::BeginCombo("##ClipName", anim.CurrentClipName.c_str()))
{
    auto& animSystem = GetAnimationSystem();
    for (const auto& [clipName, clip] : animSystem.GetAllClips())
    {
        bool selected = (anim.CurrentClipName == clipName);
        if (ImGui::Selectable(clipName.c_str(), selected))
        {
            anim.Play(clipName, true);
        }
    }
    ImGui::EndCombo();
}

// Edit clip button
ImGui::SameLine();
if (ImGui::SmallButton("Edit"))
{
    auto* clip = animSystem.GetClip(anim.CurrentClipName);
    if (clip)
    {
        m_AnimationEditor->OpenClip(clip);
    }
}
```

---

### **3. Content Browser ↔ Animation System**

**Current Gap:** .anim.json files exist in assets folder but no special handling.

**Proposed Integration:**
```
ContentBrowserPanel
    │
    ├─ Detect .anim.json files
    │      │
    │      ├─ Show animation icon (🎬)
    │      ├─ Hover preview (play animation)
    │      │
    │      └─ Double-click → Open in Animation Editor
    │
    └─ Drag-drop to entity
           │
           └─ Auto-add AnimationComponent + assign clip
```

**Implementation:**
```cpp
// ContentBrowserPanel - File rendering
if (fileType == ".anim.json")
{
    // Animation file icon
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
    if (ImGui::ImageButton("🎬", thumbnailSize))
    {
        // Double-click to edit
        m_AnimationEditor->OpenClip(filepath);
    }
    ImGui::PopStyleColor();
    
    // Drag-drop source
    if (ImGui::BeginDragDropSource())
    {
        ImGui::SetDragDropPayload("ANIMATION_CLIP", filepath.c_str(), filepath.size() + 1);
        ImGui::Text("🎬 %s", filename.c_str());
        ImGui::EndDragDropSource();
    }
}

// InspectorPanel - Drag-drop target on entity
if (ImGui::BeginDragDropTarget())
{
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ANIMATION_CLIP"))
    {
        std::string clipPath = (const char*)payload->Data;
        
        // Auto-add AnimationComponent if missing
        if (!entity.HasComponent<AnimationComponent>())
        {
            entity.AddComponent<AnimationComponent>();
        }
        
        // Load clip and assign
        GetAnimationSystem().LoadAnimationClip(clipPath);
        
        auto& anim = entity.GetComponent<AnimationComponent>();
        anim.Play(GetClipNameFromPath(clipPath));
    }
    ImGui::EndDragDropTarget();
}
```

---

### **4. Viewport ↔ Animation Preview**

**Current Gap:** No animation preview in edit mode (only play mode).

**Proposed Integration:**
```
ViewportPanel
    │
    ├─ When entity selected
    │      │
    │      └─ If has AnimationComponent
    │             │
    │             ├─ Show preview controls (toolbar overlay)
    │             │      │
    │             │      ├─ Play/Pause button
    │             │      └─ Frame slider
    │             │
    │             └─ Update sprite in real-time (edit mode)
```

**Implementation:**
```cpp
// ViewportPanel - Overlay UI
if (selectedEntity && selectedEntity.HasComponent<AnimationComponent>())
{
    auto& anim = selectedEntity.GetComponent<AnimationComponent>();
    
    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::Begin("Animation Preview", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize);
    
    if (ImGui::Button(anim.Playing ? "⏸" : "▶"))
    {
        anim.Playing = !anim.Playing;
    }
    
    ImGui::SameLine();
    ImGui::SliderInt("Frame", &anim.FrameIndex, 0, clip->GetFrameCount() - 1);
    
    ImGui::End();
    
    // CRITICAL: Update sprite manually in edit mode
    if (!m_EditorState->IsPlayMode())
    {
        GetAnimationSystem().UpdateAnimation(selectedEntity, deltaTime);
    }
}
```

---

## Missing Components

### **1. AnimationEditorPanel** 🔴 CRITICAL

**Purpose:** Visual timeline editor for creating/editing animation clips

**Required Features:**

**A. Timeline View:**
- Horizontal timeline showing frames
- Frame thumbnails (64x64)
- Draggable frame handles for duration adjustment
- Add/remove frames (drag from sprite sheet or content browser)
- Reorder frames (drag-and-drop)

**B. Clip Management:**
- New/Open/Save/Save As buttons
- Clip properties editor (name, loop, playback speed)
- Clip library list (shows all .anim.json in project)

**C. Event Markers:**
- Add event markers to specific frames
- Event name editor
- Visual indicators on timeline

**D. Preview Controls:**
- Play/Pause/Stop buttons
- Frame-by-frame stepping (< >)
- Loop toggle
- Speed control
- Scrubber for seeking

**E. Frame Properties:**
- Duration input (seconds or milliseconds)
- Texture path display
- UV coordinates display

**F. Preview Viewport:**
- Embedded viewport showing animated sprite
- Grid background
- Zoom controls
- Current frame indicator

---

### **2. Animation Clip Library Manager** 🟡 HIGH

**Purpose:** Central manager for all animation clips in project

**Required Features:**

**A. Clip Discovery:**
- Auto-scan `assets/animations/` folder
- Detect all `.anim.json` files
- Load clips on editor startup
- Watch for file changes (hot-reload)

**B. Clip Browser:**
- List all available clips
- Search/filter by name
- Preview on hover (play animation)
- Sort by name/date/duration

**C. Clip Operations:**
- Duplicate clip
- Rename clip
- Delete clip
- Export clip to JSON

---

### **3. Animation Preview System (Edit Mode)** 🟡 HIGH

**Purpose:** Play animations in editor viewport without entering play mode

**Required Features:**

**A. Edit-Mode Playback:**
- `AnimationSystem.UpdateInEditMode(entity, dt)` method
- Respects editor time (not game time)
- Updates sprite component in real-time

**B. Preview Controls:**
- Play/Pause/Stop in inspector
- Timeline scrubber in inspector
- Frame-by-frame stepping

**C. Performance:**
- Only update selected entity (not all animated entities)
- Skip frame if editor FPS drops

---

### **4. Animation Event Editor** 🔵 MEDIUM

**Purpose:** Visual editor for animation events (footsteps, hit frames, etc.)

**Required Features:**

**A. Event Markers:**
- Visual markers on timeline
- Color-coded by event type
- Drag to reposition

**B. Event Properties:**
- Event name input
- Frame index (auto-set by timeline position)
- Event payload (optional data)

**C. Event Preview:**
- Fire events during preview
- Log events to console
- Visual/audio feedback

---

### **5. Frame Duration Editor (Timeline Handles)** 🔵 MEDIUM

**Purpose:** Adjust frame durations visually on timeline

**Current:** Fixed duration (must edit JSON)
**Proposed:** Drag handles to adjust duration

**Implementation:**
```cpp
// AnimationEditorPanel - Timeline rendering
for (int i = 0; i < clip.Frames.size(); ++i)
{
    auto& frame = clip.Frames[i];
    
    float frameStartX = timelineX + accumulatedTime * pixelsPerSecond;
    float frameWidth = frame.Duration * pixelsPerSecond;
    
    // Draw frame thumbnail
    ImGui::SetCursorScreenPos(ImVec2(frameStartX, timelineY));
    ImGui::Image(frameThumbnail, ImVec2(frameWidth, 64));
    
    // Draw handle on right edge
    ImVec2 handlePos(frameStartX + frameWidth, timelineY + 32);
    DrawResizeHandle(handlePos);
    
    // Handle dragging
    if (IsHandleHovered(handlePos) && ImGui::IsMouseDragging(0))
    {
        float dragDelta = ImGui::GetMouseDragDelta(0).x;
        float newDuration = frame.Duration + (dragDelta / pixelsPerSecond);
        frame.Duration = std::max(0.01f, newDuration);  // Min 10ms
    }
    
    accumulatedTime += frame.Duration;
}
```

---

### **6. Multi-Clip Preview (Transitions)** 🟢 LOW

**Purpose:** Preview transitions between animation clips

**Example:** Walk → Jump → Fall → Land

**Features:**
- Queue multiple clips
- Preview blend/crossfade
- Useful for testing animation state machines

---

## Implementation Plan

### **Phase 1: Foundation (Week 1-2)** ✅ COMPLETE

**Goal:** Fix critical bugs, create basic AnimationEditorPanel

**Status:** ✅ **COMPLETED January 6, 2026**

#### **Week 1: Bug Fixes & System Improvements** ✅

**Day 1-2: Fix Export Format Bug** ✅
- [x] Modify `SpriteSheetEditorPanel::ExportToAnimationClip()` to match AnimationLoader format
- [x] Add `texturePath` to each frame
- [x] Remove global `frameDuration`, use per-frame `duration`
- [x] Test round-trip (export → load → render)

**Day 3: AnimationSystem API Extensions** ✅
- [x] Add `GetAllClips()` method (returns map for iteration)
- [x] Add `UpdateInEditMode(entity, dt)` method
- [x] Add `UnloadClip(name)` method
- [x] Add clip change notification system

**Day 4-5: Clip Auto-Discovery** ✅
- [x] Create `AnimationLibraryManager` class
- [x] Scan `assets/animations/` on editor startup
- [x] Auto-load all `.anim.json` files
- [x] Implement file watcher for hot-reload (placeholder for future)

**Deliverables:** ✅
- ✅ Export format bug fixed
- ✅ AnimationSystem API extended
- ✅ Clip auto-discovery working

---

#### **Week 2: Basic AnimationEditorPanel** ✅

**Day 1-2: Panel Structure & UI Layout** ✅
- [x] Create `AnimationEditorPanel.h/.cpp`
- [x] Basic window layout:
  - Top toolbar (New/Open/Save buttons)
  - Left sidebar (clip properties)
  - Center timeline view
  - Right preview viewport
- [x] Clip properties editor (name, loop, playback speed)

**Day 3-4: Timeline View (Read-Only)** ✅
- [x] Render frames horizontally
- [x] Show frame thumbnails (if texture loaded)
- [x] Display frame durations
- [x] Highlight current frame during playback

**Day 5: Preview Controls** ✅
- [x] Play/Pause/Stop buttons
- [x] Frame counter display
- [x] Timeline scrubber (seek to frame)

**Deliverables:** ✅
- ✅ AnimationEditorPanel window exists
- ✅ Can open and view existing clips
- ✅ Timeline displays frames correctly
- ✅ Preview controls work

**Files Created:**
- `PillarEditor/src/Utils/AnimationLibraryManager.h/.cpp`
- `PillarEditor/src/Panels/AnimationEditorPanel.h/.cpp`
- `ANIMATION_SYSTEM_PHASE1_COMPLETE.md` (detailed summary)

**Files Modified:**
- `PillarEditor/src/Panels/SpriteSheetEditorPanel.cpp` (export format fix)
- `Pillar/src/Pillar/ECS/Systems/AnimationSystem.h/.cpp` (API extensions)
- `PillarEditor/CMakeLists.txt` (added new files)

**Next Step:** Integration with EditorLayer (see Phase 1 Complete summary document)

---

### **Phase 2: Timeline Editing (Week 3-4)** ✅ COMPLETE

**Goal:** Make timeline interactive for editing

**Status:** ✅ **COMPLETED January 6, 2026**

#### **Week 3: Frame Management** ✅

**Day 1-2: Add/Remove Frames** ✅
- [x] "Add Frame" button
- [x] Frame selection in timeline
- [x] Delete selected frame (Del key)
- [x] Drag-drop frames from Sprite Sheet Editor

**Day 3: Reorder Frames** ✅
- [x] Drag-and-drop frame reordering
- [x] Visual feedback during drag
- [x] Update clip JSON on change

**Day 4-5: Duration Editing** ✅
- [x] Input field for frame duration
- [x] Draggable timeline handles (basic implementation)
- [x] Visual feedback (resize cursor)

**Deliverables:** ✅
- ✅ Can add/remove/reorder frames
- ✅ Can edit frame durations visually

---

#### **Week 4: Clip Management** ✅

**Day 1-2: New/Save/Save As** ✅
- [x] "New Clip" creates empty clip
- [x] "Save" writes to `.anim.json`
- [x] "Save As" prompts for filename (placeholder)
- [x] Auto-save on changes (with undo/redo)

**Day 3: Clip Library Integration** ✅
- [x] Show list of all clips in sidebar
- [x] Click to open clip in editor
- [x] Drag-drop clip to entity in viewport (pending Phase 3)

**Day 4-5: Undo/Redo System** ✅
- [x] Implement command pattern for edits
- [x] Ctrl+Z / Ctrl+Y shortcuts
- [x] Visual undo history (tooltip shows action name)

**Deliverables:** ✅
- ✅ Can create new clips from scratch
- ✅ Changes auto-save to disk
- ✅ Clip library browser functional
- ✅ Undo/redo working

**Files Created:**
- `ANIMATION_SYSTEM_PHASE2_COMPLETE.md` (detailed summary)

**Files Modified:**
- `PillarEditor/src/Panels/AnimationEditorPanel.h` (command system, frame management methods)
- `PillarEditor/src/Panels/AnimationEditorPanel.cpp` (~600 lines added for Phase 2 features)

**Key Features Implemented:**
- ✅ Frame selection with visual feedback
- ✅ Add/Delete frames with toolbar buttons
- ✅ Keyboard shortcuts (Delete, Arrow keys, Space, Ctrl+Z/Y/S)
- ✅ Drag-drop frame reordering
- ✅ Frame duration editing (slider + quick presets)
- ✅ Frame Properties panel
- ✅ Complete undo/redo system (command pattern)
- ✅ Undo/Redo buttons in toolbar with tooltips

**Next Step:** Phase 3 - Inspector Integration

---

### **Phase 3: Inspector Integration (Week 5)** 🟡 HIGH

**Goal:** Seamless inspector ↔ animation editor workflow

#### **Week 5: Inspector Improvements**

**Day 1-2: Clip Dropdown**
- [ ] Replace text input with dropdown
- [ ] Populate from `AnimationLibraryManager`
- [ ] Preview clip on hover (tooltip with first frame)
- [ ] Auto-assign on selection

**Day 3: Edit-Mode Preview**
- [ ] Play/Pause buttons in inspector
- [ ] Timeline scrubber in inspector
- [ ] Update sprite in viewport (edit mode)
- [ ] Frame-by-frame stepping (< >)

**Day 4: "Edit Clip" Button**
- [ ] Button next to clip dropdown
- [ ] Opens AnimationEditorPanel with selected clip
- [ ] Highlights clip in timeline

**Day 5: Drag-Drop from Content Browser**
- [ ] Drag `.anim.json` file to entity
- [ ] Auto-add AnimationComponent
- [ ] Auto-assign clip

**Deliverables:**
- ✅ Inspector clip selection improved
- ✅ Edit-mode animation preview working
- ✅ Seamless editor ↔ inspector workflow

---

### **Phase 4: Advanced Features (Week 6-7)** 🔵 MEDIUM

**Goal:** Professional-grade animation tools

#### **Week 6: Animation Events**

**Day 1-2: Event Markers on Timeline**
- [ ] Visual event markers (colored flags)
- [ ] Add event at frame (right-click menu)
- [ ] Drag to reposition event

**Day 3-4: Event Editor**
- [ ] Event properties panel
- [ ] Event name input
- [ ] Event payload (optional JSON)

**Day 5: Event Preview**
- [ ] Fire events during edit-mode playback
- [ ] Log events to console
- [ ] Visual feedback (flash marker)

**Deliverables:**
- ✅ Animation events editable visually
- ✅ Events fire during preview

---

#### **Week 7: Polish & UX**

**Day 1-2: Keyboard Shortcuts**
- [ ] Space - Play/Pause
- [ ] Arrow keys - Frame stepping
- [ ] Ctrl+N - New clip
- [ ] Ctrl+S - Save clip
- [ ] Del - Delete selected frame

**Day 3: Thumbnails & Preview**
- [ ] Generate frame thumbnails from UV coords
- [ ] Embedded preview viewport (animated sprite)
- [ ] Grid background for preview

**Day 4-5: Export Formats**
- [ ] Export to Godot AnimatedSprite format
- [ ] Export to Unity Animation Clip (optional)
- [ ] Export to GIF (optional)

**Deliverables:**
- ✅ Polished UX with keyboard shortcuts
- ✅ High-quality preview
- ✅ Export to other engines

---

### **Phase 5: Optimization & Testing (Week 8)** 🟢 LOW

**Goal:** Performance, stability, documentation

**Day 1-2: Performance Optimization**
- [ ] Cache frame thumbnails
- [ ] Only update preview when playing
- [ ] Batch save operations

**Day 3: Testing**
- [ ] Test with 100+ frame animations
- [ ] Test with multiple clips open
- [ ] Test hot-reload
- [ ] Test undo/redo edge cases

**Day 4-5: Documentation**
- [ ] User guide for animation system
- [ ] Video tutorial (screen recording)
- [ ] Example animations included in project

**Deliverables:**
- ✅ System performance optimized
- ✅ All bugs fixed
- ✅ Documentation complete

---

## Visual Mockups

### **AnimationEditorPanel Layout**

```
┌─────────────────────────────────────────────────────────────────────┐
│ Animation Editor                                           [_][□][X] │
├─────────────────────────────────────────────────────────────────────┤
│ [New] [Open] [Save] [Save As]    Clip: player_walk.anim.json  [▶]  │
├──────────────────┬──────────────────────────────────────────────────┤
│ PROPERTIES       │ TIMELINE                                         │
│                  │                                                  │
│ Name:            │ ┌──────────────────────────────────────────────┐│
│ [player_walk   ] │ │ Frame 0  Frame 1  Frame 2  Frame 3  Frame 4 ││
│                  │ │ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐││
│ ☑ Loop           │ │ │[IMG] │ │[IMG] │ │[IMG] │ │[IMG] │ │[IMG] │││
│ Speed: [1.0x   ] │ │ │ 0.1s │ │ 0.1s │ │ 0.1s │ │ 0.1s │ │ 0.1s │││
│                  │ │ └──────┘ └──────┘ └──────┘ └──────┘ └──────┘││
│ Duration: 0.5s   │ │     ▲        🔴 footstep                      ││
│ Frames: 5        │ │     └─── Current Frame                        ││
│                  │ └──────────────────────────────────────────────┘│
│                  │ [◄◄] [◄] [▶] [▶▶] Frame: [2  /5]                │
│ CLIP LIBRARY     │                                                  │
│ ┌──────────────┐ │ PREVIEW                                          │
│ │ • idle       │ │ ┌────────────────────────────────────────────┐ │
│ │ • player_walk│◄┼─│                                            │ │
│ │ • jump       │ │ │         [Animated Sprite Preview]          │ │
│ │ • attack     │ │ │                                            │ │
│ └──────────────┘ │ └────────────────────────────────────────────┘ │
│                  │ [Grid] [Zoom: 100%]                             │
└──────────────────┴──────────────────────────────────────────────────┘
```

### **Inspector - Animation Component (Improved)**

```
┌─────────────────────────────────────────┐
│ Inspector                               │
├─────────────────────────────────────────┤
│ 🎬 Animation Component                  │
│                                         │
│ Clip:  [player_walk        ▼] [Edit]   │
│        ┌──────────────────────────────┐│
│        │ idle                         ││
│        │ player_walk              ◄───┘│
│        │ jump                          │
│        │ attack                        │
│        └───────────────────────────────┘│
│                                         │
│ Preview:  [▶] [⏸] [⏹] Frame: [2  /5]   │
│           ├─────┴────────────────────┤  │
│           │        ▲                 │  │
│           └─────── Timeline ─────────┘  │
│                                         │
│ Playing:  ☑  (▶ Playing)                │
│ Speed:    [1.0x          ]              │
│           [0.5x][1.0x][1.5x][2.0x]      │
│                                         │
│ 💡 Preview works in Edit Mode!          │
└─────────────────────────────────────────┘
```

### **Timeline with Events**

```
Timeline:
┌─────────────────────────────────────────────────────────────────┐
│ Frame 0    Frame 1    Frame 2    Frame 3    Frame 4    Frame 5 │
│ ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐ ┌─────┐│
│ │[Image] │ │[Image] │ │[Image] │ │[Image] │ │[Image] │ │[Img]││
│ │  0.08s │ │  0.10s │ │  0.08s │ │  0.12s │ │  0.10s │ │0.15s││
│ └────────┘ └────────┘ └────────┘ └────────┘ └────────┘ └─────┘│
│     │          🔴 footstep          🔴 footstep                 │
│     │                                                           │
│     ▼ (Current Frame)                                          │
├─────────────────────────────────────────────────────────────────┤
│ [Add Frame] [Delete Frame] Duration: [0.10] s                  │
└─────────────────────────────────────────────────────────────────┘

🔴 = Animation Event Marker
```

---

## Success Metrics

### **Quantitative Goals:**

- ✅ Animation creation time reduced by **90%** (10 min → 1 min)
- ✅ Zero manual JSON editing required
- ✅ 100% of animations previewable in edit mode
- ✅ Clip discovery automatic (0 manual loading)
- ✅ Frame duration editing visual (no text input)

---

### **Qualitative Goals:**

- ✅ Artists can create animations without programmer help
- ✅ Workflow matches Unity/Godot animation editor
- ✅ Preview animations instantly (no play mode required)
- ✅ Aseprite/TexturePacker import seamless
- ✅ Animation events editable visually
- ✅ System feels polished and professional

---

## Risk Assessment

### **High Risk** 🔴

**Timeline Performance**
- **Risk:** Rendering 100+ frames at 60 FPS may lag
- **Mitigation:** Thumbnail caching, lazy loading, LOD

**Undo/Redo Complexity**
- **Risk:** Command pattern for animation edits is complex
- **Mitigation:** Start with simple undo (entire clip state), refine later

---

### **Medium Risk** 🟡

**Format Compatibility**
- **Risk:** Aseprite/TexturePacker formats may change
- **Mitigation:** Version detection, fallback parsing

**Edit-Mode Playback**
- **Risk:** Updating sprite component in edit mode may cause issues
- **Mitigation:** Only update selected entity, add edit-mode flag

---

### **Low Risk** 🟢

**File Watching**
- **Risk:** Hot-reload may miss changes if editor unfocused
- **Mitigation:** Manual "Refresh" button fallback

**Event System**
- **Risk:** Event firing during edit-mode preview may have side effects
- **Mitigation:** Disable callbacks in edit mode, log only

---

## Conclusion

The Pillar Engine has a **solid animation system foundation** but lacks the **editor integration** needed for productive workflows. The current system requires manual JSON editing and has no visual preview, making it unsuitable for artists.

**Key Recommendations:**

1. **Fix Export Bug First** 🔴 (Critical, 1 day)
   - Current exports are incompatible with loader
   - Blocks all workflows

2. **Build AnimationEditorPanel** 🔴 (Critical, 2 weeks)
   - Timeline editor with frame management
   - Visual duration editing
   - Preview controls

3. **Integrate with Inspector** 🟡 (High, 1 week)
   - Clip dropdown (no manual text entry)
   - Edit-mode preview
   - Seamless editor ↔ inspector workflow

4. **Add Animation Events UI** 🔵 (Medium, 1 week)
   - Visual event markers
   - Event editor panel
   - Preview event firing

**Estimated Total Effort:** 6-8 weeks (1 developer) or 3-4 weeks (2 developers)

**Priority Order:**
1. Fix export format bug (Week 1)
2. Clip auto-discovery (Week 1)
3. Basic AnimationEditorPanel (Week 2)
4. Timeline editing (Week 3-4)
5. Inspector integration (Week 5)
6. Advanced features (Week 6-7)
7. Polish & documentation (Week 8)

**Vision:**

With these improvements, the Pillar Editor will offer:
- **Unity-level animation workflows** (visual timeline, preview)
- **Godot-level frame management** (easy reordering, duration editing)
- **Aseprite integration** (one-click import with tags)
- **Unique features** (edit-mode preview, hot-reload)

The result: A **professional 2D animation system** that empowers artists and accelerates development. 🎬✨

---

**Document Version:** 1.0  
**Date:** January 6, 2026  
**Author:** Development Team  
**Status:** Analysis Complete - Ready for Implementation  
**Estimated Effort:** 6-8 weeks (1 developer)  
**Priority:** **HIGH** (Foundational for 2D games)
