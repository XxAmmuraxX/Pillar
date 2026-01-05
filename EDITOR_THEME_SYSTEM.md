# Pillar Editor Visual Theme System

**Date:** January 1, 2026  
**Status:** ✅ Complete  
**Version:** 1.0  

---

## Overview

The Pillar Editor now features a modern, sleek visual theme system designed for extended development sessions. The theme is inspired by professional IDEs like JetBrains products and VS Code, with careful attention to:

- **Visual hierarchy** - Clear separation between UI elements
- **Reduced eye strain** - Balanced contrast and warm tones
- **Professional aesthetics** - Polished, modern appearance
- **User comfort** - Generous spacing and smooth interactions

---

## Key Visual Enhancements

### 1. **Typography & Fonts** 🔤

**Font Loading Strategy:**
- **Primary:** Segoe UI (16px) - Windows 10/11 default, clean and modern
- **Fallback:** Consolas (15px) - Monospace alternative
- **Default:** ImGui's built-in ProggyClean if system fonts unavailable

**Configuration:**
```cpp
ImFontConfig fontConfig;
fontConfig.OversampleH = 2;      // Horizontal oversampling for crisp text
fontConfig.OversampleV = 1;      // Vertical oversampling
fontConfig.PixelSnapH = true;    // Snap to pixel grid for sharpness
```

**Benefits:**
- Crisp, readable text at all sizes
- Professional appearance
- Better emoji/icon support
- Improved long-session readability

---

### 2. **Spacing & Layout** 📏

**Enhanced Spacing Values:**
```cpp
WindowPadding:      10px × 10px  (was 8×8)   - More breathing room
FramePadding:       8px × 5px    (was 5×4)   - Larger interactive areas
ItemSpacing:        10px × 6px   (was 8×4)   - Better visual separation
ItemInnerSpacing:   6px × 4px    (was 4×4)   - Cleaner inline elements
IndentSpacing:      22px         (was 20px)  - Clearer hierarchy
ScrollbarSize:      16px         (was 14px)  - Easier to grab
```

**Alignment:**
- **Window Titles:** Centered (0.5, 0.5)
- **Button Text:** Centered (0.5, 0.5)

**Impact:**
- Less cramped UI
- Easier click targets
- Better visual flow
- Professional polish

---

### 3. **Rounded Corners** 🔵

**Modern Rounding Values:**
```cpp
WindowRounding:    6px  (was 4px)
ChildRounding:     5px  (was 4px)
FrameRounding:     4px  (was 3px)
PopupRounding:     5px  (was 4px)
ScrollbarRounding: 9px  (was 6px)
TabRounding:       5px  (was 4px)
```

**Anti-Aliasing:**
- All rendering uses anti-aliased lines and fills
- Texture-based AA for performance
- Smooth, crisp edges throughout

---

### 4. **Color Palette** 🎨

#### **Background Hierarchy**
```
Main Window:    #21232626 (13%, 14%, 15%)  - Deep, rich base
Popup/Menu:     #29292C   (16%, 17%, 18%)  - Slightly elevated
Title Bar:      #1A1B1E   (10%, 11%, 12%)  - Darker for contrast
Empty Dock:     #1C1D21   (11%, 12%, 13%)  - Subtle distinction
```

#### **Interactive Elements**
```
Button Default:  #3D4247  (24%, 26%, 28%)
Button Hovered:  #52565E  (32%, 34%, 37%)
Button Active:   #669ED9  (40%, 62%, 85%)  - Blue accent

Frame/Input:     #31333A  (19%, 20%, 22%)
Frame Hovered:   #3B3E43  (23%, 24%, 26%)
Frame Active:    #434851  (26%, 28%, 30%)
```

#### **Accent Colors**
```
Primary Accent:  #669ED9 (40%, 62%, 85%)  - Vibrant blue
Hovered Accent:  #73B3F2 (45%, 70%, 95%)  - Lighter blue
Checkmarks:      #B3F2F2 (45%, 70%, 95%)  - Bright blue
```

#### **Text**
```
Primary Text:    #E6E6E8 (90%, 90%, 91%)  - High contrast
Disabled Text:   #80828A (50%, 51%, 52%)  - Muted
Selected BG:     #669ED9 @ 35% alpha      - Subtle highlight
```

#### **Borders & Separators**
```
Border:          #484A4F @ 60% alpha (28%, 29%, 31%)
Separator:       #484A4F @ 60% alpha
Hovered:         #669ED9 @ 78% alpha - Blue feedback
```

---

### 5. **Component-Specific Styling** 🎛️

#### **Tabs**
- **Inactive:** Dark (#29292C)
- **Active:** Slightly lighter (#35373D)
- **Hovered:** Blue accent (#669ED9 @ 90%)
- **Unfocused:** Darker (#24252A)

#### **Headers & Collapsibles**
- **Default:** Semi-transparent dark (#3D4247 @ 80%)
- **Hovered:** Lighter (#52565E @ 90%)
- **Active:** Blue accent (#669ED9)

#### **Scrollbars**
- **Background:** Very dark (#1C1D21)
- **Grab:** Medium gray (#52565B)
- **Hovered:** Lighter gray (#6B6F76)
- **Active:** Even lighter (#85898E)

#### **Sliders & Checkboxes**
- **Checkmark:** Bright blue (#73B3F2)
- **Slider Grab:** Accent blue (#669ED9)
- **Active:** Lighter blue (#81B8F2)

---

## Visual Design Principles

### **1. Depth Through Subtle Variations**
Instead of harsh contrasts, the theme uses subtle color variations to create visual depth:
- Main windows are darkest
- Popups/menus are slightly lighter
- Interactive elements are more prominent
- Active elements use accent colors

### **2. Smooth Interactions**
All interactive elements have three states with smooth visual transitions:
- **Default:** Visible but not distracting
- **Hovered:** Clear feedback that element is interactive
- **Active/Pressed:** Strong visual confirmation

### **3. Accessibility**
- High contrast text (90% white on dark background)
- Large click targets (enhanced padding)
- Clear visual feedback
- Consistent color language

### **4. Professional Polish**
- Rounded corners for modern feel
- Anti-aliased rendering
- Centered text alignment where appropriate
- Generous spacing prevents cramping

---

## Usage Guidelines

### **For New Panels:**

1. **Trust the theme** - Let the global style do the work
2. **Use style colors** - Don't hardcode colors in panels
3. **Respect spacing** - Use `ImGui::Spacing()` for vertical gaps
4. **Follow patterns** - Match existing panel layouts

### **Custom Styling:**

When you need to override styles:
```cpp
// Push temporary style changes
ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));

// Your custom UI
ImGui::Button("Special Button");

// Always pop in reverse order
ImGui::PopStyleColor();
ImGui::PopStyleVar();
```

### **Color Coding Conventions:**

**Use these semantic colors consistently:**
- **Blue (#669ED9)** - Primary actions, selections, focus
- **Green (#4CAF50)** - Success, active/enabled states
- **Yellow (#FFC107)** - Warnings, neutral status
- **Red (#F44336)** - Errors, destructive actions
- **Gray** - Disabled, secondary information

---

## Customization

### **Adjusting Colors:**

All colors are defined in `EditorLayer::SetupImGuiStyle()`. To customize:

1. Find the color category you want to change
2. Modify the `ImVec4(r, g, b, a)` values
3. Colors are in 0.0-1.0 range
4. Use online color pickers and divide by 255

Example:
```cpp
// Change button color from gray to dark blue
colors[ImGuiCol_Button] = ImVec4(0.15f, 0.25f, 0.45f, 1.00f);
```

### **Adjusting Spacing:**

Modify values in the spacing section:
```cpp
style.WindowPadding = ImVec2(12.0f, 12.0f);  // More padding
style.ItemSpacing = ImVec2(12.0f, 8.0f);     // More spacing
```

### **Loading Custom Fonts:**

To add your own font:
```cpp
io.Fonts->AddFontFromFileTTF("path/to/font.ttf", 16.0f, &fontConfig);
```

Common font locations:
- **Windows:** `C:/Windows/Fonts/`
- **Custom:** `PillarEditor/assets/fonts/` (create this folder)

---

## Performance Notes

**Font Loading:**
- Fonts are loaded once at startup
- Failed loads fall back gracefully to default font
- No runtime performance impact

**Color Changes:**
- All colors set once during initialization
- No per-frame overhead
- Styles are cached by ImGui

**Anti-Aliasing:**
- Uses hardware-accelerated rendering
- Texture-based AA for efficiency
- Minimal performance impact

---

## Technical Details

### **Implementation Location:**
- **Function:** `EditorLayer::SetupImGuiStyle()`
- **File:** `PillarEditor/src/EditorLayer.cpp` (lines ~157-240)
- **Called:** Once in `EditorLayer::OnAttach()`

### **Dependencies:**
- ImGui 1.89+ (docking branch)
- System fonts (optional, has fallback)
- No external libraries required

### **Configuration Files:**
- Theme is code-defined (no external config)
- ImGui layout saved in `imgui.ini`
- Font config defined in code

---

## Before & After Comparison

### **Before:**
- ❌ Basic dark theme with minimal customization
- ❌ Tight spacing felt cramped
- ❌ Default ImGui font (ProggyClean 13px)
- ❌ Sharp corners throughout
- ❌ Less contrast between UI elements
- ❌ Smaller interactive targets

### **After:**
- ✅ Sophisticated, modern theme
- ✅ Generous spacing for comfort
- ✅ Professional system font (Segoe UI 16px)
- ✅ Smooth rounded corners
- ✅ Clear visual hierarchy
- ✅ Larger, easier click targets
- ✅ Polished, professional appearance
- ✅ Better long-session usability

---

## Future Enhancements

### **Possible Additions:**

1. **Theme Variants:**
   - Light theme option
   - High contrast mode
   - Custom theme loading from JSON/YAML

2. **Font Options:**
   - Font size slider in preferences
   - Font family selection
   - Icon font integration (FontAwesome, etc.)

3. **Color Customization:**
   - Runtime color editor
   - Theme presets (Dark, Light, Blue, Purple, etc.)
   - Per-panel color overrides

4. **Advanced Features:**
   - Smooth theme transitions
   - Time-based themes (day/night)
   - Accessibility presets (dyslexia-friendly, colorblind modes)

5. **Performance:**
   - GPU-accelerated rendering
   - Custom shader effects
   - Blur effects for popups

---

## Developer Notes

### **For Future Maintainers:**

1. **Color Consistency:** When adding new UI elements, use the existing color palette
2. **Spacing Uniformity:** Follow the established spacing guidelines
3. **Font Handling:** Always check if font loaded successfully before using
4. **Style Push/Pop Balance:** Always pop styles in reverse order of pushing

### **Common Pitfalls:**

⚠️ **Don't:**
- Hardcode colors in panels (use `ImGui::GetStyle().Colors[...]`)
- Mix spacing values inconsistently
- Forget to pop style overrides
- Load fonts multiple times

✅ **Do:**
- Use the global style colors
- Follow spacing patterns from existing panels
- Balance PushStyleVar/PopStyleVar calls
- Test on different display scales

### **Testing Checklist:**

Before committing theme changes:
- [ ] Test all panels (Hierarchy, Inspector, Viewport, etc.)
- [ ] Check all interactive elements (buttons, inputs, sliders)
- [ ] Verify text readability
- [ ] Test hover/active states
- [ ] Check at different window sizes
- [ ] Verify docking visuals
- [ ] Test modal dialogs
- [ ] Check console output styling

---

## Conclusion

The Pillar Editor's visual theme system provides a modern, professional foundation for the entire editor UI. The carefully crafted color palette, generous spacing, and polished interactions create a comfortable development environment suitable for extended coding sessions.

The theme strikes a balance between aesthetics and usability, ensuring that the editor not only looks great but also enhances developer productivity through clear visual hierarchy and intuitive interactions.

**Enjoy your sleek, modern editor! ✨**

---

## References

**Inspiration Sources:**
- Visual Studio Code (Microsoft)
- JetBrains IDEs (IntelliJ IDEA, Rider, CLion)
- GitHub Dark Theme
- Material Design Dark Theme
- Unity Editor

**ImGui Resources:**
- [ImGui GitHub](https://github.com/ocornut/imgui)
- [ImGui Style Editor](Built into ImGui - Demo > Style Editor)
- [ImGui Color Tool](https://www.imgui.org/)

---

**Document Version:** 1.0  
**Last Updated:** January 1, 2026  
**Maintained By:** Pillar Engine Team
