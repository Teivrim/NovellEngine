#include "EditorPanels.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cctype>
#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#include "WinAPIGraphics.hpp"
#endif
namespace fs = std::filesystem;

// Scrollbar helper
static void DrawScrollbar(GraphicsAPI* gfx, int cx, int cy, int cw, int ch, int offset, int totalItems, int itemH) {
    int totalH = totalItems * itemH;
    if (totalH <= ch || totalItems == 0) return;
    float ratio = (float)ch / (float)totalH;
    int thumbH = std::max(8, (int)(ratio * ch));
    float maxThumbY = (float)(ch - thumbH);
    float maxOffset = (float)(totalH - ch);
    int thumbY = cy + (int)((float)offset * itemH / maxOffset * maxThumbY);
    int sx = cx + cw - 8;
    gfx->DrawRect(sx, cy, 6, ch, 18, 19, 24);
    gfx->DrawRect(sx + 1, thumbY, 4, thumbH, 50, 55, 65);
}

// ========== HIERARCHY ==========
HierarchyPanel::HierarchyPanel() : EditorPanel("Hierarchy"), selectedIndex(-1), scrollOffset(0), editingFilter(0) {}

bool HierarchyPanel::IsFiltered(int i) const {
    if (filter.empty()) return false;
    std::string fn = objects[i].name;
    for (auto& c : fn) c = (char)tolower(c);
    std::string f = filter;
    for (auto& c : f) c = (char)tolower(c);
    return fn.find(f) == std::string::npos;
}

bool HierarchyPanel::IsMulti(int i) const {
    for (auto& s : multiSel) if (s == i) return true;
    return false;
}

void HierarchyPanel::SyncMulti() {
    multiSel.clear();
    if (selectedIndex >= 0 && selectedIndex < (int)objects.size()) multiSel.push_back(selectedIndex);
}

void HierarchyPanel::Draw(GraphicsAPI* gfx) {
    DrawFrame(gfx);
    int filterH = 18;
    int startY = contentRect.y + filterH + 4;
    // Filter bar
    gfx->DrawRect(contentRect.x + 2, contentRect.y + 2, contentRect.w - 4, filterH, 22, 24, 32);
    gfx->DrawRect(contentRect.x + 2, contentRect.y + 2, contentRect.w - 4, 1, 35, 38, 45);
    if (editingFilter) {
        gfx->RenderText(filter + "|", contentRect.x + 6, contentRect.y + 2, 255, 255, 200);
    } else {
        std::string fl = filter.empty() ? "Filter objects..." : filter;
        gfx->RenderText(fl, contentRect.x + 6, contentRect.y + 2, filter.empty() ? 80 : 200, filter.empty() ? 80 : 200, filter.empty() ? 100 : 220);
    }
    // Object list with filter
    int y = startY;
    int maxY = rect.y + rect.h - 26;
    int visibleCount = 0;
    for (int i = scrollOffset; i < (int)objects.size(); i++) {
        if (IsFiltered(i)) continue;
        if (y + 24 > maxY) break;
        int rowX = contentRect.x + 2, rowW = contentRect.w - 4;
        bool sel = (i == selectedIndex);
        bool multi = IsMulti(i);
        if (sel) gfx->DrawRect(rowX, y, rowW, 22, 40, 60, 100);
        else if (multi) gfx->DrawRect(rowX, y, rowW, 22, 50, 50, 75);
        else if (visibleCount % 2 == 0) gfx->DrawRect(rowX, y, rowW, 22, 22, 24, 32);
        if (sel) gfx->DrawRect(rowX, y, 2, 22, 74, 125, 180);
        int dotColor = 100;
        if (objects[i].type == "background") dotColor = 60;
        else if (objects[i].type == "sprite") dotColor = 90;
        else if (objects[i].type == "text") dotColor = 120;
        else if (objects[i].type == "ellipse") dotColor = 150;
        else if (objects[i].type == "rectangle") dotColor = 180;
        gfx->DrawRect(rowX + 6, y + 7, 8, 8, 0, dotColor, dotColor + 30);
        if (!objects[i].visible) gfx->DrawRect(rowX + 6, y + 7, 8, 2, 180, 60, 60);
        std::string label = objects[i].name;
        if (label.size() > 18) label = label.substr(0, 17) + ".";
        gfx->RenderText(label, rowX + 18, y + 2, objects[i].visible ? 200 : 100, objects[i].visible ? 200 : 100, objects[i].visible ? 220 : 120);
        int tbx = rowX + rowW - 70;
        gfx->DrawRect(tbx, y + 3, 60, 17, 30, 30, 40);
        gfx->DrawRect(tbx, y + 3, 60, 1, 40, 42, 52);
        gfx->RenderText(objects[i].type, tbx + 4, y + 4, 80, 80, 100);
        int vx = rowX + rowW - 24;
        gfx->DrawRect(vx, y + 3, 18, 17, objects[i].visible ? 40 : 55, objects[i].visible ? 55 : 40, objects[i].visible ? 50 : 40);
        gfx->RenderText(objects[i].visible ? "V" : "H", vx + 4, y + 4, objects[i].visible ? 80 : 140, objects[i].visible ? 180 : 100, objects[i].visible ? 80 : 60);
        y += 24;
        visibleCount++;
    }
    if (objects.empty()) gfx->RenderText("No objects", contentRect.x + 8, startY, 80, 80, 100);
    else if (visibleCount == 0) gfx->RenderText("No matches", contentRect.x + 8, startY, 100, 80, 80);
    DrawScrollbar(gfx, contentRect.x, contentRect.y, contentRect.w, contentRect.h - 6, scrollOffset, (int)objects.size(), 24);
    int tby = rect.y + rect.h - 24;
    // Toolbar with styled shape buttons
    gfx->DrawRect(rect.x + 1, tby, rect.w - 2, 23, 22, 24, 32);
    gfx->DrawRect(rect.x + 1, tby, rect.w - 2, 2, 40, 45, 55);
    int bw = std::max(40, (rect.w - 6) / 4);
    auto drawBtn = [&](int x, const char* label, int r, int g, int b) {
        gfx->DrawRect(x, tby + 3, bw - 2, 19, 30, 33, 43);
        gfx->DrawRect(x, tby + 3, 1, 19, 50, 55, 65);
        gfx->DrawRect(x + bw - 3, tby + 3, 1, 19, 50, 55, 65);
        gfx->DrawRect(x, tby + 3 + 19 - 1, bw - 2, 1, 20, 22, 30);
        gfx->RenderText(label, x + 4, tby + 4, r, g, b);
    };
    drawBtn(rect.x + 3, "Rect", 150, 200, 150);
    drawBtn(rect.x + 5 + bw, "Ellipse", 200, 200, 150);
    drawBtn(rect.x + 7 + bw*2, "Text", 150, 150, 200);
    drawBtn(rect.x + 9 + bw*3, "Sprite", 200, 150, 150);
}

bool HierarchyPanel::HandleClick(int mx, int my) {
    int tby = rect.y + rect.h - 24;
    if (my >= tby && my <= tby + 23 && mx >= rect.x && mx <= rect.x + rect.w) {
        int bw = std::max(40, (rect.w - 6) / 4);
        if (mx < rect.x + 6 + bw) { AddObject("Rectangle", "rectangle"); SyncMulti(); return true; }
        if (mx < rect.x + 8 + bw*2) { AddObject("Ellipse", "ellipse"); SyncMulti(); return true; }
        if (mx < rect.x + 10 + bw*3) { AddObject("Text", "text"); SyncMulti(); return true; }
        AddObject("Sprite", "sprite"); SyncMulti(); return true;
    }
    if (!contentRect.Contains(mx, my)) return false;
    // Filter bar click
    int filterH = 18;
    if (my >= contentRect.y + 2 && my <= contentRect.y + 2 + filterH) {
        editingFilter = 1;
        return true;
    }
    int y = contentRect.y + filterH + 6;
    int maxY = rect.y + rect.h - 26;
#ifdef _WIN32
    bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
#else
    bool ctrl = false;
#endif
    int visibleCount = 0;
    for (int i = scrollOffset; i < (int)objects.size(); i++) {
        if (IsFiltered(i)) continue;
        if (y + 24 > maxY) break;
        int rowX = contentRect.x + 2, rowW = contentRect.w - 4;
        if (mx >= rowX && mx <= rowX + rowW && my >= y && my <= y + 22) {
            if (mx >= rowX + rowW - 48 && mx < rowX + rowW - 24) { objects.erase(objects.begin() + i); multiSel.clear(); if (selectedIndex >= (int)objects.size()) selectedIndex = (int)objects.size() - 1; return true; }
            if (mx >= rowX + rowW - 24 && mx < rowX + rowW) { objects[i].visible = !objects[i].visible; return true; }
            if (ctrl) {
                if (IsMulti(i)) { multiSel.erase(std::remove(multiSel.begin(), multiSel.end(), i), multiSel.end()); }
                else { multiSel.push_back(i); selectedIndex = i; }
            } else {
                selectedIndex = i; SyncMulti();
            }
            return true;
        }
        y += 24;
        visibleCount++;
    }
    editingFilter = 0;
    return true;
}

int HierarchyPanel::AddObject(const std::string& name, const std::string& type, int px, int py) {
    EditorObject obj;
    obj.name = name; obj.type = type;
    obj.posX = (px >= 0) ? px : (50 + ((int)objects.size() * 30) % 400);
    obj.posY = (py >= 0) ? py : (50 + ((int)objects.size() * 30) % 300);
    obj.sizeW = 100; obj.sizeH = 100; obj.visible = true;
    if (type == "text") { obj.sizeW = 200; obj.sizeH = 30; obj.text = "Text"; obj.colorR = 100; obj.colorG = 100; obj.colorB = 130; }
    if (type == "ellipse") { obj.sizeW = 100; obj.sizeH = 100; obj.colorR = 80; obj.colorG = 70; obj.colorB = 90; }
    if (type == "rectangle") { obj.colorR = 120; obj.colorG = 100; obj.colorB = 80; }
    if (type == "sprite") { obj.colorR = 60; obj.colorG = 80; obj.colorB = 100; }
    if (type == "background") { obj.colorR = 30; obj.colorG = 40; obj.colorB = 50; obj.sizeW = 800; obj.sizeH = 600; }
    objects.push_back(obj);
    selectedIndex = (int)objects.size() - 1;
    SyncMulti();
    return selectedIndex;
}

void HierarchyPanel::DeleteSelected() {
    if (objects.empty()) return;
    // Delete all multi-selected (including primary)
    if (!multiSel.empty()) {
        std::sort(multiSel.begin(), multiSel.end(), std::greater<int>());
        for (int idx : multiSel) if (idx >= 0 && idx < (int)objects.size()) objects.erase(objects.begin() + idx);
        multiSel.clear();
    } else if (selectedIndex >= 0 && selectedIndex < (int)objects.size()) {
        objects.erase(objects.begin() + selectedIndex);
    }
    if (selectedIndex >= (int)objects.size()) selectedIndex = (int)objects.size() - 1;
}

void HierarchyPanel::ToggleVisible() {
    if (!multiSel.empty()) {
        for (int idx : multiSel) if (idx >= 0 && idx < (int)objects.size()) objects[idx].visible = !objects[idx].visible;
    } else if (selectedIndex >= 0 && selectedIndex < (int)objects.size()) {
        objects[selectedIndex].visible = !objects[selectedIndex].visible;
    }
}

// ========== INSPECTOR ==========
InspectorPanel::InspectorPanel() : EditorPanel("Inspector"), target(nullptr), editingField(0), compAddOpen(false) {}

void InspectorPanel::Draw(GraphicsAPI* gfx) {
    DrawFrame(gfx);
    if (!target) { gfx->RenderText("Select an object", contentRect.x + 6, contentRect.y + 6, 100, 100, 120); return; }
    int y = contentRect.y + 6, lx = contentRect.x + 6;
    auto fld = [&](int id, const std::string& label, const std::string& val, int x, int y, int fw) {
        gfx->RenderText(label, x, y, 140, 140, 160);
        int fx = x + 55, fy = y, fw2 = fw > 0 ? fw : contentRect.w - 65;
        if (editingField == id) {
            gfx->DrawRect(fx, fy, fw2, 20, 60, 50, 35);
            gfx->RenderText(editBuf, fx + 4, fy + 2, 255, 255, 200);
        } else {
            gfx->DrawRect(fx, fy, fw2, 20, 22, 24, 32);
            gfx->DrawRect(fx, fy, 1, 20, 35, 38, 45);
            gfx->DrawRect(fx + fw2 - 1, fy, 1, 20, 35, 38, 45);
            gfx->RenderText(val, fx + 4, fy + 2, 200, 200, 200);
        }
    };
    // Section: Transform
    gfx->RenderText("Transform", lx, y, 100, 110, 130); y += 18;
    fld(1, "Name:", target->name, lx, y, 0); y += 26;
    gfx->RenderText("Type:", lx, y, 140, 140, 160); gfx->RenderText(target->type, lx + 55, y + 2, 180, 180, 200); y += 26;
    fld(2, "PosX:", std::to_string(target->posX), lx, y, 60); y += 26;
    fld(3, "PosY:", std::to_string(target->posY), lx, y, 60); y += 26;
    fld(4, "W:", std::to_string(target->sizeW), lx, y, 60); y += 26;
    fld(5, "H:", std::to_string(target->sizeH), lx, y, 60); y += 26;
    // Separator
    gfx->DrawRect(lx, y, contentRect.w - 12, 1, 35, 38, 45); y += 6;
    // Section: Appearance
    gfx->RenderText("Appearance", lx, y, 100, 110, 130); y += 18;
    fld(6, "Sprite:", target->spritePath, lx, y, 0); y += 26;
    gfx->RenderText("Color:", lx, y, 140, 140, 160);
    gfx->DrawRect(lx + 55, y, 20, 20, target->colorR, target->colorG, target->colorB);
    gfx->DrawRect(lx + 55, y, 20, 20, 74, 125, 180);
    int cs = contentRect.w - 70;
    fld(7, "R:", std::to_string(target->colorR), lx + 80, y, cs/3 - 5);
    fld(8, "G:", std::to_string(target->colorG), lx + 80 + cs/3, y, cs/3 - 5);
    fld(9, "B:", std::to_string(target->colorB), lx + 80 + cs*2/3, y, cs/3 - 5); y += 26;
    // Separator
    gfx->DrawRect(lx, y, contentRect.w - 12, 1, 35, 38, 45); y += 6;
    // Section: Timing
    gfx->RenderText("Timing", lx, y, 100, 110, 130); y += 18;
    fld(10, "StartT:", std::to_string(target->startTick), lx, y, 50); y += 26;
    fld(11, "EndT:", std::to_string(target->endTick), lx, y, 50); y += 26;
    gfx->RenderText("Visible: " + std::string(target->visible ? "Yes" : "No"), lx, y, 140, 140, 160); y += 26;
    // Separator
    gfx->DrawRect(lx, y, contentRect.w - 12, 1, 35, 38, 45); y += 6;
    // Section: Components
    gfx->RenderText("Components", lx, y, 100, 110, 130); y += 18;
    // Show existing components
    for (int i = 0; i < (int)target->components.size(); i++) {
        std::string& comp = target->components[i];
        // Component row card
        gfx->DrawRect(lx + 4, y, contentRect.w - 10, 22, 28, 30, 38);
        gfx->DrawRect(lx + 4, y, contentRect.w - 10, 1, 40, 42, 50);
        gfx->DrawRect(lx + 6, y + 4, 12, 12, 60, 100, 140);
        gfx->RenderText(comp, lx + 22, y + 3, 180, 180, 200);
        // Remove button (X)
        int btnX = lx + contentRect.w - 30;
        gfx->DrawRect(btnX, y + 3, 16, 16, 100, 50, 50);
        gfx->RenderText("X", btnX + 4, y + 4, 255, 180, 180);
        y += 25;
    }
    // Add component button
    gfx->DrawRect(lx + 4, y, contentRect.w - 10, 22, 40, 55, 40);
    gfx->DrawRect(lx + 4, y, contentRect.w - 10, 1, 60, 80, 55);
    gfx->RenderText("+ Add Component", lx + 10, y + 3, 140, 200, 140);
    y += 24;
    // Component add dropdown
    if (compAddOpen) {
        std::vector<std::string> compTypes = {"Sprite", "Text", "Collider", "Audio"};
        gfx->DrawRect(lx + 6, y, contentRect.w - 14, (int)compTypes.size() * 22, 22, 24, 34);
        gfx->DrawRect(lx + 6, y, contentRect.w - 14, 1, 50, 55, 70);
        for (int i = 0; i < (int)compTypes.size(); i++) {
            gfx->DrawRect(lx + 8, y + i * 22, contentRect.w - 18, 20, 35, 40, 52);
            gfx->DrawRect(lx + 8, y + i * 22, 3, 20, 74, 125, 180);
            gfx->RenderText(compTypes[i], lx + 16, y + i * 22 + 2, 200, 200, 220);
        }
    }
}

bool InspectorPanel::HandleClick(int mx, int my) {
    if (!contentRect.Contains(mx, my)) { editingField = 0; return false; }
    if (!target) return true;
    int y = contentRect.y + 6, lx = contentRect.x + 6;
    auto fldR = [&](int, int y) -> Rect { return {lx + 61, y, contentRect.w - 70, 20}; };
    if (fldR(1, y).Contains(mx, my)) { editingField = 1; editBuf = target->name; return true; } y += 26;
    y += 26;
    if (fldR(2, y).Contains(mx, my)) { editingField = 2; editBuf = std::to_string(target->posX); return true; } y += 26;
    if (fldR(3, y).Contains(mx, my)) { editingField = 3; editBuf = std::to_string(target->posY); return true; } y += 26;
    if (fldR(4, y).Contains(mx, my)) { editingField = 4; editBuf = std::to_string(target->sizeW); return true; } y += 26;
    if (fldR(5, y).Contains(mx, my)) { editingField = 5; editBuf = std::to_string(target->sizeH); return true; } y += 26;
    if (fldR(6, y).Contains(mx, my)) { editingField = 6; editBuf = target->spritePath; return true; } y += 26;
    // Color swatch click → open ChooseColor dialog
    Rect colorSwatch = {lx + 55, y, 20, 20};
    if (colorSwatch.Contains(mx, my)) {
        static COLORREF custColors[16] = {};
        CHOOSECOLORA cc = {}; cc.lStructSize = sizeof(cc); cc.hwndOwner = GetActiveWindow();
        cc.rgbResult = RGB(target->colorR, target->colorG, target->colorB);
        cc.lpCustColors = custColors; cc.Flags = CC_RGBINIT | CC_FULLOPEN;
        if (ChooseColorA(&cc)) {
            target->colorR = GetRValue(cc.rgbResult);
            target->colorG = GetGValue(cc.rgbResult);
            target->colorB = GetBValue(cc.rgbResult);
        }
        editingField = 0; return true;
    }
    int cs = contentRect.w - 70;
    Rect rf7 = {lx + 80, y, cs/3 - 5, 20}; Rect rf8 = {lx + 80 + cs/3, y, cs/3 - 5, 20}; Rect rf9 = {lx + 80 + cs*2/3, y, cs/3 - 5, 20};
    if (rf7.Contains(mx, my)) { editingField = 7; editBuf = std::to_string(target->colorR); return true; }
    if (rf8.Contains(mx, my)) { editingField = 8; editBuf = std::to_string(target->colorG); return true; }
    if (rf9.Contains(mx, my)) { editingField = 9; editBuf = std::to_string(target->colorB); return true; }
    y += 26;
    if (fldR(10, y).Contains(mx, my)) { editingField = 10; editBuf = std::to_string(target->startTick); return true; } y += 26;
    if (fldR(11, y).Contains(mx, my)) { editingField = 11; editBuf = std::to_string(target->endTick); return true; } y += 26;
    // Visibility line
    y += 26;
    // Component section: skip header
    y += 22;
    // Component remove buttons
    for (int i = 0; i < (int)target->components.size(); i++) {
        int btnX = lx + contentRect.w - 30;
        Rect btnRect = {btnX, y + 3, 16, 16};
        if (btnRect.Contains(mx, my)) {
            target->RemoveComponent(target->components[i]);
            editingField = 0; return true;
        }
        y += 25;
    }
    // Add component button
    Rect addBtn = {lx + 4, y, contentRect.w - 10, 22};
    if (addBtn.Contains(mx, my)) { compAddOpen = !compAddOpen; editingField = 0; return true; }
    y += 24;
    // Component add menu
    if (compAddOpen) {
        std::vector<std::string> compTypes = {"Sprite", "Text", "Collider", "Audio"};
        for (int i = 0; i < (int)compTypes.size(); i++) {
            int ciy = y + i * 22;
            if (mx >= lx + 8 && mx <= lx + contentRect.w - 18 && my >= ciy && my <= ciy + 20) {
                target->AddComponent(compTypes[i]);
                compAddOpen = false;
                editingField = 0; return true;
            }
        }
        // Click outside menu → close
        if (!addBtn.Contains(mx, my)) { compAddOpen = false; }
    }
    editingField = 0;
    return true;
}

void InspectorPanel::CommitEdit() {
    if (editingField == 0 || !target) return;
    if (editingField == 1) target->name = editBuf;
    else if (editingField == 2) target->posX = atoi(editBuf.c_str());
    else if (editingField == 3) target->posY = atoi(editBuf.c_str());
    else if (editingField == 4) target->sizeW = atoi(editBuf.c_str());
    else if (editingField == 5) target->sizeH = atoi(editBuf.c_str());
    else if (editingField == 6) target->spritePath = editBuf;
    else if (editingField == 7) { target->colorR = std::max(0, std::min(255, atoi(editBuf.c_str()))); }
    else if (editingField == 8) { target->colorG = std::max(0, std::min(255, atoi(editBuf.c_str()))); }
    else if (editingField == 9) { target->colorB = std::max(0, std::min(255, atoi(editBuf.c_str()))); }
    else if (editingField == 10) { target->startTick = std::max(0, atoi(editBuf.c_str())); }
    else if (editingField == 11) { target->endTick = std::max(1, atoi(editBuf.c_str())); }
    editingField = 0;
}

void InspectorPanel::SetTarget(EditorObject* obj) { target = obj; editingField = 0; }

// ========== ASSET BROWSER ==========
AssetBrowserPanel::AssetBrowserPanel() : EditorPanel("Asset Browser"), scrollOffset(0), selectedAsset(false), selAssetIndex(-1), currentDir("assets/Images"), editingField(0), editingIndex(-1) {}

void AssetBrowserPanel::CancelSelect() { selectedAsset = false; selAssetIndex = -1; selAssetPath.clear(); }

void AssetBrowserPanel::ScanDirectory(const std::string& dir) {
    printf("[ScanDir] %s cwd=", dir.c_str());
    char cwd[260]; GetCurrentDirectoryA(260, cwd); printf("%s\n", cwd);
    files.clear();
    if (!fs::exists(dir)) {
        printf("[ScanDir] dir does not exist! Attempting to create...\n");
        fs::create_directories(dir);
        if (!fs::exists(dir)) { printf("[ScanDir] could not create directory!\n"); return; }
    }
    // Add ".." if not at root
    if (dir != "assets/Images") files.push_back("..");
    int count = 0;
    for (auto& entry : fs::directory_iterator(dir)) {
        std::string name = entry.path().filename().string();
        files.push_back(name);
        count++;
    }
    std::sort(files.begin() + (files.size() - count), files.end());
    printf("[ScanDir] found %d entries in %s:\n", count, dir.c_str());
    for (auto& f : files) printf("  %s\n", f.c_str());
}

void AssetBrowserPanel::ScanDirectory() { ScanDirectory(currentDir); }

int AssetBrowserPanel::GetFileIndexAt(int mx, int my) const {
    if (!contentRect.Contains(mx, my)) return -1;
    int cols = std::max(1, contentRect.w / 90), cellW = contentRect.w / cols;
    int y = contentRect.y + 6, idx = scrollOffset;
    while (idx < (int)files.size()) {
        for (int c = 0; c < cols && idx < (int)files.size(); c++, idx++) {
            int x = contentRect.x + c * cellW + 4;
            if (mx >= x && mx <= x + cellW - 8 && my >= y && my <= y + 70) return idx;
        }
        y += 78;
        if (y > contentRect.y + contentRect.h) break;
    }
    return -1;
}

void AssetBrowserPanel::Draw(GraphicsAPI* gfx) {
    DrawFrame(gfx);
    // Current directory path with darker background bar
    int barH = 20;
    gfx->DrawRect(contentRect.x, contentRect.y, contentRect.w, barH, 22, 24, 32);
    gfx->DrawRect(contentRect.x, contentRect.y + barH, contentRect.w, 1, 35, 38, 45);
    gfx->RenderText(currentDir, contentRect.x + 4, contentRect.y + 2, 160, 160, 180);
    int startY = contentRect.y + barH + 4;
    int cols = std::max(1, contentRect.w / 90), cellW = contentRect.w / cols;
    int y = startY, idx = scrollOffset;
    while (idx < (int)files.size()) {
        for (int c = 0; c < cols && idx < (int)files.size(); c++, idx++) {
            int x = contentRect.x + c * cellW + 4;
            std::string fullPath = currentDir + "/" + files[idx];
            bool isDir = (files[idx] == "..") || fs::is_directory(fullPath);
            bool sel = (selectedAsset && idx == selAssetIndex);
            // Thumbnail card background
            if (sel && !isDir) {
                gfx->DrawRect(x - 1, y - 1, cellW - 6, 72, 50, 70, 50);
                gfx->DrawRect(x - 1, y - 1, cellW - 6, 72, 74, 180, 100);
            } else {
                gfx->DrawRect(x, y, cellW - 8, 70, 20, 22, 30);
                gfx->DrawRect(x, y, cellW - 8, 1, 40, 42, 50);
                gfx->DrawRect(x, y + 69, cellW - 8, 1, 35, 38, 45);
            }
            if (isDir) {
                gfx->DrawRect(x + 4, y + 4, 16, 16, 60, 100, 160);
                gfx->DrawRect(x + 4, y + 4, 16, 1, 80, 140, 200);
                gfx->RenderText("[DIR]", x + 24, y + 4, 100, 180, 255);
            } else {
                std::string ext = files[idx].substr(files[idx].find_last_of(".") + 1);
                for (auto& c : ext) c = (char)tolower(c);
                if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" || ext == "gif")
                    gfx->DrawImage(x + 2, y + 2, cellW - 12, 48, fullPath);
            }
            // Rename editing
            if (editingField == 1 && editingIndex == idx) {
                gfx->DrawRect(x + 2, y + 28, cellW - 12, 20, 60, 50, 35);
                gfx->RenderText(editBuf + "|", x + 6, y + 30, 255, 255, 200);
            } else {
                std::string n = files[idx]; if (n.size() > 12) n = n.substr(0, 11) + ".";
                gfx->RenderText(n, x + 4, y + (isDir ? 26 : 52), 180, 180, 200);
            }
        }
        y += 78;
        if (y > contentRect.y + contentRect.h) break;
    }
    if (files.empty()) gfx->RenderText("No assets in " + currentDir, contentRect.x + 6, contentRect.y + 6, 80, 80, 100);
    int totalRows = ((int)files.size() + cols - 1) / cols;
    int curRow = scrollOffset / cols;
    DrawScrollbar(gfx, contentRect.x, contentRect.y, contentRect.w, contentRect.h - 6, curRow, totalRows, 78);
}

bool AssetBrowserPanel::HandleClick(int mx, int my) {
    int idx = GetFileIndexAt(mx, my);
    if (idx >= 0) {
        std::string fullPath = currentDir + "/" + files[idx];
        if (files[idx] == "..") {
            size_t pos = currentDir.find_last_of("/\\");
            if (pos != std::string::npos) { currentDir = currentDir.substr(0, pos); ScanDirectory(); }
            return true;
        }
        if (fs::is_directory(fullPath)) {
            currentDir = fullPath;
            ScanDirectory();
            return true;
        }
        selectedAsset = true; selAssetIndex = idx; selAssetPath = fullPath; return true;
    }
    CancelSelect();
    return contentRect.Contains(mx, my);
}

void AssetBrowserPanel::HandleKey(int key) {
    if (editingField == 0) return;
    if (key == VK_RETURN || key == VK_ESCAPE) {
        if (editingField == 1 && editingIndex >= 0 && editingIndex < (int)files.size() && !editBuf.empty()) {
            std::string oldPath = currentDir + "/" + files[editingIndex];
            std::string newPath = currentDir + "/" + editBuf;
            if (key == VK_RETURN && oldPath != newPath) {
                fs::rename(oldPath, newPath);
                ScanDirectory();
            }
        }
        editingField = 0; editingIndex = -1;
        return;
    }
    if (key == VK_BACK && !editBuf.empty()) { editBuf.pop_back(); }
}

// ========== SCENE PANEL ==========
ScenePanel::ScenePanel() : EditorPanel("Scene"), selectedIndex(-1), activeGizmo(GIZMO_NONE), zoom(1.0f), panX(0), panY(0), showGrid(true), snapEnabled(false) {}

void ScenePanel::DrawGrid(GraphicsAPI* gfx) {
    int gridSize = std::max(10, (int)(40 * zoom));
    int ox = contentRect.x + (panX % gridSize), oy = contentRect.y + (panY % gridSize);
    for (int x = ox; x < contentRect.x + contentRect.w; x += gridSize)
        gfx->DrawRect(x, contentRect.y, 1, contentRect.h, 35, 35, 45);
    for (int y = oy; y < contentRect.y + contentRect.h; y += gridSize)
        gfx->DrawRect(contentRect.x, y, contentRect.w, 1, 35, 35, 45);
}

GizmoPart ScenePanel::HitTestGizmo(int mx, int my, EditorObject* obj) {
    if (!obj) return GIZMO_NONE;
    int dx = contentRect.x + (int)(obj->posX * zoom) + panX, dy = contentRect.y + (int)(obj->posY * zoom) + panY;
    int sw = (int)(obj->sizeW * zoom), sh = (int)(obj->sizeH * zoom), hs = 8;
    if (Rect{dx + sw/2 - 12, dy - 20, 24, 16}.Contains(mx, my)) return GIZMO_MOVE;
    if (Rect{dx - hs, dy - hs, hs*2, hs*2}.Contains(mx, my)) return GIZMO_RESIZE_TL;
    if (Rect{dx + sw - hs, dy - hs, hs*2, hs*2}.Contains(mx, my)) return GIZMO_RESIZE_TR;
    if (Rect{dx - hs, dy + sh - hs, hs*2, hs*2}.Contains(mx, my)) return GIZMO_RESIZE_BL;
    if (Rect{dx + sw - hs, dy + sh - hs, hs*2, hs*2}.Contains(mx, my)) return GIZMO_RESIZE_BR;
    if (mx >= dx && mx <= dx + sw && my >= dy && my <= dy + sh) return GIZMO_MOVE;
    return GIZMO_NONE;
}

void ScenePanel::DrawGizmo(GraphicsAPI* gfx, EditorObject* obj) {
    if (!obj) return;
    int dx = contentRect.x + (int)(obj->posX * zoom) + panX, dy = contentRect.y + (int)(obj->posY * zoom) + panY;
    int sw = (int)(obj->sizeW * zoom), sh = (int)(obj->sizeH * zoom), hs = 6;
    // Selection outline with dashed-like effect
    gfx->DrawRect(dx - 2, dy - 2, sw + 4, 2, 255, 200, 50);
    gfx->DrawRect(dx - 2, dy + sh, sw + 4, 2, 255, 200, 50);
    gfx->DrawRect(dx - 2, dy, 2, sh, 255, 200, 50);
    gfx->DrawRect(dx + sw, dy, 2, sh, 255, 200, 50);
    // Corner brackets
    int bl = 8;
    gfx->DrawRect(dx + 2, dy - 2, bl, 2, 255, 240, 180);
    gfx->DrawRect(dx + sw - bl, dy - 2, bl, 2, 255, 240, 180);
    gfx->DrawRect(dx + 2, dy + sh, bl, 2, 255, 240, 180);
    gfx->DrawRect(dx + sw - bl, dy + sh, bl, 2, 255, 240, 180);
    // Move handle (filled)
    gfx->DrawRect(dx + sw/2 - 10, dy - 16, 20, 12, 60, 80, 120);
    gfx->DrawRect(dx + sw/2 - 10, dy - 16, 20, 1, 74, 125, 180);
    gfx->RenderText("M", dx + sw/2 - 4, dy - 15, 255, 220, 100);
    // Resize handles (filled)
    gfx->DrawRect(dx - hs, dy - hs, hs*2, hs*2, 50, 60, 80);
    gfx->DrawRect(dx + sw - hs, dy - hs, hs*2, hs*2, 50, 60, 80);
    gfx->DrawRect(dx - hs, dy + sh - hs, hs*2, hs*2, 50, 60, 80);
    gfx->DrawRect(dx + sw - hs, dy + sh - hs, hs*2, hs*2, 50, 60, 80);
}

void ScenePanel::Draw(GraphicsAPI* gfx) {
    DrawFrame(gfx);
    gfx->DrawRect(contentRect.x, contentRect.y, contentRect.w, contentRect.h, 20, 20, 30);
    if (showGrid) DrawGrid(gfx);
    // Zoom badge with tool buttons
    int zx = contentRect.x + contentRect.w - 70, zy = contentRect.y + 4;
    gfx->DrawRect(zx, zy, 62, 18, 18, 19, 24);
    gfx->DrawRect(zx, zy, 62, 1, 35, 38, 45);
    char zb[16]; snprintf(zb, 16, "%.0f%%", zoom * 100.0f);
    gfx->RenderText(zb, zx + 4, zy + 2, 140, 140, 160);
    DrawToolbar(gfx);
}

void ScenePanel::DrawToolbar(GraphicsAPI* gfx) {
    int tby = contentRect.y + contentRect.h - 22;
    gfx->DrawRect(contentRect.x, tby, contentRect.w, 22, 16, 17, 22);
    gfx->DrawRect(contentRect.x, tby, contentRect.w, 1, 30, 35, 45);
    auto tbBtn = [&](int x, const char* label, int r, int g, int b, bool active = false) {
        int bx = contentRect.x + x + 2, by = tby + 2, bw = 36, bh = 18;
        if (active) {
            gfx->DrawRect(bx, by, bw, bh, 45, 60, 80);
            gfx->DrawRect(bx, by, bw, 1, 74, 125, 180);
        } else {
            gfx->DrawRect(bx, by, bw, bh, 25, 27, 35);
            gfx->DrawRect(bx, by, bw, 1, 40, 42, 50);
        }
        gfx->RenderText(label, bx + 4, by + 2, r, g, b);
    };
    auto sep = [&](int x) { gfx->DrawRect(contentRect.x + x, tby + 3, 1, 16, 40, 42, 50); };
    tbBtn(2, "[-]", 180, 180, 200); tbBtn(40, "[+]", 180, 180, 200); tbBtn(78, "Fit", 140, 200, 140);
    sep(122);
    tbBtn(126, "Grid", showGrid ? 150 : 100, showGrid ? 210 : 100, showGrid ? 150 : 100, showGrid);
    tbBtn(164, "Snap", snapEnabled ? 210 : 100, snapEnabled ? 180 : 100, snapEnabled ? 100 : 100, snapEnabled);
    sep(208);
    tbBtn(212, "L", 200, 200, 200); tbBtn(250, "H", 200, 200, 200); tbBtn(288, "R", 200, 200, 200);
    tbBtn(326, "T", 200, 200, 200); tbBtn(364, "V", 200, 200, 200); tbBtn(402, "B", 200, 200, 200);
    sep(446);
    tbBtn(450, "Copy", 160, 160, 200); tbBtn(488, "Paste", 160, 200, 160);
}

int ScenePanel::HitToolbar(int mx, int my) {
    int tby = contentRect.y + contentRect.h - 22;
    if (my < tby || my > tby + 22) return ST_NONE;
    int rx = mx - contentRect.x;
    auto hit = [&](int start, int w, SceneTool t) { return (rx >= start && rx < start + w) ? t : ST_NONE; };
    SceneTool t;
    t = hit(2, 36, ST_ZOOM_OUT); if (t != ST_NONE) return t;
    t = hit(40, 36, ST_ZOOM_IN); if (t != ST_NONE) return t;
    t = hit(78, 36, ST_FIT); if (t != ST_NONE) return t;
    t = hit(126, 36, ST_GRID); if (t != ST_NONE) return t;
    t = hit(164, 36, ST_SNAP); if (t != ST_NONE) return t;
    t = hit(212, 36, ST_ALIGN_L); if (t != ST_NONE) return t;
    t = hit(250, 36, ST_ALIGN_H); if (t != ST_NONE) return t;
    t = hit(288, 36, ST_ALIGN_R); if (t != ST_NONE) return t;
    t = hit(326, 36, ST_ALIGN_T); if (t != ST_NONE) return t;
    t = hit(364, 36, ST_ALIGN_V); if (t != ST_NONE) return t;
    t = hit(402, 36, ST_ALIGN_B); if (t != ST_NONE) return t;
    t = hit(450, 36, ST_COPY); if (t != ST_NONE) return t;
    t = hit(488, 36, ST_PASTE); if (t != ST_NONE) return t;
    return ST_NONE;
}

void ScenePanel::HandleScroll(int delta) {
    if (delta > 0) zoom *= 1.1f; else zoom /= 1.1f;
    if (zoom < 0.1f) zoom = 0.1f;
    if (zoom > 5.0f) zoom = 5.0f;
}

bool ScenePanel::HandleClick(int mx, int my) {
    if (!contentRect.Contains(mx, my)) { activeGizmo = GIZMO_NONE; return false; }
    return true;
}

void ScenePanel::HandleDrag(int /*mx*/, int /*my*/) {
}

// ========== SCRIPT PANEL ==========
ScriptPanel::ScriptPanel() : EditorPanel("Script"), selectedLine(-1), scrollOffset(0), editingLine(-1), editingField(0) {}

void ScriptPanel::Draw(GraphicsAPI* gfx) {
    DrawFrame(gfx);
    int y = contentRect.y + 4;
    for (int i = scrollOffset; i < (int)lines.size(); i++) {
        int ih = 88;
        if (y + ih > contentRect.y + contentRect.h) break;
        // Card-like background
        bool sel = (i == selectedLine);
        if (sel) {
            gfx->DrawRect(contentRect.x + 2, y, contentRect.w - 4, ih, 35, 45, 70);
            gfx->DrawRect(contentRect.x + 2, y, 2, ih, 74, 125, 180);
        } else {
            gfx->DrawRect(contentRect.x + 2, y, contentRect.w - 4, ih, 20, 22, 30);
            gfx->DrawRect(contentRect.x + 2, y, contentRect.w - 4, 1, 30, 33, 42);
        }
        // Tick badge
        gfx->DrawRect(contentRect.x + contentRect.w - 52, y + 2, 40, 16, 40, 45, 60);
        gfx->RenderText("T:" + std::to_string(lines[i].tick), contentRect.x + contentRect.w - 48, y + 3, 140, 140, 160);
        // Character name area
        int chw = std::min(200, contentRect.w / 2);
        bool editingChar = (editingField == 1 && editingLine == i);
        if (editingChar) {
            gfx->DrawRect(contentRect.x + 12, y + 2, chw, 20, 60, 50, 35);
            gfx->RenderText(editBuf + "|", contentRect.x + 16, y + 3, 255, 255, 200);
        } else {
            gfx->RenderText(lines[i].characterName.empty() ? "(narrator)" : lines[i].characterName, contentRect.x + 14, y + 3, 220, 200, 150);
        }
        // Dialogue text area
        bool editingText = (editingField == 2 && editingLine == i);
        if (editingText) {
            gfx->DrawRect(contentRect.x + 12, y + 24, contentRect.w - 28, 36, 60, 50, 35);
            gfx->RenderText(editBuf + "|", contentRect.x + 16, y + 26, 255, 255, 200);
        } else {
            std::string t = lines[i].text; if (t.size() > 40) t = t.substr(0, 39) + "...";
            gfx->DrawRect(contentRect.x + 12, y + 24, contentRect.w - 28, 34, 25, 27, 35);
            gfx->RenderText(t, contentRect.x + 16, y + 28, 200, 200, 200);
        }
        // Sprite and bg path on bottom row
        std::string sp = lines[i].spritePath.empty() ? "" : lines[i].spritePath;
        std::string bg = lines[i].backgroundPath.empty() ? "" : lines[i].backgroundPath;
        if (sp.size() > 18) sp = sp.substr(0, 17) + ".";
        if (bg.size() > 18) bg = bg.substr(0, 17) + ".";
        gfx->RenderText("SPR:" + sp, contentRect.x + 14, y + 62, sp.empty() ? 80 : 160, sp.empty() ? 80 : 160, sp.empty() ? 80 : 180);
        gfx->RenderText("BG:" + bg, contentRect.x + contentRect.w / 2 + 4, y + 62, bg.empty() ? 80 : 140, bg.empty() ? 80 : 140, bg.empty() ? 80 : 180);
        y += ih + 2;
    }
    if (lines.empty()) {
        gfx->RenderText("No script lines. Click +Add or", contentRect.x + 8, contentRect.y + 6, 80, 80, 100);
        gfx->RenderText("RClick object > Create Script Line", contentRect.x + 8, contentRect.y + 24, 80, 80, 100);
    }
    DrawScrollbar(gfx, contentRect.x, contentRect.y, contentRect.w, contentRect.h - 6, scrollOffset, (int)lines.size(), 90);
    int tby = rect.y + rect.h - 24;
    gfx->DrawRect(rect.x + 1, tby, rect.w - 2, 23, 25, 27, 35);
    gfx->DrawRect(rect.x + 1, tby, rect.w - 2, 1, 40, 45, 55);
    int bw2 = rect.w / 4;
    auto drawBtn = [&](int x, const char* label, int r, int g, int b) {
        gfx->DrawRect(x, tby + 2, bw2 - 2, 20, 35, 40, 50);
        gfx->RenderText(label, x + 4, tby + 3, r, g, b);
    };
    drawBtn(rect.x + 1, "+Add", 140, 200, 140);
    drawBtn(rect.x + 3 + bw2, "Del", 200, 140, 140);
    drawBtn(rect.x + 5 + bw2*2, "Up", 140, 140, 200);
    drawBtn(rect.x + 7 + bw2*3, "Dn", 140, 140, 200);
}

bool ScriptPanel::HandleClick(int mx, int my) {
    int tby = rect.y + rect.h - 24, bw2 = rect.w / 4;
    if (my >= tby && my <= tby + 23 && mx >= rect.x && mx <= rect.x + rect.w) {
        if (mx < rect.x + bw2) { editingField = 0; AddLine(); }
        else if (mx < rect.x + bw2*2) { editingField = 0; DeleteLine(); }
        else if (mx < rect.x + bw2*3) { editingField = 0; MoveUp(); }
        else { editingField = 0; MoveDown(); }
        return false;
    }
    if (!contentRect.Contains(mx, my)) { editingField = 0; return false; }
    int y = contentRect.y + 4;
    for (int i = scrollOffset; i < (int)lines.size(); i++) {
        int ih = 88;
        if (y + ih > contentRect.y + contentRect.h) break;
        if (mx >= contentRect.x + 2 && mx <= contentRect.x + contentRect.w - 2 && my >= y && my <= y + ih) {
            selectedLine = i;
            int chw = std::min(200, contentRect.w / 2);
            // Character name area (top)
            if (my >= y + 2 && my <= y + 20 && mx >= contentRect.x + 12 && mx <= contentRect.x + 12 + chw) {
                editingLine = i; editingField = 1; editBuf = lines[i].characterName;
            } else if (my >= y + 24 && my <= y + 60) {
                editingLine = i; editingField = 2; editBuf = lines[i].text;
            } else if (my >= y + 62 && my <= y + 82) {
                if (mx < contentRect.x + contentRect.w / 2) {
                    // Click sprite area → open file dialog
                    char fileBuf[260] = {}; OPENFILENAMEA ofn = {};
                    ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = GetActiveWindow();
                    ofn.lpstrFile = fileBuf; ofn.nMaxFile = 260;
                    ofn.lpstrFilter = "Images (*.png;*.jpg;*.bmp)\0*.png;*.jpg;*.jpeg;*.bmp\0All\0*.*\0";
                    ofn.Flags = OFN_FILEMUSTEXIST;
                    if (GetOpenFileNameA(&ofn)) { lines[i].spritePath = fileBuf; }
                } else {
                    editingLine = i; editingField = 4; editBuf = lines[i].backgroundPath;
                }
            } else {
                editingField = 0;
            }
            return true;
        }
        y += ih + 2;
    }
    selectedLine = -1; editingField = 0; return true;
}

void ScriptPanel::AddLine() {
    ScriptLine sl; sl.text = "New dialogue"; sl.characterName = "Char";
    sl.tick = (int)lines.size();
    lines.push_back(sl); selectedLine = (int)lines.size() - 1;
}

void ScriptPanel::DeleteLine() {
    if (selectedLine < 0 || selectedLine >= (int)lines.size()) return;
    lines.erase(lines.begin() + selectedLine);
    if (selectedLine >= (int)lines.size()) selectedLine = (int)lines.size() - 1;
}

void ScriptPanel::MoveUp() {
    if (selectedLine <= 0) return;
    std::swap(lines[selectedLine], lines[selectedLine - 1]); selectedLine--;
}

void ScriptPanel::MoveDown() {
    if (selectedLine < 0 || selectedLine >= (int)lines.size() - 1) return;
    std::swap(lines[selectedLine], lines[selectedLine + 1]); selectedLine++;
}

void ScriptPanel::HandleKey(int key) {
    if (editingField == 0) return;
    if (key == VK_RETURN || key == VK_ESCAPE) {
        if (editingLine >= 0 && editingLine < (int)lines.size()) {
            if (editingField == 1) lines[editingLine].characterName = editBuf;
            else if (editingField == 2) lines[editingLine].text = editBuf;
            else if (editingField == 3) lines[editingLine].spritePath = editBuf;
            else if (editingField == 4) lines[editingLine].backgroundPath = editBuf;
        }
        editingField = 0;
        return;
    }
    if (key == VK_BACK && !editBuf.empty()) {
        editBuf.pop_back();
        if (editingLine >= 0 && editingLine < (int)lines.size()) {
            if (editingField == 1) lines[editingLine].characterName = editBuf;
            else if (editingField == 2) lines[editingLine].text = editBuf;
            else if (editingField == 3) lines[editingLine].spritePath = editBuf;
            else if (editingField == 4) lines[editingLine].backgroundPath = editBuf;
        }
    }
}

// ========== TIMELINE ==========
TimelinePanel::TimelinePanel() : EditorPanel("Timeline"), scrollOffset(0), dragMode(0), dragObj(-1), dragStartTick(0), dragEndTick(0), dragMouseStartX(0) {}

void TimelinePanel::Draw(GraphicsAPI* gfx) { DrawFrame(gfx); }

void TimelinePanel::Draw(GraphicsAPI* gfx, const std::vector<EditorObject>& objects, int selectedIndex, int currentTick) {
    DrawFrame(gfx);
    gfx->DrawRect(contentRect.x, contentRect.y, contentRect.w, contentRect.h, 18, 19, 24);
    int headerH = 20, rowH = 22, tickW = 24;
    int y = contentRect.y + headerH;
    int viewTicks = contentRect.w / tickW;
    // Header
    gfx->DrawRect(contentRect.x, contentRect.y, contentRect.w, headerH, 25, 27, 35);
    for (int t = 0; t < viewTicks + 2; t++) {
        int tx = contentRect.x + 4 + t * tickW;
        gfx->RenderText(std::to_string(t + scrollOffset), tx, contentRect.y + 3, 120, 120, 140);
        // Tick separator line
        if (t > 0) gfx->DrawRect(tx - 4, contentRect.y + headerH, 1, contentRect.h - headerH, 25, 27, 35);
    }
    // Current tick marker
    int cx = contentRect.x + 4 + (currentTick - scrollOffset) * tickW;
    if (cx >= contentRect.x && cx <= contentRect.x + contentRect.w) {
        gfx->DrawRect(cx, contentRect.y, 2, contentRect.h, 255, 200, 50);
        gfx->DrawRect(cx - 1, contentRect.y, 4, headerH, 255, 200, 50);
    }
    for (int i = 0; i < (int)objects.size(); i++) {
        int rowY = y + i * rowH;
        if (rowY + rowH > contentRect.y + contentRect.h) break;
        // Row background
        bool sel = (i == selectedIndex);
        if (sel) gfx->DrawRect(contentRect.x, rowY, contentRect.w, rowH - 1, 35, 45, 70);
        else if (i % 2 == 0) gfx->DrawRect(contentRect.x, rowY, contentRect.w, rowH - 1, 20, 21, 28);
        else gfx->DrawRect(contentRect.x, rowY, contentRect.w, rowH - 1, 16, 17, 22);
        // Object name
        std::string label = objects[i].name; if (label.size() > 12) label = label.substr(0, 11) + ".";
        gfx->RenderText(label, contentRect.x + 4, rowY + 2, sel ? 220 : 180, sel ? 220 : 180, sel ? 230 : 200);
        // Duration bar
        int barX = contentRect.x + 4 + (objects[i].startTick - scrollOffset) * tickW;
        int barW = (objects[i].endTick - objects[i].startTick) * tickW;
        if (barW < 4) barW = 4;
        if (barX + barW >= contentRect.x && barX <= contentRect.x + contentRect.w) {
            int clipX = barX < contentRect.x ? contentRect.x : barX;
            int clipW = (barX + barW > contentRect.x + contentRect.w) ? contentRect.x + contentRect.w - clipX : barW - (clipX - barX);
            gfx->DrawRect(clipX, rowY + 10, clipW, rowH - 14, objects[i].colorR, objects[i].colorG, objects[i].colorB);
            gfx->DrawRect(clipX, rowY + 10, clipW, rowH - 14, 200, 200, 200);
        }
    }
}

bool TimelinePanel::HandleClick(int mx, int my) { return contentRect.Contains(mx, my); }

bool TimelinePanel::HandleClick(int mx, int my, std::vector<EditorObject>& objects, int& selectedIndex, int& currentTick) {
    if (!contentRect.Contains(mx, my)) { dragMode = 0; return false; }
    int headerH = 20, rowH = 22, tickW = 24, y = contentRect.y + headerH;
    if (my < contentRect.y + headerH) {
        int tick = (mx - contentRect.x - 4) / tickW + scrollOffset;
        if (tick >= 0) currentTick = tick;
        return true;
    }
    for (int i = 0; i < (int)objects.size(); i++) {
        int rowY = y + i * rowH;
        if (rowY + rowH > contentRect.y + contentRect.h) break;
        if (my >= rowY && my <= rowY + rowH - 1 && mx >= contentRect.x && mx <= contentRect.x + contentRect.w) {
            selectedIndex = i;
            int clickTick = (mx - contentRect.x - 4) / tickW + scrollOffset;
            if (clickTick >= 0) currentTick = clickTick;
            int barX = contentRect.x + 4 + (objects[i].startTick - scrollOffset) * tickW;
            int barW = (objects[i].endTick - objects[i].startTick) * tickW;
            if (mx >= barX && mx <= barX + 6) { dragMode = 2; dragObj = i; dragStartTick = objects[i].startTick; dragEndTick = objects[i].endTick; dragMouseStartX = mx; }
            else if (mx >= barX + barW - 6 && mx <= barX + barW) { dragMode = 3; dragObj = i; dragStartTick = objects[i].startTick; dragEndTick = objects[i].endTick; dragMouseStartX = mx; }
            else if (mx >= barX && mx <= barX + barW) { dragMode = 1; dragObj = i; dragStartTick = objects[i].startTick; dragEndTick = objects[i].endTick; dragMouseStartX = mx; }
            return true;
        }
    }
    return true;
}

void TimelinePanel::HandleDrag(int mx, int /*my*/, std::vector<EditorObject>& objects) {
    if (dragMode == 0 || dragObj < 0 || dragObj >= (int)objects.size()) { dragMode = 0; return; }
    int tickW = 24, dTick = (mx - dragMouseStartX) / tickW;
    auto& obj = objects[dragObj];
    if (dragMode == 1) {
        int newStart = dragStartTick + dTick; if (newStart < 0) newStart = 0;
        obj.startTick = newStart; obj.endTick = newStart + (dragEndTick - dragStartTick);
    } else if (dragMode == 2) {
        int newStart = dragStartTick + dTick; if (newStart < 0) newStart = 0;
        if (newStart < obj.endTick - 1) obj.startTick = newStart;
    } else if (dragMode == 3) {
        int newEnd = dragEndTick + dTick;
        if (newEnd <= obj.startTick + 1) newEnd = obj.startTick + 1;
        obj.endTick = newEnd;
    }
}

// ========== EDITOR WINDOW ==========
EditorWindow::EditorWindow()
    : panelSlots(),
      hierarchy(nullptr), inspector(nullptr), assetBrowser(nullptr), scenePanel(nullptr), scriptPanel(nullptr), timeline(nullptr),
      currentTick(0),
      dragSplitter(-1), leftRatio(0.20f), rightRatio(0.80f), bottomRatio(0.50f), timelineRatio(0.12f),
      activeMenu(-1), menuHovered(-1), showDebug(false),
      scenePanning(false), panStartX(0), panStartY(0), panStartPanX(0), panStartPanY(0), lastMsgTimer(0),
      previewWin(nullptr), snapSize(20), dirty(false) {
    DBG("=== EditorWindow constructor ===");
    RegisterPanel(new HierarchyPanel(), PANEL_HIERARCHY);
    RegisterPanel(new InspectorPanel(), PANEL_INSPECTOR);
    RegisterPanel(new ScenePanel(), PANEL_SCENE);
    RegisterPanel(new AssetBrowserPanel(), PANEL_ASSET_BROWSER);
    RegisterPanel(new ScriptPanel(), PANEL_SCRIPT);
    RegisterPanel(new TimelinePanel(), PANEL_TIMELINE);
    DBG("Panels registered: %zu", panelSlots.size());
    LoadRecentFiles();
    lastMsg = "EDITOR LOADED"; lastMsgTimer = 9999;
}

EditorWindow::~EditorWindow() {
    DBG("=== EditorWindow destructor ===");
    StopPreview();
    for (auto* slot : panelSlots) delete slot;
    panelSlots.clear();
}

void EditorWindow::RegisterPanel(EditorPanel* panel, int id) {
    DBG("RegisterPanel id=%d '%s'", id, panel->title.c_str());
    panel->id = id;
    while ((int)panelSlots.size() <= id) panelSlots.push_back(new EditorPanelSlot(nullptr));
    if (panelSlots[id]->panel) { DBG("WARN: overwriting panel slot %d", id); delete panelSlots[id]->panel; }
    panelSlots[id]->panel = panel;
    switch (id) {
        case PANEL_HIERARCHY: hierarchy = dynamic_cast<HierarchyPanel*>(panel); break;
        case PANEL_INSPECTOR: inspector = dynamic_cast<InspectorPanel*>(panel); break;
        case PANEL_SCENE: scenePanel = dynamic_cast<ScenePanel*>(panel); break;
        case PANEL_ASSET_BROWSER: assetBrowser = dynamic_cast<AssetBrowserPanel*>(panel); break;
        case PANEL_SCRIPT: scriptPanel = dynamic_cast<ScriptPanel*>(panel); break;
        case PANEL_TIMELINE: timeline = dynamic_cast<TimelinePanel*>(panel); break;
        default: DBG("WARN: unknown panel id %d", id); break;
    }
}

EditorPanel* EditorWindow::GetPanel(int id) const {
    if (id < 0 || id >= (int)panelSlots.size()) return nullptr;
    return panelSlots[id]->panel;
}

void EditorWindow::TogglePanel(int id) {
    EditorPanel* p = GetPanel(id);
    if (p) { p->visible = !p->visible; DBG("TogglePanel id=%d '%s' -> %s", id, p->title.c_str(), p->visible ? "visible" : "hidden"); }
}

void EditorWindow::MarkDirty() {
    if (!dirty) {
        dirty = true;
        lastMsg = "Project modified (unsaved)"; lastMsgTimer = 120;
    }
}

void EditorWindow::SaveCurrentProject() {
    if (currentProjectPath.empty()) {
        char fileBuf[256] = {};
        OPENFILENAMEA ofn = {}; ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = GetActiveWindow(); ofn.lpstrFile = fileBuf; ofn.nMaxFile = sizeof(fileBuf);
        ofn.lpstrFilter = "Visual Novel Engine (*.vne)\0*.vne\0All Files (*.*)\0*.*\0";
        ofn.lpstrDefExt = "vne"; ofn.Flags = OFN_OVERWRITEPROMPT;
        if (!GetSaveFileNameA(&ofn)) { lastMsg = "Save cancelled"; lastMsgTimer = 60; return; }
        currentProjectPath = fileBuf;
    }
    PushUndo();
    std::ofstream out(currentProjectPath);
    if (!out) { lastMsg = "FAILED to save: " + currentProjectPath; lastMsgTimer = 120; return; }
    out << hierarchy->objects.size() << "\n";
    for (auto& o : hierarchy->objects) {
        out << o.name << "\n" << o.type << "\n" << o.posX << " " << o.posY << " " << o.sizeW << " " << o.sizeH << " "
            << o.colorR << " " << o.colorG << " " << o.colorB << " " << (o.visible ? 1 : 0) << " "
            << o.startTick << " " << o.endTick << "\n" << o.text << "\n" << o.spritePath << "\n"
            << o.components.size() << "\n";
        for (auto& c : o.components) out << c << "\n";
    }
    out << scriptPanel->lines.size() << "\n";
    for (auto& l : scriptPanel->lines) {
        out << l.tick << "\n" << l.characterName << "\n" << l.text << "\n" << l.spritePath << "\n" << l.backgroundPath << "\n";
    }
    out.close();
    dirty = false;
    PushRecentFile(currentProjectPath);
    lastMsg = "Saved: " + currentProjectPath; lastMsgTimer = 120;
}

void EditorWindow::LoadProjectFrom(const std::string& path) {
    std::ifstream in(path);
    if (!in) { lastMsg = "FAILED to open: " + path; lastMsgTimer = 120; return; }
    PushUndo();
    hierarchy->objects.clear(); scriptPanel->lines.clear();
    currentProjectPath = path;
    int objCount; in >> objCount; in.ignore();
    for (int i = 0; i < objCount; i++) {
        EditorObject o;
        std::getline(in, o.name); std::getline(in, o.type);
        in >> o.posX >> o.posY >> o.sizeW >> o.sizeH >> o.colorR >> o.colorG >> o.colorB;
        int vis; in >> vis; o.visible = (vis != 0);
        in >> o.startTick >> o.endTick; in.ignore();
        std::getline(in, o.text); std::getline(in, o.spritePath);
        int compCount; in >> compCount; in.ignore();
        for (int j = 0; j < compCount; j++) { std::string c; std::getline(in, c); o.components.push_back(c); }
        hierarchy->objects.push_back(o);
    }
    int lineCount; in >> lineCount; in.ignore();
    for (int i = 0; i < lineCount; i++) {
        ScriptLine l; in >> l.tick; in.ignore();
        std::getline(in, l.characterName); std::getline(in, l.text);
        std::getline(in, l.spritePath); std::getline(in, l.backgroundPath);
        scriptPanel->lines.push_back(l);
    }
    in.close();
    dirty = false;
    hierarchy->selectedIndex = -1; hierarchy->SyncMulti();
    PushRecentFile(currentProjectPath);
    lastMsg = "Opened: " + path; lastMsgTimer = 120;
}

void EditorWindow::DuplicateSelected() {
    if (hierarchy->selectedIndex < 0 || hierarchy->selectedIndex >= (int)hierarchy->objects.size()) return;
    PushUndo();
    EditorObject c = hierarchy->objects[hierarchy->selectedIndex];
    c.name += " (copy)"; c.posX += 20; c.posY += 20;
    hierarchy->objects.push_back(c);
    hierarchy->selectedIndex = (int)hierarchy->objects.size() - 1;
    hierarchy->SyncMulti();
    MarkDirty();
}

void EditorWindow::MoveSelectedUp() {
    if (hierarchy->selectedIndex <= 0) return;
    PushUndo();
    std::swap(hierarchy->objects[hierarchy->selectedIndex], hierarchy->objects[hierarchy->selectedIndex - 1]);
    hierarchy->selectedIndex--;
    hierarchy->SyncMulti();
    MarkDirty();
}

void EditorWindow::MoveSelectedDown() {
    if (hierarchy->selectedIndex < 0 || hierarchy->selectedIndex >= (int)hierarchy->objects.size() - 1) return;
    PushUndo();
    std::swap(hierarchy->objects[hierarchy->selectedIndex], hierarchy->objects[hierarchy->selectedIndex + 1]);
    hierarchy->selectedIndex++;
    hierarchy->SyncMulti();
    MarkDirty();
}

void EditorWindow::BringToFront() {
    if (hierarchy->selectedIndex < 0 || hierarchy->selectedIndex >= (int)hierarchy->objects.size() - 1) return;
    PushUndo();
    EditorObject tmp = hierarchy->objects[hierarchy->selectedIndex];
    hierarchy->objects.erase(hierarchy->objects.begin() + hierarchy->selectedIndex);
    hierarchy->objects.push_back(tmp);
    hierarchy->selectedIndex = (int)hierarchy->objects.size() - 1;
    hierarchy->SyncMulti();
    MarkDirty();
}

void EditorWindow::SendToBack() {
    if (hierarchy->selectedIndex <= 0) return;
    PushUndo();
    EditorObject tmp = hierarchy->objects[hierarchy->selectedIndex];
    hierarchy->objects.erase(hierarchy->objects.begin() + hierarchy->selectedIndex);
    hierarchy->objects.insert(hierarchy->objects.begin(), tmp);
    hierarchy->selectedIndex = 0;
    hierarchy->SyncMulti();
    MarkDirty();
}

void EditorWindow::ZoomToFitScene() {
    if (hierarchy->objects.empty()) { scenePanel->zoom = 1.0f; scenePanel->panX = 0; scenePanel->panY = 0; return; }
    int minX = 99999, minY = 99999, maxX = 0, maxY = 0;
    for (auto& o : hierarchy->objects) {
        if (o.posX < minX) minX = o.posX;
        if (o.posY < minY) minY = o.posY;
        if (o.posX + o.sizeW > maxX) maxX = o.posX + o.sizeW;
        if (o.posY + o.sizeH > maxY) maxY = o.posY + o.sizeH;
    }
    int cw = scenePanel->contentRect.w, ch = scenePanel->contentRect.h;
    if (cw < 10 || ch < 10) return;
    float zx = (float)cw / (float)(maxX - minX + 50);
    float zy = (float)ch / (float)(maxY - minY + 50);
    scenePanel->zoom = std::min(zx, zy);
    if (scenePanel->zoom < 0.1f) scenePanel->zoom = 0.1f;
    if (scenePanel->zoom > 5.0f) scenePanel->zoom = 5.0f;
    scenePanel->panX = (int)(-(minX - 20) * scenePanel->zoom) + (cw - (int)((maxX - minX + 40) * scenePanel->zoom)) / 2;
    scenePanel->panY = (int)(-(minY - 20) * scenePanel->zoom) + (ch - (int)((maxY - minY + 40) * scenePanel->zoom)) / 2;
    lastMsg = "Zoom to fit"; lastMsgTimer = 60;
}

void EditorWindow::ToggleGrid() {
    scenePanel->showGrid = !scenePanel->showGrid;
    lastMsg = scenePanel->showGrid ? "Grid: ON" : "Grid: OFF";
    lastMsgTimer = 60;
}

void EditorWindow::ToggleSnap() {
    scenePanel->snapEnabled = !scenePanel->snapEnabled;
    lastMsg = scenePanel->snapEnabled ? "Snap: ON (hold Shift to drag-snap)" : "Snap: OFF";
    lastMsgTimer = 60;
}

void EditorWindow::AlignSelected(int mode) {
    std::vector<int> idxs;
    if (!hierarchy->multiSel.empty()) idxs = hierarchy->multiSel;
    else if (hierarchy->selectedIndex >= 0 && hierarchy->selectedIndex < (int)hierarchy->objects.size()) idxs.push_back(hierarchy->selectedIndex);
    if (idxs.size() < 2) { lastMsg = "Align needs 2+ objects selected"; lastMsgTimer = 60; return; }
    PushUndo();
    int minX = 999999, minY = 999999, maxX = -999999, maxY = -999999;
    for (int i : idxs) {
        auto& o = hierarchy->objects[i];
        if (o.posX < minX) minX = o.posX;
        if (o.posY < minY) minY = o.posY;
        if (o.posX + o.sizeW > maxX) maxX = o.posX + o.sizeW;
        if (o.posY + o.sizeH > maxY) maxY = o.posY + o.sizeH;
    }
    for (int i : idxs) {
        auto& o = hierarchy->objects[i];
        switch (mode) {
            case ST_ALIGN_L: o.posX = minX; break;
            case ST_ALIGN_H: o.posX = (minX + maxX) / 2 - o.sizeW / 2; break;
            case ST_ALIGN_R: o.posX = maxX - o.sizeW; break;
            case ST_ALIGN_T: o.posY = minY; break;
            case ST_ALIGN_V: o.posY = (minY + maxY) / 2 - o.sizeH / 2; break;
            case ST_ALIGN_B: o.posY = maxY - o.sizeH; break;
            default: break;
        }
    }
    MarkDirty();
    lastMsg = "Aligned " + std::to_string(idxs.size()) + " objects";
    lastMsgTimer = 60;
}

void EditorWindow::CopySelected() {
    clipboard.clear();
    if (!hierarchy->multiSel.empty()) {
        for (int i : hierarchy->multiSel) if (i >= 0 && i < (int)hierarchy->objects.size()) clipboard.push_back(hierarchy->objects[i]);
    } else if (hierarchy->selectedIndex >= 0 && hierarchy->selectedIndex < (int)hierarchy->objects.size()) {
        clipboard.push_back(hierarchy->objects[hierarchy->selectedIndex]);
    }
    if (clipboard.empty()) { lastMsg = "Nothing to copy"; lastMsgTimer = 60; }
    else { lastMsg = "Copied " + std::to_string(clipboard.size()) + " object(s)"; lastMsgTimer = 60; }
}

void EditorWindow::PasteSelected() {
    if (clipboard.empty()) { lastMsg = "Clipboard empty"; lastMsgTimer = 60; return; }
    PushUndo();
    int start = (int)hierarchy->objects.size();
    for (auto c : clipboard) {
        c.posX += 20; c.posY += 20;
        c.name += " (copy)";
        hierarchy->objects.push_back(c);
    }
    hierarchy->selectedIndex = (int)hierarchy->objects.size() - 1;
    hierarchy->multiSel.clear();
    for (int i = start; i < (int)hierarchy->objects.size(); i++) hierarchy->multiSel.push_back(i);
    scenePanel->selectedIndex = hierarchy->selectedIndex;
    inspector->SetTarget(&hierarchy->objects[hierarchy->selectedIndex]);
    MarkDirty();
    lastMsg = "Pasted " + std::to_string(clipboard.size()) + " object(s)";
    lastMsgTimer = 60;
}

void EditorWindow::LoadRecentFiles() {
    recentFiles.clear();
    std::ifstream in("recent.dat");
    if (!in) return;
    std::string line;
    while (std::getline(in, line)) { if (!line.empty()) recentFiles.push_back(line); }
    in.close();
}

void EditorWindow::SaveRecentFiles() {
    std::ofstream out("recent.dat");
    if (!out) return;
    for (auto& p : recentFiles) out << p << "\n";
    out.close();
}

void EditorWindow::PushRecentFile(const std::string& path) {
    for (auto it = recentFiles.begin(); it != recentFiles.end(); ) {
        if (*it == path) it = recentFiles.erase(it); else ++it;
    }
    recentFiles.insert(recentFiles.begin(), path);
    if ((int)recentFiles.size() > 5) recentFiles.resize(5);
    SaveRecentFiles();
}

void EditorWindow::Layout(int winW, int winH) {
    int visSpl = 4, hitSpl = 8, menuH = 52;
    int timelineH = std::max(40, (int)(winH * timelineRatio));
    int vPos = (int)(winW * leftRatio);
    int vPos2 = (int)(winW * rightRatio);
    int hPos = (int)(winH * bottomRatio);
    if (vPos < 80) vPos = 80;
    if (vPos2 > winW - 80) vPos2 = winW - 80;
    if (vPos2 - vPos < 100) vPos2 = vPos + 100;
    if (hPos < 120) hPos = 120;
    if (hPos > winH - 80 - timelineH) hPos = winH - 80 - timelineH;
    int bottomH = winH - hPos - visSpl - timelineH;
    hierarchy->SetRect({0, menuH, vPos, hPos - menuH});
    scriptPanel->SetRect({0, hPos + visSpl, vPos, bottomH});
    scenePanel->SetRect({vPos + visSpl, menuH, vPos2 - vPos - visSpl, hPos - menuH});
    inspector->SetRect({vPos2 + visSpl, menuH, winW - vPos2 - visSpl, winH - menuH - timelineH});
    assetBrowser->SetRect({vPos + visSpl, hPos + visSpl, vPos2 - vPos - visSpl, bottomH});
    timeline->SetRect({0, winH - timelineH, winW, timelineH});
    splitterR1 = {vPos - 2, menuH, hitSpl, hPos - menuH};
    splitterR2 = {vPos2 - 2, menuH, hitSpl, winH - menuH - timelineH};
    splitterR3 = {0, hPos - 2, winW, hitSpl};
    splitterR4 = {0, winH - timelineH - 2, winW, hitSpl};
}

int EditorWindow::HitTestSplitter(int mx, int my) {
    if (splitterR1.Contains(mx, my)) return 1;
    if (splitterR2.Contains(mx, my)) return 2;
    if (splitterR3.Contains(mx, my)) return 3;
    if (splitterR4.Contains(mx, my)) return 4;
    return -1;
}

void EditorWindow::SyncSelection() {
    if (hierarchy->selectedIndex >= 0 && hierarchy->selectedIndex < (int)hierarchy->objects.size()) {
        scenePanel->selectedIndex = hierarchy->selectedIndex;
        inspector->SetTarget(&hierarchy->objects[hierarchy->selectedIndex]);
    } else if (scenePanel->selectedIndex >= 0 && scenePanel->selectedIndex < (int)hierarchy->objects.size()) {
        hierarchy->selectedIndex = scenePanel->selectedIndex;
        inspector->SetTarget(&hierarchy->objects[scenePanel->selectedIndex]);
    } else {
        inspector->SetTarget(nullptr);
    }
}

void EditorWindow::AddShape(const std::string& type, int mx, int my) {
    int sx = (int)((mx - scenePanel->contentRect.x - scenePanel->panX) / scenePanel->zoom);
    int sy = (int)((my - scenePanel->contentRect.y - scenePanel->panY) / scenePanel->zoom);
    int idx = hierarchy->AddObject(type, type, sx, sy);
    printf("[AddShape] type=%s mx=%d my=%d -> sx=%d sy=%d idx=%d objs=%zu\n", type.c_str(), mx, my, sx, sy, idx, hierarchy->objects.size());
    scenePanel->selectedIndex = idx;
    inspector->SetTarget(&hierarchy->objects[idx]);
    MarkDirty();
}

// ========== DRAWING ==========
void EditorWindow::DrawMenuBar(GraphicsAPI* gfx) {
    int mh = 26, ww = gfx->GetWidth();
    // Menu bar background with subtle gradient
    gfx->DrawRect(0, 0, ww, mh, 14, 15, 20);
    gfx->DrawRect(0, 0, ww, 1, 35, 40, 50);
    gfx->DrawRect(0, mh - 1, ww, 1, 35, 40, 50);
    const char* labels[] = {"  File", "  Edit", "  Windows", "  Help"};
    int mpos[] = {0, 50, 100, 170};
    for (int i = 0; i < 4; i++) {
        int mw = (i < 3) ? (mpos[i+1] - mpos[i]) : 60;
        if (activeMenu == i || menuHovered == i) {
            gfx->DrawRect(mpos[i], 1, mw, mh - 2, 30, 34, 45);
            gfx->DrawRect(mpos[i], mh - 2, mw, 1, 74, 125, 180);
        }
        gfx->RenderText(labels[i], mpos[i] + 4, 4, activeMenu == i ? 255 : 180, activeMenu == i ? 220 : 180, activeMenu == i ? 180 : 190);
    }
    if (activeMenu < 0) return;
    // Dropdown menu items with keyboard shortcuts
    static const char* items[4][12] = {
        {"New Project\tCtrl+N", "Open\tCtrl+O", "Save\tCtrl+S", "Save As...", "---", "Import Assets...", "Export Script...", "Import Script...", "---", "Preview...\tF5", "---", "Exit"},
        {"Undo\tCtrl+Z", "Redo\tCtrl+Y", "---", "Cut", "Copy\tCtrl+C", "Paste\tCtrl+V", "Duplicate\tCtrl+D", "---", "Delete Selected\tDel", "---", "Grid\tCtrl+G", "Snap\tCtrl+M"},
        {"Toggle Hierarchy", "Toggle Inspector", "Toggle Asset Browser", "Toggle Scene", "Toggle Script"},
        {"About", "Controls"}
    };
    // Calculate actual item count per menu
    int itemCount = 0; while (items[activeMenu][itemCount] != nullptr) itemCount++;
    int dy = mh, dw = 200;
    int menuH = itemCount * 22 + ((activeMenu == 0 && !recentFiles.empty()) ? ((int)recentFiles.size() + 2) * 22 : 0);
    // Dropdown background with border
    gfx->DrawRect(mpos[activeMenu], dy, dw, menuH, 22, 24, 32);
    gfx->DrawRect(mpos[activeMenu], dy, dw, 1, 50, 55, 70);
    gfx->DrawRect(mpos[activeMenu] + dw - 1, dy, 1, menuH, 40, 45, 55);
    gfx->DrawRect(mpos[activeMenu] - 1, dy, 1, menuH, 40, 45, 55);
    gfx->DrawRect(mpos[activeMenu] - 1, dy + menuH - 1, dw + 1, 1, 40, 45, 55);
    int mi = 0;
    for (; items[activeMenu][mi] != nullptr; mi++) {
        int iy = dy + mi * 22;
        if (mi == menuHovered)
            gfx->DrawRect(mpos[activeMenu] + 1, iy, dw - 2, 22, 38, 55, 85);
        std::string s = items[activeMenu][mi];
        if (s == "---") {
            gfx->DrawRect(mpos[activeMenu] + 8, iy + 10, dw - 16, 1, 45, 45, 55);
        } else {
            size_t tab = s.find('\t');
            if (tab != std::string::npos) {
                std::string label = s.substr(0, tab);
                std::string shortcut = s.substr(tab + 1);
                gfx->RenderText(label, mpos[activeMenu] + 8, iy + 3, mi == menuHovered ? 255 : 190, mi == menuHovered ? 230 : 190, mi == menuHovered ? 200 : 200);
                gfx->RenderText(shortcut, mpos[activeMenu] + dw - 80, iy + 3, 120, 120, 140);
            } else {
                gfx->RenderText(s, mpos[activeMenu] + 8, iy + 3, mi == menuHovered ? 255 : 190, mi == menuHovered ? 230 : 190, mi == menuHovered ? 200 : 200);
            }
        }
    }
    // Recent files under File menu
    if (activeMenu == 0 && !recentFiles.empty()) {
        int ry = dy + mi * 22;
        gfx->DrawRect(mpos[activeMenu] + 4, ry, dw - 8, 1, 50, 55, 65);
        ry += 4;
        gfx->RenderText("Recent:", mpos[activeMenu] + 8, ry, 100, 100, 120); ry += 22;
        for (int ri = 0; ri < (int)recentFiles.size(); ri++) {
            std::string shortName = recentFiles[ri];
            size_t sl = shortName.find_last_of("/\\");
            if (sl != std::string::npos) shortName = shortName.substr(sl + 1);
            if (shortName.size() > 22) shortName = shortName.substr(0, 21) + ".";
            int rhit = mi + 1 + ri;
            if (rhit == menuHovered)
                gfx->DrawRect(mpos[activeMenu] + 1, ry, dw - 2, 22, 38, 55, 85);
            gfx->RenderText(std::to_string(ri + 1) + ". " + shortName, mpos[activeMenu] + 8, ry + 3, rhit == menuHovered ? 255 : 160, rhit == menuHovered ? 230 : 160, rhit == menuHovered ? 200 : 180);
            ry += 22;
        }
    }
}

void EditorWindow::DrawToolbar(GraphicsAPI* gfx) {
    int ty = 26, th = 26, ww = gfx->GetWidth();
    gfx->DrawRect(0, ty, ww, th, 20, 21, 27);
    gfx->DrawRect(0, ty, ww, 1, 35, 40, 50);
    gfx->DrawRect(0, ty + th - 1, ww, 1, 30, 34, 42);
    struct TB { int x; const char* label; };
    TB btns[] = {
        {6, "Undo"}, {46, "Redo"}, {90, "Cut"}, {126, "Copy"}, {166, "Paste"},
        {206, "Dup"}, {242, "Del"}, {282, "Grid"}, {322, "Snap"},
        {366, "Zoom-"}, {406, "Zoom+"}, {446, "Fit"}, {486, "Play"}
    };
    for (int i = 0; i < (int)(sizeof(btns) / sizeof(btns[0])); i++) {
        int bw = 36, bh = 20;
        bool isGrid = (std::string(btns[i].label) == "Grid");
        bool isSnap = (std::string(btns[i].label) == "Snap");
        bool active = (isGrid && scenePanel->showGrid) || (isSnap && scenePanel->snapEnabled);
        if (active) {
            gfx->DrawRect(btns[i].x, ty + 3, bw, bh, 45, 60, 80);
            gfx->DrawRect(btns[i].x, ty + 3, bw, 1, 74, 125, 180);
        } else {
            gfx->DrawRect(btns[i].x, ty + 3, bw, bh, 25, 27, 35);
            gfx->DrawRect(btns[i].x, ty + 3, bw, 1, 40, 42, 50);
        }
        gfx->RenderText(btns[i].label, btns[i].x + 3, ty + 5, active ? 160 : 200, active ? 220 : 200, active ? 160 : 220);
    }
}

int EditorWindow::HitToolbar(int mx, int my) {
    int ty = 26, th = 26;
    if (my < ty || my > ty + th) return -1;
    struct TB { int x; const char* label; };
    TB btns[] = {
        {6, "Undo"}, {46, "Redo"}, {90, "Cut"}, {126, "Copy"}, {166, "Paste"},
        {206, "Dup"}, {242, "Del"}, {282, "Grid"}, {322, "Snap"},
        {366, "Zoom-"}, {406, "Zoom+"}, {446, "Fit"}, {486, "Play"}
    };
    for (int i = 0; i < (int)(sizeof(btns) / sizeof(btns[0])); i++) {
        if (mx >= btns[i].x && mx <= btns[i].x + 36) return i;
    }
    return -1;
}

void EditorWindow::DrawSceneObjects(GraphicsAPI* gfx) {
    auto& cp = scenePanel->contentRect;
    gfx->DrawRect(cp.x, cp.y, cp.w, cp.h, 20, 20, 30);
    if (scenePanel->showGrid) scenePanel->DrawGrid(gfx);
    gfx->SetClipRect(cp.x, cp.y, cp.w, cp.h);
    for (int i = 0; i < (int)hierarchy->objects.size(); i++) {
        auto& obj = hierarchy->objects[i];
        if (!obj.visible) continue;
        int dx = cp.x + (int)(obj.posX * scenePanel->zoom) + scenePanel->panX;
        int dy = cp.y + (int)(obj.posY * scenePanel->zoom) + scenePanel->panY;
        int sw = (int)(obj.sizeW * scenePanel->zoom), sh = (int)(obj.sizeH * scenePanel->zoom);
        bool sel = (i == scenePanel->selectedIndex);
        if (obj.type == "ellipse") {
            int cx = dx + sw/2, cy = dy + sh/2, rx = sw/2, ry = sh/2;
            for (int row = -ry; row <= ry; row++) {
                int hw = (int)(rx * sqrt(1.0 - ((double)row*row)/((double)ry*ry)));
                gfx->DrawRect(cx - hw, cy + row, hw*2, 1, obj.colorR, obj.colorG, obj.colorB);
            }
            if (sel) { gfx->DrawRect(cx - rx, cy, rx*2, 1, 255, 255, 200); gfx->DrawRect(cx, cy - ry, 1, ry*2, 255, 255, 200); }
        } else if (obj.type == "text") {
            gfx->DrawRect(dx, dy, sw, sh, obj.colorR, obj.colorG, obj.colorB);
            gfx->DrawRect(dx, dy, sw, 1, obj.colorR + 30, obj.colorG + 30, obj.colorB + 30);
            gfx->DrawRect(dx, dy + sh - 1, sw, 1, obj.colorR - 20, obj.colorG - 20, obj.colorB - 20);
            gfx->RenderText(obj.text.empty() ? obj.name : obj.text, dx + 4, dy + 4, 200, 200, 220);
        } else if (obj.type == "sprite" && !obj.spritePath.empty()) {
            gfx->DrawImage(dx, dy, sw, sh, obj.spritePath);
            if (sel) { gfx->DrawRect(dx, dy, sw, sh, 74, 125, 180); }
        } else {
            gfx->DrawRect(dx, dy, sw, sh, obj.colorR, obj.colorG, obj.colorB);
            gfx->DrawRect(dx, dy, sw, 1, obj.colorR + 30, obj.colorG + 30, obj.colorB + 30);
            gfx->DrawRect(dx, dy + sh - 1, sw, 1, obj.colorR - 20, obj.colorG - 20, obj.colorB - 20);
            gfx->RenderText(obj.name, dx + 4, dy + 4, 200, 200, 220);
        }
        if (i == scenePanel->selectedIndex) scenePanel->DrawGizmo(gfx, &obj);
        // Component visual indicators
        for (auto& comp : obj.components) {
            if (comp == "Collider") {
                int cw = 4; int ccol = 120;
                gfx->DrawRect(dx, dy, sw, cw, ccol, 180, ccol);
                gfx->DrawRect(dx, dy + sh - cw, sw, cw, ccol, 180, ccol);
                gfx->DrawRect(dx, dy, cw, sh, ccol, 180, ccol);
                gfx->DrawRect(dx + sw - cw, dy, cw, sh, ccol, 180, ccol);
            }
            if (comp == "Text" && obj.type != "text") {
                if (!obj.text.empty())
                    gfx->RenderText(obj.text, dx + 2, dy + sh + 4, 160, 160, 200);
            }
        }
    }
    gfx->ResetClip();
}

void EditorWindow::StartPreview() {
    if (scriptPanel->lines.empty()) return;
    if (IsPreviewActive()) { StopPreview(); return; }
    previewWin = new PreviewWindow();
    if (!previewWin->Create("Novell Engine - Preview", 960, 640)) {
        delete previewWin; previewWin = nullptr;
        lastMsg = "Preview window creation failed"; lastMsgTimer = 120;
        return;
    }
    currentTick = scriptPanel->lines[0].tick;
    lastMsg = "Preview started in separate window"; lastMsgTimer = 120;
    RenderPreview();
}

void EditorWindow::StopPreview() {
    if (previewWin) { previewWin->Destroy(); delete previewWin; previewWin = nullptr; }
    lastMsg = "Preview closed"; lastMsgTimer = 60;
}

void EditorWindow::PollPreviewEvents() {
    if (!IsPreviewActive()) return;
    previewWin->PollEvents(eventQueue);
    if (!previewWin->IsRunning()) { StopPreview(); }
}

void EditorWindow::RenderPreview() {
    if (!IsPreviewActive()) return;
    if (!previewWin->IsRunning()) { StopPreview(); return; }
    previewWin->BeginFrame();
    int w = previewWin->GetWidth(), h = previewWin->GetHeight();
    previewWin->Clear(15, 15, 25);
    int maxTick = 0;
    for (auto& obj : hierarchy->objects) if (obj.endTick > maxTick) maxTick = obj.endTick;
    ScriptLine* sl = nullptr;
    for (auto& l : scriptPanel->lines) { if (l.tick == currentTick) { sl = &l; break; } }
    auto isActive = [&](EditorObject& obj) { return obj.visible && currentTick >= obj.startTick && currentTick < obj.endTick; };
    // Background
    for (auto& obj : hierarchy->objects)
        if (obj.type == "background" && isActive(obj) && !obj.spritePath.empty()) { previewWin->DrawImage(0, 0, w, h, obj.spritePath); break; }
    if (sl && !sl->backgroundPath.empty()) previewWin->DrawImage(0, 0, w, h, sl->backgroundPath);
    // Sprites
    for (auto& obj : hierarchy->objects) {
        if (obj.type == "sprite" && isActive(obj) && !obj.spritePath.empty()) {
            int sw = w / 3, sh = h * 2 / 3; if (sh > h - 140) sh = h - 140;
            previewWin->DrawImage(40, h - sh - 120, sw, sh, obj.spritePath);
        }
    }
    // Dialogue box
    int boxH = 130, boxY = h - boxH - 10;
    previewWin->DrawRect(8, boxY, w - 16, boxH, 12, 12, 18);
    previewWin->DrawRect(8, boxY, w - 16, 2, 74, 125, 180);
    previewWin->DrawRect(9, boxY + 2, w - 18, boxH - 4, 25, 27, 35);
    if (sl) {
        if (!sl->characterName.empty()) {
            previewWin->DrawRect(16, boxY + 8, 120, 22, 40, 45, 55);
            previewWin->DrawRect(16, boxY + 8, 120, 1, 74, 125, 180);
            previewWin->RenderText(sl->characterName, 24, boxY + 10, 255, 220, 100);
        }
        int tx = 24, ty = boxY + 36, lineH = 20, maxW2 = w - 68;
        std::string remaining = sl->text;
        while (!remaining.empty()) {
            if (ty + lineH > boxY + boxH - 8) break;
            std::string line; size_t brk = remaining.find('\n');
            if (brk != std::string::npos) { line = remaining.substr(0, brk); remaining = remaining.substr(brk + 1); }
            else { line = remaining; remaining.clear(); }
            while ((int)line.size() * 8 > maxW2 && line.size() > 1) {
                size_t sp = line.rfind(' ', maxW2 / 8);
                if (sp == std::string::npos) sp = maxW2 / 8 - 1;
                previewWin->RenderText(line.substr(0, sp), tx, ty, 240, 240, 240); ty += lineH;
                if (ty + lineH > boxY + boxH - 8) break;
                line = line.substr(sp + 1);
            }
            previewWin->RenderText(line, tx, ty, 240, 240, 240); ty += lineH;
        }
    }
    // Tick counter / info overlay
    previewWin->DrawRect(w - 130, 6, 120, 22, 15, 15, 20);
    previewWin->DrawRect(w - 130, 6, 120, 1, 50, 55, 65);
    previewWin->RenderText("Tick " + std::to_string(currentTick), w - 120, 10, 150, 190, 220);
    previewWin->DrawRect(6, h - 24, 200, 18, 15, 15, 20);
    previewWin->RenderText("Space/Enter next | Esc = exit", 10, h - 22, 80, 80, 100);
    previewWin->EndFrame();
}

void EditorWindow::DrawSplitters(GraphicsAPI* gfx) {
    auto drawGrip = [&](int x, int y, int w, int h, bool vert, bool active) {
        int bg = active ? 55 : 35, fg = active ? 80 : 55, bri = active ? 90 : 55;
        (void)bri;
        if (vert) {
            gfx->DrawRect(x + 2, y, 4, h, bg, bg + 3, bri);
            for (int gy = y + 8; gy < y + h - 8; gy += 6)
                gfx->DrawRect(x + 3, gy, 2, 2, fg, fg + 3, bri + 5);
            if (active) { gfx->DrawRect(x + 1, y, 6, h, 74, 125, 180); }
        } else {
            gfx->DrawRect(x, y + 2, w, 4, bg, bg + 3, bri);
            for (int gx = x + 8; gx < x + w - 8; gx += 6)
                gfx->DrawRect(gx, y + 3, 2, 2, fg, fg + 3, bri + 5);
            if (active) { gfx->DrawRect(x, y + 1, w, 6, 74, 125, 180); }
        }
    };
    drawGrip(splitterR1.x, splitterR1.y, splitterR1.w, splitterR1.h, true, dragSplitter == 1);
    drawGrip(splitterR2.x, splitterR2.y, splitterR2.w, splitterR2.h, true, dragSplitter == 2);
    drawGrip(splitterR3.x, splitterR3.y, splitterR3.w, splitterR3.h, false, dragSplitter == 3);
    drawGrip(splitterR4.x, splitterR4.y, splitterR4.w, splitterR4.h, false, dragSplitter == 4);
}

void EditorWindow::DrawDebugOverlay(GraphicsAPI* gfx) {
    if (!showDebug) return;
    int ww = gfx->GetWidth(), wh = gfx->GetHeight();
    gfx->DrawRect(0, 0, ww, wh, 0, 0, 0);
    gfx->DrawRect(ww / 2, 0, ww / 2, wh, 16, 16, 24);
    auto outline = [&](Rect r, int r2, int g2, int b2) { gfx->DrawRect(r.x, r.y, r.w, r.h, r2, g2, b2); };
    outline(hierarchy->rect, 255, 0, 0); outline(hierarchy->contentRect, 255, 100, 0);
    outline(inspector->rect, 0, 255, 0); outline(inspector->contentRect, 100, 255, 0);
    outline(scenePanel->rect, 0, 0, 255); outline(scenePanel->contentRect, 0, 100, 255);
    outline(assetBrowser->rect, 255, 255, 0); outline(assetBrowser->contentRect, 200, 200, 0);
    outline(scriptPanel->rect, 255, 0, 255); outline(scriptPanel->contentRect, 200, 0, 200);
    gfx->DrawRect(splitterR1.x, splitterR1.y, splitterR1.w, splitterR1.h, 255, 255, 255);
    gfx->DrawRect(splitterR2.x, splitterR2.y, splitterR2.w, splitterR2.h, 255, 255, 255);
    gfx->DrawRect(splitterR3.x, splitterR3.y, splitterR3.w, splitterR3.h, 255, 255, 255);
    gfx->DrawRect(splitterR4.x, splitterR4.y, splitterR4.w, splitterR4.h, 255, 255, 255);
    outline(timeline->rect, 200, 100, 200);
    int ly = 4;
    auto info = [&](const std::string& l, const std::string& v) { gfx->RenderText(l + ": " + v, 4, ly, 200, 200, 200); ly += 14; };
    info("Objects", std::to_string(hierarchy->objects.size()));
    info("Hier.sel", std::to_string(hierarchy->selectedIndex));
    info("Scene.sel", std::to_string(scenePanel->selectedIndex));
    info("zoom", std::to_string(scenePanel->zoom));
    info("pan", std::to_string(scenePanel->panX) + "," + std::to_string(scenePanel->panY));
    info("dragSpl", std::to_string(dragSplitter));
    info("ctxMenu", ctxMenu.active ? "active" : "off");
    info("selAsset", assetBrowser->selectedAsset ? "true" : "false");
    info("activeMenu", std::to_string(activeMenu));
    for (int i = 0; i < std::min((int)hierarchy->objects.size(), 15); i++) {
        auto& o = hierarchy->objects[i];
        info("[" + std::to_string(i) + "]", o.name + " " + o.type + " v=" + (o.visible ? "1" : "0"));
    }
    gfx->RenderText("F1=close debug", ww / 2 + 4, 4, 255, 100, 100);
}

void EditorWindow::Draw(GraphicsAPI* gfx) {
    DBG("Draw begin");
    RenderPreview();
    // Background gradient
    gfx->DrawRect(0, 0, gfx->GetWidth(), gfx->GetHeight(), 13, 13, 17);
    gfx->DrawRect(0, 0, gfx->GetWidth(), gfx->GetHeight(), 16, 16, 22);
    DrawMenuBar(gfx);
    DrawToolbar(gfx);
        // Update window title
        std::string title = "Novell Engine";
        if (!currentProjectPath.empty()) {
            size_t sl = currentProjectPath.find_last_of("/\\");
            title += " - " + ((sl != std::string::npos) ? currentProjectPath.substr(sl + 1) : currentProjectPath);
        }
        if (dirty) title += " *";
        SetWindowTextA(GetActiveWindow(), title.c_str());
        for (auto* slot : panelSlots) {
            if (!slot || !slot->panel || !slot->panel->visible) continue;
            slot->panel->Draw(gfx);
        }
        // Timeline needs extra data
        if (timeline && timeline->visible && hierarchy)
            timeline->Draw(gfx, hierarchy->objects, hierarchy->selectedIndex, currentTick);
    DrawSceneObjects(gfx);
    DrawSplitters(gfx);
    ctxMenu.Draw(gfx);
    DrawDebugOverlay(gfx);
    if (lastMsgTimer > 0) {
        int y = gfx->GetHeight() - 26;
        gfx->DrawRect(0, y, gfx->GetWidth(), 26, 16, 17, 22);
        gfx->DrawRect(0, y, gfx->GetWidth(), 2, 30, 35, 45);
        // Left side: lastMsg with fade
        int fade = (lastMsgTimer < 30) ? (lastMsgTimer * 8) : 255;
        int fc = fade * 200 / 255;
        gfx->RenderText(lastMsg, 12, y + 5, fc, fc, fade * 150 / 255);
        // Right side: project info
        std::string info = "Objects:" + std::to_string(hierarchy->objects.size()) +
            " Tick:" + std::to_string(currentTick) +
            " Snap:" + std::to_string(snapSize) +
            " Zoom:" + std::to_string((int)(scenePanel->zoom * 100)) + "%" +
            (dirty ? " *" : "");
        int iw = (int)info.size() * 8;
        gfx->RenderText(info, gfx->GetWidth() - iw - 12, y + 5, dirty ? 220 : 140, dirty ? 200 : 140, dirty ? 100 : 150);
        lastMsgTimer--;
    } else {
        // Always show info bar
        int y = gfx->GetHeight() - 26;
        gfx->DrawRect(0, y, gfx->GetWidth(), 26, 16, 17, 22);
        gfx->DrawRect(0, y, gfx->GetWidth(), 2, 30, 35, 45);
        std::string info = "Objects:" + std::to_string(hierarchy->objects.size()) +
            " Tick:" + std::to_string(currentTick) +
            " Snap:" + std::to_string(snapSize) +
            " Zoom:" + std::to_string((int)(scenePanel->zoom * 100)) + "%" +
            (dirty ? " *UNSAVED*" : "");
        int iw = (int)info.size() * 8;
        gfx->RenderText(info, gfx->GetWidth() - iw - 12, y + 5, dirty ? 220 : 140, dirty ? 200 : 140, dirty ? 100 : 150);
    }
    DBG("Draw end");
}

// ========== EVENT HANDLING ==========
void EditorWindow::HandleMenuClick(int mx, int my) {
    int mh = 26;
    int mpos[] = {0, 50, 100, 170};
    // Click on menu bar
    if (my < mh) {
        for (int i = 0; i < 4; i++) {
            int nextX = (i < 3) ? mpos[i+1] : mpos[i] + 60;
            if (mx >= mpos[i] && mx < nextX) {
                activeMenu = (activeMenu == i) ? -1 : i;
                menuHovered = -1; return;
            }
        }
        activeMenu = -1; menuHovered = -1; return;
    }
    // Menu dropdown is open - check if click is within bounds
    if (activeMenu >= 0) {
        int dx = mpos[activeMenu];
        // Calculate total menu height for bounds check
        int itemCount = 0;
        static const char* items[4][12] = {
            {"New Project\tCtrl+N", "Open\tCtrl+O", "Save\tCtrl+S", "Save As...", "---", "Import Assets...", "Export Script...", "Import Script...", "---", "Preview...\tF5", "---", "Exit"},
            {"Undo\tCtrl+Z", "Redo\tCtrl+Y", "---", "Cut", "Copy\tCtrl+C", "Paste\tCtrl+V", "Duplicate\tCtrl+D", "---", "Delete Selected\tDel", "---", "Grid\tCtrl+G", "Snap\tCtrl+M"},
            {"Toggle Hierarchy", "Toggle Inspector", "Toggle Asset Browser", "Toggle Scene", "Toggle Script"},
            {"About", "Controls"}
        };
        while (items[activeMenu][itemCount] != nullptr) itemCount++;
        int menuH = itemCount * 22 + ((activeMenu == 0 && !recentFiles.empty()) ? ((int)recentFiles.size() + 2) * 22 : 0);
        if (mx < dx || mx > dx + 200 || my < mh || my > mh + menuH) { activeMenu = -1; menuHovered = -1; return; }
        static const char* actions[4][12] = {
        {"New Project", "Open", "Save", "Save As...", "", "Import Assets...", "Export Script...", "Import Script...", "", "Preview...", "", "Exit"},
        {"Undo", "Redo", "", "Cut", "Copy", "Paste", "Duplicate", "", "Delete Selected", "", "Grid", "Snap"},
            {"Toggle Hierarchy", "Toggle Inspector", "Toggle Asset Browser", "Toggle Scene", "Toggle Script"},
            {"About", "Controls"}
        };
        int idx = (my - mh) / 22;
        if (idx >= 0 && idx < 12 && actions[activeMenu][idx] != nullptr && strlen(actions[activeMenu][idx]) > 0) {
            std::string act = actions[activeMenu][idx];
            printf("[MenuAct] %s\n", act.c_str());
            activeMenu = -1; menuHovered = -1;
            if (act == "Import Assets...") {
                printf("  -> Import Assets dialog\n");
                char fileBuf[260] = {}; OPENFILENAMEA ofn = {};
                ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = GetActiveWindow();
                ofn.lpstrFile = fileBuf; ofn.nMaxFile = 260;
                ofn.lpstrFilter = "Images (*.png;*.jpg;*.bmp;*.gif)\0*.png;*.jpg;*.jpeg;*.bmp;*.gif\0All\0*.*\0";
                ofn.Flags = OFN_FILEMUSTEXIST;
                if (GetOpenFileNameA(&ofn)) {
                    std::string src(fileBuf);
                    size_t sep = src.find_last_of("\\/");
                    std::string fn = (sep != std::string::npos) ? src.substr(sep+1) : src;
                    printf("  Copy %s -> %s/%s\n", src.c_str(), assetBrowser->currentDir.c_str(), fn.c_str());
                    CopyFileA(src.c_str(), (assetBrowser->currentDir + "/" + fn).c_str(), FALSE);
                    assetBrowser->ScanDirectory();
                }
            } else if (act == "Save") {
                SaveCurrentProject();
            } else if (act == "Save As...") {
                std::string oldPath = currentProjectPath;
                currentProjectPath.clear();
                SaveCurrentProject();
                if (currentProjectPath.empty()) currentProjectPath = oldPath;
            } else if (act == "Open") {
                printf("  -> Open\n");
                char fileBuf[260] = {}; OPENFILENAMEA ofn = {};
                ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = GetActiveWindow();
                ofn.lpstrFile = fileBuf; ofn.nMaxFile = 260;
                ofn.lpstrFilter = "Novell Project (*.vne)\0*.vne\0All\0*.*\0";
                ofn.Flags = OFN_FILEMUSTEXIST;
                if (GetOpenFileNameA(&ofn)) {
                    LoadProjectFrom(fileBuf);
                    SyncSelection();
                } else { lastMsg = "Open cancelled"; lastMsgTimer = 60; }
            } else if (act == "New Project") {
                printf("  -> New Project\n");
                hierarchy->objects.clear(); scriptPanel->lines.clear(); assetBrowser->files.clear();
                hierarchy->selectedIndex = -1; scenePanel->selectedIndex = -1; inspector->SetTarget(nullptr);
                hierarchy->AddObject("Background", "background"); SyncSelection();
                currentProjectPath.clear(); dirty = false;
            } else if (act == "Delete Selected" && hierarchy->selectedIndex >= 0) { printf("  -> DeleteSelected\n"); hierarchy->DeleteSelected(); SyncSelection(); MarkDirty(); }
            else if (act == "Duplicate" && hierarchy->selectedIndex >= 0) { printf("  -> Duplicate\n"); DuplicateSelected(); }
            else if (act == "Cut" && hierarchy->selectedIndex >= 0) { printf("  -> Cut\n"); CopySelected(); PushUndo(); hierarchy->DeleteSelected(); SyncSelection(); MarkDirty(); }
            else if (act == "Copy") { printf("  -> Copy\n"); CopySelected(); }
            else if (act == "Paste") { printf("  -> Paste\n"); PasteSelected(); }
            else if (act == "Grid") { printf("  -> Grid\n"); ToggleGrid(); }
            else if (act == "Snap") { printf("  -> Snap\n"); ToggleSnap(); }
            else if (act == "Undo") { printf("  -> Undo\n"); Undo(); }
            else if (act == "Redo") { printf("  -> Redo\n"); Redo(); }
            else if (act == "Export Script...") {
                printf("  -> Export Script\n");
                char fileBuf[260] = {}; OPENFILENAMEA ofn = {};
                ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = GetActiveWindow();
                ofn.lpstrFile = fileBuf; ofn.nMaxFile = 260;
                ofn.lpstrFilter = "Script files (*.txt)\0*.txt\0All\0*.*\0";
                ofn.lpstrDefExt = "txt"; ofn.Flags = OFN_OVERWRITEPROMPT;
                if (GetSaveFileNameA(&ofn)) {
                    std::ofstream ofs(fileBuf);
                    for (auto& l : scriptPanel->lines)
                        ofs << l.tick << "|" << l.characterName << "|" << l.text << "|" << l.spritePath << "|" << l.backgroundPath << "\n";
                    lastMsg = "Script exported: " + std::string(fileBuf); lastMsgTimer = 120;
                }
            } else if (act == "Import Script...") {
                printf("  -> Import Script\n");
                char fileBuf[260] = {}; OPENFILENAMEA ofn = {};
                ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = GetActiveWindow();
                ofn.lpstrFile = fileBuf; ofn.nMaxFile = 260;
                ofn.lpstrFilter = "Script files (*.txt)\0*.txt\0All\0*.*\0";
                ofn.Flags = OFN_FILEMUSTEXIST;
                if (GetOpenFileNameA(&ofn)) {
                    std::ifstream ifs(fileBuf);
                    scriptPanel->lines.clear();
                    std::string line;
                    while (std::getline(ifs, line)) {
                        ScriptLine l;
                        std::string sTK; std::istringstream ss(line);
                        std::getline(ss, sTK, '|'); l.tick = atoi(sTK.c_str());
                        std::getline(ss, l.characterName, '|');
                        std::getline(ss, l.text, '|');
                        std::getline(ss, l.spritePath, '|');
                        std::getline(ss, l.backgroundPath, '|');
                        scriptPanel->lines.push_back(l);
                    }
                    lastMsg = "Script imported: " + std::string(fileBuf); lastMsgTimer = 120;
                }
            } else if (act == "Preview...") {
                printf("  -> Preview\n");
                if (!scriptPanel->lines.empty()) { StartPreview(); }
            } else if (act == "About") {
                MessageBoxA(GetActiveWindow(), "Novell Engine v0.3.0\n\nVisual novel editor with timeline,\nscript panel, hierarchy, scene editor,\ninspector, and asset browser.\n\nBuild: MinGW g++ 14.2.0, GDI+", "About Novell Engine", MB_OK);
            } else if (act == "Controls") {
                MessageBoxA(GetActiveWindow(),
                    "F1       Toggle debug overlay\n"
                    "F5       Start preview\n"
                    "F11      Fullscreen\n"
                    "Ctrl+N   New project\n"
                    "Ctrl+O   Open project\n"
                    "Ctrl+S   Save project\n"
                    "Ctrl+Z   Undo\n"
                    "Ctrl+Y   Redo\n"
                    "Ctrl+C   Copy selection\n"
                    "Ctrl+V   Paste clipboard\n"
                    "Ctrl+D   Duplicate object\n"
                    "Ctrl+F   Zoom to fit\n"
                    "Ctrl+G   Toggle grid\n"
                    "Ctrl+M   Toggle snap\n"
                    "R/E/T    Add rect/ellipse/text\n"
                    "Del      Delete selected\n"
                    "Shift    Drag with snap\n"
                    "[ / ]    Snap grid size\n"
                    "RClick   Context menu",
                    "Controls", MB_OK);
            } else if (act == "Exit") { exit(0); }
            // Recent file click check: if index is past menu items and first menu is File
            if (idx >= itemCount && activeMenu == 0 && !recentFiles.empty()) {
                int ri = idx - itemCount - 2; // skip separator + "Recent:" label
                if (ri >= 0 && ri < (int)recentFiles.size()) {
                    LoadProjectFrom(recentFiles[ri]);
                    SyncSelection();
                }
            }
        }
        activeMenu = -1; menuHovered = -1;
    }
}

void EditorWindow::HandleClick(int mx, int my) {
    DBG("HandleClick(%d,%d) activeMenu=%d ctxMenu=%d", mx, my, activeMenu, ctxMenu.active);
    PushUndo();
    if (IsPreviewActive()) {
        currentTick++;
        RenderPreview();
        return;
    }
    if (activeMenu >= 0 || my < 26) {
        HandleMenuClick(mx, my);
        return;
    }
    // Global toolbar (y 26..52)
    if (my >= 26 && my < 52) {
        int t = HitToolbar(mx, my);
        if (t >= 0) {
            const char* labels[] = {"Undo", "Redo", "Cut", "Copy", "Paste", "Dup", "Del", "Grid", "Snap", "Zoom-", "Zoom+", "Fit", "Play"};
            std::string act = labels[t];
            if (act == "Undo") Undo();
            else if (act == "Redo") Redo();
            else if (act == "Cut") { CopySelected(); PushUndo(); hierarchy->DeleteSelected(); SyncSelection(); MarkDirty(); }
            else if (act == "Copy") CopySelected();
            else if (act == "Paste") PasteSelected();
            else if (act == "Dup") DuplicateSelected();
            else if (act == "Del" && hierarchy->selectedIndex >= 0) { PushUndo(); hierarchy->DeleteSelected(); SyncSelection(); MarkDirty(); }
            else if (act == "Grid") ToggleGrid();
            else if (act == "Snap") ToggleSnap();
            else if (act == "Zoom-") { scenePanel->zoom /= 1.2f; if (scenePanel->zoom < 0.1f) scenePanel->zoom = 0.1f; }
            else if (act == "Zoom+") { scenePanel->zoom *= 1.2f; if (scenePanel->zoom > 5.0f) scenePanel->zoom = 5.0f; }
            else if (act == "Fit") ZoomToFitScene();
            else if (act == "Play") { if (!scriptPanel->lines.empty()) StartPreview(); }
        }
        return;
    }
    // Context menu
    if (ctxMenu.active) {
        int hit = ctxMenu.HitTest(mx, my);
        DBG("ctxMenu hit=%d", hit);
        if (hit >= 0) {
            std::string act = ctxMenu.items[hit];
            printf("[Click] ctxMenu action=%s\n", act.c_str());
            ctxMenu.Close();
            printf("[CtxAct] %s\n", act.c_str());
            if (act == "Add Rectangle") { printf("  -> AddShape rect\n"); AddShape("rectangle", ctxMenu.mx, ctxMenu.my); }
            else if (act == "Add Ellipse") { printf("  -> AddShape ellipse\n"); AddShape("ellipse", ctxMenu.mx, ctxMenu.my); }
            else if (act == "Add Text") { printf("  -> AddShape text\n"); AddShape("text", ctxMenu.mx, ctxMenu.my); }
            else if (act == "Add Image") { printf("  -> AddShape sprite\n"); AddShape("sprite", ctxMenu.mx, ctxMenu.my); }
            else if (act == "Create Script Line" && hierarchy->selectedIndex >= 0) {
                printf("  -> CreateScriptLine\n");
                auto& obj = hierarchy->objects[hierarchy->selectedIndex];
                ScriptLine sl; sl.characterName = obj.name; sl.text = obj.text;
                if (obj.type == "sprite") sl.spritePath = obj.spritePath;
                sl.tick = (int)scriptPanel->lines.size();
                scriptPanel->lines.push_back(sl);
                scriptPanel->selectedLine = (int)scriptPanel->lines.size() - 1;
                MarkDirty();
            }
            else if (act == "Duplicate") { DuplicateSelected(); }
            else if (act == "Copy") { CopySelected(); }
            else if (act == "Paste") { PasteSelected(); }
            else if (act == "Move Up") { MoveSelectedUp(); }
            else if (act == "Move Down") { MoveSelectedDown(); }
            else if (act == "Align Left") { AlignSelected(ST_ALIGN_L); }
            else if (act == "Align Center H") { AlignSelected(ST_ALIGN_H); }
            else if (act == "Align Right") { AlignSelected(ST_ALIGN_R); }
            else if (act == "Align Top") { AlignSelected(ST_ALIGN_T); }
            else if (act == "Align Center V") { AlignSelected(ST_ALIGN_V); }
            else if (act == "Align Bottom") { AlignSelected(ST_ALIGN_B); }
            else if (act == "Bring to Front") { BringToFront(); }
            else if (act == "Send to Back") { SendToBack(); }
            else if (act == "Delete" && hierarchy->selectedIndex >= 0) { printf("  -> DeleteSelected\n"); hierarchy->DeleteSelected(); SyncSelection(); MarkDirty(); }
            else if (act == "Hide" && hierarchy->selectedIndex >= 0) { printf("  -> ToggleVisible\n"); hierarchy->ToggleVisible(); MarkDirty(); }
            else if (act == "New Folder") { printf("  -> NewFolder\n"); bool ok = fs::create_directories(assetBrowser->currentDir + "/NewFolder"); printf("  create_directories=%d\n", ok); assetBrowser->ScanDirectory(); }
            else if (act == "Add Empty File") { printf("  -> AddEmptyFile\n"); std::ofstream ofs(assetBrowser->currentDir + "/new_file.txt"); ofs.close(); assetBrowser->ScanDirectory(); }
            else if (act == "Add Material") { printf("  -> AddMaterial\n"); std::ofstream ofs(assetBrowser->currentDir + "/new_material.mat"); ofs.close(); assetBrowser->ScanDirectory(); }
            else if (act == "Refresh") { printf("  -> Refresh\n"); assetBrowser->ScanDirectory(); }
            else if (act == "Rename" && assetBrowser->selectedAsset && assetBrowser->selAssetIndex >= 0) {
                printf("  -> Rename\n");
                assetBrowser->editingField = 1; assetBrowser->editingIndex = assetBrowser->selAssetIndex;
                assetBrowser->editBuf = assetBrowser->files[assetBrowser->selAssetIndex];
            }
            else if (act == "Open" && assetBrowser->selectedAsset && assetBrowser->selAssetIndex >= 0) {
                printf("  -> Open dir\n");
                assetBrowser->HandleClick(ctxMenu.mx, ctxMenu.my);
            }
            else if (act == "Delete" && assetBrowser->selectedAsset && assetBrowser->selAssetIndex >= 0) {
                printf("  -> Delete\n");
                std::string delPath = assetBrowser->currentDir + "/" + assetBrowser->files[assetBrowser->selAssetIndex];
                fs::remove(delPath);
                assetBrowser->CancelSelect();
                assetBrowser->ScanDirectory();
            }
            return;
        }
        ctxMenu.Close();
    }
    // Clear transient state
    scenePanning = false;
    dragSplitter = -1;
    // Splitter
    int spl = HitTestSplitter(mx, my);
    if (spl > 0) { dragSplitter = spl; return; }
    // Asset placement
    if (assetBrowser->selectedAsset) {
        std::string fp = assetBrowser->selAssetPath;
        std::string fn = fp; size_t p = fn.find_last_of("/\\"); if (p != std::string::npos) fn = fn.substr(p + 1);
        if (hierarchy->rect.Contains(mx, my)) {
            int hidx = hierarchy->AddObject(fn, "sprite");
            hierarchy->objects[hidx].spritePath = fp;
            SyncSelection(); assetBrowser->CancelSelect(); return;
        }
        if (scenePanel->contentRect.Contains(mx, my)) {
            for (int i = (int)hierarchy->objects.size() - 1; i >= 0; i--) {
                auto& obj = hierarchy->objects[i];
                if (!obj.visible) continue;
                int dx = scenePanel->contentRect.x + (int)(obj.posX * scenePanel->zoom) + scenePanel->panX;
                int dy = scenePanel->contentRect.y + (int)(obj.posY * scenePanel->zoom) + scenePanel->panY;
                int sw = (int)(obj.sizeW * scenePanel->zoom), sh = (int)(obj.sizeH * scenePanel->zoom);
                if (mx >= dx && mx <= dx + sw && my >= dy && my <= dy + sh) {
                    scenePanel->selectedIndex = i; hierarchy->selectedIndex = i;
                    inspector->SetTarget(&hierarchy->objects[i]); assetBrowser->CancelSelect(); return;
                }
            }
            int sx = (int)((mx - scenePanel->contentRect.x - scenePanel->panX) / scenePanel->zoom);
            int sy = (int)((my - scenePanel->contentRect.y - scenePanel->panY) / scenePanel->zoom);
            int idx = hierarchy->AddObject(fn, "sprite", sx, sy);
            hierarchy->objects[idx].spritePath = fp;
            scenePanel->selectedIndex = idx; hierarchy->selectedIndex = idx;
            inspector->SetTarget(&hierarchy->objects[idx]); assetBrowser->CancelSelect(); return;
        }
        assetBrowser->CancelSelect();
    }
    // Panel dispatch (per-panel, early return)
    if (hierarchy->rect.Contains(mx, my)) { hierarchy->HandleClick(mx, my); SyncSelection(); if (hierarchy->selectedIndex >= 0) MarkDirty(); return; }
    if (inspector->contentRect.Contains(mx, my)) { inspector->HandleClick(mx, my); MarkDirty(); return; }
    if (scriptPanel->rect.Contains(mx, my)) { scriptPanel->HandleClick(mx, my); MarkDirty(); return; }
    if (timeline->rect.Contains(mx, my)) { timeline->HandleClick(mx, my, hierarchy->objects, hierarchy->selectedIndex, currentTick); SyncSelection(); MarkDirty(); return; }
    if (assetBrowser->contentRect.Contains(mx, my)) { assetBrowser->HandleClick(mx, my); return; }
    // Scene
    if (scenePanel->contentRect.Contains(mx, my)) {
        // Scene toolbar buttons
        int tool = scenePanel->HitToolbar(mx, my);
        if (tool != ST_NONE) {
            switch (tool) {
                case ST_ZOOM_OUT: scenePanel->zoom /= 1.2f; if (scenePanel->zoom < 0.1f) scenePanel->zoom = 0.1f; break;
                case ST_ZOOM_IN: scenePanel->zoom *= 1.2f; if (scenePanel->zoom > 5.0f) scenePanel->zoom = 5.0f; break;
                case ST_FIT: ZoomToFitScene(); break;
                case ST_GRID: ToggleGrid(); break;
                case ST_SNAP: ToggleSnap(); break;
                case ST_ALIGN_L: case ST_ALIGN_H: case ST_ALIGN_R: case ST_ALIGN_T: case ST_ALIGN_V: case ST_ALIGN_B: AlignSelected(tool); break;
                case ST_COPY: CopySelected(); break;
                case ST_PASTE: PasteSelected(); break;
                default: break;
            }
            return;
        }
        scenePanel->HandleClick(mx, my);
        // Gizmo on selected object
        int si = scenePanel->selectedIndex;
        if (si >= 0 && si < (int)hierarchy->objects.size()) {
            GizmoPart gp = scenePanel->HitTestGizmo(mx, my, &hierarchy->objects[si]);
            if (gp != GIZMO_NONE) {
                scenePanel->activeGizmo = gp;
                scenePanel->gizmoStartX = mx; scenePanel->gizmoStartY = my;
                scenePanel->gizmoObjX = hierarchy->objects[si].posX;
                scenePanel->gizmoObjY = hierarchy->objects[si].posY;
                scenePanel->gizmoObjW = hierarchy->objects[si].sizeW;
                scenePanel->gizmoObjH = hierarchy->objects[si].sizeH;
                return;
            }
        }
        // Pick object
        for (int i = (int)hierarchy->objects.size() - 1; i >= 0; i--) {
            auto& obj = hierarchy->objects[i];
            if (!obj.visible) continue;
            int dx = scenePanel->contentRect.x + (int)(obj.posX * scenePanel->zoom) + scenePanel->panX;
            int dy = scenePanel->contentRect.y + (int)(obj.posY * scenePanel->zoom) + scenePanel->panY;
            int sw = (int)(obj.sizeW * scenePanel->zoom), sh = (int)(obj.sizeH * scenePanel->zoom);
            if (mx >= dx && mx <= dx + sw && my >= dy && my <= dy + sh) {
                scenePanel->selectedIndex = i; hierarchy->selectedIndex = i;
                inspector->SetTarget(&hierarchy->objects[i]); return;
            }
        }
        // Empty space - pan mode
        hierarchy->selectedIndex = -1; scenePanel->selectedIndex = -1; inspector->SetTarget(nullptr);
        scenePanning = true;
        panStartX = mx; panStartY = my;
        panStartPanX = scenePanel->panX; panStartPanY = scenePanel->panY;
    }
}

void EditorWindow::HandleRClick(int mx, int my) {
    printf("[RClick] at %d,%d\n", mx, my);
    lastMsg = "RClick at " + std::to_string(mx) + "," + std::to_string(my);
    lastMsgTimer = 9999;
    if (ctxMenu.active) { ctxMenu.Close(); return; }
    bool inHier = hierarchy->contentRect.Contains(mx, my);
    bool inScene = scenePanel->contentRect.Contains(mx, my);
    bool inAsset = assetBrowser->contentRect.Contains(mx, my);
    lastMsg += " H=" + std::to_string(inHier) + " S=" + std::to_string(inScene) + " A=" + std::to_string(inAsset);
    if (inHier) {
        for (int i = 0; i < (int)hierarchy->objects.size(); i++) {
            int y = hierarchy->contentRect.y + 4 + i * 24;
            if (y + 24 > hierarchy->rect.y + hierarchy->rect.h - 26) break;
            if (i >= hierarchy->scrollOffset && mx >= hierarchy->contentRect.x + 2 && mx <= hierarchy->contentRect.x + hierarchy->contentRect.w - 2 && my >= y && my <= y + 22) {
                hierarchy->selectedIndex = i;
                ctxMenu.Open(mx, my, {"Create Script Line", "---", "Duplicate", "Copy", "Paste", "---", "Move Up", "Move Down", "Bring to Front", "Send to Back", "---", "Delete", "Hide"});
                lastMsg += " HIER_ITEM";
                return;
            }
        }
        lastMsg += " HIER_EMPTY";
        return;
    }
    if (inScene) {
        for (int i = (int)hierarchy->objects.size() - 1; i >= 0; i--) {
            auto& obj = hierarchy->objects[i];
            int dx = scenePanel->contentRect.x + (int)(obj.posX * scenePanel->zoom) + scenePanel->panX;
            int dy = scenePanel->contentRect.y + (int)(obj.posY * scenePanel->zoom) + scenePanel->panY;
            int sw = (int)(obj.sizeW * scenePanel->zoom), sh = (int)(obj.sizeH * scenePanel->zoom);
            if (mx >= dx && mx <= dx + sw && my >= dy && my <= dy + sh) {
                scenePanel->selectedIndex = i;
                ctxMenu.Open(mx, my, {"Duplicate", "Copy", "Paste", "---", "Align Left", "Align Center H", "Align Right", "Align Top", "Align Center V", "Align Bottom", "---", "Move Up", "Move Down", "Bring to Front", "Send to Back", "---", "Delete", "Hide"});
                lastMsg += " SCENE_OBJ";
                return;
            }
        }
        ctxMenu.Open(mx, my, {"Add Rectangle", "Add Ellipse", "Add Text", "Add Image"});
        lastMsg += " SCENE_ADD";
        return;
    }
    if (inAsset) {
        printf("[RClick] AssetBrowser hit! files=%zu\n", assetBrowser->files.size());
        int fi = assetBrowser->GetFileIndexAt(mx, my);
        printf("[RClick] GetFileIndexAt=%d\n", fi);
        if (fi >= 0) {
            assetBrowser->selectedAsset = true; assetBrowser->selAssetIndex = fi; assetBrowser->selAssetPath = assetBrowser->currentDir + "/" + assetBrowser->files[fi];
            std::string fullPath = assetBrowser->currentDir + "/" + assetBrowser->files[fi];
            if (fs::is_directory(fullPath)) {
                ctxMenu.Open(mx, my, {"Open"});
            } else {
                ctxMenu.Open(mx, my, {"Place on Scene", "Place in Hierarchy", "---", "Rename", "Delete"});
            }
            lastMsg += " ASSET_PLACE";
            printf("[RClick] ctxMenu opened: asset actions at %d,%d\n", mx, my);
        } else {
            ctxMenu.Open(mx, my, {"New Folder", "Add Empty File", "Add Material", "Refresh"});
            lastMsg += " ASSET_ADD";
            printf("[RClick] ctxMenu opened: New Folder etc at %d,%d\n", mx, my);
        }
        return;
    }
    lastMsg += " NOWHERE";
}

void EditorWindow::HandleDrag(int mx, int my) {
    DBG("HandleDrag(%d,%d) dragSplitter=%d", mx, my, dragSplitter);
    timeline->HandleDrag(mx, my, hierarchy->objects);
    int winW = inspector->rect.x + inspector->rect.w;
    int winH = timeline->rect.y + timeline->rect.h;
    if (winW <= 0 || winH <= 0) return;
    if (dragSplitter == 1) {
        leftRatio = (float)mx / winW;
        if (leftRatio < 0.05f) leftRatio = 0.05f;
        if (leftRatio > 0.50f) leftRatio = 0.50f;
        return;
    }
    if (dragSplitter == 2) {
        rightRatio = (float)mx / winW;
        if (rightRatio < leftRatio + 0.10f) rightRatio = leftRatio + 0.10f;
        if (rightRatio > 0.95f) rightRatio = 0.95f;
        return;
    }
    if (dragSplitter == 3) {
        bottomRatio = (float)my / winH;
        if (bottomRatio < 0.30f) bottomRatio = 0.30f;
        if (bottomRatio > 0.90f) bottomRatio = 0.90f;
        return;
    }
    if (dragSplitter == 4) {
        timelineRatio = (float)(winH - my) / winH;
        if (timelineRatio < 0.04f) timelineRatio = 0.04f;
        if (timelineRatio > 0.50f) timelineRatio = 0.50f;
        return;
    }
    if (scenePanning && scenePanel->contentRect.Contains(mx, my)) {
        scenePanel->panX = panStartPanX + (mx - panStartX);
        scenePanel->panY = panStartPanY + (my - panStartY);
        return;
    }
    if (scenePanel->activeGizmo != GIZMO_NONE && scenePanel->selectedIndex >= 0 && scenePanel->selectedIndex < (int)hierarchy->objects.size()) {
        auto& obj = hierarchy->objects[scenePanel->selectedIndex];
        int dx = mx - scenePanel->gizmoStartX, dy = my - scenePanel->gizmoStartY;
        int adx = (int)(dx / scenePanel->zoom), ady = (int)(dy / scenePanel->zoom);
        int snap = 1;
#ifdef _WIN32
        if (scenePanel->snapEnabled || (GetAsyncKeyState(VK_SHIFT) & 0x8000)) snap = snapSize;
#endif
        switch (scenePanel->activeGizmo) {
            case GIZMO_MOVE:
                obj.posX = scenePanel->gizmoObjX + adx;
                obj.posY = scenePanel->gizmoObjY + ady;
                if (snap > 1) {
                    obj.posX = (obj.posX / snap) * snap;
                    obj.posY = (obj.posY / snap) * snap;
                }
                break;
            case GIZMO_RESIZE_TL: obj.posX = scenePanel->gizmoObjX + adx; obj.posY = scenePanel->gizmoObjY + ady; obj.sizeW = scenePanel->gizmoObjW - adx; obj.sizeH = scenePanel->gizmoObjH - ady; break;
            case GIZMO_RESIZE_TR: obj.posY = scenePanel->gizmoObjY + ady; obj.sizeW = scenePanel->gizmoObjW + adx; obj.sizeH = scenePanel->gizmoObjH - ady; break;
            case GIZMO_RESIZE_BL: obj.posX = scenePanel->gizmoObjX + adx; obj.sizeW = scenePanel->gizmoObjW - adx; obj.sizeH = scenePanel->gizmoObjH + ady; break;
            case GIZMO_RESIZE_BR: obj.sizeW = scenePanel->gizmoObjW + adx; obj.sizeH = scenePanel->gizmoObjH + ady; break;
            default: break;
        }
        if (obj.sizeW < 10) obj.sizeW = 10;
        if (obj.sizeH < 10) obj.sizeH = 10;
    }
}

void EditorWindow::HandleScroll(int delta, int mouseY) {
    int mouseX = 0;
#ifdef _WIN32
    POINT pt; GetCursorPos(&pt);
    HWND hw = GetActiveWindow(); if (hw) { ScreenToClient(hw, &pt); mouseX = pt.x; }
#endif
    if (hierarchy->rect.Contains(mouseX, mouseY) && hierarchy->visible) {
        hierarchy->scrollOffset += delta > 0 ? -1 : 1;
        if (hierarchy->scrollOffset < 0) hierarchy->scrollOffset = 0;
        int maxOff = std::max(0, (int)hierarchy->objects.size() - 1);
        if (hierarchy->scrollOffset > maxOff) hierarchy->scrollOffset = maxOff;
    } else if (scriptPanel->rect.Contains(mouseX, mouseY) && scriptPanel->visible) {
        scriptPanel->scrollOffset += delta > 0 ? -1 : 1;
        if (scriptPanel->scrollOffset < 0) scriptPanel->scrollOffset = 0;
        int maxOff = std::max(0, (int)scriptPanel->lines.size() - 1);
        if (scriptPanel->scrollOffset > maxOff) scriptPanel->scrollOffset = maxOff;
    } else if (assetBrowser->rect.Contains(mouseX, mouseY) && assetBrowser->visible) {
        assetBrowser->scrollOffset += delta > 0 ? -1 : 1;
        if (assetBrowser->scrollOffset < 0) assetBrowser->scrollOffset = 0;
        int maxOff = std::max(0, (int)assetBrowser->files.size() - 1);
        if (assetBrowser->scrollOffset > maxOff) assetBrowser->scrollOffset = maxOff;
    } else if (timeline->rect.Contains(mouseX, mouseY) && timeline->visible) {
        timeline->scrollOffset += delta > 0 ? -1 : 1;
        if (timeline->scrollOffset < 0) timeline->scrollOffset = 0;
    } else {
        scenePanel->HandleScroll(delta);
    }
}

void EditorWindow::HandleKey(int key) {
    DBG("HandleKey key=%d (0x%x) preview=%d inspF=%d scrF=%d", key, key, IsPreviewActive(), inspector ? inspector->editingField : -1, scriptPanel ? scriptPanel->editingField : -1);
    if (IsPreviewActive()) {
        if (key == VK_ESCAPE) { StopPreview(); return; }
        if (key == VK_SPACE || key == VK_RETURN) { currentTick++; RenderPreview(); }
        return;
    }
    if (inspector->editingField) {
        if (key == VK_RETURN || key == VK_ESCAPE) { inspector->CommitEdit(); MarkDirty(); }
        else if (key == VK_BACK && !inspector->editBuf.empty()) { inspector->editBuf.pop_back(); }
        return;
    }
    if (scriptPanel->editingField) {
        bool wasEditing = (scriptPanel->editingField != 0);
        scriptPanel->HandleKey(key);
        if (wasEditing && scriptPanel->editingField == 0) MarkDirty();
        return;
    }
    if (assetBrowser->editingField) { assetBrowser->HandleKey(key); return; }
    // Hierarchy filter editing
    if (hierarchy->editingFilter) {
        if (key == VK_RETURN || key == VK_ESCAPE) { hierarchy->editingFilter = 0; return; }
        if (key == VK_BACK && !hierarchy->filter.empty()) { hierarchy->filter.pop_back(); return; }
        return;
    }
    if (key == VK_F1) { showDebug = !showDebug; return; }
    if (key == VK_F5) { if (!scriptPanel->lines.empty()) StartPreview(); return; }
    if (key == VK_DELETE && hierarchy->selectedIndex >= 0) { PushUndo(); hierarchy->DeleteSelected(); SyncSelection(); MarkDirty(); return; }
    // Ctrl+ shortcuts
    if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
        if (key == 'Z') { Undo(); return; }
        if (key == 'Y') { Redo(); return; }
        if (key == 'N') {
            hierarchy->objects.clear(); scriptPanel->lines.clear(); assetBrowser->files.clear();
            hierarchy->selectedIndex = -1; scenePanel->selectedIndex = -1; inspector->SetTarget(nullptr);
            hierarchy->AddObject("Background", "background"); SyncSelection();
            currentProjectPath.clear(); dirty = false;
            lastMsg = "New project"; lastMsgTimer = 60; return;
        }
        if (key == 'S') { SaveCurrentProject(); return; }
        if (key == 'O') {
            char fileBuf[260] = {}; OPENFILENAMEA ofn = {};
            ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = GetActiveWindow();
            ofn.lpstrFile = fileBuf; ofn.nMaxFile = 260;
            ofn.lpstrFilter = "Novell Project (*.vne)\0*.vne\0All\0*.*\0";
            ofn.Flags = OFN_FILEMUSTEXIST;
            if (GetOpenFileNameA(&ofn)) { LoadProjectFrom(fileBuf); SyncSelection(); }
            return;
        }
        if (key == 'D') { DuplicateSelected(); return; }
        if (key == 'F') { ZoomToFitScene(); return; }
        if (key == 'C') { CopySelected(); return; }
        if (key == 'V') { PasteSelected(); return; }
        if (key == 'G') { ToggleGrid(); return; }
        if (key == 'M') { ToggleSnap(); return; }
    }
    if (key == 'R' || key == 'r') { AddShape("rectangle", scenePanel->contentRect.x + 100, scenePanel->contentRect.y + 100); return; }
    if (key == 'E' || key == 'e') { AddShape("ellipse", scenePanel->contentRect.x + 200, scenePanel->contentRect.y + 150); return; }
    if (key == 'T' || key == 't') { AddShape("text", scenePanel->contentRect.x + 300, scenePanel->contentRect.y + 200); return; }
    if (key == VK_OEM_4) { snapSize = std::max(1, snapSize - 5); lastMsg = "Snap: " + std::to_string(snapSize); lastMsgTimer = 60; MarkDirty(); return; }
    if (key == VK_OEM_6) { snapSize += 5; lastMsg = "Snap: " + std::to_string(snapSize); lastMsgTimer = 60; MarkDirty(); return; }
    if (key == 'A' || key == 'a') {
        std::ofstream("assets/Images/test_asset.png").close();
        assetBrowser->ScanDirectory();
        lastMsg = "Created test_asset.png";
        lastMsgTimer = 120;
        return;
    }
}

void EditorWindow::PushUndo() {
    if ((int)undoStack.size() > 50) undoStack.erase(undoStack.begin());
    EditorSnapshot s;
    s.objects = hierarchy->objects;
    s.lines = scriptPanel->lines;
    undoStack.push_back(s);
    redoStack.clear();
}

void EditorWindow::Undo() {
    if (undoStack.empty()) return;
    EditorSnapshot cur; cur.objects = hierarchy->objects; cur.lines = scriptPanel->lines;
    redoStack.push_back(cur);
    hierarchy->objects = undoStack.back().objects;
    scriptPanel->lines = undoStack.back().lines;
    undoStack.pop_back();
    SyncSelection();
    lastMsg = "Undo"; lastMsgTimer = 60;
}

void EditorWindow::Redo() {
    if (redoStack.empty()) return;
    EditorSnapshot cur; cur.objects = hierarchy->objects; cur.lines = scriptPanel->lines;
    undoStack.push_back(cur);
    hierarchy->objects = redoStack.back().objects;
    scriptPanel->lines = redoStack.back().lines;
    redoStack.pop_back();
    SyncSelection();
    lastMsg = "Redo"; lastMsgTimer = 60;
}

void EditorWindow::HandleFileDrop(const std::string& filePath) {
    printf("[Drop] %s\n", filePath.c_str());
    lastMsg = "Dropped: " + filePath;
    lastMsgTimer = 9999;
    // Copy file to current asset dir
    size_t sep = filePath.find_last_of("/\\");
    std::string fn = (sep != std::string::npos) ? filePath.substr(sep + 1) : filePath;
    std::string dest = assetBrowser->currentDir + "/" + fn;
    CopyFileA(filePath.c_str(), dest.c_str(), FALSE);
    assetBrowser->ScanDirectory();
    // Place sprite in scene
    int sx = 50 + ((int)hierarchy->objects.size() * 30) % 400;
    int sy = 50 + ((int)hierarchy->objects.size() * 30) % 300;
    int idx = hierarchy->AddObject(fn, "sprite", sx, sy);
    hierarchy->objects[idx].spritePath = dest;
    scenePanel->selectedIndex = idx;
    hierarchy->selectedIndex = idx;
    inspector->SetTarget(&hierarchy->objects[idx]);
}

void EditorWindow::HandleEvents(const std::vector<WindowEvent>& events) {
    std::vector<WindowEvent> allEvents = events;
    allEvents.insert(allEvents.end(), eventQueue.begin(), eventQueue.end());
    eventQueue.clear();
    for (auto& e : allEvents) {
        switch (e.type) {
            case WindowEvent::MOUSE_CLICK: HandleClick(e.param1, e.param2); break;
            case WindowEvent::MOUSE_RCLICK: HandleRClick(e.param1, e.param2); break;
            case WindowEvent::MOUSE_DRAG: HandleDrag(e.param1, e.param2); break;
            case WindowEvent::MOUSE_SCROLL: HandleScroll(e.param1, e.param2); break;
            case WindowEvent::CHAR_INPUT: {
                int c = e.param1;
                if (c <= 32) break;
                // Convert UTF-16 code unit to UTF-8
                char utf8Buf[4] = {};
                int utf8Len = 0;
#ifdef _WIN32
                wchar_t wc = (wchar_t)c;
                utf8Len = WideCharToMultiByte(CP_UTF8, 0, &wc, 1, utf8Buf, 4, nullptr, nullptr);
                if (utf8Len <= 0) break;
#endif
                std::string utf8Str(utf8Buf, utf8Len > 0 ? utf8Len : 1);
                if (utf8Len <= 0) { utf8Str = std::string(1, (char)c); }
                if (scriptPanel->editingField && scriptPanel->editBuf.size() < 80) {
                    scriptPanel->editBuf += utf8Str;
                    if (scriptPanel->editingLine >= 0 && scriptPanel->editingLine < (int)scriptPanel->lines.size()) {
                        if (scriptPanel->editingField == 1) scriptPanel->lines[scriptPanel->editingLine].characterName = scriptPanel->editBuf;
                        else if (scriptPanel->editingField == 2) scriptPanel->lines[scriptPanel->editingLine].text = scriptPanel->editBuf;
                        else if (scriptPanel->editingField == 3) scriptPanel->lines[scriptPanel->editingLine].spritePath = scriptPanel->editBuf;
                        else if (scriptPanel->editingField == 4) scriptPanel->lines[scriptPanel->editingLine].backgroundPath = scriptPanel->editBuf;
                    }
                } else if (inspector->editingField && inspector->editBuf.size() < 40 && utf8Len == 1) {
                    inspector->editBuf += (char)c;
                } else if (assetBrowser->editingField && assetBrowser->editBuf.size() < 60) {
                    assetBrowser->editBuf += utf8Str;
                } else if (hierarchy->editingFilter && hierarchy->filter.size() < 30) {
                    hierarchy->filter += utf8Str;
                    hierarchy->scrollOffset = 0;
                }
                break;
            }
            case WindowEvent::KEY_DOWN: HandleKey(e.param1); break;
            default: break;
        }
    }
}




