# DJ Audio-Reactive Visualizer (Windows & macOS Apple Silicon)

A turnkey, high-performance C++20 cross-platform (**Windows x64** & **macOS Apple Silicon M1/M2/M3/M4**) real-time audio-reactive visualizer designed for live DJ performances, stage projectors, and LED walls.

---

## Key Features

### Live DJ & Performance Features
- **Zero-Setup System Audio & Line-In Capture**:
  - **Windows**: Automatic WASAPI Loopback captures system audio from any DJ software (rekordbox, Serato, Traktor, Ableton, VirtualDJ, Spotify) plus WASAPI Line-In/Microphone capture.
  - **macOS (13.0+)**: Native `ScreenCaptureKit` captures system audio directly with zero virtual audio drivers required, plus `CoreAudio` / `AVAudioEngine` support for external DJ mixers, audio interfaces, and microphones.
- **Dual-Display Stage Management**: Primary DJ Control Console on your laptop (Display 1) + dedicated borderless fullscreen Stage Projector Output on secondary displays/projectors (Display 2).
- **Live Hot-Reloading HLSL Shader Engine**:
  - Edit any `.hlsl` shader in `src/shaders/` and save (`Ctrl+S` / `Cmd+S`) — the built-in `#include` preprocessor and HLSL-to-GLSL 330 Core transpiler hot-reloads shaders live in `<100ms` on both Windows and Mac.
- **In-App 1-Click Auto-Updater**:
  - Checks GitHub Releases on startup and via the top toolbar (`Check for Updates`), downloads the latest Windows or macOS release, strips macOS Gatekeeper quarantine flags automatically, swaps the binary/app bundle, and relaunches in one click.
- **MIDI Controller Support**: Map physical knobs and faders via Windows `WinMM` or macOS `CoreMIDI`.

---

## Downloading & Running (For DJs / End Users)

### macOS (Apple Silicon M1 / M2 / M3 / M4, macOS 13.0+)
1. Download **`DJVisualizer-macOS-arm64.zip`** from the repository's [**Releases**](../../releases/latest) page and extract it.
2. **First Launch**: Double-click **`Launch_DJVisualizer_Mac.command`** (this automatically clears macOS Gatekeeper quarantine on the unsigned `.app` bundle and launches **`DJVisualizer.app`**). After the first run, you can open **`DJVisualizer.app`** directly anytime.
3. **Permissions**: On first launch, macOS will prompt for **Screen & System Audio Recording** (used by `ScreenCaptureKit` to hear desktop audio) and **Microphone** (for external DJ mixer / audio interface line-in). Click **Allow**.
4. **Updating**: When a new version is pushed, click **`Update Available — Click to Install & Restart`** in the top bar of the app.

### Windows (Windows 10 / 11 64-bit)
1. Download **`DJVisualizer-Windows-x64.zip`** from the [**Releases**](../../releases/latest) page and extract it.
2. Double-click **`DJVisualizer.exe`** (or `Launch_DJVisualizer.bat`).
3. **Updating**: Click **`Update Available — Click to Install & Restart`** in the top bar whenever a new release is available.

---

## Building from Source

### Windows
- **Requirements**: Visual Studio 2022 (Desktop C++ workload) + CMake 3.20+
- **Build**:
  ```powershell
  cmake -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build --config Release
  ```
- **Run**: `build\bin\DJVisualizer.exe`

### macOS (Apple Silicon)
- **Requirements**: Xcode Command Line Tools (`xcode-select --install`) + CMake 3.20+ (`brew install cmake`)
- **Build & Package**:
  ```bash
  chmod +x scripts/package_mac.sh
  ./scripts/package_mac.sh
  ```
- **Output**: `dist_mac/DJVisualizer/DJVisualizer.app` and `dist_mac/DJVisualizer-macOS-arm64.zip`

---

## Publishing an Update (CI/CD Workflow)
1. Increment `CURRENT_VERSION` in [`src/control/AutoUpdater.h`](src/control/AutoUpdater.h) (e.g. `"1.0.1"`).
2. Commit and push a version tag:
   ```bash
   git add -A
   git commit -m "Release v1.0.1"
   git tag v1.0.1
   git push origin main --tags
   ```
3. GitHub Actions ([`.github/workflows/release.yml`](.github/workflows/release.yml)) will automatically compile both `DJVisualizer-Windows-x64.zip` and `DJVisualizer-macOS-arm64.zip` and publish them to GitHub Releases.

---

## Hotkeys & Shortcuts
- `F11`: Toggle Fullscreen Presentation Mode on the primary window.
- `ESC`: Exit Fullscreen or close the Stage Projector window.
- `Spacebar`: Manual Tap Tempo / Beat Sync.
- `Ctrl+S` / `Cmd+S` in any `src/shaders/*.hlsl` file: Instant live shader hot-reload.
