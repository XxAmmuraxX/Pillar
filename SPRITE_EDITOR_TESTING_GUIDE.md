# Sprite Sheet Editor Testing Guide

**Created:** January 5, 2026  
**Status:** Phase 2 Complete (2.1-2.5)  
**Next Phase:** Phase 3 - Layer Management

---

## ✅ Implemented Features

### Phase 2.1: Core Sprite Sheet Editor UI
- Visual grid overlay on textures
- Mouse-based cell selection
- Apply UV coordinates to selected sprites

### Phase 2.2: Grid Configuration
- Manual grid setup (columns, rows, cell size)
- Grid presets (8x8, 16x16, 32x32, 64x64, 128x128)
- Padding/spacing support
- Draggable grid handles (visual adjustment)
- Auto-save/load metadata (.spritesheet.json)

### Phase 2.3: Frame Library
- 64x64 thumbnail display
- Add/remove frames
- Hover preview (256x256)
- Export to animation clip (.anim.json)

### Phase 2.4: TexturePacker Import
- Parse TexturePacker JSON (hash and array formats)
- Handle trimmed sprites
- Handle rotated sprites (90° clockwise)
- Auto-load frames into library

### Phase 2.5: Aseprite Import ✨ NEW
- Parse Aseprite JSON format
- Extract frame durations (milliseconds)
- Parse animation tags (named sequences)
- Support forward/reverse/ping-pong animations
- Auto-create animation clips (.anim.json) per tag

---

## 🧪 How to Test

### Prerequisites

**Build the editor:**
```powershell
# From Developer PowerShell for VS 2022
cmake -S . -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
```

**Launch the editor:**
```powershell
.\bin\Debug-x64\PillarEditor\PillarEditor.exe
```

---

## Test Scenario 1: Basic Texture Loading

### Method 1: File Dialog
1. Open **Sprite Sheet Editor** panel (View menu)
2. Click **"Load Texture..."** button
3. Select an image file (e.g., `Sandbox/assets/textures/character_walk_cycle.png`)
4. ✅ Texture should appear in editor

### Method 2: Drag-and-Drop
1. Open **Content Browser** panel
2. Navigate to `assets/textures/`
3. **Drag** an image file from Content Browser
4. **Drop** it onto the Sprite Sheet Editor (on the "Load a texture" text area)
5. ✅ Texture should load automatically

---

## Test Scenario 2: Grid Configuration

### Manual Grid Setup
1. Load a sprite sheet texture
2. Adjust **Columns** and **Rows** sliders
3. Adjust **Cell Width** and **Cell Height**
4. Add **Padding** (border around entire sheet)
5. Add **Spacing** (gap between cells)
6. ✅ Grid overlay should update in real-time

### Grid Presets
1. Click preset buttons: **8x8**, **16x16**, **32x32**, **64x64**, **128x128**
2. ✅ Grid should snap to preset configuration

### Draggable Handles (Optional Feature)
1. Hover over grid lines (orange circles appear at intersections)
2. Drag handles to resize cells visually
3. ✅ Grid should adjust, maintaining equal cell sizes
4. ✅ Cursor changes to resize arrows when hovering

### Metadata Persistence
1. Configure a grid for a texture
2. Close the editor
3. Relaunch and load the same texture
4. ✅ Grid settings should restore from `.spritesheet.json` file

---

## Test Scenario 3: Frame Selection & Library

### Select Frames
1. Load a sprite sheet with visible grid
2. **Click** on a cell in the grid
3. ✅ Cell should highlight with yellow border
4. ✅ UV coordinates displayed in sidebar

### Build Frame Library
1. Select a cell
2. Click **"Add Frame"** button
3. Repeat for multiple frames
4. ✅ Thumbnails appear in "Frame Library" section
5. ✅ Each frame shows name (e.g., "Frame 0")

### Hover Preview
1. Hover mouse over thumbnail in frame library
2. ✅ Tooltip appears with 256x256 preview and UV coords

### Remove Frames
1. Click **"Remove"** button next to a frame
2. ✅ Frame disappears from library

### Export Animation
1. Add multiple frames to library (in sequence)
2. Click **"Export to Animation"**
3. Enter filename (e.g., `test_animation.anim.json`)
4. ✅ File created in current directory with frame sequence

---

## Test Scenario 4: TexturePacker Import

### Setup
1. Export a sprite sheet from TexturePacker with:
   - **Format:** JSON (Hash) or JSON (Array)
   - **Data file:** `sprite_sheet.json`
   - **Texture file:** `sprite_sheet.png`
2. Place both files in same directory (e.g., `assets/textures/`)

### Import
1. Load the `.png` file in Sprite Sheet Editor
2. Click **"Import TexturePacker"** button
3. ✅ Editor finds `.json` file automatically (same name as texture)
4. ✅ Frames load into Frame Library
5. ✅ Frame names match TexturePacker export
6. ✅ Rotated frames tagged with `[ROTATED]`
7. ✅ Trimmed frames tagged with `[TRIMMED]`

### Verify UV Coordinates
1. Select a frame from library
2. Check UV coordinates in sidebar
3. ✅ UVs should match frame position in atlas
4. ✅ Rotated frames handled correctly (90° CW rotation)

---

## Test Scenario 5: Aseprite Import ✨ NEW

### Setup
1. Create or open a sprite animation in Aseprite
2. Add animation tags:
   - Select frames → Tag menu → New Tag
   - Name: "walk", "idle", "attack", etc.
   - Set animation direction: Forward, Reverse, or Ping-pong
3. Export with **File → Export Sprite Sheet**:
   - **Format:** JSON Data
   - **Output file:** `character.json` + `character.png`
   - **Options:** Enable "Merge Duplicates" (optional)
4. Place both files in same directory

### Import Frames
1. Load the `.png` file in Sprite Sheet Editor
2. Click **"Import Aseprite"** button
3. ✅ Editor finds `.json` file automatically
4. ✅ Frames load into Frame Library
5. ✅ Frame names show duration: `"sprite_0 (100ms)"`
6. ✅ Console logs: "Imported N frames from Aseprite"

### Auto-Create Animation Clips
After import, check console for messages like:
```
[INFO] Creating animation clips from 3 tags...
[SUCCESS] Created animation clip: walk.anim.json
[SUCCESS] Created animation clip: idle.anim.json
[SUCCESS] Created animation clip: attack.anim.json
```

### Verify Animation Files
1. Check current directory for `.anim.json` files (one per tag)
2. Open a file (e.g., `walk.anim.json`) in text editor:
   ```json
   {
       "name": "walk",
       "loop": true,
       "fps": 10.0,
       "frames": [
           {
               "uvMin": [0.0, 0.5],
               "uvMax": [0.25, 0.75],
               "duration": 0.1
           },
           ...
       ]
   }
   ```
3. ✅ Frame sequence matches tag range (FromFrame → ToFrame)
4. ✅ Durations preserved from Aseprite export
5. ✅ Ping-pong animations include reverse sequence

### Test Animation Directions
- **Forward:** Frames 0→1→2→3 (loops back to 0)
- **Reverse:** Frames 3→2→1→0 (loops back to 3)
- **Ping-pong:** Frames 0→1→2→3→2→1 (excludes endpoints to avoid duplicate)

---

## Common Issues & Solutions

### ❌ "No texture loaded. Load a texture first."
**Solution:** Click "Load Texture..." or drag-and-drop from Content Browser first

### ❌ "TexturePacker JSON not found: ..."
**Solution:** Ensure `.json` file has **exact same name** as texture (e.g., `sprite.png` → `sprite.json`)

### ❌ "Aseprite JSON not found: ..."
**Solution:** Ensure `.json` file has **exact same name** as texture

### ❌ "Invalid file type. Drag an image file..."
**Solution:** Only image files (.png, .jpg, .bmp, .tga) can be loaded in Sprite Sheet Editor

### ❌ Grid doesn't appear
**Solution:** 
1. Increase columns/rows sliders
2. Check cell width/height aren't zero
3. Use "Auto-Detect Grid" button

### ❌ Frame library empty after import
**Solution:**
1. Check console for error messages
2. Verify JSON format is correct (TexturePacker Hash/Array or Aseprite format)
3. Ensure texture dimensions match JSON metadata

### ❌ Draggable handles don't work
**Solution:**
1. Hover directly over grid line intersections (orange circles)
2. Cursor should change to resize arrows
3. This is an optional feature - use manual sliders if needed

---

## Sample Assets for Testing

### Included Assets
- **Basic Texture:** `Sandbox/assets/textures/pillar_logo.png`
- **Sprite Sheet:** `Sandbox/assets/textures/character_walk_cycle.png`

### Create Your Own Test Assets

**Simple 2x2 Sprite Sheet:**
1. Create a 64x64 PNG with 4 distinct 32x32 regions
2. Load in editor
3. Set grid: Columns=2, Rows=2, CellWidth=32, CellHeight=32

**TexturePacker Export:**
1. Download TexturePacker Free edition
2. Drag multiple sprite images into it
3. Set format to "JSON (Hash)"
4. Export to `test_sprites.json` + `test_sprites.png`

**Aseprite Export:**
1. Open/create sprite animation in Aseprite
2. Add tags for different animations
3. Export: File → Export Sprite Sheet
4. Choose JSON Data format

---

## Next Steps

### Phase 3: Layer Management (Week 5 - Not Yet Implemented)
- Named layer system (Background, Default, UI, etc.)
- Layer Editor panel with visibility/lock toggles
- Drag-and-drop layer reordering
- Integration with sprite rendering

### Phase 4: Advanced Features (Weeks 6-7 - Not Yet Implemented)
- Material system (custom shaders per sprite)
- Nine-slice scaling for UI elements
- Viewport gizmos (outline, pivot, flip indicators)
- Animation preview in inspector
- QoL features (color presets, batch edit, copy/paste)

---

## Testing Checklist

Use this checklist to verify all features:

**Basic Functionality:**
- [ ] Load texture via file dialog
- [ ] Load texture via drag-and-drop
- [ ] Grid overlay visible
- [ ] Cell selection works (yellow highlight)
- [ ] UV coordinates update on selection

**Grid Configuration:**
- [ ] Manual sliders adjust grid
- [ ] Preset buttons work (8x8, 16x16, etc.)
- [ ] Padding affects entire sheet
- [ ] Spacing affects cell gaps
- [ ] Draggable handles resize cells
- [ ] Metadata saves/loads (.spritesheet.json)

**Frame Library:**
- [ ] Add frame button works
- [ ] Thumbnails display correctly (64x64)
- [ ] Hover preview shows (256x256)
- [ ] Remove frame button works
- [ ] Clear library button works
- [ ] Export animation creates .anim.json

**TexturePacker Import:**
- [ ] Auto-finds .json file
- [ ] Parses hash format
- [ ] Parses array format
- [ ] Loads all frames
- [ ] Preserves frame names
- [ ] Tags rotated/trimmed frames

**Aseprite Import:**
- [ ] Auto-finds .json file
- [ ] Parses frames with durations
- [ ] Parses animation tags
- [ ] Creates .anim.json per tag
- [ ] Handles forward direction
- [ ] Handles reverse direction
- [ ] Handles ping-pong direction
- [ ] Preserves frame timings

---

## Reporting Issues

If you encounter bugs, please report with:
1. **Steps to reproduce** (detailed sequence)
2. **Expected behavior** (what should happen)
3. **Actual behavior** (what actually happened)
4. **Console output** (error messages)
5. **Test asset** (if applicable, share .png/.json files)

---

**Happy Testing!** 🎨✨
