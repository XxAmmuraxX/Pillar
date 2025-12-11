# Animation System - Comprehensive User-Friendly Design

**Date:** December 11, 2025  
**Goal:** Create an intuitive, no-code animation workflow for sprite sheet animations

---

## Current System Analysis

### Existing Architecture
- **AnimationClip**: Collection of frames with timing, loop settings, events
- **AnimationFrame**: TexturePath + Duration + UV coordinates (UVMin, UVMax)
- **AnimationComponent**: References clips by name, tracks playback state
- **AnimationSystem**: Manages clip library, updates SpriteComponent each frame

### Current Pain Points
1. **Requires Programming**: Users must write code to create animation clips
2. **Manual UV Calculation**: Must calculate UV coordinates manually (e.g., frame 3 of 6 = 0.5 to 0.6667)
3. **No Visual Feedback**: Can't preview animations before adding to game
4. **JSON Format Complex**: Hand-writing .anim.json files is error-prone
5. **No Sprite Sheet Tools**: No grid slicer, frame picker, or atlas support

---

## Design Goals

### User Experience Principles
1. **Visual First**: See sprite sheets, pick frames visually
2. **Automatic Calculations**: System calculates UV coordinates
3. **Immediate Feedback**: Preview animations instantly
4. **Flexible Workflows**: Support multiple creation methods
5. **No Code Required**: Entire workflow in editor UI

### Technical Requirements
1. **Maintain Compatibility**: Don't break existing AnimationClip/Frame/Component structure
2. **Performance**: No performance degradation in runtime
3. **Serialization**: Save/load complete animation projects
4. **Extensibility**: Easy to add texture packer support later

---

## Proposed Solution: Three-Tier Animation Creation

### Tier 1: Visual Sprite Sheet Slicer (PRIORITY)
**For users with simple uniform grids**

#### Workflow:
1. **Import Sprite Sheet**
   - Drag & drop PNG/JPG into Animation Manager
   - Or use file browser dialog
   - Display texture in preview window

2. **Define Grid**
   - **Grid Mode**: Uniform grid slicing
     - Input: Rows, Columns
     - Auto-calculate: Frame width/height
     - Visual overlay: Show grid lines on texture
   - **Frame Size Mode**: Specify frame dimensions
     - Input: Frame Width, Frame Height (pixels)
     - Auto-calculate: Rows, Columns
     - Handle non-perfect divisions (warn user)

3. **Select Frames**
   - Visual grid with numbered frames (0-based or 1-based toggle)
   - Click to select individual frames
   - Shift+Click for range selection
   - Ctrl+Click for multi-selection
   - "Select All" / "Select Row" / "Select Column" buttons
   - Highlight selected frames in different color

4. **Configure Animation**
   - **Name**: Input field (e.g., "player_walk")
   - **Frame Duration**: Global or per-frame
     - "Use Same Duration for All": Checkbox
     - Duration input (seconds): 0.1f default
     - Or per-frame table with duration override
   - **Playback Speed**: Multiplier (0.1x - 5.0x)
   - **Loop**: Checkbox (default: true)
   - **Frame Order**: 
     - Auto (left-to-right, top-to-bottom)
     - Custom (drag & drop frames to reorder)
     - Ping-pong (forward then reverse)

5. **Preview**
   - Play button with speed control
   - Scrub through frames with timeline
   - Show current frame index/time
   - Display FPS (calculated)
   - Loop/Ping-pong preview

6. **Add Events** (Optional)
   - Timeline view below preview
   - Click frame to add event marker
   - Event properties:
     - Frame Index (auto-filled)
     - Event Name (e.g., "footstep", "attack_hit")
     - Description (optional)
   - Color-coded event types

7. **Create Clip**
   - "Create Animation Clip" button
   - Validates settings (name unique, frames selected)
   - Generates AnimationClip with calculated UV coordinates
   - Adds to AnimationSystem library
   - Shows success notification

#### UI Layout:
```
+--------------------------------------------------+
|  Animation Manager - Sprite Sheet Slicer         |
+--------------------------------------------------+
| [Load Sprite Sheet]  [Clear]                     |
+--------------------------------------------------+
|  Texture Preview          |  Settings            |
|  +---------------------+  |  Name: [___________] |
|  |                     |  |  Grid: [4] x [6]     |
|  |  [Sprite Sheet]     |  |  Frame: 32x32 px     |
|  |  with grid overlay  |  |  Duration: [0.1] sec |
|  |                     |  |  Loop: [x]           |
|  +---------------------+  |  Speed: [1.0]x       |
|                           |                      |
|  Frame Selection:         |  [Preview Animation] |
|  [0][1][2][3][4][5]       |  +----------------+  |
|  [6][7][8][9][10][11]     |  |   Preview      |  |
|  ...                      |  |   Window       |  |
|                           |  +----------------+  |
|  Selected: 0-5 (6 frames) |  Frame: 3/6  0.3s   |
|                           |                      |
|  Events Timeline:         |  [Create Clip]       |
|  [====|====|====]         |                      |
+--------------------------------------------------+
```

### Tier 2: Manual Frame Definition (INTERMEDIATE)
**For users with irregular sprite sheets or texture atlases**

#### Workflow:
1. **Load Texture**: Same as Tier 1
2. **Add Frames Manually**:
   - "Add Frame" button
   - Each frame shows:
     - Thumbnail preview (zoomed region)
     - UV Min (x, y) inputs (0-1 range OR pixel coordinates)
     - UV Max (x, y) inputs (0-1 range OR pixel coordinates)
     - Duration input
     - Delete button
   - Drag frames to reorder
3. **Visual Frame Picker** (advanced):
   - Click and drag rectangle on texture
   - Shows pixel coordinates and UV coordinates
   - "Add as Frame" button
4. **Rest of workflow**: Same as Tier 1 (preview, events, create)

#### UI Layout:
```
+--------------------------------------------------+
|  Manual Frame Editor                              |
+--------------------------------------------------+
|  Texture: character_atlas.png  [Change Texture]  |
+--------------------------------------------------+
|  Frame 1:  [Thumbnail]  UV: (0.0, 0.0) to (0.25, 0.5)  Duration: 0.1s  [Delete] |
|  Frame 2:  [Thumbnail]  UV: (0.25, 0.0) to (0.5, 0.5)  Duration: 0.1s  [Delete] |
|  [+ Add Frame]                                    |
+--------------------------------------------------+
|  Visual Picker:                                   |
|  +----------------------+                         |
|  |  [Texture Preview]   |  Selection: 32x32 px   |
|  |  Click & drag box    |  UV: (0.5, 0.5) to     |
|  |                      |       (0.625, 0.625)   |
|  +----------------------+  [Add as Frame]         |
+--------------------------------------------------+
```

### Tier 3: JSON Import/Export (ADVANCED)
**For users with external tools or batch processing**

#### Features:
- Import `.anim.json` files (existing format)
- Export created clips to JSON for version control
- Batch import multiple clips from folder
- JSON schema validation with error messages

#### JSON Format Example:
```json
{
  "name": "player_walk",
  "loop": true,
  "playbackSpeed": 1.0,
  "frames": [
    {
      "texturePath": "character_walk.png",
      "duration": 0.1,
      "uvMin": [0.0, 0.0],
      "uvMax": [0.1667, 1.0]
    },
    {
      "texturePath": "character_walk.png",
      "duration": 0.1,
      "uvMin": [0.1667, 0.0],
      "uvMax": [0.3334, 1.0]
    }
  ],
  "events": [
    {
      "frameIndex": 2,
      "eventName": "footstep_left"
    },
    {
      "frameIndex": 5,
      "eventName": "footstep_right"
    }
  ]
}
```

---

## Animation Manager Panel - Complete Redesign

### Panel Modes
1. **Library View** (default)
   - Grid of animation clip thumbnails
   - Search/filter bar
   - Create New / Import buttons
   - Clip properties (name, frames, duration, loop)
   - Preview on hover
   - Right-click context menu (rename, duplicate, delete, export)

2. **Sprite Sheet Slicer** (Tier 1 editor)
   - Opens when "Create from Sprite Sheet" clicked
   - Full workflow as described above

3. **Manual Editor** (Tier 2 editor)
   - Opens when "Create from Frames" clicked
   - Frame-by-frame definition

4. **Settings**
   - Default frame duration
   - Default playback speed
   - Frame index numbering (0-based or 1-based)
   - Grid overlay color
   - Event marker colors

### Toolbar Actions
```
[Create ▼]  [Import]  [Export Selected]  [Delete Selected]  [Settings ⚙]  [?]
  |
  +-- From Sprite Sheet (Tier 1)
  +-- From Frames (Tier 2)
  +-- From JSON (Tier 3)
  +-- Blank Clip
```

---

## Inspector Panel - AnimationComponent Enhancements

### Current State Issues
- Text input for clip name (easy to typo)
- No visual feedback of available clips
- No preview in inspector
- No quick access to Animation Manager

### Proposed Improvements

#### 1. Clip Selection
- **Dropdown instead of text input**
  - Populated from AnimationSystem clip library
  - Grouped by prefix (e.g., "player_*", "enemy_*")
  - Searchable/filterable
  - Shows clip info on hover (frame count, duration, loop)
  - "Open in Animation Manager" button

#### 2. Inline Preview
- Small preview window (128x128 or 256x256)
- Play/Pause/Stop controls
- Scrubber timeline
- Current frame indicator

#### 3. Quick Actions
- "Edit Clip" button - opens Animation Manager with clip selected
- "Duplicate & Edit" - creates copy for variations
- "Reset to Defaults" - resets playback state

#### 4. Advanced Properties (Collapsible)
- Frame callbacks (per-frame actions)
- Blend settings (for future animation blending)
- Override playback speed (entity-specific)

#### UI Mockup:
```
+--------------------------------------------------+
| AnimationComponent                         [-][x]|
+--------------------------------------------------+
| Current Clip: [player_walk ▼] [Edit in Manager] |
|                                                   |
| Preview:        Frame: 3/6    Time: 0.3s         |
| +-------------+  ●━━━━━━○━━━━━━  [▶] [⏸] [⏹]    |
| |   [Frame]   |  Looping: Yes                    |
| |   Preview   |  Duration: 0.6s                  |
| +-------------+  Speed: 1.0x                     |
|                                                   |
| Playback Controls:                                |
|   Playing: [x]     Speed: [====|====] 1.0x       |
|   Frame:   3       Time:  0.31s                  |
|                                                   |
| ▼ Advanced                                        |
|   Frame Events: ...                               |
+--------------------------------------------------+
```

---

## Implementation Phases

### Phase 1: Foundation (Week 1)
**Goal: Basic visual sprite sheet slicer**

- [ ] Refactor AnimationManagerPanel into multi-mode panel
- [ ] Add texture preview widget (display loaded texture)
- [ ] Implement grid overlay rendering
- [ ] Add grid size inputs (rows, columns)
- [ ] Implement frame selection UI (clickable grid cells)
- [ ] Calculate UV coordinates from grid position
- [ ] Create AnimationClip from selected frames
- [ ] Basic preview playback (no scrubbing yet)

**Deliverables:**
- Can create simple looping animation from uniform grid sprite sheet
- Clip added to library and usable in AnimationComponent

### Phase 2: Enhanced Editing (Week 2)
**Goal: Polished creation workflow**

- [ ] Add per-frame duration editor
- [ ] Implement frame reordering (drag & drop)
- [ ] Add timeline scrubber for preview
- [ ] Implement animation events editor
- [ ] Add "Frame Size Mode" (specify pixel dimensions)
- [ ] Add playback speed control in preview
- [ ] Add ping-pong playback option
- [ ] Improve visual feedback (selected frames highlight)

**Deliverables:**
- Full-featured sprite sheet slicer with events
- Multiple playback modes (loop, once, ping-pong)

### Phase 3: Inspector Integration (Week 3)
**Goal: Better AnimationComponent workflow**

- [ ] Replace clip name text input with dropdown
- [ ] Populate dropdown from AnimationSystem library
- [ ] Add inline preview in inspector
- [ ] Implement "Edit Clip" button (opens Animation Manager)
- [ ] Add clip info tooltips (frame count, duration)
- [ ] Add search/filter in dropdown
- [ ] Implement clip grouping (by prefix)

**Deliverables:**
- Inspector shows available clips clearly
- Easy navigation between inspector and manager
- Preview animations without entering play mode

### Phase 4: Manual Editor (Week 4)
**Goal: Support irregular sprite sheets**

- [ ] Create Manual Frame Editor mode
- [ ] Add frame list with thumbnail previews
- [ ] Implement UV coordinate inputs (0-1 range and pixel mode)
- [ ] Add visual frame picker (click & drag on texture)
- [ ] Auto-calculate UV from pixel coordinates
- [ ] Support multiple textures per clip (texture atlas)
- [ ] Add frame validation (UV in bounds, texture exists)

**Deliverables:**
- Can create animations from texture atlases
- Visual frame selection tool
- Mixed-texture animations supported

### Phase 5: Polish & Advanced Features (Week 5)
**Goal: Production-ready tooling**

- [ ] Add clip library search/filter
- [ ] Implement clip renaming/duplication
- [ ] Add JSON import/export (Tier 3)
- [ ] Batch import from folder
- [ ] Add settings panel (defaults, preferences)
- [ ] Implement clip thumbnails (first frame preview)
- [ ] Add keyboard shortcuts (common operations)
- [ ] Write comprehensive documentation
- [ ] Create tutorial project with sample animations

**Deliverables:**
- Complete animation authoring suite
- Production-quality UX
- User documentation

### Phase 6: Future Enhancements (Post-MVP)
**Optional advanced features**

- [ ] Texture Packer atlas support (.tpsheet, .json)
- [ ] Aseprite .ase/.aseprite file import
- [ ] Frame-by-frame sprite editor (draw directly)
- [ ] Animation blending/transitions editor
- [ ] Animation state machine visual editor
- [ ] Sprite sheet auto-detection (find grid automatically)
- [ ] Animation compression (shared frame deduplication)
- [ ] Multi-layer animations (compositing)

---

## Technical Architecture Changes

### New Classes to Add

#### 1. `SpriteSheetSlicer` (Editor Tool)
```cpp
class SpriteSheetSlicer
{
public:
    void LoadTexture(const std::string& path);
    void SetGridSize(int rows, int columns);
    void SetFrameSize(int width, int height); // Alternative to grid
    void SelectFrame(int index, bool addToSelection = false);
    void SelectRange(int start, int end);
    std::vector<AnimationFrame> GenerateFrames(float duration);
    glm::vec4 CalculateUVForFrame(int row, int col);
    void DrawUI(); // ImGui rendering
    
private:
    Texture2D m_Texture;
    int m_Rows = 1, m_Columns = 1;
    std::vector<bool> m_SelectedFrames;
    int m_TextureWidth = 0, m_TextureHeight = 0;
};
```

#### 2. `AnimationPreview` (Preview Widget)
```cpp
class AnimationPreview
{
public:
    void SetClip(const AnimationClip* clip);
    void Play();
    void Pause();
    void Stop();
    void SetPlaybackSpeed(float speed);
    void Update(float deltaTime);
    void DrawUI(int width, int height); // Render preview
    int GetCurrentFrame() const;
    
private:
    const AnimationClip* m_Clip = nullptr;
    int m_CurrentFrame = 0;
    float m_PlaybackTime = 0.0f;
    bool m_Playing = false;
    float m_PlaybackSpeed = 1.0f;
};
```

#### 3. `AnimationClipEditor` (Modal Editor)
```cpp
class AnimationClipEditor
{
public:
    enum class Mode { SpriteSheet, Manual, JSON };
    
    void Open(Mode mode, AnimationClip* clipToEdit = nullptr);
    void Close();
    bool IsOpen() const;
    void DrawUI(); // Full modal window
    
    // Save edited clip to AnimationSystem
    void SaveClip();
    
private:
    Mode m_Mode;
    SpriteSheetSlicer m_Slicer;
    AnimationPreview m_Preview;
    AnimationClip m_WorkingClip; // Edited clip (not yet saved)
    bool m_IsOpen = false;
};
```

### AnimationSystem Enhancements

#### Add Iteration Support
```cpp
// In AnimationSystem.h
std::vector<std::string> GetAllClipNames() const;
const std::unordered_map<std::string, AnimationClip>& GetClipLibrary() const;

// For inspector dropdown population
void GetClipNamesGrouped(std::map<std::string, std::vector<std::string>>& outGroups);
```

#### Add Clip Management
```cpp
bool RenameClip(const std::string& oldName, const std::string& newName);
AnimationClip* DuplicateClip(const std::string& sourceName, const std::string& newName);
bool ExportClip(const std::string& name, const std::string& filePath);
```

### AnimationManagerPanel Redesign

```cpp
class AnimationManagerPanel
{
public:
    enum class ViewMode { Library, ClipEditor };
    
    void OnImGuiRender() override;
    
private:
    void DrawLibraryView();      // Grid of clips with search
    void DrawClipEditor();       // Sprite sheet slicer or manual editor
    void DrawToolbar();          // Create/Import/Export buttons
    void DrawClipContextMenu();  // Right-click menu
    
    ViewMode m_ViewMode = ViewMode::Library;
    AnimationClipEditor m_Editor;
    std::string m_SearchFilter;
    AnimationClip* m_SelectedClip = nullptr;
};
```

---

## User Workflow Examples

### Example 1: Simple Walk Cycle
1. Open Animation Manager
2. Click "Create → From Sprite Sheet"
3. Load `character_walk.png` (6 frames in horizontal strip)
4. Set Grid: 1 row, 6 columns
5. Click "Select All"
6. Set Duration: 0.1 seconds
7. Name: "player_walk"
8. Enable Loop
9. Preview (looks good!)
10. Click "Create Clip"
11. Done! ✅

**Time estimate: 30 seconds**

### Example 2: Attack with Events
1. Open Animation Manager
2. Click "Create → From Sprite Sheet"
3. Load `character_attack.png` (4 frames)
4. Set Grid: 1 row, 4 columns
5. Select frames 0-3
6. Set durations: [0.05, 0.1, 0.15, 0.1] (charge, swing, impact, recover)
7. Name: "player_attack"
8. Disable Loop (should play once)
9. Add Event: Frame 2, Name "deal_damage"
10. Add Event: Frame 2, Name "play_swoosh_sound"
11. Preview (perfect timing!)
12. Click "Create Clip"
13. In Inspector: Select entity, AnimationComponent, choose "player_attack"
14. Test in play mode ✅

**Time estimate: 2 minutes**

### Example 3: Complex Texture Atlas
1. Open Animation Manager
2. Click "Create → From Frames"
3. Load `character_atlas.png` (multiple animations in one texture)
4. Click "Visual Picker"
5. Click and drag on texture to select first frame region
6. Click "Add as Frame" (UV calculated automatically)
7. Repeat for each frame (8 frames total)
8. Reorder frames by dragging
9. Set durations per frame
10. Name: "player_roll"
11. Preview (looks smooth!)
12. Click "Create Clip" ✅

**Time estimate: 3-4 minutes**

---

## Testing Strategy

### Unit Tests
- UV coordinate calculation (various grid sizes)
- Frame selection logic (individual, range, multi-select)
- AnimationClip generation from frames
- JSON import/export (round-trip)
- Clip renaming/duplication

### Integration Tests
- Full workflow: Import texture → Create clip → Use in scene
- Preview playback accuracy (frame timing)
- AnimationSystem library updates (add/remove/rename)
- Inspector dropdown population

### User Testing
- First-time user: Can they create an animation without documentation?
- Power user: Can they create complex animation quickly?
- Edge cases: Non-uniform grids, invalid UV, missing textures
- Performance: 100+ clips in library, large sprite sheets (4096x4096)

---

## Performance Considerations

### Texture Loading
- **Problem**: Loading large sprite sheets multiple times
- **Solution**: Texture caching in editor (reuse texture pointers)
- **Optimization**: Lazy load textures (only when panel visible)

### Preview Rendering
- **Problem**: Multiple previews animating simultaneously
- **Solution**: Throttle updates (30 FPS preview is sufficient)
- **Optimization**: Pause off-screen previews

### Large Sprite Sheets
- **Problem**: 4096x4096 textures cause UI lag
- **Solution**: Generate mipmap/thumbnail for preview (512x512 max)
- **Optimization**: Background thread texture loading

### Clip Library Size
- **Problem**: 1000+ clips slow down dropdown
- **Solution**: Virtual scrolling (only render visible items)
- **Optimization**: Cache search results

---

## Migration Path (For Existing Code)

### Backwards Compatibility
- **Existing AnimationClips**: Continue to work unchanged
- **JSON Format**: Parser supports old and new formats
- **AnimationComponent**: No changes needed
- **AnimationSystem**: Extended API, old methods still work

### Migration Tool
- Optional tool to import old JSON clips to new editor format
- Batch conversion script for projects with many clips

---

## Documentation Plan

### User Documentation
1. **Quick Start Tutorial**: Create first animation in 5 minutes
2. **Sprite Sheet Slicer Guide**: All features explained
3. **Manual Editor Guide**: Texture atlas workflow
4. **Events & Callbacks**: How to use animation events in gameplay
5. **Best Practices**: Performance, organization, naming conventions

### Developer Documentation
1. **API Reference**: AnimationSystem, Clip, Frame, Component
2. **Editor Extension**: How to customize animation tools
3. **JSON Format Specification**: Schema documentation
4. **Animation System Internals**: How UV updates work

### Video Tutorials
1. Creating a walk cycle (2 min)
2. Attack animation with damage event (5 min)
3. Complex multi-animation character (10 min)

---

## Success Metrics

### Quantitative
- **Creation Time**: < 1 minute for simple 6-frame animation
- **Error Rate**: < 5% of created clips have issues (wrong UV, missing frames)
- **Adoption**: 90%+ of users prefer visual editor over JSON
- **Performance**: No lag with 500+ clips in library

### Qualitative
- Users report: "Easy to use", "Intuitive", "Saves time"
- No common confusion points in user testing
- Reduces support questions about animation setup

---

## Risks & Mitigations

### Risk 1: Scope Creep
- **Issue**: Feature requests expand indefinitely
- **Mitigation**: Stick to phased plan, defer "nice-to-haves" to Phase 6

### Risk 2: Performance Degradation
- **Issue**: Large sprite sheets slow down editor
- **Mitigation**: Profile early, implement texture caching/mipmaps

### Risk 3: UX Complexity
- **Issue**: Too many options overwhelm users
- **Mitigation**: Simple mode by default, advanced features hidden in collapsible sections

### Risk 4: Breaking Changes
- **Issue**: New system incompatible with existing clips
- **Mitigation**: Maintain backwards compatibility, add migration tool

---

## Open Questions

1. **Frame Numbering**: 0-based (programmer friendly) or 1-based (artist friendly)?
   - **Proposal**: User preference in settings, default 0-based

2. **Multiple Textures per Clip**: Should one clip reference multiple sprite sheets?
   - **Proposal**: Yes, but warn if mixing textures (performance impact)

3. **Animation Compression**: Should we deduplicate identical frames?
   - **Proposal**: Defer to Phase 6, not critical for MVP

4. **Undo/Redo in Editor**: Do we need history in clip editor?
   - **Proposal**: Yes, integrate with existing undo/redo system

5. **Real-time Collaboration**: Should clips auto-reload if file changes?
   - **Proposal**: Phase 6 feature, add file watcher

---

## Conclusion

This plan provides a **complete, user-friendly animation workflow** that eliminates the need for programming. The **three-tier approach** (visual slicer, manual editor, JSON) caters to all skill levels and use cases.

**Key Innovations:**
- ✅ Visual sprite sheet slicer (no UV math required)
- ✅ Inline animation preview (immediate feedback)
- ✅ Dropdown clip selection (no typos)
- ✅ Event timeline editor (visual event placement)
- ✅ Flexible frame selection (uniform grids or texture atlases)

**Implementation is phased** over 5 weeks, delivering value incrementally:
- **Week 1**: Basic slicer (80% of use cases)
- **Week 2**: Polished editing (events, reordering)
- **Week 3**: Inspector integration (better workflow)
- **Week 4**: Manual editor (texture atlases)
- **Week 5**: Production polish (export, batch tools)

This plan **balances power with simplicity**, providing a professional-grade animation tool while remaining **accessible to beginners**.

---

**Next Steps:**
1. Review and approve this plan
2. Create detailed task breakdown for Phase 1
3. Begin implementation with sprite sheet slicer foundation
4. User test early prototype (Week 1) for feedback

