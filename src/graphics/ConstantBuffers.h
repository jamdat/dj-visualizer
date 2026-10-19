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

// Matches HLSL cbuffer / GLSL std140 uniform AudioVisualizerParams : register(b0) (16-byte aligned)
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
    float CustomParam7 = 0.0f; // Glitch Trigger Override

    // Vector 6 (offset 80): Primary Palette Color
    Float4 ColorPrimary = { 1.0f, 0.15f, 0.0f, 1.0f }; // Daft Orange / Red

    // Vector 7 (offset 96): Secondary Palette Color
    Float4 ColorSecondary = { 0.0f, 0.7f, 1.0f, 1.0f }; // Cyan Neon

    // Vector 8 (offset 112): Accent Palette Color
    Float4 ColorAccent = { 1.0f, 0.9f, 0.1f, 1.0f }; // Yellow Gold

    // Vector 9 (offset 128): Global Render FX Overrides
    float BloomIntensity = 1.0f;
    float GlitchRate = 0.3f;
    float WireframeDensity = 1.0f;
    float StrobeFlash = 0.0f;

    // Vector 10 (offset 144): Ingredient Layers
    int BackgroundType = 0;    // 0 = Starfield & Nebula, 1 = Warp Tunnel, 2 = LED Soundwave Void, 3 = Stage Arena, 4 = Obsidian Ocean
    int ShapeType = 0;         // 0 = Liquid Blobs, 1 = Alive Pyramid, 2 = Gyroscope Rings, 3 = Obsidian Monolith, 4 = Torus Knot, 5 = Hypercube Tesseract, 6 = Geodesic Sphere, 7 = Crystal Cluster, 8 = None
    float FluidityAmount = 1.0f; // 0.0 to 2.0 (fluid ripples, domain warping)
    float ParticleIntensity = 1.0f; // 0.0 to 2.0 (audio firefly particles)

    // Vector 11 (offset 160): Extra Dynamic Controls
    float LaserIntensity = 1.0f; // 0.0 to 2.0 (volumetric laser matrix)
    float VortexTwist = 0.0f;    // -3.14 to 3.14 (rotational fluid torque)
    float Bpm = 128.0f;
    float AspectRatio = 1.777778f;

    // Vector 12 (offset 176): Duplication & Modifiers
    int DuplicationMode = 0;     // 0 = Single, 1 = Orbital Ring, 2 = Triad Array, 3 = Radial Kaleidoscope, 4 = Infinite Depth
    float DuplicationParam0 = 1.0f; // Orbit Radius / Spacing
    float DuplicationParam1 = 0.5f; // Satellite Scale
    float CameraDrift = 1.0f;    // 0.0 = Static, 1.0 = Animated camera orbit/drift

    // Vector 13 (offset 192): Composable 3D Environment Layer Weights
    float SkyLayerWeight = 1.0f;       // Deep Space Starfield & Nebula (0.0 to 1.5)
    float WarpLayerWeight = 0.0f;      // Relativistic Hyperdrive Streaks & Conduit (0.0 to 1.5)
    float FloorLayerWeight = 1.0f;     // 3D Ray-Plane TRON Grid & Obsidian Mirror Floor (0.0 to 1.5)
    float SoundwaveLayerWeight = 0.5f; // 3D Horizon Oscilloscope & FFT Equalizer Wall (0.0 to 1.5)

    // Vector 14 (offset 208): HUD, Material Mode & Stage Framing
    float HudLayerWeight = 0.3f;       // Sci-Fi HUD Telemetry Rings & Hex Framing (0.0 to 1.5)
    int MaterialMode = 0;              // 0 = Smoked Obsidian & Neon (TRON), 1 = Alive LED Matrix (Daft), 2 = Sci-Fi HUD, 3 = Liquid Chrome
    float StageTrussWeight = 0.5f;     // 3D Overhead Concert Truss Framing (0.0 to 1.5)
    float ShapeCrossfade = 1.0f;       // Smooth transition envelope (0.0 to 1.0)
};

} // namespace Visualizer::Graphics


