# DJ Audio-Reactive HLSL Visualizer (Daft Alive 2007 Edition)

A turnkey, high-performance C++20 / Direct3D 11 real-time audio-reactive visualizer designed for live DJ performances, stage projectors, LED walls, and Unreal Engine 5 virtual production.

---

## Key Features

### Live DJ & Performance Features:
- **Zero-Setup Audio Capture**: Automatic WASAPI Loopback captures music from any DJ software (rekordbox, Serato, Traktor, Ableton, VirtualDJ, Spotify) with no virtual cables required.
- **Dual-Display Management**: Primary DJ control dashboard on laptop (Display 1) + dedicated borderless fullscreen Stage Output on projector / LED wall (Display 2).
- **Curated Concert Themes**:
  1. 🔺 **Alive Pyramid & LED Matrix**: Iconic Daft Punk 2007 wireframe stage pyramid and animated stadium matrix.
  2. 🌆 **Tron Synthwave Horizon**: Endless 80s outrun perspective grid undulating to audio spectrum with retro sun.
  3. 💎 **3D Raymarched Crystals**: Morphing fractal monoliths and kaleidoscope tunnels pulsating to sub-bass.
  4. ⚡ **Concert Laser Matrix**: Sweeping moving head laser fans, beam haze, and stage blinders.
- **Auto-Pilot Mode**: Automatic scene transitions, camera orbit motion, and strobe bursts on beat drops.
- **Preset System**: Save and load complete stage setups as human-readable `.json` files.
- **MIDI Controller Support**: Map physical knobs and faders from DJ mixers directly to visual parameters.

### HLSL Live Shader Engine & Integration:
- **Unreal-Style HLSL Shaders**: Pure HLSL (`src/shaders/*.hlsl`) utilizing standard Unreal math, SDF primitives, noise functions, and camera rays.
- **Live Runtime Hot-Reloading**: Modify any `.hlsl` shader and press `Ctrl+S` — updates live on screen in <50ms without crashing.
- **Spout 2 GPU Texture Sharing**: Zero-latency Direct3D 11 shared texture handle (`DJVisualizer`) streaming directly into Unreal Engine 5, OBS Studio, and Resolume Arena.
- **Unreal Engine 5 Bridge (`ue5_bridge/`)**: Ready-to-use Material Custom Node snippets, `.ush` headers, Niagara particle equations, and Audio Synesthesia mapping guides.

---

## Quick Start & Building

### 1. Requirements
- Windows 10 / 11 (64-bit)
- Visual Studio 2022 (MSVC C++20) with Desktop C++ Workload
- CMake 3.20+

### 2. Build via Script
Double-click `scripts\build.bat` or run:
```cmd
scripts\build.bat
```

### 3. Launch
- **VS Code**: Press **`F5`** (automatically builds any modified code and launches the visualizer).
- **Scripts**: Double-click `scripts\run.bat` or launch `build\bin\DJVisualizer.exe`.

### 4. Package Deliverable for Client
Run the automated packaging script to produce a standalone zero-dependency portable ZIP:
```cmd
scripts\package.bat
```
Output:
- Portable folder: `dist\DJVisualizer\`
- Client ZIP bundle: `dist\DJVisualizer_v1.0.zip` (contains `.exe`, runtime DLLs, presets, shaders, and 1-click launcher)

---

## Hotkeys & Shortcuts
- `F11`: Toggle Fullscreen Presentation Mode on Primary Screen.
- `ESC`: Exit Fullscreen or close Stage Projector Window.
- `Ctrl+S` in any `.hlsl` file: Instant live shader recompilation.

---

## Unreal Engine 5 Integration
See [`ue5_bridge/UE5_Integration_Guide.md`](ue5_bridge/UE5_Integration_Guide.md) for full instructions on setting up Spout 2 streaming, Material Custom Nodes, and Niagara particle vectors in UE5.
