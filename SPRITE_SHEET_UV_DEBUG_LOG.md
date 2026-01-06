# Sprite Sheet UV Application Bug - Debug Log

**Date:** January 6, 2026  
**Issue:** Applying a sprite sheet cell to a sprite renders the full texture instead of just the selected cell  
**Status:** ❌ UNRESOLVED

---

## Problem Description

When using the Sprite Sheet Editor to select a cell from a sprite sheet and applying it to an entity:
- **Expected:** Only the selected cell region is rendered (e.g., 96x96 pixels from a 576x96 texture)
- **Actual:** The entire texture (576x96) is rendered, ignoring the UV coordinates

**Test Case:**
- Texture: 576x96 pixels (6 columns × 1 row)
- Grid: 6 columns × 1 row, 96x96 cell size
- Selected cell: [2, 0] (third cell from left)
- Expected UV: (0.333, 0.0) to (0.500, 1.0) ✓ CORRECT
- Expected size: 96x96 pixels = 0.96 × 0.96 world units
- Result: Full 576x96 texture rendered

---

## Investigation Timeline

### ✅ Investigation 1: UV Coordinate Calculation
**Hypothesis:** UV coordinates are calculated incorrectly  
**Status:** RULED OUT

**Evidence:**
```
Cell calculation: texSize=(576x96) gridConfig=(6cols x 1rows) derivedCell=(96x96)
Selected cell [2, 0]
Pixel coords: (192.0, 0.0) to (288.0, 96.0)
UV calculation: MinY = 1.0 - 96/96 = 0.0
UV calculation: MaxY = 1.0 - 0/96 = 1.0
Final UV coords: (0.333333, 0.0) to (0.500000, 1.0)
```

**Conclusion:** UV coordinates are **CORRECT**. The X coordinates (0.333 to 0.5) represent exactly 1/6th of the texture width, and Y coordinates (0.0 to 1.0) cover the full height (which is correct for a single-row sprite sheet).

**Code Location:** `SpriteSheetEditorPanel.cpp` lines 589-616

---

### ✅ Investigation 2: UV Coordinate Propagation to SpriteComponent
**Hypothesis:** UV coordinates are not being applied to the sprite component  
**Status:** RULED OUT

**Evidence:**
```
Applying UV: Min(0.333333, 0.0) Max(0.500000, 1.0)
✅ VERIFICATION - sprite UVs after setting: Min(0.333333, 0.0) Max(0.500000, 1.0) LockUV=true
```

**Conclusion:** UV coordinates **ARE** being written to `sprite.TexCoordMin` and `sprite.TexCoordMax`, and `LockUV` flag is set to prevent AnimationSystem from overwriting them.

**Code Location:** `SpriteSheetEditorPanel.cpp` lines 197-207

---

### ✅ Investigation 3: AnimationSystem Interference
**Hypothesis:** AnimationSystem is overwriting UVs during gameplay  
**Status:** RULED OUT

**Evidence:**
- `sprite.LockUV = true` is set when applying cell
- AnimationSystem checks `if (!sprite.LockUV)` before updating UVs
- AnimationSystem respects the lock flag

**Code Location:** `AnimationSystem.cpp` lines 195-199

---

### ✅ Investigation 4: Renderer UV Parameter Passing
**Hypothesis:** Renderer is not receiving UV coordinates  
**Status:** RULED OUT (assumed based on code inspection)

**Evidence:**
```cpp
// Renderer2DBackend.cpp:183
void DrawSprite(const TransformComponent& transform, const SpriteComponent& sprite)
{
    // ...
    DrawQuad(position, size, sprite.Color, sprite.Texture,
             sprite.TexCoordMin,  // ✓ Passed
             sprite.TexCoordMax,  // ✓ Passed
             sprite.FlipX, sprite.FlipY);
}
```

**Conclusion:** UV coordinates are correctly passed from SpriteComponent to the renderer.

**Note:** Added logging to verify UVs in renderer, but logs not seen in user output (might not have rendered yet, or logs not visible).

---

### ✅ Investigation 5: Batch Renderer UV Handling
**Hypothesis:** OpenGLBatchRenderer2D is not using UV coordinates correctly  
**Status:** RULED OUT (code inspection)

**Evidence:**
```cpp
// OpenGLBatchRenderer2D.cpp:393-398
float uvMinX = flipX ? texCoordMax.x : texCoordMin.x;
float uvMaxX = flipX ? texCoordMin.x : texCoordMax.x;
float uvMinY = flipY ? texCoordMax.y : texCoordMin.y;
float uvMaxY = flipY ? texCoordMin.y : texCoordMax.y;

glm::vec2 texCoords[4] = {
    { uvMinX, uvMinY },  // Bottom-left
    { uvMaxX, uvMinY },  // Bottom-right
    { uvMaxX, uvMaxY },  // Top-right
    { uvMinX, uvMaxY }   // Top-left
};
```

**Conclusion:** Batch renderer correctly assigns UV coordinates to quad vertices.

---

### ❌ Investigation 6: Sprite Size vs UV Region Mismatch (ATTEMPTED FIX - FAILED)
**Hypothesis:** Sprite size doesn't match the UV region, causing visual mismatch  
**Status:** FIX ATTEMPTED - DID NOT RESOLVE ISSUE

**Original Issue:**
- Sprite size was calculated using `m_CellWidth` and `m_CellHeight` (stored configuration values)
- UV coordinates were calculated using derived cell dimensions (texture size ÷ grid)
- These could be different if grid was manually adjusted

**Fix Applied:**
Changed sprite size calculation to use the **same derived cell dimensions** as UV calculation:
```cpp
// Before:
glm::vec2 newNativeSize((float)m_CellWidth / pixelsPerUnit, 
                        (float)m_CellHeight / pixelsPerUnit);

// After:
float derivedCellPixelWidth = availableWidth / (float)m_GridColumns;
float derivedCellPixelHeight = availableHeight / (float)m_GridRows;
glm::vec2 newNativeSize(derivedCellPixelWidth / pixelsPerUnit, 
                        derivedCellPixelHeight / pixelsPerUnit);
```

**Result:** Issue persists - full texture still rendered

**Code Location:** `SpriteSheetEditorPanel.cpp` lines 207-222

---

## What We Know For Sure

✅ **UV coordinates are calculated correctly** (0.333 to 0.5 in X)  
✅ **UV coordinates are written to SpriteComponent**  
✅ **LockUV flag is set to prevent overwriting**  
✅ **Renderer receives and uses UV parameters in code**  
✅ **Batch renderer sets up UV coordinates on vertices**  
✅ **Sprite size is now calculated consistently with UVs**  

---

## What We DON'T Know

❓ **Are the UV coordinates actually reaching the GPU?**
- Added logging in DrawSprite to track UVs when LockUV=true
- User logs don't show 🎨 or 📚 emoji logs (rendering might not have happened yet, or logs not visible)

❓ **Is the shader receiving and using UV coordinates correctly?**
- Shader code looks correct (fragment shader samples `texture(u_Textures[texIndex], v_TexCoord)`)
- No verification that shader is actually executing with correct UVs

❓ **Is there a texture binding issue?**
- Could the wrong texture or texture slot be bound?
- Multiple textures might be loaded for the same entity

❓ **Is the sprite being rendered at all after applying?**
- User might be looking at the old sprite instance
- Scene might need to be refreshed or re-entered play mode

❓ **Are there multiple SpriteComponents on the entity?**
- Entity might have multiple sprites, one with correct UVs and one rendering full texture

---

## Potential Root Causes (Remaining)

### 🔴 Theory 1: Shader Not Using UV Coordinates
**Description:** The fragment shader might be ignoring `v_TexCoord` and always sampling (0,0) to (1,1)

**How to Test:**
1. Add logging in shader (not possible with GLSL)
2. Render a quad with known UV coordinates and verify visual result
3. Check if flip flags work (they use same UV system)

**Code to Check:**
- `OpenGLBatchRenderer2D.cpp` lines 102-120 (fragment shader)

---

### 🔴 Theory 2: Vertex Data Not Uploaded to GPU
**Description:** Batch renderer builds correct vertex data but doesn't upload it correctly

**How to Test:**
1. Add logging before `glBufferSubData` call showing UV values
2. Verify buffer size and stride calculations
3. Check if vertex attribute pointers are set correctly

**Code to Check:**
- `OpenGLBatchRenderer2D.cpp` Flush() method (around line 256-290)

---

### 🔴 Theory 3: Scene Refresh Required
**Description:** Changes to SpriteComponent require exiting and re-entering play mode

**How to Test:**
1. Apply sprite sheet cell in edit mode
2. Enter play mode (or restart scene)
3. Check if sprite renders correctly

**User Action Required:** Verify if testing in play mode vs edit mode

---

### 🔴 Theory 4: Entity Has Multiple Sprite Rendering Paths
**Description:** Entity might have both a SpriteComponent AND an AnimationComponent or other rendering component

**How to Test:**
1. Check entity components in inspector
2. Verify only one sprite is being rendered
3. Temporarily disable AnimationComponent if present

**Code to Check:**
- `SpriteRenderSystem.cpp` - verify it only renders SpriteComponent entities
- Check if entity has AnimationComponent that might be rendering

---

### 🔴 Theory 5: Texture Slot/Binding Issue
**Description:** Sprite might be rendering but sampling from wrong texture or wrong part of atlas

**How to Test:**
1. Load a completely different texture (different visual)
2. Apply cell and see if ANY change occurs
3. Check texture slot assignments in batch renderer

**Code to Check:**
- `OpenGLBatchRenderer2D.cpp` GetOrAddTextureSlot() method
- Verify texture IDs match between what's set and what's bound

---

### 🔴 Theory 6: Size is Overriding UV Effect
**Description:** Even with correct UVs, if sprite.Size is wrong, it might visually look like full texture

**How to Test:**
1. Manually set sprite.Size to 1.0 x 1.0 and see if it changes appearance
2. Check if Transform.Scale is interfering
3. Verify pixel-per-unit calculations

**Status:** Partially tested - size calculation fixed but issue persists

---

## Next Steps for Debugging

### Priority 1: Verify UV Coordinates Reach Renderer
**Action:** Check console logs for 🎨 and 📚 emoji logs during rendering
- If logs appear with correct UVs → Problem is in GPU/shader
- If logs don't appear → Sprite not being rendered, or logging disabled

### Priority 2: Test with Visual Marker
**Action:** Apply a cell, then change sprite color to bright red
- If color changes → Sprite IS rendering, UV issue in GPU
- If color doesn't change → Sprite not rendering at all

### Priority 3: Test Flip Flags
**Action:** Set `sprite.FlipX = true` manually
- If sprite flips → UV system works, issue elsewhere
- If sprite doesn't flip → UV system broken

### Priority 4: Minimal Reproduction
**Action:** Create new scene with single sprite entity
1. Create entity with SpriteComponent only (no animation, no other components)
2. Load sprite sheet texture
3. Manually set UVs in code: `sprite.TexCoordMin = {0.333, 0}; sprite.TexCoordMax = {0.5, 1};`
4. Check if it renders correctly
- If it works → Issue is in Sprite Sheet Editor apply logic
- If it doesn't work → Issue is in rendering pipeline

### Priority 5: Check Active Scene State
**Action:** Verify scene is in play mode vs edit mode
- Some systems may only activate in play mode
- Scene might need refresh after component changes

---

## Code Changes Made (For Rollback)

### File: `Renderer2DBackend.cpp`
```cpp
// ADDED: Logging for sprites with LockUV=true
if (hasTexture && sprite.LockUV)
{
    PIL_CORE_INFO("🎨 DrawSprite (LockUV=true) - Pos({}, {}) Size({}, {}) UV: ({}, {}) to ({}, {})", 
                  position.x, position.y, size.x, size.y,
                  sprite.TexCoordMin.x, sprite.TexCoordMin.y,
                  sprite.TexCoordMax.x, sprite.TexCoordMax.y);
}
```

### File: `SpriteRenderSystem.cpp`
```cpp
// ADDED: Logging before rendering sprites with locked UVs
if (sprite.LockUV && sprite.Texture)
{
    PIL_CORE_INFO("📚 SpriteRenderSystem: Rendering sprite with LockUV=true, UV: ({}, {}) to ({}, {})",
        sprite.TexCoordMin.x, sprite.TexCoordMin.y,
        sprite.TexCoordMax.x, sprite.TexCoordMax.y);
}
```

### File: `SpriteSheetEditorPanel.cpp`
```cpp
// CHANGED: Use derived cell dimensions instead of stored m_CellWidth/m_CellHeight
float derivedCellPixelWidth = availableWidth / (float)m_GridColumns;
float derivedCellPixelHeight = availableHeight / (float)m_GridRows;
glm::vec2 newNativeSize(derivedCellPixelWidth / pixelsPerUnit, 
                        derivedCellPixelHeight / pixelsPerUnit);

// ADDED: Logging to show derived vs stored dimensions
ConsolePanel::Log("  📏 Cell dimensions: derived=(" + std::to_string((int)derivedCellPixelWidth) + "x" + 
                std::to_string((int)derivedCellPixelHeight) + ") vs stored=(" + 
                std::to_string(m_CellWidth) + "x" + std::to_string(m_CellHeight) + 
                ") → Size=(" + std::to_string(newNativeSize.x) + ", " + std::to_string(newNativeSize.y) + ")", 
                LogLevel::Info);

// ADDED: Enhanced verification logging
ConsolePanel::Log("  ✅ VERIFICATION - sprite UVs after setting: Min(" + std::to_string(sprite.TexCoordMin.x) + ", " + 
                std::to_string(sprite.TexCoordMin.y) + ") Max(" + std::to_string(sprite.TexCoordMax.x) + ", " + 
                std::to_string(sprite.TexCoordMax.y) + ") LockUV=" + (sprite.LockUV ? "true" : "false"), LogLevel::Info);
```

---

## User Logs (Latest)

```
Loaded sprite sheet metadata:
Loaded grid configuration from metadata
Loaded sprite sheet
Cell calculation: texSize=(576x96) gridConfig=(6cols x 1rows) derivedCell=(96x96) padding=0 spacing=0
Selected cell [2, 0]
Pixel coords: (192.000000, 0.000000) to (288.000000, 96.000000)
UV calculation: MinY = 1.0 - 96.000000/96.000000 = 0.000000
UV calculation: MaxY = 1.0 - 0.000000/96.000000 = 1.000000
Final UV coords: (0.333333, 0.000000) to (0.500000, 1.000000)
Applying to entity: Enemy
Old: Sprite.Size=(1.000000, 1.000000) Transform. Effective Size (0.800000, 0.800000)
Applying UV: Min(0.333333, 0.000000) Max(0.500000, 1.000000)
Selected cell: [2, 0]
✅ VERIFICATION - sprite UVs after setting: (0.333333, 0.000000) Max(0.500000, 1.000000) LockUV=true
New: Sprite. Transform Scale= (1.000000, 1.000000) Effective Size
Applied frame [2, 0] to sprite
```

**Notable:**
- No 🎨 or 📚 logs visible (renderer/system logs not appearing)
- UV coordinates confirmed correct at editor level
- LockUV flag confirmed set
- Size calculation log not visible yet (might be in next build)

---

## Recommendations

1. **Verify rendering is happening** - Check if sprite appears at all after applying cell
2. **Test with completely different texture** - Ensure SOME change occurs when applying
3. **Check play mode vs edit mode** - Scene state might matter
4. **Inspect entity components** - Verify no conflicting rendering components
5. **Check Transform.Scale** - Might be visually stretching sprite
6. **Create minimal test case** - Single entity, single sprite, manually set UVs in code
7. **Review shader source** - Ensure fragment shader uses v_TexCoord correctly
8. **Check GPU debugger** - Use RenderDoc or similar to inspect actual vertex/texture data on GPU

---

## Conclusion

The bug is **NOT** in:
- UV coordinate calculation ✓
- UV coordinate storage in SpriteComponent ✓
- UV coordinate passing through renderer API ✓
- Batch renderer vertex setup ✓

The bug is **LIKELY** in one of:
- GPU-side shader execution or texture sampling
- Scene state / refresh timing
- Conflicting components or render paths
- Texture binding / slot management
- Vertex data upload to GPU

**Recommended next action:** Create minimal reproduction case with manually set UVs to isolate whether issue is in editor or in rendering pipeline.
