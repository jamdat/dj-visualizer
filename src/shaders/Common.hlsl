// =============================================================================
// Common.hlsl - Unreal-Style HLSL Library & Audio Visualizer Uniforms
// Compatible with Direct3D 11 Shader Model 5.0 and Unreal Engine 5 Custom Nodes
// =============================================================================

#ifndef COMMON_HLSL
#define COMMON_HLSL

cbuffer AudioVisualizerParams : register(b0)
{
    // Vector 1: Time & Screen
    float Time;
    float DeltaTime;
    float2 Resolution;

    // Vector 2: Frequency Bands
    float AudioSubBass;
    float AudioKick;
    float AudioLowMid;
    float AudioHighMid;

    // Vector 3: Dynamics & Energy
    float AudioTreble;
    float BeatPulse;
    float DropIntensity;
    float EnergyAccumulator;

    // Vector 4: UI Custom Parameters 0..3
    float CustomParam0; // Zoom / Camera distance
    float CustomParam1; // Speed / Complexity
    float CustomParam2; // Density / Grid Scale
    float CustomParam3; // Glow / Distortion

    // Vector 5: UI Custom Parameters 4..7
    float CustomParam4; // Rotation / Tilt
    float CustomParam5; // Audio Sensitivity
    float CustomParam6; // Laser Fan Spread
    float CustomParam7; // Glitch Trigger Override

    // Vector 6: Palette Colors
    float4 ColorPrimary;
    float4 ColorSecondary;
    float4 ColorAccent;

    // Vector 7: Global FX Overrides
    float BloomIntensity;
    float GlitchRate;
    float WireframeDensity;
    float StrobeFlash;

    // Vector 10: Ingredient Layers
    int BackgroundType;    // 0 = Starfield & Nebula, 1 = Warp Tunnel, 2 = LED Soundwave Void, 3 = Stage Arena, 4 = Obsidian Ocean
    int ShapeType;         // 0 = Liquid Blobs, 1 = Alive Pyramid, 2 = Gyroscope Rings, 3 = Obsidian Monolith, 4 = Torus Knot, 5 = Hypercube Tesseract, 6 = Geodesic Sphere, 7 = Crystal Cluster, 8 = None
    float FluidityAmount;  // Fluid Morph amount
    float ParticleIntensity; // Audio sparks

    // Vector 11: Extra Dynamic Controls
    float LaserIntensity;  // Volumetric lasers
    float VortexTwist;     // Bass Vortex torque
    float Bpm;
    float AspectRatio;

    // Vector 12: Duplication & Macro Modifiers
    int DuplicationMode;   // 0 = Single, 1 = Orbital Ring, 2 = Triad Array, 3 = Radial Kaleidoscope, 4 = Infinite Depth
    float DuplicationParam0; // Orbit Radius / Spacing
    float DuplicationParam1; // Satellite Scale
    float CameraDrift;    // Camera drift toggle

    // Vector 13: Composable 3D Environment Layer Weights
    float SkyLayerWeight;       // Deep Space Starfield & Nebula (0.0 to 1.5)
    float WarpLayerWeight;      // Relativistic Hyperdrive Streaks & Conduit (0.0 to 1.5)
    float FloorLayerWeight;     // 3D Ray-Plane TRON Grid & Obsidian Mirror Floor (0.0 to 1.5)
    float SoundwaveLayerWeight; // 3D Horizon Oscilloscope & FFT Equalizer Wall (0.0 to 1.5)

    // Vector 14: HUD, Material Mode & Stage Framing
    float HudLayerWeight;       // Sci-Fi HUD Telemetry Rings & Hex Framing (0.0 to 1.5)
    int MaterialMode;           // 0 = Smoked Obsidian & Neon (TRON), 1 = Alive LED Matrix (Daft), 2 = Sci-Fi HUD, 3 = Liquid Chrome
    float StageTrussWeight;     // 3D Overhead Concert Truss Framing (0.0 to 1.5)
    float ShapeCrossfade;       // Smooth transition envelope (0.0 to 1.0)
};


// Texture Registers
Texture2D AudioDataTexture : register(t0);  // Row 0 = 512-bin FFT, Row 1 = 512-sample Waveform
Texture2D NoiseTexture     : register(t1);  // Perlin / Blue noise
Texture2D LogoTexture      : register(t2);  // Logo / Alpha mask
Texture2D ColorLUT         : register(t3);  // 1D/2D Neon Color Ramp
Texture2D LensDirtTexture  : register(t4);  // Optical dirt / flare
Texture2D SceneTexture     : register(t5);  // Post-process input buffer

// Sampler Registers
SamplerState SamplerLinearClamp : register(s0);
SamplerState SamplerLinearWrap  : register(s1);
SamplerState SamplerPointClamp  : register(s2);

struct VSOutput
{
    float4 Position : SV_Position;
    float2 UV : TEXCOORD0;
};

// -----------------------------------------------------------------------------
// Audio Texture Sampling Helpers
// -----------------------------------------------------------------------------
float GetAudioFFT(float normalizedFreq)
{
    float u = clamp(normalizedFreq, 0.001, 0.999);
    return AudioDataTexture.SampleLevel(SamplerLinearClamp, float2(u, 0.25), 0).r;
}

float GetAudioWave(float normalizedTime)
{
    float u = clamp(normalizedTime, 0.001, 0.999);
    return AudioDataTexture.SampleLevel(SamplerLinearClamp, float2(u, 0.75), 0).r;
}

// -----------------------------------------------------------------------------
// Color Space Conversions
// -----------------------------------------------------------------------------
float3 HSVtoRGB(float3 c)
{
    float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * lerp(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

float3 RGBtoHSV(float3 c)
{
    float4 K = float4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    float4 p = lerp(float4(c.bg, K.wz), float4(c.gb, K.xy), step(c.b, c.g));
    float4 q = lerp(float4(p.xyw, c.r), float4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return float3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

// -----------------------------------------------------------------------------
// Transforms & Math Utilities
// -----------------------------------------------------------------------------
float2 Rotate2D(float2 p, float angle)
{
    float s = sin(angle);
    float c = cos(angle);
    return float2(p.x * c - p.y * s, p.x * s + p.y * c);
}

float Hash11(float p)
{
    p = frac(p * 0.1031);
    p *= p + 33.33;
    p *= p + p;
    return frac(p);
}

float Hash21(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.x + p3.y) * p3.z);
}

float2 Hash22(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * float3(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.xx + p3.yz) * p3.zy);
}

// 2D Simplex-style Noise
float Noise2D(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);
    float2 u = f * f * (3.0 - 2.0 * f);

    return lerp(
        lerp(Hash21(i + float2(0.0, 0.0)), Hash21(i + float2(1.0, 0.0)), u.x),
        lerp(Hash21(i + float2(0.0, 1.0)), Hash21(i + float2(1.0, 1.0)), u.x),
        u.y
    );
}

// -----------------------------------------------------------------------------
// Signed Distance Functions (SDF) for Raymarching
// -----------------------------------------------------------------------------
float SDFSphere(float3 p, float r)
{
    return length(p) - r;
}

float SDFBox(float3 p, float3 b)
{
    float3 d = abs(p) - b;
    return min(max(d.x, max(d.y, d.z)), 0.0) + length(max(d, 0.0));
}

float SDFPyramid(float3 p, float h)
{
    float m2 = h * h + 0.25;
    p.xz = abs(p.xz);
    p.xz = (p.z > p.x) ? p.zx : p.xz;
    p.xz -= 0.5;

    float3 q = float3(p.z, h * p.y - 0.5 * p.x, h * p.x + 0.5 * p.y);
    float s = max(-q.x, 0.0);
    float t = clamp((q.y - 0.5 * p.z) / (m2 + 0.25), 0.0, 1.0);

    float a = m2 * (q.x + s) * (q.x + s) + q.y * q.y;
    float b = m2 * (q.x + 0.5 * t) * (q.x + 0.5 * t) + (q.y - m2 * t) * (q.y - m2 * t);

    float d2 = min(q.y, -q.x * m2 - q.y * 0.5) > 0.0 ? 0.0 : min(a, b);
    return sqrt((d2 + q.z * q.z) / m2) * sign(max(q.z, -p.y));
}

float SDFOctahedron(float3 p, float s)
{
    p = abs(p);
    return (p.x + p.y + p.z - s) * 0.57735027;
}

float SDFTorus(float3 p, float2 t)
{
    float2 q = float2(length(p.xz) - t.x, p.y);
    return length(q) - t.y;
}

float SDFGyroscope(float3 p, float r, float thickness, float t)
{
    float3 p1 = p;
    p1.xy = Rotate2D(p1.xy, t * 1.2);
    float d1 = SDFTorus(p1, float2(r, thickness));

    float3 p2 = p;
    p2.yz = Rotate2D(p2.yz, t * 1.5 + 1.0);
    float d2 = SDFTorus(p2, float2(r * 0.75, thickness));

    float3 p3 = p;
    p3.xz = Rotate2D(p3.xz, t * 0.8 + 2.0);
    float d3 = SDFTorus(p3, float2(r * 0.52, thickness));

    return min(d1, min(d2, d3));
}

float SDFTorusKnot(float3 p, float r, float tubeR, float t)
{
    float2 q = float2(length(p.xz) - r, p.y);
    float a = atan2(p.z, p.x);
    q = Rotate2D(q, a * 1.5 + t);
    q.x = abs(q.x) - r * 0.42;
    return length(q) - tubeR;
}

float SDFBoxFrame(float3 p, float3 b, float e)
{
    p = abs(p) - b;
    float3 q = abs(p + e) - e;
    return min(min(
        length(max(float3(p.x, q.y, q.z), 0.0)) + min(max(p.x, max(q.y, q.z)), 0.0),
        length(max(float3(q.x, p.y, q.z), 0.0)) + min(max(q.x, max(p.y, q.z)), 0.0)),
        length(max(float3(q.x, q.y, p.z), 0.0)) + min(max(q.x, max(q.y, p.z)), 0.0));
}

float SDFTesseract(float3 p, float size, float innerSize, float t)
{
    float dOuter = SDFBoxFrame(p, float3(size, size, size), size * 0.06);
    float3 pInner = p;
    pInner.xy = Rotate2D(pInner.xy, t * 1.3);
    pInner.yz = Rotate2D(pInner.yz, t * 0.9);
    float dInner = SDFBoxFrame(pInner, float3(innerSize, innerSize, innerSize), innerSize * 0.08);
    return min(dOuter, dInner);
}

float SDFIcosahedron(float3 p, float r)
{
    const float phi = 1.6180339887;
    float3 n1 = normalize(float3(phi, 1.0, 0.0));
    float3 n2 = normalize(float3(1.0, 0.0, phi));
    float3 n3 = normalize(float3(0.0, phi, 1.0));
    p = abs(p);
    float d1 = dot(p, n1);
    float d2 = dot(p, n2);
    float d3 = dot(p, n3);
    return max(max(d1, d2), d3) - r;
}

float SDFHexPrism(float3 p, float2 h)
{
    const float3 k = float3(-0.8660254, 0.5, 0.57735026);
    p = abs(p);
    p.xy -= 2.0 * min(dot(k.xy, p.xy), 0.0) * k.xy;
    float2 d = float2(
        length(p.xy - float2(clamp(p.x, -k.z * h.x, k.z * h.x), h.x)) * sign(p.y - h.x),
        p.z - h.y
    );
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
}

float SDFHexSpire(float3 p, float r, float h)
{
    float dPrism = SDFHexPrism(p.xzy, float2(r, h));
    float dOct = SDFOctahedron(p - float3(0.0, h * 0.4, 0.0), h * 1.3);
    return max(dPrism, dOct);
}

// TRON Recognizer twin-leg portal arch
float SDFRecognizer(float3 p, float scale)
{
    float s = max(scale, 0.001);
    float3 q = p / s;
    // Twin legs: symmetric across X with subtle taper
    float3 pLeg = q;
    pLeg.x = abs(pLeg.x) - 0.95;
    pLeg.x -= (q.y < 0.8) ? (0.8 - q.y) * 0.12 : 0.0;
    float dLeg = SDFBox(pLeg - float3(0.0, -0.15, 0.0), float3(0.24, 0.95, 0.42));
    // Heavy upper cross-arch bridging the legs
    float dArch = SDFBox(q - float3(0.0, 0.82, 0.0), float3(1.25, 0.22, 0.46));
    // Floating upper command visor / cockpit bridge
    float dVisor = SDFBox(q - float3(0.0, 1.12, 0.0), float3(0.72, 0.15, 0.35));
    float d = min(dLeg, min(dArch, dVisor));
    return d * s;
}

// Concentric counter-rotating beveled TRON identity disc rings
float SDFIdentityDisc(float3 p, float r, float t)
{
    // Outer primary beveled ring
    float3 p1 = p;
    p1.xz = Rotate2D(p1.xz, t);
    float dOuter = SDFTorus(p1, float2(r, 0.065));
    // Inner concentric ring counter-rotating
    float3 p2 = p;
    p2.xz = Rotate2D(p2.xz, -t * 1.3);
    float dInner = SDFTorus(p2, float2(r * 0.72, 0.045));
    // Center hub disc ring
    float dHub = SDFTorus(p, float2(r * 0.42, 0.035));
    // Box frame radial segmentation / cutouts
    float3 pFrame = p1;
    float dFrame = SDFBoxFrame(pFrame, float3(r * 1.05, 0.08, r * 1.05), 0.035);
    float dRings = min(dOuter, min(dInner, dHub));
    return min(dRings, dFrame);
}

// Hollow faceted icosahedral/hex shield combined with 2 thin tilted orbital telemetry rings
float SDFHudContainmentSphere(float3 p, float r, float t)
{
    float dShell = max(SDFIcosahedron(p, r), -SDFSphere(p, r * 0.92));
    // Orbiting telemetry ring 1 (tilted 45 deg)
    float3 pRing1 = p;
    pRing1.xy = Rotate2D(pRing1.xy, 0.785398);
    pRing1.xz = Rotate2D(pRing1.xz, t * 1.2);
    float dRing1 = SDFTorus(pRing1, float2(r * 1.25, 0.022));

    // Orbiting telemetry ring 2 (tilted -45 deg around different axis)
    float3 pRing2 = p;
    pRing2.yz = Rotate2D(pRing2.yz, -0.65);
    pRing2.xz = Rotate2D(pRing2.xz, -t * 0.9);
    float dRing2 = SDFTorus(pRing2, float2(r * 1.38, 0.018));

    return min(dShell, min(dRing1, dRing2));
}

// Central hexagonal obsidian monolith flanked by 6 surrounding hexagonal data pillars
float SDFHexMonolithCluster(float3 p, float scale, float bass, float mid, float treble)
{
    float s = max(scale, 0.01);
    float3 q = p / s;
    // Central hexagonal obsidian monolith
    float centralHeight = 1.6 + bass * 0.4;
    float dCenter = SDFHexPrism(q.xzy, float2(0.42, centralHeight));

    // 6 surrounding hexagonal data pillars
    float dPillars = 1000.0;
    const float stepA = 6.2831853 / 6.0;
    [unroll]
    for (int i = 0; i < 6; ++i)
    {
        float angle = float(i) * stepA;
        float2 offset = float2(cos(angle), sin(angle)) * 0.95;
        float pillarHeight = 0.65;
        if (i == 0 || i == 3) pillarHeight += bass * 0.75;
        else if (i == 1 || i == 4) pillarHeight += mid * 0.75;
        else pillarHeight += treble * 0.75;

        float3 pPillar = q - float3(offset.x, pillarHeight - 0.9, offset.y);
        float dP = SDFHexPrism(pPillar.xzy, float2(0.22, pillarHeight));
        dPillars = min(dPillars, dP);
    }

    return min(dCenter, dPillars) * s;
}

float2 FluidDomainWarp(float2 p, float time, float audioReact)
{
    float2 q = float2(
        Noise2D(p + float2(0.0, time * 0.3)),
        Noise2D(p + float2(5.2, 1.3 - time * 0.25))
    );
    float2 r = float2(
        Noise2D(p + 3.0 * q + float2(1.7, 9.2 + time * 0.2)),
        Noise2D(p + 3.0 * q + float2(8.3, 2.8 - time * 0.18))
    );
    return (r - 0.5) * 2.0 * audioReact;
}


// -----------------------------------------------------------------------------
// Realistic Concert Strobe & Stage Shutter FX
// -----------------------------------------------------------------------------
float3 ApplyStrobe(float3 sceneColor, float strobeAmount, float time)
{
    if (strobeAmount <= 0.001) return sceneColor;

    // 20 Hz stroboscopic shutter (1 frame ON, 2 frames OFF at 60 FPS)
    // 33% duty cycle flash window produces authentic freeze-frame illusion
    float strobeCycle = frac(time * 20.0);
    float flashShutter = step(0.67, strobeCycle);

    // Strobe flash intensity envelope
    float strobeIntensity = strobeAmount * flashShutter;

    // Direct scene illumination & specular/edge ignition (preserves 3D structure & colors)
    float3 flashedColor = sceneColor * (1.0 + strobeIntensity * 3.8);
    flashedColor += (sceneColor + 0.08) * (strobeIntensity * 1.4);
    flashedColor += float3(0.92, 0.96, 1.0) * (strobeIntensity * 0.35);

    // Off-phase shutter contrast dip deepens the stroboscopic persistence
    float dip = 1.0 - (strobeAmount * (1.0 - flashShutter) * 0.25);
    return lerp(sceneColor * dip, flashedColor, flashShutter);
}

#endif // COMMON_HLSL

