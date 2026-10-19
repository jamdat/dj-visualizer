# DJ Audio-Reactive HLSL Visualizer Domain & Architecture

## Overview & Purpose
- **Description**: High-performance, standalone C++20 cross-platform (Windows x64 & macOS Apple Silicon `arm64`) real-time audio-reactive visualizer designed for live DJ performances on stage projectors and LED walls. Built as a turnkey deliverable with an intuitive Dear ImGui DJ dashboard, a live hot-reloading HLSL shader pipeline, and an integrated 1-click GitHub Releases auto-updater.
- **Target Audience / End Users**: DJs, VJs, and live performers needing a plug-and-play, zero-setup visualizer on Windows or Mac.

## Tech Stack & Tooling
- **Language / Runtime**: C++20 (plus Objective-C++ `.mm` on macOS)
- **Windowing & Graphics API**: Unified **GLFW 3.4 + OpenGL 3.3 Core** across Windows and macOS (`src/graphics/GL33Loader.h`)
- **Shader Pipeline**: Pure HLSL (`src/shaders/*.hlsl`) with in-memory `#include` resolution and automatic **HLSL-to-GLSL 330 Core transpiler** (`src/graphics/ShaderCompiler.cpp`) supporting live hot-reload (`<100ms`)
- **Audio Capture & DSP**:
  - **Windows**: WASAPI Loopback (system audio) & WASAPI Line-In/Microphone (`src/audio/WasapiAudioCapture.cpp`)
  - **macOS (13.0+)**: `ScreenCaptureKit` (`SCStream` system audio loopback without virtual audio drivers) & `CoreAudio` / `AVAudioEngine` Line-In/Microphone (`src/audio/MacAudioCapture.mm`)
  - **Shared DSP**: 1024-sample Hann-windowed radix-2 FFT spectrum analyzer, 4-band separator, and beat/onset detector (`src/audio/AudioAnalyzer.cpp`)
- **UI & Control**: Dear ImGui (`imgui_impl_glfw` + `imgui_impl_opengl3`), JSON preset manager, auto-pilot beat switcher, MIDI controller (`WinMM` on Windows, `CoreMIDI` on macOS), and GitHub Releases in-app auto-updater (`src/control/AutoUpdater.cpp`)
- **Build & CI/CD**: CMake 3.20+ (`CMakeLists.txt` with `FetchContent` for GLFW 3.4) and GitHub Actions (`.github/workflows/release.yml`) building `DJVisualizer-Windows-x64.zip` and `DJVisualizer-macOS-arm64.zip`

## Project Architecture & Directory Layout
- `src/audio/`: Cross-platform audio capture (`WasapiAudioCapture.h/.cpp`, `MacAudioCapture.mm`) and shared FFT/beat DSP (`AudioAnalyzer.h/.cpp`)
- `src/graphics/`:
  - `WindowManager.h/.cpp`: GLFW 3.4 dual-window manager (Control Console window + secondary fullscreen Stage Projector window sharing OpenGL context textures)
  - `D3D11Renderer.h/.cpp`: Cross-platform OpenGL 3.3 Core renderer (scene & post-process FBOs, `std140` UBO, per-context VAOs for Control and Stage windows)
  - `ShaderCompiler.h/.cpp`: Live `.hlsl` file watcher, `#include` preprocessor, and HLSL-to-GLSL 330 Core transpiler
  - `TextureManager.h/.cpp`: Dynamic `512x2` `GL_R32F` audio FFT/waveform texture (`t0`) and procedural textures (`t1`..`t4`)
  - `ConstantBuffers.h`: Cross-platform 16-byte aligned `AudioVisualizerParams` (272 bytes, 17 vectors, `static_assert` guarded) matching HLSL `cbuffer` and GLSL `std140` layout
- `src/shaders/`: Unified Dad Punk HLSL shaders (`Common.hlsl` library, `ModularMixer.hlsl` composable stage engine)
  - **4-Sided Alive Pyramid**: Top-center apex continuing physical PVC pyramid frame, 5 reactive face patterns (VU bars, chevrons, scanner, solid glow, rainbow)
  - **Custom Daft Logo**: 5x7 dot-matrix font renderer with live editable text buffer (`CustomText[4]`, default `"DAD PUNK"`)
  - **TRON Grid Floor**: Ray-plane floor ($y = -1.35$) with obsidian mirror reflections & light-cycle trails
  - **Volumetric Lasers & Spooky FX**: Apex lasers over the crowd, Halloween moon, bats, jack-o'-lanterns
  - **Photosafety**: Zero strobe, brightness capped at 85%, flash rate limiter ($\le 3$ spikes/sec)
- `src/control/`: `PresetManager` (6 curated Dad Punk scenes: AlivePyramid, TronGridRun, SpookyPunk, LedWall, DadPunkHero, ChillAttract), `AutoPilot` (non-destructive `UserSettings` -> `DirectorState` -> `GpuParams` pipeline), `MidiController` (WinMM / CoreMIDI), and `AutoUpdater` (GitHub Releases API + 1-click self-update)
- `src/ui/`: `DjDashboard.h/.cpp` (Dear ImGui live control console with big mood buttons, custom logo text box, 5 palette swatches, and top-bar updater UI)
- `scripts/`: Build, launch, and packaging scripts (`build.bat`, `run.bat`, `package.bat`, `package_mac.sh`)

## Agent Handoff Notes (For Visualizer Expansion / New Shader Plans)
1. **Adding or Modifying Shaders (`src/shaders/*.hlsl`)**:
   - Write standard HLSL pixel shaders with entry point `float4 PSMain(PSInput input) : SV_TARGET` and `#include "Common.hlsl"`.
   - `ShaderCompiler::TranspileHLSLToGLSL330` automatically converts HLSL idioms (`lerp`, `saturate`, `frac`, `ddx`/`ddy`, `atan2`, `mul`, `SampleLevel`, `[unroll]`, float `f` suffixes, and `cbuffer AudioVisualizerParams`) to GLSL 330 Core at runtime on both Windows and macOS.
2. **Adding New Uniforms (`ConstantBuffers.h` & `Common.hlsl`)**:
   - `AudioVisualizerParams` in `src/graphics/ConstantBuffers.h` and `cbuffer AudioVisualizerParams : register(b0)` in `src/shaders/Common.hlsl` **must stay byte-for-byte identical** and aligned to 16-byte (`Float4`) boundaries for `std140` uniform block compatibility (currently 272 bytes).
3. **Texture Slots**:
   - `t0` (`AudioSpectrumTex`): `512x2` `R32F` texture (Row 0 = FFT spectrum, Row 1 = oscilloscope waveform).
   - `t1` (`SceneColorTex`, in post-process pass): Rendered scene FBO texture.
   - `t2` (`NoiseTex`), `t3` (`LogoTex`), `t4` (`LutTex`), `t5` (`DirtTex`): Procedural lookup textures bound by `TextureManager::bindAll`.
4. **Automatic Release & Git Submission Rule (Zero-Friction Workflow)**:
   - **Proactive Prompt at Stopping Points**: Whenever a feature, bugfix, or milestone reaches a good stopping point and passes local build verification, the agent MUST automatically ask the user: *"Ready to commit, push, and build a new Windows + Mac release?"* (do not wait for the user to ask how to submit to git).
   - **Small Patch Version Increments Only**: Only increment the patch number by `+0.0.1` (e.g., `v1.0.1` -> `v1.0.2` -> `v1.0.3`) in both `CMakeLists.txt` (`DJVISUALIZER_VERSION`) and `src/control/AutoUpdater.h` (`DJVISUALIZER_VERSION`). **Never** do drastic major (`2.0.0`) or minor (`1.1.0`) version jumps unless the user explicitly asks for it.
   - **Automated Push & CI Verification**: Once confirmed by the user, the agent automatically stages changes, commits, creates the matching git tag (`git tag v1.0.x`), pushes (`git push origin master --tags`), and verifies the GitHub Actions Windows + macOS release build.
