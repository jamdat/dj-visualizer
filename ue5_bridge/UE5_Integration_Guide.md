# Unreal Engine 5 Bridge & Integration Guide

This guide details how to stream real-time audio-reactive visuals from the **DJ Visualizer** directly into **Unreal Engine 5 (UE5.2 / UE5.3 / UE5.4+)** for virtual concert stages, 3D LED volume walls, broadcast graphics, and virtual production.

---

## 1. Zero-Latency GPU Video Streaming (Spout 2)

The visualizer includes a built-in DirectX 11 shared texture sender registered as **`DJVisualizer`**.

### Setup in Unreal Engine 5:
1. Install the free **OWL Spout / NDI Plugin (Off-World Live)** or **Spout-UE5** plugin in your UE5 project.
2. In your UE5 Content Browser, right-click -> **Media** -> **OWL Spout Receiver**.
3. Set **Sender Name** to `DJVisualizer`.
4. Create a **Material** with the Spout Receiver Texture Sample and apply it to:
   - Stage LED Panels / Jumbotrons
   - Dynamic 3D stage geometry
   - Post-Process Volume screen effects

---

## 2. Material Custom Node HLSL Snippets (`ue5_bridge/MaterialCustomNodes/`)

If you want to render the visualizer shaders **natively inside the UE5 Material Graph**:

1. Open your Material in Unreal Engine 5.
2. Add a **Custom** node.
3. Set **Output Type** to `CMOT Float 3` or `CMOT Float 4`.
4. Copy the code from the corresponding snippet in `ue5_bridge/MaterialCustomNodes/`:
   - `CustomNode_AlivePyramid.hlsl`
   - `CustomNode_SynthwaveGrid.hlsl`
   - `CustomNode_RaymarchedCrystals.hlsl`
   - `CustomNode_PostProcessGlitch.hlsl`
5. Connect the input pins specified at the top of each file (`UV`, `Time`, `AudioSubBass`, `AudioKick`, `AudioLowMid`, `AudioHighMid`, `ColorPrimary`, `ColorSecondary`).

---

## 3. Unreal Shader Header (`ue5_bridge/Shaders/AliveShaders.ush`)

To use the shared visualizer math functions across multiple custom nodes or Niagara scripts:
1. Place `AliveShaders.ush` into your UE5 project's `Shaders/` directory or include it via `#include "/Project/AliveShaders.ush"`.
2. Access all SDF primitives (`SDFPyramid`, `SDFOctahedron`), `HSVtoRGB`, `Noise2D`, and raymarching functions directly.

---

## 4. Audio Synesthesia & MPC Parameter Mapping

To drive UE5 native materials with the same 5-band frequency tracking:

| Visualizer Uniform | Unreal Audio Synesthesia Feature | Material Parameter Collection (MPC) |
| :--- | :--- | :--- |
| `AudioSubBass` | Frequency Spectrum (20-60 Hz) | `MPC_Audio.SubBass` |
| `AudioKick` | Constant Q Peak (60-250 Hz) | `MPC_Audio.Kick` |
| `AudioLowMid` | Frequency Band (250-500 Hz) | `MPC_Audio.LowMid` |
| `AudioHighMid` | Frequency Band (500-2000 Hz) | `MPC_Audio.HighMid` |
| `AudioTreble` | Frequency Band (2000-16000 Hz)| `MPC_Audio.Treble` |
| `BeatPulse` | Onset Detector Trigger | `MPC_Audio.BeatPulse` |
| `DropIntensity`| Loudness Transient Spike | `MPC_Audio.DropIntensity` |

---

## 5. Niagara GPU Particle HLSL Math (`ue5_bridge/Niagara/NiagaraAudioMath.hlsl`)

Use the functions in `NiagaraAudioMath.hlsl` inside Niagara Custom HLSL Modules to drive:
- Audio-reactive velocity vortex fields
- Particle ribbon audio wave oscillations
- Explosive drop particle shockwaves
