#include "WinAPIGraphics.hpp"
#include "ProjectManager.hpp"
#include "EditorPanels.hpp"
#include <ctime>
#include <algorithm>
#include <filesystem>
#include <cstdio>
#include <memory>
#ifdef _WIN32
#include <windows.h>
#endif
namespace fs = std::filesystem;

static std::string TimeStr() {
    time_t t = time(nullptr);
    std::string s = ctime(&t); s.pop_back();
    return s;
}

int main() {
#ifdef _WIN32
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
#endif
    printf("=== VNE v3.0 STARTED ===\n");
    // Set working directory to exe's parent directory (project root)
    char exePath[260]; GetModuleFileNameA(NULL, exePath, 260);
    std::string exeDir(exePath); size_t pos = exeDir.find_last_of("\\/");
    if (pos != std::string::npos) exeDir = exeDir.substr(0, pos);
    pos = exeDir.find_last_of("\\/"); // go up from build\ to project root
    if (pos != std::string::npos) exeDir = exeDir.substr(0, pos);
    SetCurrentDirectoryA(exeDir.c_str());
    char cwd[260]; GetCurrentDirectoryA(260, cwd); printf("CWD=%s\n", cwd);

    WinAPIGraphics gfx;
    if (!gfx.Init("VNE v3.0 - Rewrite", 960, 640)) return 1;

    ProjectManager pm;
    pm.Load("projects.dat");

    enum Screen { PROJECT_MANAGER, EDITOR, INPUT_DIALOG };
    Screen screen = PROJECT_MANAGER;
    int selProj = -1;
    if (pm.GetCount() > 0) selProj = 0;

    std::string inputText, inputPrompt;
    bool inputRename = false;

    // Auto-create test project for quick testing
    if (pm.GetCount() == 0) {
        Project p; p.name = "Test Project"; p.path = "projects/Test Project";
        p.description = "Auto-created"; p.createdAt = TimeStr(); pm.Add(p);
    }
    selProj = 0;

    std::unique_ptr<EditorWindow> editor;
    fs::create_directories("assets/Images");

    while (gfx.IsRunning()) {
        std::vector<WindowEvent> events;
        gfx.PollEvents(events);

        for (auto& e : events) {
            if (e.type == WindowEvent::CLOSE) {
                if (screen == EDITOR && editor && editor->dirty) {
                    int r = MessageBoxA(gfx.GetHWND(), "Save changes before closing?", "Unsaved Changes", MB_YESNOCANCEL);
                    if (r == IDYES) editor->SaveCurrentProject();
                    if (r == IDCANCEL) { continue; }
                }
                gfx.Shutdown(); return 0;
            }

            if (screen == PROJECT_MANAGER) {
                if (e.type == WindowEvent::MOUSE_CLICK) {
                    int mx = e.param1, my = e.param2;
                    int listY = 120;
                    for (int i = 0; i < pm.GetCount(); i++) {
                        if (mx >= 40 && mx <= gfx.GetWidth() - 40 && my >= listY && my <= listY + 40) selProj = i;
                        listY += 44;
                    }
                    int btnY = gfx.GetHeight() - 70, btnW = 140, btnH = 36, startX = 60;
                    if (mx >= startX && mx <= startX + btnW && my >= btnY && my <= btnY + btnH) {
                        screen = INPUT_DIALOG; inputText.clear(); inputPrompt = "Enter project name:"; inputRename = false;
                    }
                    if (mx >= startX + 160 && mx <= startX + 160 + btnW && my >= btnY && my <= btnY + btnH && selProj >= 0 && selProj < pm.GetCount()) {
                        pm.Delete(pm.List()[selProj].name); selProj = std::min(selProj, pm.GetCount() - 1);
                    }
                    if (mx >= startX + 320 && mx <= startX + 320 + btnW && my >= btnY && my <= btnY + btnH && selProj >= 0 && selProj < pm.GetCount()) {
                        screen = INPUT_DIALOG; inputText = pm.List()[selProj].name; inputPrompt = "Rename project to:"; inputRename = true;
                    }
                    if (mx >= startX + 480 && mx <= startX + 480 + btnW && my >= btnY && my <= btnY + btnH && selProj >= 0 && selProj < pm.GetCount()) {
                        editor = std::make_unique<EditorWindow>();
                        std::string projPath = pm.List()[selProj].path + "/scene.vne";
                        if (fs::exists(projPath)) editor->LoadProjectFrom(projPath);
                        else {
                            editor->hierarchy->AddObject("Background", "background");
                            editor->hierarchy->selectedIndex = 0;
                            editor->scenePanel->selectedIndex = 0;
                            editor->inspector->SetTarget(&editor->hierarchy->objects[0]);
                        }
                        editor->assetBrowser->ScanDirectory();
                        editor->scriptPanel->AddLine();
                        screen = EDITOR;
                    }
                }
                if (e.type == WindowEvent::MOUSE_DBLCLICK && selProj >= 0 && selProj < pm.GetCount()) {
                    editor = std::make_unique<EditorWindow>();
                    std::string projPath = pm.List()[selProj].path + "/scene.vne";
                    if (fs::exists(projPath)) editor->LoadProjectFrom(projPath);
                    else {
                        editor->hierarchy->AddObject("Background", "background");
                        editor->hierarchy->selectedIndex = 0;
                        editor->scenePanel->selectedIndex = 0;
                        editor->inspector->SetTarget(&editor->hierarchy->objects[0]);
                    }
                    editor->assetBrowser->ScanDirectory();
                    editor->scriptPanel->AddLine();
                    screen = EDITOR;
                }
                if (e.type == WindowEvent::KEY_DOWN && e.param1 == VK_ESCAPE) { gfx.Shutdown(); return 0; }
                if (e.type == WindowEvent::KEY_DOWN && e.param1 == VK_RETURN && selProj >= 0) {
                    editor = std::make_unique<EditorWindow>();
                    std::string projPath = pm.List()[selProj].path + "/scene.vne";
                    if (fs::exists(projPath)) editor->LoadProjectFrom(projPath);
                    else {
                        editor->hierarchy->AddObject("Background", "background");
                        editor->hierarchy->selectedIndex = 0; editor->scenePanel->selectedIndex = 0;
                        editor->inspector->SetTarget(&editor->hierarchy->objects[0]);
                    }
                    editor->assetBrowser->ScanDirectory();
                    editor->scriptPanel->AddLine();
                    screen = EDITOR;
                }
            }
            else if (screen == INPUT_DIALOG) {
                if (e.type == WindowEvent::CHAR_INPUT) {
                    char c = (char)e.param1;
                    if (c >= 32 && c <= 126 && inputText.size() < 40) inputText += c;
                }
                if (e.type == WindowEvent::KEY_DOWN) {
                    if (e.param1 == VK_RETURN && !inputText.empty()) {
                        if (inputRename && selProj >= 0 && selProj < pm.GetCount()) pm.Rename(pm.List()[selProj].name, inputText);
                        else if (!inputRename) {
                            Project p; p.name = inputText; p.path = "projects/" + inputText; p.description = "Created from Project Manager"; p.createdAt = TimeStr();
                            pm.Add(p); selProj = pm.GetCount() - 1;
                        }
                        screen = PROJECT_MANAGER;
                    }
                    if (e.param1 == VK_ESCAPE) { screen = PROJECT_MANAGER; }
                    if (e.param1 == VK_BACK && !inputText.empty()) { inputText.pop_back(); }
                }
            }
            else if (screen == EDITOR) {
                if (e.type == WindowEvent::KEY_DOWN && e.param1 == VK_ESCAPE && editor && !editor->IsPreviewActive()) {
                    if (editor->dirty) {
                        int r = MessageBoxA(gfx.GetHWND(), "Save changes before leaving?\nYes = Save   No = Discard   Cancel = Stay", "Unsaved Changes", MB_YESNOCANCEL);
                        if (r == IDYES) editor->SaveCurrentProject();
                        if (r == IDCANCEL) continue;
                    }
                    screen = PROJECT_MANAGER;
                }
            }
        }

        if (screen == EDITOR && editor) {
            editor->Layout(gfx.GetWidth(), gfx.GetHeight());
            editor->PollPreviewEvents();
            editor->HandleEvents(events);
            std::vector<std::string> drops; gfx.GetDroppedFiles(drops);
            for (auto& path : drops) editor->HandleFileDrop(path);
        }

        if (screen == PROJECT_MANAGER) {
            gfx.BeginFrame(); gfx.Clear(18, 18, 25);
            // Header
            gfx.DrawRect(0, 0, gfx.GetWidth(), 80, 20, 22, 30);
            gfx.DrawRect(0, 78, gfx.GetWidth(), 2, 45, 50, 60);
            gfx.RenderText("VISUAL NOVEL ENGINE", 40, 14, 255, 255, 255);
            gfx.RenderText("Project Manager", 40, 38, 160, 160, 190);
            gfx.RenderText("v0.3.0", 40, 56, 100, 100, 120);
            // Recent count badge
            std::string projCount = std::to_string(pm.GetCount()) + " project" + (pm.GetCount() != 1 ? "s" : "");
            int pcw = (int)projCount.size() * 9;
            gfx.DrawRect(gfx.GetWidth() - pcw - 50, 12, pcw + 30, 24, 35, 45, 70);
            gfx.RenderText(projCount, gfx.GetWidth() - pcw - 40, 16, 140, 200, 255);
            // Separator
            gfx.DrawRect(30, 95, gfx.GetWidth() - 60, 1, 40, 42, 50);
            gfx.RenderText("Your Projects:", 40, 104, 140, 140, 160);
            int listY = 128;
            if (pm.GetCount() == 0) gfx.RenderText("(No projects. Click 'Add Project' to create one.)", 50, listY, 90, 90, 110);
            else for (int i = 0; i < pm.GetCount(); i++) {
                int rh = 42;
                if (i == selProj) {
                    gfx.DrawRect(40, listY, gfx.GetWidth() - 80, rh, 35, 45, 75);
                    gfx.DrawRect(40, listY, 3, rh, 74, 125, 180);
                    gfx.RenderText(pm.List()[i].name, 55, listY + 4, 255, 220, 100);
                    gfx.RenderText("Path: " + pm.List()[i].path, 55, listY + 22, 140, 140, 160);
                } else {
                    gfx.DrawRect(40, listY, gfx.GetWidth() - 80, rh, 22, 24, 32);
                    gfx.DrawRect(40, listY, gfx.GetWidth() - 80, 1, 32, 34, 42);
                    gfx.RenderText(pm.List()[i].name, 55, listY + 4, 200, 200, 200);
                    gfx.RenderText("Path: " + pm.List()[i].path, 55, listY + 22, 100, 100, 120);
                }
                listY += rh + 2;
            }
            int btnY = gfx.GetHeight() - 70, btnW = 140, startX = 60;
            auto drawBtn = [&](int x, const char* label, int r, int g, int b) {
                gfx.DrawRect(x, btnY, btnW, 34, r, g, b);
                gfx.DrawRect(x, btnY, btnW, 1, r + 20, g + 20, b + 20);
                gfx.DrawRect(x, btnY + 33, btnW, 1, r - 10, g - 10, b - 10);
                gfx.RenderText(label, x + 14, btnY + 7, 220, 220, 230);
            };
            drawBtn(startX, "Add Project", 55, 70, 100);
            drawBtn(startX + 160, "Delete", 100, 60, 60);
            drawBtn(startX + 320, "Rename", 70, 70, 100);
            drawBtn(startX + 480, "Open Editor", 60, 100, 60);
            gfx.RenderText("Double-click or Enter to open | Esc to exit", 40, gfx.GetHeight() - 30, 80, 80, 100);
            gfx.EndFrame();
        }
        else if (screen == INPUT_DIALOG) {
            gfx.BeginFrame(); gfx.Clear(18, 18, 25);
            int dlgW = 440, dlgH = 150, dlgX = (gfx.GetWidth() - dlgW) / 2, dlgY = (gfx.GetHeight() - dlgH) / 2;
            gfx.DrawRect(dlgX, dlgY, dlgW, dlgH, 25, 27, 35);
            gfx.DrawRect(dlgX, dlgY, dlgW, 2, 74, 125, 180);
            gfx.DrawRect(dlgX + 1, dlgY + 2, dlgW - 2, dlgH - 3, 30, 32, 42);
            gfx.RenderText(inputPrompt, dlgX + 20, dlgY + 20, 220, 220, 230);
            gfx.DrawRect(dlgX + 20, dlgY + 50, dlgW - 40, 32, 18, 19, 26);
            gfx.DrawRect(dlgX + 20, dlgY + 50, dlgW - 40, 1, 40, 42, 50);
            gfx.RenderText(inputText + (inputText.size() < 40 ? "_" : ""), dlgX + 26, dlgY + 55, 255, 255, 200);
            gfx.RenderText("Enter = confirm   Escape = cancel", dlgX + 20, dlgY + dlgH - 24, 100, 100, 120);
            gfx.EndFrame();
        }
        else if (screen == EDITOR && editor) {
            gfx.BeginFrame();
            editor->Layout(gfx.GetWidth(), gfx.GetHeight());
            editor->Draw(&gfx);
            gfx.EndFrame();
            POINT pt; GetCursorPos(&pt);
            ScreenToClient(gfx.GetHWND(), &pt);
            int spl = editor->HitTestSplitter((int)pt.x, (int)pt.y);
            if (spl == 1 || spl == 2) gfx.SetCustomCursor("sizewe");
            else if (spl == 3 || spl == 4) gfx.SetCustomCursor("sizens");
            else gfx.SetCustomCursor("arrow");
        }

        Sleep(16);
    }
    return 0;
}


