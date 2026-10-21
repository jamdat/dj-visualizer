#pragma once

#include <cstdint>

namespace Visualizer::Graphics {

struct Float2 {
    float x = 0.0f;
    float y = 0.0f;
};

struct Float4 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;
};

struct UInt4 {
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t z = 0;
    uint32_t w = 0;
};

// Matches HLSL cbuffer / GLSL std140 uniform AudioVisualizerParams : register(b0) (16-byte aligned, 272 bytes)
struct alignas(16) AudioVisualizerParams {
    // Vector 1 (offset 0): General Time & Screen
    float Time = 0.0f;
    float DeltaTime = 0.0f;
    Float2 Resolution = { 1920.0f, 1080.0f };

    // Vector 2 (offset 16): Frequency Bands
    float AudioSubBass = 0.0f;
    float AudioKick = 0.0f;
    float AudioLowMid = 0.0f;
    float AudioHighMid = 0.0f;

    // Vector 3 (offset 32): Dynamics & Energy
    float AudioTreble = 0.0f;
    float BeatPulse = 0.0f;
    float DropIntensity = 0.0f;
    float EnergyAccumulator = 0.0f;

    // Vector 4 (offset 48): UI Custom Parameters 0..3
    float CustomParam0 = 1.0f; // Zoom / Camera distance
    float CustomParam1 = 1.0f; // Speed / Complexity
    float CustomParam2 = 0.5f; // Density / Grid Scale
    float CustomParam3 = 1.0f; // Glow / Distortion

    // Vector 5 (offset 64): UI Custom Parameters 4..7
    float CustomParam4 = 0.0f; // Rotation / Tilt
    float CustomParam5 = 1.0f; // Audio Sensitivity
    float CustomParam6 = 0.0f; // Laser Fan / Array Spread
    float CustomParam7 = 0.0f; // Reserved / Override

    // Vector 6 (offset 80): Primary Palette Color
    Float4 ColorPrimary = { 1.0f, 0.15f, 0.0f, 1.0f }; // Daft Orange / Red

    // Vector 7 (offset 96): Secondary Palette Color
    Float4 ColorSecondary = { 0.0f, 0.7f, 1.0f, 1.0f }; // Cyan Neon

    // Vector 8 (offset 112): Accent Palette Color
    Float4 ColorAccent = { 1.0f, 0.9f, 0.1f, 1.0f }; // Yellow Gold

    // Vector 9 (offset 128): Global Render FX & Photosafety
    float BloomIntensity = 1.0f;
    float GlitchRate = 0.0f;
    float WireframeDensity = 1.0f;
    float MasterBrightness = 0.85f; // 0.0 to 1.0 (default 0.85 kid-safe cap)

    // Vector 10 (offset 144): Dad Punk Primary Layers
    float PyramidWeight = 1.0f;      // 0.0 to 1.5 (Alive 4-sided LED pyramid)
    float LogoWeight = 1.0f;         // 0.0 to 1.5 ("DAD PUNK" LED dot-matrix / chrome logo)
    float SpookyWeight = 0.0f;       // 0.0 to 1.5 (Halloween moon, bats, jack-o'-lanterns)
    float TrailsWeight = 0.0f;       // 0.0 to 1.5 (TRON light-cycle trails on grid)

    // Vector 11 (offset 160): Modes & Dynamic Controls
    int PyramidPattern = 0;          // 0 = Horizontal VU strips, 1 = Rising Chevrons, 2 = Scanner Sweep, 3 = Solid Glow, 4 = Rainbow
    int LogoStyle = 0;               // 0 = LED Dot-Matrix, 1 = Chrome Bevel
    float SafetyDamping = 1.0f;      // Photosensitivity limiter state (1.0 = normal, <1.0 = damped)
    float ParticleIntensity = 1.0f;  // 0.0 to 2.0 (bats or stage sparks)

    // Vector 12 (offset 176): Environment & Lasers
    float LaserIntensity = 1.0f;     // 0.0 to 2.0 (volumetric apex lasers)
    float SkyLayerWeight = 1.0f;     // 0.0 to 1.5 (starfield & moon)
    float FloorLayerWeight = 1.0f;   // 0.0 to 1.5 (3D ray-plane TRON grid & mirror floor)
    float Bpm = 128.0f;

    // Vector 13 (offset 192): Additional Scene Weights & Screen
    float AspectRatio = 1.777778f;
    float HorizonWeight = 0.5f;      // Horizon glow
    float LedWallWeight = 0.0f;      // Full LED wall
    float ShapeCrossfade = 1.0f;     // Smooth scene crossfade envelope (0.0 to 1.0)

    // Vector 14..17 (offset 208..271): Custom Text (16 packed ASCII characters)
    UInt4 CustomText[4] = {
        { 'D', 'A', 'D', 'T' },
        { ' ', 'P', 'U', 'N' },
        { 'K', 0, 0, 0 },
        { 0, 0, 0, 0 }
    };
};

static_assert(sizeof(AudioVisualizerParams) == 272, "AudioVisualizerParams must be exactly 272 bytes for std140 layout");

} // namespace Visualizer::Graphics
