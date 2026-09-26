#pragma once
#include "GraphicsAPI.hpp"
#include "ProjectManager.hpp"
#include <string>
#include <vector>
#include <functional>
#include <cstdlib>
#include <cstdio>

#define DBG(fmt, ...) do { printf("[%s:%d] " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__); fflush(stdout); } while(0)

struct Rect {
    int x, y, w, h;
    bool Contains(int px, int py) const { return px >= x && px <= x + w && py >= y && py <= y + h; }
};

struct ContextMenu {
    bool active;
    int mx, my;
    std::vector<std::string> items;
    int hovered;
    ContextMenu() : active(false), mx(0), my(0), hovered(-1) {}
    void Open(int x, int y, const std::vector<std::string>& opts) { active = true; mx = x; my = y; items = opts; hovered = -1; }
    void Close() { active = false; items.clear(); }
    int HitTest(int x, int y) {
        if (!active) return -1;
        int cw = GetWidth();
        if (x < mx || x > mx + cw || y < my || y > my + (int)items.size() * 24) return -1;
        return (y - my) / 24;
    }
    int GetWidth() const {
        int maxW = 160;
        for (auto& i : items) {
            int w = (int)i.size() * 9;
            if (w > maxW) maxW = w;
        }
        return maxW;
    }
    void Draw(GraphicsAPI* gfx) {
        if (!active) return;
        int cw = GetWidth();
        gfx->DrawRect(mx, my, cw, (int)items.size() * 24, 45, 45, 55);
        for (int i = 0; i < (int)items.size(); i++) {
            int iy = my + i * 24;
            if (i == hovered) gfx->DrawRect(mx + 2, iy, cw - 4, 22, 55, 80, 120);
            gfx->RenderText(items[i], mx + 8, iy + 3, 200, 200, 220);
        }
    }
};

struct EditorObject {
    std::string name, type, spritePath;
    int posX, posY, sizeW, sizeH;
    std::string text;
    bool visible;
    int colorR, colorG, colorB;
    int startTick, endTick;
    std::vector<std::string> components;
    EditorObject() : posX(50), posY(50), sizeW(100), sizeH(100), visible(true), colorR(70), colorG(80), colorB(100), startTick(0), endTick(9999) {}
    bool HasComponent(const std::string& comp) const {
        for (auto& c : components) if (c == comp) return true;
        return false;
    }
    void AddComponent(const std::string& comp) {
        if (!HasComponent(comp)) components.push_back(comp);
    }
    void RemoveComponent(const std::string& comp) {
        for (auto it = components.begin(); it != components.end(); ) {
            if (*it == comp) it = components.erase(it);
            else ++it;
        }
    }
};

struct ScriptLine {
    std::string text, characterName, spritePath, backgroundPath;
    int tick;
    ScriptLine() : tick(0) {}
};

enum GizmoPart { GIZMO_NONE, GIZMO_MOVE, GIZMO_RESIZE_TL, GIZMO_RESIZE_TR, GIZMO_RESIZE_BL, GIZMO_RESIZE_BR };

struct EditorPanel {
    std::string title;
    Rect rect, contentRect;
    bool visible;
    int id;
    EditorPanel(const std::string& t) : title(t), visible(true), id(-1) { DBG("Panel '%s' created", t.c_str()); }
    virtual ~EditorPanel() { DBG("Panel '%s' destroyed", title.c_str()); }
    virtual void Draw(GraphicsAPI* gfx) = 0;
    virtual bool HandleClick(int mx, int my) = 0;
    virtual void HandleDrag(int mx, int my) { (void)mx; (void)my; }
    virtual void HandleScroll(int delta, int mouseY) { (void)delta; (void)mouseY; }
    virtual void HandleKey(int key) { (void)key; }
    virtual const char* GetType() const { return "EditorPanel"; }
    void SetRect(const Rect& r) {
        rect = r; contentRect = {r.x + 2, r.y + 22, r.w - 4, r.h - 24};
        DBG("Panel '%s' rect=(%d,%d %dx%d) content=(%d,%d %dx%d)", title.c_str(), r.x, r.y, r.w, r.h, contentRect.x, contentRect.y, contentRect.w, contentRect.h);
    }
    void DrawTitleBar(GraphicsAPI* gfx) {
        // Gradient-like title bar (two rects)
        gfx->DrawRect(rect.x, rect.y, rect.w, 20, 28, 30, 38);
        gfx->DrawRect(rect.x, rect.y + 18, rect.w, 3, 45, 50, 60);
        // Accent line
        gfx->DrawRect(rect.x, rect.y + 20, rect.w, 1, 74, 125, 180);
        // Title text
        gfx->RenderText(title, rect.x + 6, rect.y + 2, 180, 190, 210);
    }
    void DrawFrame(GraphicsAPI* gfx) {
        DrawTitleBar(gfx);
        // Subtle border
        gfx->DrawRect(rect.x, rect.y, 1, rect.h, 35, 38, 45);
        gfx->DrawRect(rect.x + rect.w - 1, rect.y, 1, rect.h, 35, 38, 45);
        gfx->DrawRect(rect.x, rect.y + rect.h - 1, rect.w, 1, 35, 38, 45);
    }
};

struct HierarchyPanel : EditorPanel {
    std::vector<EditorObject> objects;
    int selectedIndex, scrollOffset;
    std::vector<int> multiSel;
    std::string filter;
    int editingFilter;
    HierarchyPanel();
    void Draw(GraphicsAPI* gfx) override;
    bool HandleClick(int mx, int my) override;
    int AddObject(const std::string& name, const std::string& type, int px = -1, int py = -1);
    void DeleteSelected();
    void ToggleVisible();
    void SyncMulti();
    bool IsMulti(int i) const;
    bool IsFiltered(int i) const;
};

struct InspectorPanel : EditorPanel {
    EditorObject* target;
    int editingField;
    std::string editBuf;
    bool compAddOpen;
    InspectorPanel();
    void Draw(GraphicsAPI* gfx) override;
    bool HandleClick(int mx, int my) override;
    void CommitEdit();
    void SetTarget(EditorObject* obj);
};

struct AssetBrowserPanel : EditorPanel {
    std::vector<std::string> files;
    int scrollOffset;
    bool selectedAsset;
    int selAssetIndex;
    std::string selAssetPath;
    std::string currentDir;
    int editingField, editingIndex;
    std::string editBuf;
    AssetBrowserPanel();
    void ScanDirectory(const std::string& dir);
    void ScanDirectory();
    void Draw(GraphicsAPI* gfx) override;
    bool HandleClick(int mx, int my) override;
    int GetFileIndexAt(int mx, int my) const;
    void CancelSelect();
    void HandleKey(int key);
};

enum SceneTool { ST_NONE, ST_ZOOM_OUT, ST_ZOOM_IN, ST_FIT, ST_GRID, ST_SNAP, ST_ALIGN_L, ST_ALIGN_H, ST_ALIGN_R, ST_ALIGN_T, ST_ALIGN_V, ST_ALIGN_B, ST_COPY, ST_PASTE };

struct ScenePanel : EditorPanel {
    int selectedIndex;
    GizmoPart activeGizmo;
    int gizmoStartX, gizmoStartY, gizmoObjX, gizmoObjY, gizmoObjW, gizmoObjH;
    float zoom;
    int panX, panY;
    bool showGrid, snapEnabled;
    ScenePanel();
    using EditorPanel::HandleDrag;
    using EditorPanel::HandleScroll;
    void Draw(GraphicsAPI* gfx) override;
    bool HandleClick(int mx, int my) override;
    void HandleDrag(int mx, int my);
    void HandleScroll(int delta);
    void DrawGrid(GraphicsAPI* gfx);
    void DrawToolbar(GraphicsAPI* gfx);
    int HitToolbar(int mx, int my);
    GizmoPart HitTestGizmo(int mx, int my, EditorObject* obj);
    void DrawGizmo(GraphicsAPI* gfx, EditorObject* obj);
};

struct ScriptPanel : EditorPanel {
    std::vector<ScriptLine> lines;
    int selectedLine, scrollOffset;
    int editingLine, editingField;
    std::string editBuf;
    ScriptPanel();
    void Draw(GraphicsAPI* gfx) override;
    bool HandleClick(int mx, int my) override;
    void AddLine();
    void DeleteLine();
    void MoveUp();
    void MoveDown();
    void HandleKey(int key);
};

struct TimelinePanel : EditorPanel {
    int scrollOffset;
    int dragMode, dragObj;
    int dragStartTick, dragEndTick, dragMouseStartX;
    TimelinePanel();
    using EditorPanel::HandleDrag;
    void Draw(GraphicsAPI* gfx) override;
    void Draw(GraphicsAPI* gfx, const std::vector<EditorObject>& objects, int selectedIndex, int currentTick);
    bool HandleClick(int mx, int my) override;
    bool HandleClick(int mx, int my, std::vector<EditorObject>& objects, int& selectedIndex, int& currentTick);
    void HandleDrag(int mx, int my, std::vector<EditorObject>& objects);
};

struct EditorSnapshot {
    std::vector<EditorObject> objects;
    std::vector<ScriptLine> lines;
};

struct EditorPanelSlot {
    EditorPanel* panel;
    EditorPanelSlot(EditorPanel* p) : panel(p) { DBG("Slot created for '%s'", p ? p->title.c_str() : "null"); }
    EditorPanelSlot(const EditorPanelSlot&) = delete;
    EditorPanelSlot& operator=(const EditorPanelSlot&) = delete;
    ~EditorPanelSlot() { DBG("Slot destroyed for '%s'", panel ? panel->title.c_str() : "null"); delete panel; panel = nullptr; }
};

enum PanelId {
    PANEL_HIERARCHY = 0,
    PANEL_INSPECTOR,
    PANEL_SCENE,
    PANEL_ASSET_BROWSER,
    PANEL_SCRIPT,
    PANEL_TIMELINE,
    PANEL_COUNT
};

struct EditorWindow {
    std::vector<EditorPanelSlot*> panelSlots;
    HierarchyPanel* hierarchy;
    InspectorPanel* inspector;
    AssetBrowserPanel* assetBrowser;
    ScenePanel* scenePanel;
    ScriptPanel* scriptPanel;
    TimelinePanel* timeline;

    int currentTick;

    Rect splitterR1, splitterR2, splitterR3, splitterR4;
    int dragSplitter;
    float leftRatio, rightRatio, bottomRatio, timelineRatio;

    ContextMenu ctxMenu;

    int activeMenu;
    std::vector<std::string> menuItems;
    int menuHovered;

    bool showDebug;
    bool scenePanning;
    int panStartX, panStartY, panStartPanX, panStartPanY;
    std::string lastMsg;
    int lastMsgTimer;

    // Preview window (separate HWND)
    class PreviewWindow* previewWin;
    bool IsPreviewActive() const { return previewWin != nullptr; }
    void StartPreview();
    void StopPreview();
    void RenderPreview();
    void PollPreviewEvents();

    int snapSize;
    std::vector<EditorSnapshot> undoStack, redoStack;
    std::vector<WindowEvent> eventQueue;

    bool dirty;
    std::string currentProjectPath;
    std::vector<std::string> recentFiles;

    EditorWindow();
    ~EditorWindow();
    EditorWindow(const EditorWindow&) = delete;
    EditorWindow& operator=(const EditorWindow&) = delete;
    EditorWindow(EditorWindow&&) = delete;
    EditorWindow& operator=(EditorWindow&&) = delete;
    void Layout(int winW, int winH);
    void Draw(GraphicsAPI* gfx);
    void DrawPreview(GraphicsAPI* gfx);
    void HandleEvents(const std::vector<WindowEvent>& events);
    void HandleClick(int mx, int my);
    void HandleRClick(int mx, int my);
    void HandleDrag(int mx, int my);
    void HandleScroll(int delta, int mouseY);
    void HandleKey(int key);
    int HitTestSplitter(int mx, int my);
    void AddShape(const std::string& type, int mx, int my);
    void SyncSelection();
    void DrawMenuBar(GraphicsAPI* gfx);
    void DrawToolbar(GraphicsAPI* gfx);
    int HitToolbar(int mx, int my);
    void HandleMenuClick(int mx, int my);
    void DrawSceneObjects(GraphicsAPI* gfx);
    void DrawSplitters(GraphicsAPI* gfx);
    void DrawDebugOverlay(GraphicsAPI* gfx);
    void HandleFileDrop(const std::string& filePath);
    void PushUndo();
    void Undo();
    void Redo();
    void RegisterPanel(EditorPanel* panel, int id);
    EditorPanel* GetPanel(int id) const;
    void TogglePanel(int id);
    template<typename T> T* GetPanelAs(int id) const { return dynamic_cast<T*>(GetPanel(id)); }

    void MarkDirty();
    void SaveCurrentProject();
    void LoadProjectFrom(const std::string& path);
    void DuplicateSelected();
    void MoveSelectedUp();
    void MoveSelectedDown();
    void BringToFront();
    void SendToBack();
    void ZoomToFitScene();
    void AlignSelected(int mode);
    void CopySelected();
    void PasteSelected();
    void ToggleGrid();
    void ToggleSnap();
    void LoadRecentFiles();
    void SaveRecentFiles();
    void PushRecentFile(const std::string& path);
    std::vector<EditorObject> clipboard;
};
