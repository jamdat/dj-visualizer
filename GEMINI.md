# DJ Audio-Reactive HLSL Visualizer Domain & Architecture

## Overview & Purpose
- **Description**: High-performance, standalone C++ / Direct3D 11 real-time audio-reactive visualizer designed for live DJ performances on stage projectors and LED walls with Daft Punk "Alive" retro-futuristic concert themes. Built as a turnkey deliverable for DJs with an intuitive ImGui control panel and a live hot-reloading pure HLSL shader pipeline.
- **Target Audience / End Users**: DJs, VJs, and live performers needing a plug-and-play, zero-setup visualizer.

## Tech Stack & Tooling
- **Language / Runtime**: C++20 / Windows API
- **Graphics API**: Direct3D 11 (D3D11) & HLSL (Shader Model 5.0 / 6.0)
- **Audio Capture & DSP**: Windows WASAPI (Loopback & Line-in) + FFT Audio Analyzer + Beat/Onset Detector
- **UI / Control**: Dear ImGui (DJ performance dashboard, preset manager, auto-pilot)
- **Build System**: CMake (Visual Studio 2022 / MSVC / Ninja)

## Project Architecture & Directory Layout
- `src/audio/`: WASAPI loopback & line-in capture, FFT spectrum analyzer, and beat/onset detection
- `src/graphics/`: Direct3D 11 renderer, swapchains, window management, runtime HLSL hot-reloader
- `src/shaders/`: Modular HLSL shaders (Alive Pyramid, Synthwave Grid, Raymarched Crystals, Concert Lasers, Post-process Glitch)
- `src/control/`: Preset manager (JSON), auto-pilot beat switcher, live parameters
- `src/ui/`: Dear ImGui DJ live performance control dashboard
- `scripts/`: Build and launch batch scripts (`build.bat`, `run.bat`)

## Domain Constraints & Guidelines
- **Turnkey Client Portability**: Visualizer must package into a standalone portable folder with zero third-party software dependencies for the client.
- **HLSL Live Shader Pipeline**: All shader math and logic must use standard HLSL with Unreal-style structures and support instant hot-reloading (`Ctrl+S` -> <100ms update).
- **Zero Audio Latency & High FPS**: Sub-10ms audio capture latency; rock-solid 60+ FPS on dual displays.
