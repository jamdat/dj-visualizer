@echo off
setlocal enabledelayedexpansion

:: Navigate to project root directory
cd /d "%~dp0.."

echo ===============================================================================
echo            DJ Audio-Reactive Visualizer - Packaging Deliverable
echo ===============================================================================

:: 1. Ensure latest Release binary is built
echo [1/5] Building latest Release binaries...
cmake --build build --config Release
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed! Please fix build errors before packaging.
    exit /b %ERRORLEVEL%
)

if not exist "build\bin\DJVisualizer.exe" (
    echo [ERROR] DJVisualizer.exe not found in build\bin\
    exit /b 1
)

:: 2. Prepare staging directory
set "DIST_ROOT=%cd%\dist"
set "PACKAGE_DIR=%DIST_ROOT%\DJVisualizer"
set "ZIP_FILE=%DIST_ROOT%\DJVisualizer_v1.0.zip"

echo [2/5] Preparing clean distribution directory: %PACKAGE_DIR%
if exist "%PACKAGE_DIR%" rmdir /s /q "%PACKAGE_DIR%"
if exist "%ZIP_FILE%" del /f /q "%ZIP_FILE%"

mkdir "%PACKAGE_DIR%"
mkdir "%PACKAGE_DIR%\src\shaders"
mkdir "%PACKAGE_DIR%\presets"
mkdir "%PACKAGE_DIR%\assets"
if exist "assets\textures" mkdir "%PACKAGE_DIR%\assets\textures"

:: 3. Copy Executable, Dependencies, Shaders, Presets, and Assets
echo [3/5] Copying binaries and runtime assets...
copy /y "build\bin\DJVisualizer.exe" "%PACKAGE_DIR%\" >nul

if exist "build\bin\d3dcompiler_47.dll" (
    copy /y "build\bin\d3dcompiler_47.dll" "%PACKAGE_DIR%\" >nul
) else if exist "C:\Windows\System32\d3dcompiler_47.dll" (
    copy /y "C:\Windows\System32\d3dcompiler_47.dll" "%PACKAGE_DIR%\" >nul
)

:: Copy shaders (HLSL source and compiled CSO bytecode)
xcopy /s /y /q "src\shaders\*" "%PACKAGE_DIR%\src\shaders\" >nul
if exist "build\bin\src\shaders\*.cso" (
    copy /y "build\bin\src\shaders\*.cso" "%PACKAGE_DIR%\src\shaders\" >nul
)

:: Copy presets
if exist "presets\*" (
    xcopy /s /y /q "presets\*" "%PACKAGE_DIR%\presets\" >nul
) else if exist "build\bin\presets\*" (
    xcopy /s /y /q "build\bin\presets\*" "%PACKAGE_DIR%\presets\" >nul
)

:: Copy assets
if exist "assets\*" (
    xcopy /s /y /q "assets\*" "%PACKAGE_DIR%\assets\" >nul
)

:: 4. Generate Client Launcher and Readme
echo [4/5] Generating client launcher and documentation...

:: Client Launch Script
(
    echo @echo off
    echo cd /d "%%~dp0"
    echo start "" "DJVisualizer.exe"
) > "%PACKAGE_DIR%\Launch_Visualizer.bat"

:: Client Quick Start Documentation
(
    echo ===============================================================================
    echo      DJ AUDIO-REACTIVE VISUALIZER ^(Daft Alive Edition^) - CLIENT QUICK START
    echo ===============================================================================
    echo.
    echo 1. HOW TO LAUNCH
    echo -----------------
    echo - Double-click "Launch_Visualizer.bat" or "DJVisualizer.exe".
    echo - No installation, drivers, or virtual audio cables required!
    echo.
    echo 2. AUDIO INPUT ^(ZERO-SETUP AUTOMATIC WASAPI LOOPBACK^)
    echo ----------------------------------------------------
    echo - Play music through your DJ software ^(rekordbox, Serato, Traktor, Ableton,
    echo   VirtualDJ, or Spotify/browser^).
    echo - The visualizer automatically taps into Windows Master Audio Output with
    echo   zero latency and synchronizes visuals with kick drums, basslines, and beats.
    echo - If the audio level is too soft or loud, adjust the "Audio Gain" slider in
    echo   the DJ Dashboard window.
    echo.
    echo 3. DUAL-DISPLAY STAGE SETUP
    echo ---------------------------
    echo - Primary Screen ^(Laptop^): DJ Performance Control Dashboard ^(ImGui^).
    echo - Secondary Screen ^(Stage Projector / LED Wall^): Dedicated Stage Output window.
    echo - Click "Projector / LED Wall" in the dashboard to send visuals fullscreen
    echo   to the external stage screen.
    echo - Press F11 on the visualizer window to toggle borderless fullscreen.
    echo - Press ESC to exit fullscreen.
    echo.
    echo 4. THEMES ^& PRESETS
    echo -------------------
    echo - Alive Pyramid: Iconic Daft Punk Alive 2007 wireframe matrix and LEDs.
    echo - Tron Synthwave: Endless 80s perspective grid and retro audio sun.
    echo - Raymarched Crystals: Morphing fractal monoliths pulsating to sub-bass.
    echo - Concert Lasers: Sweeping moving-head laser fans and beam haze.
    echo - Use the "Auto-Pilot" toggle in the DJ Dashboard to let visuals transition
    echo   and react automatically during your set.
    echo - Save/Load custom presets anytime from the Presets panel.
    echo.
    echo 5. LIVE SHADER CUSTOMIZATION
    echo ----------------------------
    echo - All visuals are powered by pure Direct3D 11 HLSL shaders in the "src\shaders\"
    echo   folder. Modifying any .hlsl file and pressing Ctrl+S updates the visuals live
    echo   instantly without interrupting the performance.
    echo.
    echo ===============================================================================
) > "%PACKAGE_DIR%\README.txt"

:: 5. Compress to Standalone ZIP
echo [5/5] Creating portable release ZIP archive...
powershell -NoProfile -Command "Compress-Archive -Path '%PACKAGE_DIR%' -DestinationPath '%ZIP_FILE%' -Force"

if %ERRORLEVEL% neq 0 (
    echo [ERROR] Failed to create ZIP archive.
    exit /b %ERRORLEVEL%
)

echo ===============================================================================
echo [SUCCESS] Client deliverable package created successfully!
echo.
echo Portable Folder : %PACKAGE_DIR%
echo ZIP Package     : %ZIP_FILE%
echo ===============================================================================
