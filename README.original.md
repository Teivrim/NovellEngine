# Novell Engine

Visual novel editor with timeline, script panel, hierarchy, scene editor, inspector, and asset browser.

## Controls

| Key | Action |
|-----|--------|
| F1 | Toggle debug overlay |
| F5 | Toggle preview mode |
| F11 | Fullscreen |
| Shift+drag | Snap-to-grid |
| [ / ] | Decrease/increase snap grid size |
| Ctrl+Z / Ctrl+Y | Undo / Redo |
| Ctrl+C / Ctrl+V | Copy / Paste objects |
| Ctrl+G | Toggle grid |
| Ctrl+M | Toggle snap |
| Ctrl+click | Multi-select in Hierarchy |
| Ctrl+N | New project |
| Ctrl+S | Save project |
| Ctrl+O | Open project |
| Ctrl+D | Duplicate selected |
| Ctrl+F | Zoom to fit scene |
| R / E / T | Add rectangle / ellipse / text |
| Del | Delete selected |
| Scroll wheel | Scroll panels / zoom scene |
| Right-click | Context menu (add objects) |
| Drag files from Explorer | Import assets |

## Scene Toolbar

| Button | Action |
|--------|--------|
| [-] / [+] | Zoom out / in |
| Fit | Zoom to fit all objects |
| Grid | Toggle grid overlay |
| Snap | Toggle snapping on drag |
| L / H / R | Align left / center-h / right |
| T / V / B | Align top / center-v / bottom |
| Copy / Paste | Duplicate selection via clipboard |

## Build

```bat
build.bat
```

Requires: MinGW g++ (14.2.0+), Windows SDK (gdi32, gdiplus, comctl32, comdlg32)

## Build Status

Working features:
- Drag-and-drop from Windows Explorer
- Editor panels: Hierarchy, Scene, Inspector, Script, Asset Browser, Timeline
- Object lifespan with tick system
- Snap-to-grid while holding Shift
- Color picker in Inspector (RGB fields)
- Script line creation from hierarchy objects
- Save/Load project files
- Preview mode with dialogue playback per tick
