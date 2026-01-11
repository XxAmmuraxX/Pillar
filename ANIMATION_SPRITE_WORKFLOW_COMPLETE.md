# Animation & Sprite System - Complete Workflow Documentation

**Date:** January 6, 2026  
**Status:** ✅ Fully Analyzed & Documented  
**Purpose:** Comprehensive guide to how animations work end-to-end in Pillar Engine

---

## Executive Summary

This document provides a **complete, detailed workflow** of the animation and sprite systems in Pillar Engine, from asset creation to viewport rendering. The systems are functionally complete but involve multiple interconnected components that must work together correctly. Understanding this workflow is critical for troubleshooting issues and improving the user experience.

**Key Finding:** The workflow involves **12+ distinct steps** across **7 major systems**, making it complex for users and developers alike. While the architecture is sound, the number of handoffs and manual interventions creates friction.

---

## Table of Contents

1. [System Architecture Overview](#system-architecture-overview)
2. [Complete Workflow: Asset to Viewport](#complete-workflow-asset-to-viewport)
3. [Component Communication Map](#component-communication-map)
4. [Data Flow Diagrams](#data-flow-diagrams)
5. [Critical Integration Points](#critical-integration-points)
6. [Pain Points & Bottlenecks](#pain-points--bottlenecks)
7. [Troubleshooting Guide](#troubleshooting-guide)
8. [Recommendations for Improvement](#recommendations-for-improvement)

---

## System Architecture Overview

### **The 7 Core Systems**

```
┌───────────────────────────────────────────────────────────────────────┐
│                    PILLAR ANIMATION & SPRITE ARCHITECTURE              │
└───────────────────────────────────────────────────────────────────────┘

ENGINE LAYER (Pillar/)
┌─────────────────────────────────────────────────────────────────────┐
│ 1. AnimationComponent (ECS Component)                                │
│    • Stores: CurrentClipName, FrameIndex, PlaybackTime, Playing     │
│    • Methods: Play(), Pause(), Stop(), Resume()                      │
│    • Callbacks: OnAnimationEvent, OnAnimationComplete                │
└─────────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────────┐
│ 2. AnimationSystem (ECS System)                                      │
│    • Updates all AnimationComponents each frame                      │
│    • Manages animation clip library (m_AnimationLibrary)             │
│    • Advances frames based on time and duration                      │
│    • Calls UpdateSpriteFromFrame() to modify SpriteComponent         │
│    • Fires animation events via callbacks                            │
└─────────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────────┐
│ 3. SpriteComponent (ECS Component)                                   │
│    • Stores: Texture, Color, Size, TexCoordMin/Max, ZIndex          │
│    • Modified by: AnimationSystem (UV coords), User (inspector)      │
│    • Read by: SpriteRenderSystem                                     │
└─────────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────────┐
│ 4. SpriteRenderSystem (ECS System)                                   │
│    • Queries entities with SpriteComponent + TransformComponent      │
│    • Sorts by texture and Z-index for batch optimization             │
│    • Calls Renderer2DBackend::DrawSprite() for each visible sprite   │
└─────────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────────┐
│ 5. Renderer2DBackend (Rendering System)                              │
│    • Batches quads by texture                                        │
│    • Submits vertex data to GPU                                      │
│    • Applies UV coordinates, color tinting, transforms               │
│    • Final output: Pixels in viewport                                │
└─────────────────────────────────────────────────────────────────────┘

EDITOR LAYER (PillarEditor/)
┌─────────────────────────────────────────────────────────────────────┐
│ 6. AnimationLibraryManager                                           │
│    • Auto-scans assets/animations/ for .anim.json files              │
│    • Loads all clips into AnimationSystem on startup                 │
│    • Provides clip discovery for dropdowns                           │
└─────────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────────┐
│ 7. AnimationEditorPanel                                              │
│    • Visual timeline editor for creating/editing clips               │
│    • Frame management: add, delete, reorder, duration editing        │
│    • Saves clips to .anim.json files                                 │
│    • Preview playback with scrubber                                  │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Complete Workflow: Asset to Viewport

### **Phase 1: Asset Creation & Import**

#### **Step 1.1: Artist Creates Sprite Sheet**
**Who:** Artist (external tool)  
**Where:** Aseprite, Photoshop, TexturePacker, etc.  
**Output:** `player_walk.png` (sprite sheet with multiple frames)

**Example:**
```
player_walk.png (128x32 pixels)
┌────────┬────────┬────────┬────────┐
│ Frame0 │ Frame1 │ Frame2 │ Frame3 │
│  32x32 │  32x32 │  32x32 │  32x32 │
└────────┴────────┴────────┴────────┘
```

---

#### **Step 1.2: Import Texture to Editor**
**Who:** User  
**Where:** Content Browser (PillarEditor)  
**Action:** 
1. Drag `player_walk.png` into `assets/textures/`
2. Editor detects new file (file watcher)
3. Texture appears in Content Browser

**Code Flow:**
```cpp
// ContentBrowserPanel detects file
// No loading happens yet - textures load on-demand
```

**Output:** Texture file available in project

---

### **Phase 2: Frame Definition (Sprite Sheet Slicing)**

#### **Step 2.1: Open Sprite Sheet Editor**
**Who:** User  
**Where:** PillarEditor → Menu → Windows → Sprite Sheet Editor  
**Action:**
1. Click "Load Texture"
2. Select `player_walk.png`
3. Configure grid: 4 columns × 1 row, cell size 32×32

**Code Flow:**
```cpp
// SpriteSheetEditorPanel.cpp
void SpriteSheetEditorPanel::LoadTexture(const std::string& path)
{
    m_Texture = Texture2D::Create(path);
    m_TexturePath = path;
    GenerateFrameLibrary(); // Creates grid of selectable frames
}
```

**Visual Result:**
```
Sprite Sheet Editor Panel
┌────────────────────────────────┐
│ Texture Preview:               │
│ ┌────┬────┬────┬────┐          │
│ │ 0  │ 1  │ 2  │ 3  │  ← Grid │
│ └────┴────┴────┴────┘          │
│                                │
│ Frame Library: (empty)         │
└────────────────────────────────┘
```

---

#### **Step 2.2: Select Frames for Animation**
**Who:** User  
**Where:** Sprite Sheet Editor  
**Action:**
1. Click frames in desired order: 0, 1, 2, 3, 2, 1 (walk cycle)
2. Frames added to "Frame Library" list
3. See thumbnail previews of selected frames

**Code Flow:**
```cpp
// User clicks frame in grid
void SpriteSheetEditorPanel::OnFrameClicked(int column, int row)
{
    AnimationFrame frame;
    frame.TexturePath = m_TexturePath;
    
    // Calculate UV coordinates
    float u_min = column / (float)m_Columns;
    float v_min = row / (float)m_Rows;
    float u_max = (column + 1) / (float)m_Columns;
    float v_max = (row + 1) / (float)m_Rows;
    
    frame.UVMin = glm::vec2(u_min, v_min);
    frame.UVMax = glm::vec2(u_max, v_max);
    frame.Duration = 0.1f; // Default duration
    
    m_FrameLibrary.push_back(frame); // Add to library
}
```

**Visual Result:**
```
Frame Library:
┌─────┬─────┬─────┬─────┬─────┬─────┐
│  0  │  1  │  2  │  3  │  2  │  1  │ ← Selected frames
└─────┴─────┴─────┴─────┴─────┴─────┘
```

---

#### **Step 2.3: Create Animation from Frames**
**Who:** User  
**Where:** Sprite Sheet Editor  
**Action:**
1. Click "Create Animation from Frames" button
2. Enter animation name: `player_walk`
3. Animation Editor Panel opens

**Code Flow:**
```cpp
// SpriteSheetEditorPanel.cpp
if (ImGui::Button("Create Animation from Frames"))
{
    if (m_FrameLibrary.empty())
    {
        PIL_WARN("Frame library is empty!");
        return;
    }
    
    // Open Animation Editor with frame library
    m_AnimationEditorPanel->CreateFromFrames(
        m_FrameLibrary,      // Vector of AnimationFrame
        m_Texture,           // Shared texture
        m_TexturePath        // Path for serialization
    );
    m_AnimationEditorPanel->SetVisible(true);
}
```

**Output:** AnimationEditorPanel opens with frames pre-loaded

---

### **Phase 3: Animation Editing (Timeline Editor)**

#### **Step 3.1: Edit Clip Properties**
**Who:** User  
**Where:** Animation Editor Panel (timeline)  
**State:** Panel shows timeline with 6 frames

**Visual:**
```
Animation Editor Panel
┌─────────────────────────────────────────────────────────────────┐
│ Clip Name: [player_walk_______________]  [Loop: ✓] [Speed: 1.0]│
│                                                                  │
│ Timeline:                                                        │
│ ┌─────┬─────┬─────┬─────┬─────┬─────┐                          │
│ │  0  │  1  │  2  │  3  │  2  │  1  │  ← Frames                │
│ │0.1s │0.1s │0.1s │0.1s │0.1s │0.1s │  ← Durations             │
│ └─────┴─────┴─────┴─────┴─────┴─────┘                          │
│                                                                  │
│ [▶ Play] [⏸ Pause] [⏹ Stop] [+ Add Frame] [🗑 Delete Frame]   │
└─────────────────────────────────────────────────────────────────┘
```

**Actions Available:**
- **Rename clip:** Change `player_walk` to `player_walk_slow`
- **Toggle loop:** Enable/disable animation looping
- **Adjust playback speed:** Change from 1.0x to 0.8x
- **Add frames:** Insert new frames from sprite sheet
- **Delete frames:** Remove unwanted frames
- **Reorder frames:** Drag frames to new positions
- **Edit durations:** Select frame → adjust duration slider

**Code Example (Frame Duration Edit):**
```cpp
// AnimationEditorPanel.cpp
void AnimationEditorPanel::RenderFrameProperties()
{
    if (m_SelectedFrameIndex < 0 || m_SelectedFrameIndex >= m_CurrentClip.Frames.size())
        return;
    
    auto& frame = m_CurrentClip.Frames[m_SelectedFrameIndex];
    
    ImGui::Text("Frame Properties:");
    ImGui::Separator();
    
    // Duration slider
    ImGui::Text("Duration:");
    ImGui::SliderFloat("##Duration", &frame.Duration, 0.01f, 2.0f, "%.3f s");
    
    // Quick duration presets
    if (ImGui::Button("0.05s")) frame.Duration = 0.05f;
    if (ImGui::Button("0.1s"))  frame.Duration = 0.1f;
    if (ImGui::Button("0.2s"))  frame.Duration = 0.2f;
    
    // Mark clip as modified
    m_ClipModified = true;
}
```

---

#### **Step 3.2: Save Animation Clip**
**Who:** User  
**Where:** Animation Editor Panel  
**Action:** Click "💾 Save" button (or press Ctrl+S)

**Code Flow:**
```cpp
// AnimationEditorPanel.cpp
bool AnimationEditorPanel::SaveClip()
{
    if (m_CurrentClip.Name.empty())
    {
        PIL_ERROR("Cannot save clip with empty name!");
        return false;
    }
    
    // Generate filepath
    std::string filepath = "assets/animations/" + m_CurrentClip.Name + ".anim.json";
    
    // Save to JSON using AnimationLoader
    bool success = AnimationLoader::SaveToJSON(m_CurrentClip, filepath);
    
    if (success)
    {
        PIL_INFO("Saved animation clip: {}", filepath);
        
        // Reload clip in AnimationSystem (hot-reload)
        m_AnimationSystem->LoadAnimationClip(filepath);
        
        m_ClipModified = false; // Clear dirty flag
    }
    
    return success;
}
```

**File Created:**
```json
// assets/animations/player_walk.anim.json
{
  "name": "player_walk",
  "loop": true,
  "playbackSpeed": 1.0,
  "frames": [
    {
      "texturePath": "assets/textures/player_walk.png",
      "duration": 0.1,
      "uvMin": [0.0, 0.0],
      "uvMax": [0.25, 1.0]
    },
    {
      "texturePath": "assets/textures/player_walk.png",
      "duration": 0.1,
      "uvMin": [0.25, 0.0],
      "uvMax": [0.5, 1.0]
    }
    // ... remaining frames
  ],
  "events": []
}
```

**Output:** `.anim.json` file on disk

---

### **Phase 4: Clip Discovery & Loading**

#### **Step 4.1: AnimationLibraryManager Auto-Scans**
**Who:** Editor (automatic)  
**When:** Editor startup OR file save  
**Where:** EditorLayer initialization

**Code Flow:**
```cpp
// EditorLayer.cpp - OnAttach()
void EditorLayer::OnAttach()
{
    // ... other initialization ...
    
    // Create and initialize AnimationLibraryManager
    m_AnimationLibraryManager.Initialize(m_AnimationSystem.get());
    
    // Scan for animation clips
    m_AnimationLibraryManager.ScanForClips("assets/animations/");
    
    // Load all discovered clips
    size_t loadedCount = m_AnimationLibraryManager.LoadAllClips();
    PIL_INFO("Loaded {} animation clips", loadedCount);
}
```

**AnimationLibraryManager Logic:**
```cpp
// AnimationLibraryManager.cpp
void AnimationLibraryManager::ScanForClips(const std::filesystem::path& directory)
{
    m_ClipFiles.clear();
    
    if (!std::filesystem::exists(directory))
    {
        PIL_WARN("Animation directory does not exist: {}", directory.string());
        return;
    }
    
    // Recursively search for .anim.json files
    for (const auto& entry : std::filesystem::recursive_directory_iterator(directory))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".json")
        {
            std::string stem = entry.path().stem().string();
            if (stem.ends_with(".anim")) // Check for .anim.json pattern
            {
                m_ClipFiles.push_back(entry.path());
            }
        }
    }
    
    PIL_INFO("Discovered {} animation clip files", m_ClipFiles.size());
}

size_t AnimationLibraryManager::LoadAllClips()
{
    size_t successCount = 0;
    
    for (const auto& filepath : m_ClipFiles)
    {
        if (m_AnimationSystem->LoadAnimationClip(filepath.string()))
        {
            successCount++;
        }
    }
    
    return successCount;
}
```

**AnimationSystem Loading:**
```cpp
// AnimationSystem.cpp
bool AnimationSystem::LoadAnimationClip(const std::string& filePath)
{
    // AnimationLoader reads JSON and deserializes
    AnimationClip clip = AnimationLoader::LoadFromJSON(filePath);
    
    if (!clip.IsValid())
    {
        PIL_CORE_ERROR("Failed to load animation clip from: {}", filePath);
        return false;
    }
    
    // Register clip in library (key = clip name)
    m_AnimationLibrary[clip.Name] = clip;
    
    PIL_CORE_INFO("Loaded animation clip: {} ({} frames)", clip.Name, clip.GetFrameCount());
    return true;
}
```

**Result:** Animation clip now in memory, accessible by name

---

### **Phase 5: Entity Setup & Animation Assignment**

#### **Step 5.1: Create Entity with Sprite**
**Who:** User  
**Where:** Scene Hierarchy Panel  
**Action:**
1. Right-click in hierarchy → Create Entity → "Player"
2. Select "Player" entity
3. In Inspector, click "Add Component" → SpriteComponent
4. Load texture: `player_walk.png`

**Code Flow:**
```cpp
// Scene.cpp
Entity Scene::CreateEntity(const std::string& name)
{
    Entity entity = { m_Registry.create(), this };
    entity.AddComponent<TagComponent>(name);
    entity.AddComponent<TransformComponent>();
    entity.AddComponent<UUIDComponent>();
    return entity;
}

// Inspector - User adds SpriteComponent
sprite.TexturePath = "assets/textures/player_walk.png";
sprite.Texture = Texture2D::Create(sprite.TexturePath);
sprite.Size = glm::vec2(32.0f, 32.0f); // Match texture size
sprite.Color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
sprite.TexCoordMin = glm::vec2(0.0f, 0.0f); // Full texture
sprite.TexCoordMax = glm::vec2(1.0f, 1.0f);
```

**Visual Result:**
```
Scene Hierarchy:          Inspector:
┌───────────────┐        ┌─────────────────────────┐
│ Player        │   ───▶ │ SpriteComponent         │
└───────────────┘        │ Texture: player_walk... │
                         │ Color:   (1, 1, 1, 1)   │
                         │ Size:    (32, 32)       │
                         │ Z-Index: 0              │
                         └─────────────────────────┘
```

**Note:** At this point, the sprite shows frame 0 (or full texture if UV = 0-1)

---

#### **Step 5.2: Add AnimationComponent**
**Who:** User  
**Where:** Inspector Panel  
**Action:**
1. Click "Add Component" → AnimationComponent
2. **Method A (Manual):** Type clip name: `player_walk`
3. **Method B (Dropdown):** Select `player_walk` from dropdown
4. **Method C (Drag-Drop):** Drag `player_walk.anim.json` from Content Browser onto entity

**Code Flow - Method B (Dropdown):**
```cpp
// InspectorPanel.cpp - DrawAnimationComponent()
void InspectorPanel::DrawAnimationComponent(Entity entity)
{
    auto& anim = entity.GetComponent<AnimationComponent>();
    
    // Get available clips from AnimationLibraryManager
    auto& animLibManager = m_EditorLayer->GetAnimationLibraryManager();
    const auto& availableClips = animLibManager.GetAllClipNames();
    
    // Dropdown UI
    const char* currentClipName = anim.CurrentClipName.empty() ? "None" : anim.CurrentClipName.c_str();
    if (ImGui::BeginCombo("##ClipName", currentClipName))
    {
        // "None" option to clear assignment
        if (ImGui::Selectable("None", anim.CurrentClipName.empty()))
        {
            anim.CurrentClipName = "";
        }
        
        // List all available clips
        for (const auto& clipName : availableClips)
        {
            bool isSelected = (anim.CurrentClipName == clipName);
            if (ImGui::Selectable(clipName.c_str(), isSelected))
            {
                anim.CurrentClipName = clipName;
                if (anim.Playing)
                {
                    anim.Play(clipName); // Restart with new clip
                }
            }
            
            // Hover preview
            if (ImGui::IsItemHovered())
            {
                auto* clip = animLibManager.GetClip(clipName);
                if (clip && clip->IsValid())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("Duration: %.2f s", clip->GetDuration());
                    ImGui::Text("Frames: %d", clip->GetFrameCount());
                    ImGui::EndTooltip();
                }
            }
        }
        ImGui::EndCombo();
    }
}
```

**Result:** AnimationComponent now references `player_walk` clip

---

#### **Step 5.3: Animation Preview in Edit Mode (Optional)**
**Who:** User  
**Where:** Inspector Panel → Edit Mode Preview section  
**Action:**
1. Use frame navigation buttons: ◄◄ First, ◄ Prev, Next ►, Last ►►
2. Or drag frame scrubber slider
3. Sprite updates **immediately** in viewport (no play mode needed)

**Code Flow:**
```cpp
// InspectorPanel.cpp - Edit Mode Preview
if (ImGui::Button("Next ►"))
{
    auto* clip = m_EditorLayer->GetAnimationSystem()->GetClip(anim.CurrentClipName);
    if (clip && clip->IsValid())
    {
        anim.FrameIndex++;
        if (anim.FrameIndex >= clip->GetFrameCount())
            anim.FrameIndex = 0; // Loop around
        
        anim.PlaybackTime = 0.0f;
        
        // **KEY CALL**: Force update sprite in edit mode
        m_EditorLayer->GetAnimationSystem()->UpdateInEditMode(
            static_cast<entt::entity>(entity), 0.0f);
    }
}
```

**UpdateInEditMode Implementation:**
```cpp
// AnimationSystem.cpp
void AnimationSystem::UpdateInEditMode(entt::entity entity, float dt)
{
    if (!m_Scene)
        return;
    
    auto& registry = m_Scene->GetRegistry();
    auto& anim = registry.get<AnimationComponent>(entity);
    
    // Get clip
    AnimationClip* clip = GetClip(anim.CurrentClipName);
    if (!clip || !clip->IsValid())
        return;
    
    // Ensure frame index is valid
    if (anim.FrameIndex >= clip->GetFrameCount())
        anim.FrameIndex = 0;
    
    // Get current frame
    const AnimationFrame& currentFrame = clip->Frames[anim.FrameIndex];
    
    // **CRITICAL**: Update sprite component with frame data
    UpdateSpriteFromFrame(entity, currentFrame);
}
```

**Visual Result:** Sprite in viewport shows selected frame

---

### **Phase 6: Play Mode Animation Update Loop**

#### **Step 6.1: Enter Play Mode**
**Who:** User  
**Where:** Toolbar → Press "▶ Play" button  
**Action:** Editor switches from Edit mode to Play mode

**Code Flow:**
```cpp
// EditorLayer.cpp
void EditorLayer::OnPlay()
{
    m_EditorState = EditorState::Play;
    
    // Backup edit-mode scene
    m_EditorScene = std::make_shared<Scene>(*m_ActiveScene);
    
    // Start runtime
    m_ActiveScene->OnRuntimeStart();
    
    PIL_INFO("Entered play mode");
}
```

**State Change:**
```
Edit Mode → Play Mode
- AnimationSystem starts updating
- SpriteRenderSystem starts rendering
- Physics systems activate
```

---

#### **Step 6.2: AnimationSystem Update Loop (CRITICAL)**
**Who:** Engine (automatic, every frame)  
**Where:** EditorLayer::OnUpdate() → AnimationSystem::OnUpdate()  
**When:** Every frame while in Play mode

**Complete Code Flow:**
```cpp
// EditorLayer.cpp - OnUpdate()
void EditorLayer::OnUpdate(float deltaTime)
{
    if (m_EditorState == EditorState::Play)
    {
        // **STEP 1**: Update animation system
        if (m_AnimationSystem)
            m_AnimationSystem->OnUpdate(deltaTime);
        
        // **STEP 2**: Update other systems (physics, audio, etc.)
        // ...
        
        // **STEP 3**: Render systems update
        // (SpriteRenderSystem called separately)
    }
}

// AnimationSystem.cpp - OnUpdate()
void AnimationSystem::OnUpdate(float dt)
{
    if (!m_Scene)
        return;
    
    auto& registry = m_Scene->GetRegistry();
    
    // **QUERY**: Get all entities with AnimationComponent + SpriteComponent
    auto view = registry.view<AnimationComponent, SpriteComponent>();
    
    for (auto entity : view)
    {
        UpdateAnimation(entity, dt);
    }
}

void AnimationSystem::UpdateAnimation(entt::entity entity, float dt)
{
    auto& registry = m_Scene->GetRegistry();
    auto& anim = registry.get<AnimationComponent>(entity);
    
    // **STEP A**: Get animation clip from library
    AnimationClip* clip = GetClip(anim.CurrentClipName);
    if (!clip || !clip->IsValid())
    {
        PIL_CORE_WARN("Animation clip not found: {}", anim.CurrentClipName);
        return;
    }
    
    // **STEP B**: Validate frame index
    if (anim.FrameIndex >= clip->GetFrameCount())
    {
        anim.FrameIndex = 0;
        anim.PlaybackTime = 0.0f;
    }
    
    // **STEP C**: Get current frame data
    const AnimationFrame& currentFrame = clip->Frames[anim.FrameIndex];
    
    // **STEP D**: Update sprite component (ALWAYS, even if paused)
    UpdateSpriteFromFrame(entity, currentFrame);
    
    // **STEP E**: Skip time advancement if not playing
    if (!anim.IsPlaying())
        return;
    
    // **STEP F**: Advance playback time
    anim.PlaybackTime += dt * anim.PlaybackSpeed * clip->PlaybackSpeed;
    
    // **STEP G**: Check if frame duration exceeded
    if (anim.PlaybackTime >= currentFrame.Duration)
    {
        int oldFrame = anim.FrameIndex;
        AdvanceFrame(anim, *clip, entity);
        FireAnimationEvents(anim, *clip, oldFrame, entity);
    }
}

void AnimationSystem::UpdateSpriteFromFrame(entt::entity entity, const AnimationFrame& frame)
{
    auto& registry = m_Scene->GetRegistry();
    auto& sprite = registry.get<SpriteComponent>(entity);
    
    // **STEP 1**: Update texture if needed
    if (!frame.TexturePath.empty())
    {
        // Cache textures to avoid reloading every frame
        static std::unordered_map<std::string, std::shared_ptr<Texture2D>> s_TextureCache;
        
        auto it = s_TextureCache.find(frame.TexturePath);
        if (it == s_TextureCache.end())
        {
            // Load and cache new texture
            auto texture = Texture2D::Create(frame.TexturePath);
            s_TextureCache[frame.TexturePath] = texture;
            sprite.Texture = texture;
        }
        else
        {
            sprite.Texture = it->second;
        }
    }
    
    // **STEP 2**: Update UV coordinates (unless locked by editor)
    if (!sprite.LockUV)
    {
        sprite.TexCoordMin = frame.UVMin;
        sprite.TexCoordMax = frame.UVMax;
    }
}

void AnimationSystem::AdvanceFrame(AnimationComponent& anim, AnimationClip& clip, entt::entity entity)
{
    // Reset playback time for next frame
    anim.PlaybackTime = 0.0f;
    
    // Advance to next frame
    anim.FrameIndex++;
    
    // Handle looping or stopping at end
    if (anim.FrameIndex >= clip.GetFrameCount())
    {
        if (clip.Loop)
        {
            anim.FrameIndex = 0; // Loop back to start
        }
        else
        {
            anim.FrameIndex = clip.GetFrameCount() - 1; // Stay on last frame
            anim.Playing = false; // Stop playing
            
            // Fire completion callback
            if (anim.OnAnimationComplete)
                anim.OnAnimationComplete(entity);
        }
    }
}
```

**Key Insight:** AnimationSystem **modifies** SpriteComponent's `TexCoordMin` and `TexCoordMax` every frame to show the correct frame. This is the **critical handoff** between animation and sprite systems.

---

### **Phase 7: Sprite Rendering Pipeline**

#### **Step 7.1: SpriteRenderSystem Collects Sprites**
**Who:** Engine (automatic, every frame)  
**Where:** SpriteRenderSystem::OnUpdate()  
**When:** After AnimationSystem update, before frame submission

**Code Flow:**
```cpp
// SpriteRenderSystem.cpp
void SpriteRenderSystem::OnUpdate(float dt)
{
    // **STEP 1**: Collect all entities with sprite + transform
    auto view = m_Scene->GetRegistry().view<TransformComponent, SpriteComponent>();
    
    // **STEP 2**: Sort for optimal batching
    std::vector<entt::entity> sortedEntities(view.begin(), view.end());
    std::sort(sortedEntities.begin(), sortedEntities.end(),
        [&view](entt::entity a, entt::entity b) {
            const auto& spriteA = view.get<SpriteComponent>(a);
            const auto& spriteB = view.get<SpriteComponent>(b);
            
            // Sort by texture first (minimize texture swaps)
            if (spriteA.Texture.get() != spriteB.Texture.get())
                return spriteA.Texture.get() < spriteB.Texture.get();
            
            // Then by Z-index (back to front)
            return spriteA.GetFinalZIndex() < spriteB.GetFinalZIndex();
        });
    
    // **STEP 3**: Render each sprite (batch renderer accumulates)
    for (auto entity : sortedEntities)
    {
        auto& transform = view.get<TransformComponent>(entity);
        auto& sprite = view.get<SpriteComponent>(entity);
        
        // Skip invisible sprites
        if (!sprite.Visible)
            continue;
        
        RenderSprite(transform, sprite);
    }
}

void SpriteRenderSystem::RenderSprite(const TransformComponent& transform,
                                     const SpriteComponent& sprite)
{
    // Submit to batch renderer
    Renderer2DBackend::DrawSprite(transform, sprite);
}
```

---

#### **Step 7.2: Renderer2DBackend Batches & Draws**
**Who:** Engine (automatic)  
**Where:** Renderer2DBackend::DrawSprite()  

**Code Flow:**
```cpp
// Renderer2DBackend.cpp
void Renderer2DBackend::DrawSprite(const TransformComponent& transform,
                                   const SpriteComponent& sprite)
{
    // **STEP 1**: Extract sprite data
    glm::vec3 position(transform.Position.x, transform.Position.y, sprite.ZIndex);
    glm::vec2 size = sprite.Size * transform.Scale;
    glm::vec4 color = sprite.Color;
    
    // **STEP 2**: Handle flip flags (UV coordinate swapping)
    glm::vec2 uvMin = sprite.TexCoordMin;
    glm::vec2 uvMax = sprite.TexCoordMax;
    if (sprite.FlipX)
        std::swap(uvMin.x, uvMax.x);
    if (sprite.FlipY)
        std::swap(uvMin.y, uvMax.y);
    
    // **STEP 3**: Submit quad to batch renderer
    if (sprite.Texture)
    {
        DrawQuad(position, size, sprite.Texture, uvMin, uvMax, color, transform.Rotation);
    }
    else
    {
        DrawQuad(position, size, color, transform.Rotation); // Colored quad
    }
}

void Renderer2DBackend::DrawQuad(const glm::vec3& position, const glm::vec2& size,
                                 std::shared_ptr<Texture2D> texture,
                                 const glm::vec2& uvMin, const glm::vec2& uvMax,
                                 const glm::vec4& color, float rotation)
{
    // **STEP A**: Check if batch is full or texture changed
    if (m_QuadIndexCount >= MaxIndices || 
        (texture && m_CurrentTexture != texture))
    {
        FlushBatch(); // Submit current batch to GPU
    }
    
    // **STEP B**: Build transform matrix
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position);
    if (rotation != 0.0f)
        transform = glm::rotate(transform, rotation, glm::vec3(0, 0, 1));
    transform = glm::scale(transform, glm::vec3(size, 1.0f));
    
    // **STEP C**: Calculate vertex positions (4 corners)
    glm::vec4 vertices[4] = {
        transform * glm::vec4(-0.5f, -0.5f, 0.0f, 1.0f), // Bottom-left
        transform * glm::vec4( 0.5f, -0.5f, 0.0f, 1.0f), // Bottom-right
        transform * glm::vec4( 0.5f,  0.5f, 0.0f, 1.0f), // Top-right
        transform * glm::vec4(-0.5f,  0.5f, 0.0f, 1.0f)  // Top-left
    };
    
    // **STEP D**: Write vertices to buffer
    for (int i = 0; i < 4; i++)
    {
        m_QuadVertexBufferPtr->Position = vertices[i];
        m_QuadVertexBufferPtr->Color = color;
        m_QuadVertexBufferPtr->TexCoord = GetTexCoord(i, uvMin, uvMax);
        m_QuadVertexBufferPtr->TexIndex = GetTextureSlot(texture);
        m_QuadVertexBufferPtr++;
    }
    
    // **STEP E**: Update index count (6 indices per quad)
    m_QuadIndexCount += 6;
    m_QuadCount++;
}

void Renderer2DBackend::FlushBatch()
{
    if (m_QuadIndexCount == 0)
        return; // Nothing to draw
    
    // **STEP 1**: Upload vertex data to GPU
    uint32_t dataSize = (uint8_t*)m_QuadVertexBufferPtr - (uint8_t*)m_QuadVertexBufferBase;
    m_QuadVertexBuffer->SetData(m_QuadVertexBufferBase, dataSize);
    
    // **STEP 2**: Bind textures to texture slots
    for (uint32_t i = 0; i < m_TextureSlotIndex; i++)
    {
        m_TextureSlots[i]->Bind(i);
    }
    
    // **STEP 3**: Bind shader and set uniforms
    m_QuadShader->Bind();
    m_QuadShader->SetMat4("u_ViewProjection", m_ViewProjectionMatrix);
    
    // **STEP 4**: Draw call
    m_QuadVertexArray->Bind();
    glDrawElements(GL_TRIANGLES, m_QuadIndexCount, GL_UNSIGNED_INT, nullptr);
    
    // **STEP 5**: Reset batch state
    m_QuadIndexCount = 0;
    m_QuadVertexBufferPtr = m_QuadVertexBufferBase;
    m_TextureSlotIndex = 0;
    
    m_Stats.DrawCalls++;
}
```

**Key Points:**
- UV coordinates from `SpriteComponent.TexCoordMin/Max` are used to map texture
- These UV coords were set by `AnimationSystem::UpdateSpriteFromFrame()`
- Batching groups sprites by texture to minimize draw calls
- Final output: Pixels drawn to viewport framebuffer

---

#### **Step 7.3: Viewport Display**
**Who:** ViewportPanel (automatic)  
**Where:** ViewportPanel::OnImGuiRender()  
**Action:** Display rendered framebuffer as ImGui image

**Code Flow:**
```cpp
// ViewportPanel.cpp
void ViewportPanel::OnImGuiRender()
{
    ImGui::Begin("Viewport");
    
    // Get framebuffer texture
    uint32_t textureID = m_Framebuffer->GetColorAttachmentRendererID();
    
    // Display as ImGui image
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    ImGui::Image((void*)(intptr_t)textureID, viewportSize, 
                ImVec2(0, 1), ImVec2(1, 0)); // Flip Y
    
    ImGui::End();
}
```

**Final Result:** Animated sprite visible in viewport, updating every frame

---

## Component Communication Map

### **Data Flow Diagram (Detailed)**

```
┌─────────────────────────────────────────────────────────────────────┐
│                          DATA FLOW MAP                               │
└─────────────────────────────────────────────────────────────────────┘

USER ACTIONS → EDITOR PANELS → ENGINE SYSTEMS → RENDERER → GPU

┌──────────────────┐
│ 1. User Action   │
│ (Inspector)      │
└────────┬─────────┘
         │ Sets: CurrentClipName = "player_walk"
         ▼
┌──────────────────────────┐
│ 2. AnimationComponent    │
│    (Entity Data)         │
│  • CurrentClipName       │ ◄── Modified by: User, Code
│  • FrameIndex            │ ◄── Modified by: AnimationSystem
│  • PlaybackTime          │ ◄── Modified by: AnimationSystem
│  • Playing               │ ◄── Modified by: User, Code
└────────┬─────────────────┘
         │ Read by: AnimationSystem
         ▼
┌──────────────────────────┐
│ 3. AnimationSystem       │
│    (Update Loop)         │
│  • GetClip(clipName)     │ ──▶ AnimationLibrary lookup
│  • Advance frame index   │
│  • Call: UpdateSprite... │
└────────┬─────────────────┘
         │ Modifies: SpriteComponent
         ▼
┌──────────────────────────┐
│ 4. SpriteComponent       │
│    (Rendering Data)      │
│  • Texture               │ ◄── Set by: AnimationSystem
│  • TexCoordMin           │ ◄── Set by: AnimationSystem (CRITICAL!)
│  • TexCoordMax           │ ◄── Set by: AnimationSystem (CRITICAL!)
│  • Color, Size, ZIndex   │ ◄── Set by: User (inspector)
└────────┬─────────────────┘
         │ Read by: SpriteRenderSystem
         ▼
┌──────────────────────────┐
│ 5. SpriteRenderSystem    │
│    (Sorting & Submit)    │
│  • Query sprites         │
│  • Sort by texture       │
│  • Submit to renderer    │
└────────┬─────────────────┘
         │ Calls: Renderer2DBackend::DrawSprite()
         ▼
┌──────────────────────────┐
│ 6. Renderer2DBackend     │
│    (Batch Builder)       │
│  • Build transform       │
│  • Calculate vertices    │
│  • Apply UV coords       │ ◄── Uses TexCoordMin/Max from sprite
│  • Accumulate quads      │
└────────┬─────────────────┘
         │ Submits: Vertex buffer to GPU
         ▼
┌──────────────────────────┐
│ 7. OpenGL / GPU          │
│    (Rasterization)       │
│  • Bind textures         │
│  • Draw triangles        │
│  • Output pixels         │
└────────┬─────────────────┘
         │ Renders to: Framebuffer
         ▼
┌──────────────────────────┐
│ 8. ViewportPanel         │
│    (Display)             │
│  • Show framebuffer      │
│  • User sees animation   │
└──────────────────────────┘
```

---

### **Critical Handoffs**

| From                  | To                     | Data Passed                     | Method                              |
|-----------------------|------------------------|---------------------------------|-------------------------------------|
| User                  | AnimationComponent     | Clip name                       | Inspector UI → Component field      |
| AnimationLibraryMgr   | AnimationSystem        | Loaded clips                    | LoadAnimationClip()                 |
| AnimationSystem       | AnimationLibrary       | Clip lookup                     | GetClip(name) → returns pointer     |
| AnimationSystem       | SpriteComponent        | UV coordinates, Texture         | UpdateSpriteFromFrame()             |
| SpriteComponent       | SpriteRenderSystem     | Full sprite data                | ECS query (view<Sprite, Transform>) |
| SpriteRenderSystem    | Renderer2DBackend      | Transform + Sprite              | DrawSprite(transform, sprite)       |
| Renderer2DBackend     | GPU                    | Vertex buffer, textures         | glDrawElements()                    |
| GPU                   | ViewportPanel          | Rendered framebuffer            | ImGui::Image()                      |

---

## Critical Integration Points

### **1. AnimationSystem ↔ SpriteComponent**

**Location:** `AnimationSystem::UpdateSpriteFromFrame()`

**Critical Code:**
```cpp
void AnimationSystem::UpdateSpriteFromFrame(entt::entity entity, const AnimationFrame& frame)
{
    auto& sprite = m_Scene->GetRegistry().get<SpriteComponent>(entity);
    
    // **INTEGRATION POINT 1**: Texture loading
    if (!frame.TexturePath.empty())
    {
        // Texture cache to avoid reloading
        static std::unordered_map<std::string, std::shared_ptr<Texture2D>> s_TextureCache;
        
        auto it = s_TextureCache.find(frame.TexturePath);
        if (it == s_TextureCache.end())
        {
            auto texture = Texture2D::Create(frame.TexturePath);
            s_TextureCache[frame.TexturePath] = texture;
            sprite.Texture = texture;
        }
        else
        {
            sprite.Texture = it->second;
        }
    }
    
    // **INTEGRATION POINT 2**: UV coordinate update (THE CRITICAL HANDOFF)
    if (!sprite.LockUV) // Respect editor lock flag
    {
        sprite.TexCoordMin = frame.UVMin;
        sprite.TexCoordMax = frame.UVMax;
    }
}
```

**Why Critical:**
- This is the **only place** UV coordinates are transferred from animation to sprite
- If this doesn't run, animations won't show (sprite stays on first frame)
- `LockUV` flag allows editor to override (for debugging/preview)

---

### **2. AnimationLibraryManager ↔ AnimationSystem**

**Location:** `AnimationLibraryManager::Initialize()` and `LoadAllClips()`

**Critical Code:**
```cpp
void AnimationLibraryManager::Initialize(AnimationSystem* animSystem)
{
    m_AnimationSystem = animSystem;
    
    // Auto-scan and load clips
    ScanForClips("assets/animations/");
    LoadAllClips();
}

size_t AnimationLibraryManager::LoadAllClips()
{
    size_t successCount = 0;
    for (const auto& filepath : m_ClipFiles)
    {
        // **INTEGRATION POINT**: Load clip into animation system
        if (m_AnimationSystem->LoadAnimationClip(filepath.string()))
        {
            successCount++;
        }
    }
    return successCount;
}
```

**Why Critical:**
- This is **the only automatic way** clips are loaded
- Without this, user must manually call `LoadAnimationClip()` in code
- Editor depends on this for clip discovery (Inspector dropdown)

---

### **3. Inspector ↔ EditorLayer ↔ Systems**

**Location:** `InspectorPanel::DrawAnimationComponent()`

**Critical Code:**
```cpp
void InspectorPanel::DrawAnimationComponent(Entity entity)
{
    // **INTEGRATION POINT 1**: Get clip names from manager
    auto& animLibManager = m_EditorLayer->GetAnimationLibraryManager();
    const auto& availableClips = animLibManager.GetAllClipNames();
    
    // **INTEGRATION POINT 2**: Open Animation Editor
    if (ImGui::Button("✏ Edit"))
    {
        m_EditorLayer->GetAnimationEditorPanel().OpenClip(anim.CurrentClipName);
        m_EditorLayer->GetAnimationEditorPanel().SetVisible(true);
    }
    
    // **INTEGRATION POINT 3**: Edit-mode preview
    if (ImGui::Button("Next ►"))
    {
        anim.FrameIndex++;
        m_EditorLayer->GetAnimationSystem()->UpdateInEditMode(
            static_cast<entt::entity>(entity), 0.0f);
    }
}
```

**Why Critical:**
- Inspector is the **main user interface** for animation control
- Must access multiple systems through EditorLayer
- Edit-mode preview depends on direct `UpdateInEditMode()` call

---

### **4. EditorLayer System Initialization**

**Location:** `EditorLayer::OnAttach()`

**Critical Code:**
```cpp
void EditorLayer::OnAttach()
{
    // **STEP 1**: Create AnimationSystem
    m_AnimationSystem = std::make_unique<AnimationSystem>();
    
    // **STEP 2**: Initialize AnimationLibraryManager with system
    m_AnimationLibraryManager.Initialize(m_AnimationSystem.get());
    
    // **STEP 3**: Initialize AnimationEditorPanel
    m_AnimationEditorPanel->Initialize(m_AnimationSystem.get(), &m_AnimationLibraryManager);
    
    // **STEP 4**: Create scene and attach systems
    m_ActiveScene = std::make_shared<Scene>("EditorScene");
    m_AnimationSystem->OnAttach(m_ActiveScene.get());
    m_ActiveScene->SetAnimationSystem(m_AnimationSystem.get());
}
```

**Why Critical:**
- **Order matters**: AnimationSystem must exist before AnimationLibraryManager
- Scene must have AnimationSystem attached for queries to work
- All panels need references to systems for functionality

---

## Pain Points & Bottlenecks

### **1. Too Many Manual Steps**

**Problem:** Getting an animation into the viewport requires **12+ user actions**:
1. Create sprite sheet (external)
2. Import texture to project
3. Open Sprite Sheet Editor
4. Load texture
5. Configure grid
6. Click frames in order
7. Click "Create Animation from Frames"
8. Name animation
9. Edit frame durations (optional)
10. Save animation
11. Create entity
12. Add SpriteComponent
13. Add AnimationComponent
14. Select clip from dropdown
15. Enter play mode

**Impact:** High friction, slow iteration, error-prone

**Solution:** See Recommendations section

---

### **2. UV Coordinate Confusion**

**Problem:** UV coordinates are **hidden** from users but critical to animation
- Users don't understand why animation isn't showing
- No visual feedback of UV coords in Inspector
- `LockUV` flag exists but no UI for it
- Manual UV editing is code-only

**Example Error:**
```
User creates animation, assigns it, enters play mode.
Animation doesn't show - sprite stays on first frame.
Why? UVMin/Max are (0, 0) to (1, 1) - showing full texture, not frame.
User has no way to debug this in editor.
```

**Solution:** Show UV coordinates in Inspector, add visual rect indicator

---

### **3. Clip Discovery Delay**

**Problem:** AnimationLibraryManager only scans on editor startup
- If user creates new animation during session, it's not auto-discovered
- Must restart editor or manually reload
- No "Refresh" button in Inspector

**Impact:** Confusing workflow, breaks user flow

**Solution:** Add file watcher for hot-reload, or "Refresh Clips" button

---

### **4. Edit-Mode Preview Quirks**

**Problem:** Edit-mode preview works inconsistently:
- Only works if entity has both AnimationComponent AND SpriteComponent
- Doesn't show if clip name is typo'd (silent failure)
- Frame scrubber doesn't auto-update when clip changes
- No visual indication of which frame is showing

**Impact:** Users don't trust edit-mode preview, enter play mode instead

**Solution:** Better error messages, visual frame indicator in viewport

---

### **5. System Coupling**

**Problem:** Inspector depends on EditorLayer, which depends on all systems
- Tight coupling makes refactoring difficult
- Mock/test components can't be tested in isolation
- Changes to one system require recompiling many files

**Architecture Issue:**
```cpp
// InspectorPanel needs EditorLayer to access everything
m_EditorLayer->GetAnimationLibraryManager()
m_EditorLayer->GetAnimationEditorPanel()
m_EditorLayer->GetAnimationSystem()
```

**Better Design:** Dependency injection, event bus, or service locator

---

### **6. No Animation Hot-Reload**

**Problem:** Editing `.anim.json` files manually doesn't hot-reload
- AnimationEditor saves to file, then reloads into AnimationSystem
- But external edits (text editor) don't trigger reload
- User must restart editor

**Impact:** Slow iteration for advanced users who prefer JSON editing

**Solution:** File watcher + hot-reload in AnimationLibraryManager

---

## Troubleshooting Guide

### **Symptom: Animation not showing (sprite stays static)**

**Possible Causes:**

1. **Clip not loaded**
   - Check: `animLibManager.GetAllClipNames()` - is clip in list?
   - Fix: Ensure `.anim.json` file in `assets/animations/` folder
   - Fix: Restart editor to re-scan clips

2. **Clip name mismatch**
   - Check: `anim.CurrentClipName` matches clip file name exactly
   - Fix: Use dropdown instead of typing manually

3. **No SpriteComponent on entity**
   - Check: Entity has both AnimationComponent AND SpriteComponent
   - Fix: Add SpriteComponent to entity

4. **AnimationSystem not attached to scene**
   - Check: `m_ActiveScene->GetAnimationSystem()` returns non-null
   - Fix: Call `scene->SetAnimationSystem(animSystem)`

5. **Animation paused**
   - Check: `anim.Playing` is `true`
   - Fix: Click "▶ Play" in Inspector

6. **Not in play mode**
   - Check: EditorLayer state is `Play`, not `Edit`
   - Fix: Press "▶ Play" in toolbar

7. **UV coordinates wrong**
   - Check: `sprite.TexCoordMin` and `sprite.TexCoordMax` values
   - Fix: Ensure AnimationSystem is calling `UpdateSpriteFromFrame()`

---

### **Symptom: Edit-mode preview not working**

**Possible Causes:**

1. **No clip assigned**
   - Check: `anim.CurrentClipName` is not empty
   - Fix: Select clip from dropdown

2. **Clip not found**
   - Check: Clip exists in AnimationLibraryManager
   - Fix: Restart editor to reload clips

3. **Frame index out of bounds**
   - Check: `anim.FrameIndex` < `clip->GetFrameCount()`
   - Fix: Reset frame index to 0

4. **UpdateInEditMode not called**
   - Check: Inspector button click code
   - Fix: Ensure `GetAnimationSystem()->UpdateInEditMode()` is called

---

### **Symptom: Dropdown shows no clips**

**Possible Causes:**

1. **No .anim.json files in assets/animations/**
   - Fix: Create animation using Animation Editor

2. **AnimationLibraryManager not initialized**
   - Check: `animLibManager.GetAllClipNames().size() == 0`
   - Fix: Ensure `Initialize()` called in `EditorLayer::OnAttach()`

3. **Clips failed to load**
   - Check console for errors: `"Failed to load animation clip"`
   - Fix: Validate JSON format

---

## Recommendations for Improvement

### **Priority 1: Streamline Workflow**

**Goal:** Reduce 15-step workflow to 5 steps

**Proposed Simplified Workflow:**
1. Import sprite sheet to Content Browser
2. Drag sprite sheet onto entity (auto-creates SpriteComponent + AnimationComponent)
3. Inspector shows all frames in sheet as grid
4. User selects frames for animation (directly in Inspector)
5. Click "Create Animation" - done, auto-playing in viewport

**Implementation:**
- Add "Create Animation from Selection" in Inspector
- Inline frame picker (no separate Sprite Sheet Editor)
- Auto-detect frame count from texture dimensions

---

### **Priority 2: Visual UV Debugging**

**Goal:** Make UV coordinates visible and editable

**Proposed Features:**
- **UV Rect Overlay in Viewport:** Show red rectangle on sprite indicating UV bounds
- **UV Coordinate Display in Inspector:** Show TexCoordMin/Max with copy button
- **UV Lock Toggle:** Visual button to lock/unlock UV editing
- **Frame Preview Thumbnail:** Show current frame in Inspector (64x64 image)

**UI Mockup:**
```
Inspector - SpriteComponent
┌─────────────────────────────┐
│ Texture: player_walk.png    │
│ ┌─────────┐                 │
│ │ [Frame] │ ← 64x64 preview │
│ └─────────┘                 │
│ UV Rect: (0.0, 0.0) - (0.25,│
│          1.0)                │
│ [🔓 UV Unlocked] ← Toggle   │
└─────────────────────────────┘
```

---

### **Priority 3: Clip Hot-Reload**

**Goal:** Auto-reload animations when files change

**Implementation:**
```cpp
// AnimationLibraryManager.cpp
void AnimationLibraryManager::StartFileWatcher()
{
    m_FileWatcher = std::make_unique<FileWatcher>("assets/animations/");
    
    m_FileWatcher->OnFileChanged([this](const std::string& filepath) {
        if (filepath.ends_with(".anim.json"))
        {
            PIL_INFO("Reloading animation clip: {}", filepath);
            ReloadClip(filepath);
        }
    });
}
```

**User Benefit:** Edit `.anim.json` in external editor, see changes instantly

---

### **Priority 4: Edit-Mode Animation Playback**

**Goal:** Play animations in edit mode, not just scrub

**Proposed Feature:**
- Add "▶ Play in Edit Mode" button in Inspector
- Animates in viewport without entering play mode
- Stops automatically when clip finishes (if not looping)

**Implementation:**
```cpp
// EditorLayer.cpp
void EditorLayer::OnUpdate(float deltaTime)
{
    // Always update animation system, even in edit mode
    if (m_EditModeAnimationEnabled)
    {
        m_AnimationSystem->OnUpdate(deltaTime);
    }
}
```

---

### **Priority 5: Unified Animation Panel**

**Goal:** Combine Sprite Sheet Editor + Animation Editor into one panel

**Proposed UI:**
```
┌─────────────────────────────────────────────────────────────┐
│ Animation Authoring Panel                                   │
├─────────────────────────────────────────────────────────────┤
│ [Step 1: Load Texture] [Step 2: Select Frames] [Step 3...] │
│                                                              │
│ Sprite Sheet Preview:          Selected Frames:             │
│ ┌──────────────────┐          ┌────┬────┬────┬────┐        │
│ │ [Grid of frames] │          │ 0  │ 1  │ 2  │ 3  │        │
│ │ Click to select  │   ───▶   └────┴────┴────┴────┘        │
│ └──────────────────┘                                        │
│                                                              │
│ Timeline:                                                    │
│ ┌─────┬─────┬─────┬─────┐                                  │
│ │ 0   │ 1   │ 2   │ 3   │  ← Drag to adjust duration       │
│ │0.1s │0.1s │0.1s │0.1s │                                  │
│ └─────┴─────┴─────┴─────┘                                  │
│                                                              │
│ [▶ Preview] [💾 Save Animation]                            │
└─────────────────────────────────────────────────────────────┘
```

**Benefits:**
- Single workflow, no panel switching
- Visual at every step
- Faster iteration

---

## Conclusion

The Pillar Engine animation system is **architecturally sound** with clear separation of concerns between ECS components, systems, and rendering. However, the **user-facing workflow** is complex and involves many manual steps, hidden state, and potential failure points.

**Key Insights:**
1. **12+ steps** from asset to animated sprite is too many
2. **UV coordinates** are the critical handoff, but hidden from users
3. **System integration** is functional but tightly coupled
4. **Edit-mode preview** is a great feature but underutilized
5. **Clip discovery** needs hot-reload support

**Recommended Next Steps:**
1. Implement **Priority 1** (Streamline Workflow) to reduce friction
2. Add **Priority 2** (Visual UV Debugging) for better observability
3. Polish **Priority 4** (Edit-Mode Playback) for faster iteration
4. Consider **Priority 5** (Unified Panel) for Phase 4-5 of animation system

**Success Metric:** Reduce time from "import sprite sheet" to "see animation in viewport" from **10 minutes** to **2 minutes**.

---

**Document Status:** ✅ Complete & Ready for Review  
**Last Updated:** January 6, 2026  
**Author:** AI Analysis based on codebase inspection  
**Next Action:** Review with team, prioritize improvements
