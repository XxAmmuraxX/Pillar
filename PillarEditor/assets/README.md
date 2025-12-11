# PillarEditor Assets

This directory contains assets used by the Pillar Editor.

## Directory Structure

```
assets/
??? icons/          # Editor UI icons (future)
??? fonts/          # Custom fonts (future)
??? scenes/         # Default/example scenes
??? textures/       # Editor textures
```

## Controls

### Viewport Navigation
- **Middle Mouse Button**: Pan the viewport
- **Mouse Scroll Wheel**: Zoom in/out

### Selection
- **Left Click** (in Hierarchy): Select entity
- **Ctrl + Left Click**: Multi-select
- **Escape**: Deselect all

### Entity Operations  
- **Delete**: Delete selected entity
- **Ctrl+D**: Duplicate selected entity
- **F**: Focus camera on selected entity

### File Operations
- **Ctrl+N**: New Scene
- **Ctrl+O**: Open Scene
- **Ctrl+S**: Save Scene
- **Ctrl+Shift+S**: Save Scene As

## Default Layout

The editor opens with a default dockspace layout:
- **Left**: Scene Hierarchy
- **Center**: Viewport
- **Right**: Inspector, Stats
- **Bottom**: Content Browser, Console

You can reset the layout via View > Reset Layout.
