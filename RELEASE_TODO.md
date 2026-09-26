# Release TODO

## Critical Bugs
- [x] Fix: objects added via context menu use wrong coordinates (ctxMenu.mx/my)
- [x] Fix: asset drop overlaps with object selection (check existing first)
- [x] Fix: panel click dispatch order (rect vs contentRect)
- [x] Fix: HandleEvents called before Layout (panels had zero size)
- [x] Fix: dragSplitter never reset (stuck on forever)

## Editor Features
- [x] Add "Add Object" button in Hierarchy panel ([R][E][T][+])
- [x] Make top menu File > Import Assets open file dialog
- [x] Proper drag-and-drop from Asset Browser (selectedAsset + click-to-place)
- [x] Undo/Redo system
- [x] Save/Load project scenes (File > Save/Open with file dialogs, .vne format)
- [x] Multiple selection in Hierarchy (Ctrl+click)
- [x] Snap grid size control ([ / ] keys)
- [x] Script export/import (File > Export Script... / Import Script...)
- [x] Preview in separate window (File > Preview... opens second HWND)

## Scene Panel
- [x] Proper ellipse rendering (raster ellipse via horizontal strips)
- [x] Image/sprite rendering from file
- [x] Snap-to-grid option
- [x] Pan with left-click drag on empty space

## Inspector
- [x] Color picker for objects (click color swatch → ChooseColor dialog)
- [x] Add/remove components (Sprite, Text, Collider, Audio)

## Script Panel
- [x] Script saving/loading
- [x] Script line editing (inline text edit)
- [x] Character/sprite/background picker per line

## Asset Browser
- [x] Folder navigation (subdirectories)
- [x] Image preview thumbnails
- [x] Delete/rename assets
- [x] File drag from Explorer into window (WM_DROPFILES already works)

## Stability & Debug
- [x] Debug overlay (F1 toggles bounds/coords/stats/todo)
- [x] Memory leak check (image cache OK - destructor cleans up)
- [x] Handle window resize properly (WM_SIZE + per-frame Layout)
- [x] Crash on missing assets/Images folder (auto-creates directories)

## Release
- [x] Installer/packaged build (dist\NovellEngine-v0.3.0.zip)
- [x] Clean up old/unused source files
- [x] Build script (package.bat)
- [x] GitHub release v0.2.0
- [x] README with controls

## v0.3.1 Polish Pass
- [x] Scene toolbar: Grid / Snap toggles + Align L/H/R/T/V/B + Copy/Paste buttons
- [x] AlignSelected (works on multi-selection bounding box)
- [x] CopySelected / PasteSelected + clipboard (Ctrl+C / Ctrl+V)
- [x] Cut action in Edit menu
- [x] Grid toggle respects showGrid in both scene draws
- [x] Drag snapping honors Snap toggle (Snap OR Shift)
- [x] Fixed scene toolbar snap label (was showing 40*zoom)
- [x] Context menus: Copy/Paste/Align entries added
- [x] Controls dialog + README updated with new shortcuts
