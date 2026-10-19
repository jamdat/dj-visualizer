@echo off
setlocal

set "PROJECT_ROOT=%~dp0.."
pushd "%PROJECT_ROOT%"

echo ===============================================================================
echo                DJ Audio-Reactive Visualizer - Starting...
echo ===============================================================================

if exist "build\bin\DJVisualizer.exe" (
    pushd "build\bin"
    start "" "DJVisualizer.exe"
    popd
    echo [SUCCESS] Launched DJVisualizer.exe
) else if exist "build\bin\Release\DJVisualizer.exe" (
    pushd "build\bin\Release"
    start "" "DJVisualizer.exe"
    popd
    echo [SUCCESS] Launched DJVisualizer.exe
) else (
    echo [ERROR] DJVisualizer.exe not found in build\bin\
    echo Please run scripts\build.bat first to build the project.
    pause
)

popd
