@echo off
setlocal enabledelayedexpansion

:: Navigate to project root directory
cd /d "%~dp0.."

echo ===============================================================================
echo                DJ Audio-Reactive Visualizer - Build Script
echo ===============================================================================

:: 1. Detect Visual Studio Environment
set VSWHERE="C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"

if exist %VSWHERE% (
    for /f "usebackq tokens=*" %%i in (`%VSWHERE% -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        set "VS_PATH=%%i"
    )
)

if defined VS_PATH (
    if exist "!VS_PATH!\VC\Auxiliary\Build\vcvars64.bat" (
        echo [INFO] Found Visual Studio at: !VS_PATH!
        call "!VS_PATH!\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    )
)

:: 2. Create build directory
if not exist "build" (
    mkdir build
)

cd build

:: 3. Configure CMake
echo [INFO] Configuring CMake project (C++20 MSVC)...
cmake -G "Visual Studio 17 2022" -A x64 .. -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configuration failed.
    cd ..
    exit /b %ERRORLEVEL%
)

:: 4. Build Release
echo [INFO] Compiling DJVisualizer Release x64...
cmake --build . --config Release
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Compilation failed.
    cd ..
    exit /b %ERRORLEVEL%
)

cd ..

echo ===============================================================================
echo [SUCCESS] Build Complete! Executable is ready at:
echo           build\bin\DJVisualizer.exe
echo ===============================================================================
