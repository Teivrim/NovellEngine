@echo off
chcp 65001 >nul
cd /d "%~dp0"
echo Compiling Visual Novel Engine...

set CXX=g++
set CFLAGS=-std=c++20 -g -I"%~dp0include"
set LFLAGS=-static -lgdi32 -lgdiplus -lcomctl32 -lshlwapi -lcomdlg32
set SRC_DIR=%~dp0
set BUILD_DIR=%~dp0build
set OUT=%BUILD_DIR%\NovellEngine.exe

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

%CXX% -std=c++20 -g -I"%~dp0include" "%~dp0src\main.cpp" "%~dp0src\Core\ProjectManager.cpp" "%~dp0src\UI\EditorPanels.cpp" "%~dp0src\Graphics\WinAPIGraphics.cpp" -o "%OUT%" %LFLAGS%

if %ERRORLEVEL% NEQ 0 ( echo Build FAILED! & exit /b 1 )
echo Build successful! Output: %OUT%
