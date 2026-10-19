#include "Common.hlsl"

// =============================================================================
// ModularMixer.hlsl - Daft Punk "Alive 2007" & TRON Sci-Fi Visualizer Engine
// 8 Hero 3D Shapes, 5 Sci-Fi Environments, Duplication Modifiers & Stage FX
// =============================================================================

// -----------------------------------------------------------------------------
// 1. Single Hero 3D Shape Distance Field (SDF) Evaluator
// -----------------------------------------------------------------------------
float EvaluateSingleShape(float3 pShape, out bool isCoreHit, out float outGlow)
{
    outGlow = 0.0;
    isCoreHit = false;
    float d = 1000.0;

    [branch] if (ShapeType == 0) // 🔺 Alive 2007 Stage Pyramid & LED Core
    {
        float pyrSize = 1.8 * CustomParam0;
        float dPyr = SDFPyramid(pShape / pyrSize, 1.2) * pyrSize;
        float dCage = abs(dPyr) - 0.03;
        float dSolid = dPyr;

        // Inner glowing sub-bass core
        float dCore = SDFOctahedron(pShape, (0.45 + AudioSubBass * 0.45) * CustomParam0);
        if (dCore < min(dCage, dSolid))
        {
            d = dCore;
            isCoreHit = true;
        }
        else
        {
            d = min(dCage, dSolid);
        }
        outGlow = 1.0 / (1.0 + abs(dPyr) * 45.0 * WireframeDensity);
    }
    else [branch] if (ShapeType == 1) // 🚪 TRON Recognizer / Cyber Portal Arch
    {
        float scale = 1.25 * CustomParam0;
        float dRec = SDFRecognizer(pShape, scale);
        float dCore = SDFSphere(pShape - float3(0.0, 0.45 * scale, 0.0), (0.28 + AudioSubBass * 0.3) * scale);
        if (dCore < dRec)
        {
            d = dCore;
            isCoreHit = true;
        }
        else
        {
            d = dRec;
        }
        outGlow = 0.024 / (0.025 + abs(d) * 5.0);
    }
    else [branch] if (ShapeType == 2) // 💿 Identity Disc & Dyson Gyroscope
    {
        float r = 1.45 * CustomParam0;
        float tDisc = Time * 0.9 * CustomParam1;
        float dDisc = SDFIdentityDisc(pShape, r, tDisc);
        float dCore = SDFSphere(pShape, (0.35 + AudioSubBass * 0.35) * CustomParam0);
        if (dCore < dDisc)
        {
            d = dCore;
            isCoreHit = true;
        }
        else
        {
            d = dDisc;
        }
        outGlow = 0.025 / (0.025 + abs(d) * 5.0);
    }
    else [branch] if (ShapeType == 3) // 🛡️ Holographic HUD Core & Hex Shield
    {
        float r = 1.35 * CustomParam0;
        float tHud = Time * 0.8 * CustomParam1;
        float dShield = SDFHudContainmentSphere(pShape, r, tHud);
        float dCore = SDFOctahedron(pShape, (0.42 + AudioSubBass * 0.45) * CustomParam0);
        if (dCore < dShield)
        {
            d = dCore;
            isCoreHit = true;
        }
        else
        {
            d = dShield;
        }
        outGlow = 0.024 / (0.03 + abs(d) * 5.5);
    }
    else [branch] if (ShapeType == 4) // 🧊 4D Hypercube Tesseract Cage
    {
        float outerSize = 1.1 * CustomParam0;
        float innerSize = outerSize * (0.48 + AudioSubBass * 0.35);
        d = SDFTesseract(pShape, outerSize, innerSize, Time * 0.9 * CustomParam1);

        float dCore = SDFSphere(pShape, (0.28 + AudioKick * 0.35) * CustomParam0);
        if (dCore < d)
        {
            d = dCore;
            isCoreHit = true;
        }
        outGlow = 0.028 / (0.025 + abs(d) * 6.0);
    }
    else [branch] if (ShapeType == 5) // 🏢 Hexagonal Data Monolith Array
    {
        d = SDFHexMonolithCluster(pShape, CustomParam0, AudioSubBass, AudioLowMid, AudioTreble);
        float dCore = SDFSphere(pShape - float3(0.0, 1.1 * CustomParam0, 0.0), (0.25 + AudioKick * 0.3) * CustomParam0);
        if (dCore < d)
        {
            d = dCore;
            isCoreHit = true;
        }
        outGlow = 0.022 / (0.03 + abs(d) * 5.0);
    }
    else [branch] if (ShapeType == 6) // 🪢 Cyber Light-Ribbon Torus Knot
    {
        float r = 1.25 * CustomParam0;
        float tube = 0.12 * (1.0 + AudioKick * 0.35);
        d = SDFTorusKnot(pShape, r, tube, Time * 0.8 * CustomParam1);

        float dCore = SDFSphere(pShape, (0.35 + AudioSubBass * 0.3) * CustomParam0);
        if (dCore < d)
        {
            d = dCore;
            isCoreHit = true;
        }
        outGlow = 0.03 / (0.025 + abs(d) * 5.0);
    }
    else [branch] if (ShapeType == 7) // 💧 Liquid Mercury Cyber-Core
    {
        float rScale = 1.0 * CustomParam0;
        float3 c0 = float3(0.0, sin(Time * 1.2) * 0.15, 0.0);
        float d0 = SDFSphere(pShape - c0, (0.75 + AudioKick * 0.35) * rScale);

        float t = Time * 1.2 * CustomParam1;
        float3 c1 = float3(sin(t) * 1.1, cos(t * 0.8) * 0.5, cos(t) * 1.1);
        float d1 = SDFSphere(pShape - c1, (0.42 + AudioSubBass * 0.3) * rScale);

        float3 c2 = float3(cos(t * 1.1 + 2.0) * 1.2, sin(t * 0.9 + 1.0) * 0.6, sin(t * 1.1 + 2.0) * 1.2);
        float d2 = SDFSphere(pShape - c2, (0.38 + AudioLowMid * 0.25) * rScale);

        float3 c3 = float3(sin(t * 0.7 + 4.0) * 0.9, -cos(t * 1.2) * 0.7, cos(t * 0.7 + 4.0) * 0.9);
        float d3 = SDFSphere(pShape - c3, (0.32 + AudioTreble * 0.25) * rScale);

        float k = 0.5 + FluidityAmount * 0.35;
        float h1 = clamp(0.5 + 0.5 * (d1 - d0) / k, 0.0, 1.0);
        d = lerp(d1, d0, h1) - k * h1 * (1.0 - h1);

        float h2 = clamp(0.5 + 0.5 * (d2 - d) / k, 0.0, 1.0);
        d = lerp(d2, d, h2) - k * h2 * (1.0 - h2);

        float h3 = clamp(0.5 + 0.5 * (d3 - d) / k, 0.0, 1.0);
        d = lerp(d3, d, h3) - k * h3 * (1.0 - h3);

        if (FluidityAmount > 0.05)
        {
            float ripple = sin(pShape.x * 5.0 + pShape.y * 5.0 + Time * 3.0) * (0.03 + AudioSubBass * 0.06) * FluidityAmount;
            d += ripple;
        }

        outGlow = 0.018 / (0.02 + abs(d) * 4.0);
    }

    return d;
}

// -----------------------------------------------------------------------------
// 2. Hero 3D Shape Distance Field Evaluator with Duplication Modifiers
// -----------------------------------------------------------------------------
float MapHeroShape(float3 p, out int outTintMode, out float outGlow)
{
    outGlow = 0.0;
    outTintMode = 0;

    if (ShapeType == 8) // None / Transparent
    {
        return 1000.0;
    }

    float3 pShape = p;

    // Rotational torque & Bass Vortex (Controlled, subtle, 0 when Macro is OFF)
    float rotAngle = CustomParam4 + Time * 0.15 * CustomParam1;
    if (VortexTwist > 0.01)
    {
        rotAngle += VortexTwist * (0.35 + AudioSubBass * 0.2) * sin(pShape.y * 1.2 + Time * 0.8 * CustomParam1);
    }
    pShape.xz = Rotate2D(pShape.xz, rotAngle);

    // Fluid Morph ripple displacement on coordinate domain
    if (FluidityAmount > 0.1)
    {
        pShape.xy += sin(pShape.yx * 3.0 + Time * 2.0) * (0.04 + AudioSubBass * 0.05) * FluidityAmount;
    }

    float3 pEval = pShape;
    float scaleFactor = 1.0;
    int tintOverride = 0; // 0 = default, 1 = secondary, 2 = accent

    // Modifier Mode 1: Orbital Ring (Domain-fold between Center and Nearest Satellite)
    [branch] if (DuplicationMode == 1)
    {
        float orbR = (2.2 + AudioSubBass * 0.4) * DuplicationParam0;
        float satScale = max((0.38 + AudioKick * 0.1) * DuplicationParam1, 0.15);
        float phi = atan2(pShape.z, pShape.x) - Time * 0.9 * CustomParam1;
        float sector = 6.283185 / 5.0;
        float nearestIdx = round(phi / sector);
        float angle = nearestIdx * sector + Time * 0.9 * CustomParam1;
        float3 satPos = float3(cos(angle) * orbR, sin(angle * 2.0 + nearestIdx) * 0.3, sin(angle) * orbR);

        float distCenterBound = length(pShape) - 1.15 * CustomParam0;
        float distSatBound = length(pShape - satPos) - 1.15 * CustomParam0 * satScale;

        if (distSatBound < distCenterBound)
        {
            pEval = (pShape - satPos) / satScale;
            scaleFactor = satScale;
            tintOverride = 1;
        }
    }
    // Modifier Mode 2: Triad Array (Horizontal Mirrored Spread)
    else [branch] if (DuplicationMode == 2)
    {
        float sp = 2.6 * DuplicationParam0;
        float3 pSidePos = float3(abs(pShape.x) - sp, pShape.y - sin(Time * 1.5) * 0.2, pShape.z);
        float distCenterBound = length(pShape) - 1.15 * CustomParam0;
        float distSideBound = length(pSidePos) - 1.15 * CustomParam0 * 0.65;

        if (distSideBound < distCenterBound)
        {
            pEval = pSidePos / 0.65;
            scaleFactor = 0.65;
            tintOverride = 2;
        }
    }
    // Modifier Mode 3: Radial Kaleidoscope (6-Fold Polar Fold)
    else [branch] if (DuplicationMode == 3)
    {
        float2 xz = pShape.xz;
        float rXZ = length(xz);
        float phi = atan2(xz.y, xz.x);
        float sectorAngle = 6.283185 / 6.0;
        phi = abs(fmod(phi + 3.141592, sectorAngle) - sectorAngle * 0.5);
        pEval = float3(cos(phi) * rXZ - 1.6 * DuplicationParam0, pShape.y, sin(phi) * rXZ) / 0.6;
        scaleFactor = 0.6;
    }
    // Modifier Mode 4: Infinite Depth Mirror (Z-Repetition)
    else [branch] if (DuplicationMode == 4)
    {
        float repZ = 4.2 * DuplicationParam0;
        pEval = pShape;
        pEval.z = frac((pEval.z + Time * 1.8 * CustomParam1) / repZ + 0.5) * repZ - repZ * 0.5;
    }

    bool isCoreHit = false;
    float d = EvaluateSingleShape(pEval, isCoreHit, outGlow) * scaleFactor;
    outTintMode = isCoreHit ? 3 : tintOverride;
    return d;
}

float3 EvaluateHeroShapeColor(float3 pHit, int tintMode)
{
    if (tintMode == 1) return ColorSecondary.rgb;
    if (tintMode == 2) return ColorAccent.rgb;
    if (tintMode == 3) return ColorAccent.rgb * (2.6 + AudioSubBass * 3.0);

    float baseAngle = CustomParam4 + Time * 0.15 * CustomParam1;
    float3 pShape = pHit;
    pShape.xz = Rotate2D(pShape.xz, baseAngle);

    if (ShapeType == 0) // 🔺 Alive 2007 Stage Pyramid & LED Core
    {
        float ledStrip = sin(pShape.y * 22.0 * WireframeDensity + EnergyAccumulator * 2.0);
        float stripMask = smoothstep(0.65, 0.95, ledStrip);
        return lerp(ColorPrimary.rgb, ColorAccent.rgb, stripMask) * (1.2 + AudioKick * 1.8 + BeatPulse * 1.2);
    }
    if (ShapeType == 1) // 🚪 TRON Recognizer / Cyber Portal Arch
    {
        float neonEdge = abs(pShape.x) > 0.7 ? 1.0 : 0.2;
        return lerp(ColorPrimary.rgb, ColorAccent.rgb, neonEdge) * (1.2 + AudioKick * 1.5);
    }
    if (ShapeType == 2) // 💿 Identity Disc & Dyson Gyroscope
    {
        float ringPhase = length(pShape.xz);
        return lerp(ColorSecondary.rgb, ColorPrimary.rgb, sin(ringPhase * 4.0 + EnergyAccumulator * 2.0) * 0.5 + 0.5);
    }
    if (ShapeType == 3) // 🛡️ Holographic HUD Core & Hex Shield
    {
        float facetGlow = sin(pShape.x * 7.0 + pShape.y * 7.0 + EnergyAccumulator * 2.5);
        return lerp(ColorPrimary.rgb, ColorSecondary.rgb, smoothstep(-0.4, 0.7, facetGlow)) * (1.2 + BeatPulse * 1.0);
    }
    if (ShapeType == 4) // 🧊 4D Hypercube Tesseract Cage
    {
        float cageGlow = length(pShape) / (1.54 * max(CustomParam0, 0.1));
        return lerp(ColorSecondary.rgb, ColorPrimary.rgb, clamp(cageGlow, 0.0, 1.0)) * (1.2 + AudioHighMid * 1.2);
    }
    if (ShapeType == 5) // 🏢 Hexagonal Data Monolith Array
    {
        float coreGlow = 1.0 - smoothstep(-0.4, 0.9, pShape.y);
        return lerp(ColorPrimary.rgb, ColorAccent.rgb, coreGlow) * (1.2 + AudioLowMid * 1.5);
    }
    if (ShapeType == 6) // 🪢 Cyber Light-Ribbon Torus Knot
    {
        float ribbon = sin(atan2(pShape.z, pShape.x) * 6.0 + EnergyAccumulator * 3.0);
        return lerp(ColorPrimary.rgb, ColorAccent.rgb, smoothstep(0.2, 0.8, ribbon)) * (1.1 + AudioLowMid * 1.2);
    }
    if (ShapeType == 7) // 💧 Liquid Mercury Cyber-Core
    {
        return lerp(ColorPrimary.rgb, ColorSecondary.rgb, clamp(pShape.y * 0.6 + 0.5, 0.0, 1.0));
    }
    return ColorPrimary.rgb;
}

// -----------------------------------------------------------------------------
// 3. Sci-Fi Background Evaluators
// -----------------------------------------------------------------------------

float FBM2D(float2 p)
{
    float val = 0.0;
    float amp = 0.5;
    float2 shift = float2(100.0, 100.0);
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        val += amp * Noise2D(p);
        p = p * 2.02 + shift;
        amp *= 0.5;
    }
    return val;
}

// Background 0: 🌌 Hyper-Dense Starfield, Cosmic Dust & Galactic Nebulae
float3 RenderStarfieldBackground(float2 uv, float3 rd)
{
    float3 col = float3(0.0005, 0.0008, 0.0018);

    // 1. Cosmic Interstellar Dust Clouds (Domain-Warped Fractal Nebula)
    float2 nebUV = uv * 0.65 + float2(Time * 0.015, sin(Time * 0.01) * 0.05);
    float2 q = float2(FBM2D(nebUV * 1.6), FBM2D(nebUV * 1.6 + float2(5.2, 1.3)));
    float2 r = float2(FBM2D(nebUV * 2.2 + 3.5 * q + float2(1.7, 9.2) + Time * 0.01),
                      FBM2D(nebUV * 2.2 + 3.5 * q + float2(8.3, 2.8) - Time * 0.012));
    float dust = FBM2D(nebUV * 2.0 + 3.0 * r);
    float dustDensity = smoothstep(0.35, 0.85, dust);

    float3 nebPrimary = lerp(ColorPrimary.rgb * 0.18, ColorSecondary.rgb * 0.25, q.x);
    float3 nebAccent = lerp(nebPrimary, ColorAccent.rgb * 0.35, r.y);
    col += nebAccent * dustDensity * (0.85 + AudioLowMid * 0.75);

    // 2. Distant Spiral Galaxy Center
    float2 galCenter = float2(0.35, 0.22);
    float2 galDelta = uv - galCenter;
    float galDist = length(galDelta);
    float galAngle = atan2(galDelta.y, galDelta.x);
    float spiral = sin(galAngle * 2.0 - galDist * 14.0 + Time * 0.15) * 0.5 + 0.5;
    float galCore = exp(-galDist * 8.0) * (0.8 + AudioSubBass * 0.6);
    float galArms = exp(-galDist * 3.5) * spiral * 0.35;
    col += (ColorSecondary.rgb * 0.6 + ColorAccent.rgb * 0.4) * (galCore + galArms);

    // 3. Hyper-Dense Pin-Point Starlight Layers with Diffraction Spikes
    [unroll]
    for (int l = 1; l <= 4; ++l)
    {
        float layerScale = float(l) * 6.5 + 4.0;
        float2 starGrid = uv * layerScale;
        float2 id = floor(starGrid);
        float2 gUV = frac(starGrid) - 0.5;

        float rnd = Hash21(id + float(l) * 23.41);
        if (rnd > 0.42) // Dense cluster distribution
        {
            float2 pos = (Hash22(id) - 0.5) * 0.76;
            float2 dVec = gUV - pos;
            float dStar = length(dVec);

            // Razor-sharp pin-point starlight core
            float starRadius = 0.035 + (rnd > 0.88 ? 0.025 : 0.0);
            float core = pow(clamp(1.0 - dStar / starRadius, 0.0, 1.0), 24.0);

            // Cross-diffraction spikes on brightest stars
            float spikes = 0.0;
            if (rnd > 0.80)
            {
                float spikeW = 0.008;
                spikes = (exp(-abs(dVec.x) / spikeW) * exp(-abs(dVec.y) * 14.0) +
                          exp(-abs(dVec.y) / spikeW) * exp(-abs(dVec.x) * 14.0)) * 0.5;
            }

            // Scintillation / Twinkle
            float twinkle = 0.6 + sin(Time * (4.0 + rnd * 6.0) + rnd * 30.0) * 0.4;
            float3 starCol = (rnd > 0.88) ? ColorAccent.rgb : ((rnd > 0.72) ? ColorSecondary.rgb : float3(0.92, 0.96, 1.0));

            col += starCol * (core + spikes) * twinkle * (0.85 + AudioTreble * 1.5);
        }
    }

    return col;
}

// Background 1: 🌀 Star Wars / Star Trek Hyperspace Warp Tunnel
float3 RenderWarpTunnel(float2 uv, float3 rd)
{
    float angle = atan2(uv.y, uv.x);
    float radius = length(uv);
    float depth = 1.0 / max(radius, 0.015);

    // Smooth continuous monotonic progression (no instantaneous velocity/phase jumping)
    float speedBase = 2.4 + CustomParam1 * 2.8;
    float warpProgress = Time * speedBase + EnergyAccumulator * 1.4;

    float3 col = float3(0.0006, 0.0012, 0.0035);

    // 1. Hyperspace Tubular Nebula Conduit (Soft luminous volumetric warp mist)
    float cylU = angle * 0.954929658; // 6 sectors around tunnel
    float cylV = depth * 0.05 - warpProgress * 0.2;
    float conduitWave = sin(cylU * 3.0 + cylV * 2.5) * cos(cylU * 2.0 - cylV * 1.8);
    float conduitGlow = saturate(conduitWave * 0.5 + 0.5);
    float conduitFade = smoothstep(0.06, 0.4, radius) * (1.0 - smoothstep(0.9, 2.0, radius));
    float3 conduitColor = lerp(float3(0.015, 0.07, 0.25), ColorPrimary.rgb * 0.35, 0.3);
    col += conduitColor * conduitGlow * conduitFade * (0.8 + AudioLowMid * 0.4);

    // 2. Relativistic Star Streaks (Star Wars hyperdrive light filaments)
    float numStreaks = 72.0;
    float normAngle = (angle + 3.14159265) / 6.2831853;
    float sector = floor(normAngle * numStreaks);
    float rnd = Hash11(sector * 37.19 + 7.3);

    float fracA = frac(normAngle * numStreaks) - 0.5;
    float arcDist = abs(fracA) * (6.2831853 / numStreaks) * radius;
    float streakWidth = 0.004 + 0.003 * Hash11(sector * 19.8);
    float lineCore = 1.0 - saturate(arcDist / max(streakWidth, 0.001));
    lineCore = lineCore * lineCore;

    float streakSpeed = 0.85 + rnd * 0.4;
    float zCoord = frac(depth * 0.04 - warpProgress * 0.35 * streakSpeed - rnd * 8.3);
    float streakLen = 0.45 + rnd * 0.3 + AudioKick * 0.2;
    float headMask = smoothstep(0.0, 0.08, zCoord) * (1.0 - smoothstep(0.08, streakLen, zCoord));

    float3 coreWhite = float3(1.0, 1.0, 1.0);
    float3 cyanFringe = lerp(float3(0.25, 0.75, 1.0), ColorSecondary.rgb, rnd * 0.35);
    float3 streakColor = lerp(cyanFringe, coreWhite, lineCore);
    col += streakColor * lineCore * headMask * (1.2 + AudioTreble * 0.8);

    // 3. Central Vanishing Focal Point
    float corePoint = exp(-radius * 45.0);
    float coreHalo = exp(-radius * 10.0);
    col += float3(1.0, 1.0, 1.0) * corePoint * 1.5 + float3(0.25, 0.7, 1.0) * coreHalo * (0.8 + AudioSubBass * 0.5);

    // Relativistic distance fog blend
    col = lerp(float3(0.0006, 0.0012, 0.0035), col, exp(-depth * 0.015));
    col *= smoothstep(0.008, 0.05, radius);

    return col;
}

// Background 2: 📊 Refined LED Soundwave Void (Oscilloscope Ribbon + 3D FFT Audio Grid)
float3 RenderSoundwaveVoid(float2 uv, float3 rd)
{
    float3 col = float3(0.001, 0.001, 0.003);
    float horizonY = -0.05 + CustomParam4 * 0.15;

    // 1. Lower Half: 3D Perspective Reactive Soundwave Floor & Equalizer Spectrum
    if (uv.y < horizonY)
    {
        float groundY = (horizonY - uv.y) / (1.0 + horizonY);
        float depth = 1.0 / max(groundY, 0.001);
        float2 worldXZ = uv * depth * float2(0.85, 1.8) + float2(0.0, EnergyAccumulator * 1.5 * CustomParam1);

        // Soundwave floor FFT spectrum bars & undulating ripple waves
        float waveFreq = clamp(abs(worldXZ.x) * 0.18, 0.02, 0.95);
        float fftSample = GetAudioFFT(waveFreq);
        float soundBar = 1.0 - smoothstep(0.0, 0.12, abs(worldXZ.y * 0.45 - (fftSample * 3.5)));

        // Floor perspective grid
        float2 gridCoord = worldXZ * 0.7 * WireframeDensity;
        float2 gridDist = abs(frac(gridCoord - 0.5) - 0.5) / fwidth(gridCoord);
        float gridLine = 1.0 - min(min(gridDist.x, gridDist.y), 1.0);

        float3 floorCol = lerp(ColorSecondary.rgb, ColorPrimary.rgb, clamp(abs(worldXZ.x) * 0.35, 0.0, 1.0));
        floorCol = floorCol * gridLine * 0.4 + ColorAccent.rgb * soundBar * (1.0 + AudioSubBass * 1.4);

        float fog = exp(-depth * 0.045);
        float foregroundFade = 1.0 - smoothstep(0.0, 0.8, groundY);
        col = lerp(float3(0.001, 0.001, 0.003), floorCol * foregroundFade, fog);
    }
    else
    {
        // Deep void upper sky with subtle audio horizon flare
        float skyY = (uv.y - horizonY) / (1.0 - horizonY);
        float horizonGlow = exp(-skyY * 16.0);
        col += ColorPrimary.rgb * horizonGlow * 0.25 * (1.0 + AudioKick * 0.8);
    }

    // 2. Horizon Audio Waveform Oscilloscope Ribbon
    float normX = clamp(uv.x * 0.5 + 0.5, 0.002, 0.998);
    float waveSample = (GetAudioWave(normX) - 0.5) * (0.28 + AudioSubBass * 0.35);
    float distToWave = abs(uv.y - horizonY - waveSample);
    float waveCore = exp(-distToWave * 140.0);
    float waveGlow = exp(-distToWave * 22.0) * 0.45;
    float3 waveCol = lerp(ColorAccent.rgb, ColorPrimary.rgb, abs(uv.x) * 0.6);
    col += waveCol * (waveCore + waveGlow) * (1.1 + AudioKick * 0.8);

    return col;
}

// Background 3: 🏟️ Concert Stage Arena & Overhead Lighting Rig
float3 RenderStageArena(float2 uv, float3 rd)
{
    float3 col = float3(0.002, 0.002, 0.005);

    // 1. Stadium LED Matrix Blinder Backdrop
    if (uv.y > 0.05 && uv.y < 0.82)
    {
        float2 ledGrid = frac(uv * 24.0);
        float ledDist = length(ledGrid - 0.5);
        float ledMask = 1.0 - smoothstep(0.36, 0.42, ledDist);

        float fftVal = GetAudioFFT(clamp(abs(uv.x) * 0.32, 0.02, 0.95));
        float barY = (uv.y - 0.05) / 0.77;
        float idleWave = sin(uv.x * 6.0 + Time * 1.5) * 0.08 + 0.12;
        float ledBar = step(barY, max(fftVal * 1.15, idleWave));
        float3 ledColor = lerp(ColorSecondary.rgb, ColorPrimary.rgb, abs(uv.x)) * ledBar * ledMask * 0.55;
        col += ledColor;
    }

    // 2. Overhead Truss Geometric Framing
    if (uv.y > 0.78)
    {
        float trussX = abs(frac(uv.x * 8.0) - 0.5);
        float trussLine = (1.0 - smoothstep(0.0, 0.06, trussX)) * 0.45;
        col += ColorSecondary.rgb * trussLine * (0.6 + AudioTreble * 0.5);
    }

    // 3. Stage Polished Obsidian Floor & Mirror Grid
    if (uv.y < 0.0)
    {
        float floorFade = smoothstep(-0.85, -0.02, uv.y);
        float floorGridX = abs(frac(uv.x * 6.0) - 0.5);
        float floorGridZ = abs(frac(1.0 / max(abs(uv.y), 0.01) * 0.3 - Time * 0.8) - 0.5);
        float gridLine = ((1.0 - smoothstep(0.0, 0.05, floorGridX)) + (1.0 - smoothstep(0.0, 0.05, floorGridZ))) * 0.35;
        col += ColorPrimary.rgb * gridLine * floorFade * (0.6 + AudioKick * 0.6);

        // Stage edge footlights
        float footlightDist = abs(uv.y);
        float footlights = exp(-footlightDist * 20.0) * (sin(uv.x * 16.0) * 0.5 + 0.5);
        col += ColorAccent.rgb * footlights * 0.45 * (0.8 + AudioSubBass * 0.8);
    }

    return col;
}

// Background 4: 🌊 Liquid Obsidian Ocean
float3 RenderFluidBackground(float2 uv, float3 rd)
{
    float3 col = float3(0.002, 0.002, 0.005);
    float horizonY = -0.05 + CustomParam4 * 0.15;

    if (uv.y < horizonY)
    {
        float groundY = (horizonY - uv.y) / (1.0 + horizonY);
        float depth = 1.0 / max(groundY, 0.001);
        float2 worldXZ = uv * depth * float2(0.9, 1.8) + float2(0.0, EnergyAccumulator * 1.5 * CustomParam1);

        float wave1 = sin(worldXZ.x * 1.2 + worldXZ.y * 0.8 + Time * 2.0) * (0.3 + AudioSubBass * 0.4);
        float wave2 = cos(worldXZ.x * 2.5 - worldXZ.y * 1.4 + Time * 3.0) * (0.15 + AudioKick * 0.3);
        float waveHeight = (wave1 + wave2) * FluidityAmount;

        float fresnel = pow(1.0 - groundY, 4.0);
        float3 chromeBase = float3(0.005, 0.005, 0.009);

        float2 gridCoord = worldXZ * 0.6 * WireframeDensity;
        float2 gridDist = abs(frac(gridCoord - 0.5) - 0.5) / fwidth(gridCoord);
        float gridLine = 1.0 - min(min(gridDist.x, gridDist.y), 1.0);

        float3 gridColor = lerp(ColorSecondary.rgb, ColorPrimary.rgb, sin(worldXZ.y * 0.2) * 0.5 + 0.5) * 0.4;
        float fog = exp(-depth * 0.06);
        float foregroundFade = 1.0 - smoothstep(0.0, 0.7, groundY);

        float3 liquidFloor = chromeBase + gridColor * gridLine * (0.5 + AudioKick * 0.8);
        liquidFloor += ColorAccent.rgb * fresnel * (0.4 + waveHeight * 0.3);

        col = lerp(float3(0.002, 0.002, 0.004), liquidFloor * foregroundFade, fog);
    }
    else
    {
        float skyY = max((uv.y - horizonY) / (1.0 - horizonY), 0.0);
        col = lerp(ColorPrimary.rgb * 0.15, float3(0.002, 0.002, 0.005), pow(skyY, 0.6));
    }

    return col;
}

// Composable Overhead Concert Truss Framing
float3 RenderStageTruss(float2 uv, float3 rd)
{
    float3 col = float3(0.0, 0.0, 0.0);
    if (uv.y > 0.35 || rd.y > 0.15)
    {
        float trussCoordX = abs(frac(uv.x * 6.0) - 0.5);
        float trussCoordY = abs(frac(uv.y * 8.0) - 0.5);
        float trussDiag = abs(frac((uv.x + uv.y) * 4.0) - 0.5);

        float lineX = 1.0 - smoothstep(0.0, 0.05, trussCoordX);
        float lineY = 1.0 - smoothstep(0.0, 0.05, trussCoordY);
        float lineDiag = 1.0 - smoothstep(0.0, 0.04, trussDiag);

        float trussLine = max(max(lineX, lineY * 0.8), lineDiag * 0.6) * 0.55;
        float fade = smoothstep(0.3, 0.85, uv.y);
        col += ColorSecondary.rgb * trussLine * fade * (0.7 + AudioTreble * 0.6);

        // Spotlight mounts on truss nodes
        float spotNodes = pow(max(0.0, sin(uv.x * 12.0) * sin(uv.y * 16.0)), 12.0);
        col += ColorAccent.rgb * spotNodes * fade * (0.8 + AudioKick * 1.2);
    }
    return col;
}

// Composable Horizon Oscilloscope & 3D FFT Equalizer Wall
float3 RenderSoundwaveHorizon(float2 uv, float3 rd)
{
    float3 col = float3(0.0, 0.0, 0.0);
    float horizonY = -0.05 + CustomParam4 * 0.15;

    // Horizon Audio Waveform Oscilloscope Ribbon
    float normX = clamp(uv.x * 0.5 + 0.5, 0.002, 0.998);
    float waveSample = (GetAudioWave(normX) - 0.5) * (0.28 + AudioSubBass * 0.35);
    float distToWave = abs(uv.y - horizonY - waveSample);
    float waveCore = exp(-distToWave * 140.0);
    float waveGlow = exp(-distToWave * 22.0) * 0.45;
    float3 waveCol = lerp(ColorAccent.rgb, ColorPrimary.rgb, abs(uv.x) * 0.6);
    col += waveCol * (waveCore + waveGlow) * (1.1 + AudioKick * 0.8);

    // 3D FFT Equalizer Wall in the background
    if (uv.y > horizonY && uv.y < horizonY + 0.45)
    {
        float fftVal = GetAudioFFT(clamp(abs(uv.x) * 0.35, 0.02, 0.95));
        float barH = (uv.y - horizonY) / 0.45;
        if (barH < fftVal * 0.95)
        {
            float barGridX = frac(uv.x * 32.0);
            float barMask = 1.0 - smoothstep(0.38, 0.48, abs(barGridX - 0.5));
            float barFade = 1.0 - barH;
            col += lerp(ColorSecondary.rgb, ColorAccent.rgb, barH) * barMask * barFade * 0.45;
        }
    }

    return col;
}

// Sci-Fi HUD Telemetry Rings, Angular Ticks & Perimeter Radar
float3 RenderHudTelemetry(float2 uv, float3 rd)
{
    float3 col = float3(0.0, 0.0, 0.0);
    float r = length(uv);
    float angle = atan2(uv.y, uv.x);

    // Keep center area (r < 0.55) clean for Hero 3D Shape
    if (r > 0.55 && r < 1.45)
    {
        // 1. Thin orbital targeting rings
        float ring1 = 1.0 - smoothstep(0.0, 0.0035, abs(r - 0.72));
        float ring2 = 1.0 - smoothstep(0.0, 0.0025, abs(r - 0.88));
        float ring3 = 1.0 - smoothstep(0.0, 0.0020, abs(r - 1.05));

        // 2. Segmented angular ticks (36 radial divisions)
        float numTicks = 36.0;
        float tickFrac = abs(frac(angle * (numTicks / 6.2831853)) - 0.5);
        float tickMask = 1.0 - smoothstep(0.0, 0.04, tickFrac);
        float tickRing = (r > 0.86 && r < 0.90) ? tickMask : 0.0;

        // 4 cardinal major ticks (N, S, E, W)
        float cardFrac = abs(frac(angle * (4.0 / 6.2831853)) - 0.5);
        float cardTick = (1.0 - smoothstep(0.0, 0.025, cardFrac)) * ((r > 0.68 && r < 0.76) ? 1.0 : 0.0);

        // 3. Audio-reactive rotating radar arc
        float radarAngle = frac(angle / 6.2831853 - EnergyAccumulator * 0.25);
        float radarArc = smoothstep(0.85, 1.0, radarAngle) * ((r > 0.70 && r < 0.74) ? 1.0 : 0.0);

        // 4. Hexagonal outer corner framing
        float2 hexUV = abs(uv);
        float hexDist = max(hexUV.x * 0.866025 + hexUV.y * 0.5, hexUV.y);
        float hexFrame = 1.0 - smoothstep(0.0, 0.004, abs(hexDist - 1.25));

        // Color composition
        float3 ringCol = ColorSecondary.rgb * (ring1 * 0.7 + ring2 * 0.85 + ring3 * 0.5);
        float3 tickCol = ColorAccent.rgb * (tickRing * 0.9 + cardTick * 1.4 + radarArc * (1.2 + AudioKick * 1.5));
        float3 frameCol = ColorPrimary.rgb * hexFrame * (0.8 + AudioTreble * 0.8);

        col = (ringCol + tickCol + frameCol) * HudLayerWeight;
    }
    return col;
}

// -----------------------------------------------------------------------------
// 4. Volumetric Laser FX & Stage Lasers
// -----------------------------------------------------------------------------
float LaserBeamSegment(float3 ro, float3 rd, float3 laserOrigin, float3 laserDir, float beamWidth, float maxDepth)
{
    float3 w0 = ro - laserOrigin;
    float a = dot(rd, rd);
    float b = dot(rd, laserDir);
    float c = dot(laserDir, laserDir);
    float d = dot(rd, w0);
    float e = dot(laserDir, w0);

    float denom = a * c - b * b;
    if (abs(denom) < 0.0001) return 0.0;

    float sc = (b * e - c * d) / denom;
    float tc = (a * e - b * d) / denom;

    if (sc < 0.0 || tc < 0.0 || sc > maxDepth) return 0.0;

    float3 dP = w0 + sc * rd - tc * laserDir;
    float dist = length(dP);

    return 1.0 - smoothstep(beamWidth * 0.2, beamWidth, dist);
}

float3 RenderVolumetricLasers(float3 ro, float3 rd, float maxDepth)
{
    if (LaserIntensity <= 0.01) return float3(0.0, 0.0, 0.0);

    float3 col = float3(0.0, 0.0, 0.0);
    int numLasers = 8;
    float spread = 4.2 * (1.0 + AudioSubBass * 0.3);
    float fanAngle = (AudioHighMid * 0.8 + BeatPulse * 0.6) * 0.4 + CustomParam6;

    for (int i = 0; i < numLasers; ++i)
    {
        float normIdx = (float(i) / float(numLasers - 1)) * 2.0 - 1.0;
        float3 emitterPos = float3(normIdx * spread, 3.8, 7.0);

        float sweep = sin(Time * 0.9 * max(CustomParam1, 0.2) + normIdx * 1.6 + CustomParam4) * 0.7;
        float pitch = 0.12 + cos(Time * 1.1 + normIdx * 1.2) * 0.2 * (0.3 + fanAngle);
        float3 laserDir = normalize(float3(normIdx * 0.75 + sweep * 0.5, pitch, -1.0));

        float beamWidth = 0.010 * (1.0 + AudioKick * 0.6);
        float intensity = LaserBeamSegment(ro, rd, emitterPos, laserDir, beamWidth, maxDepth);

        float3 laserCol = ((uint(i) & 1u) == 0u) ? ColorPrimary.rgb : ColorSecondary.rgb;
        if (i == 3 || i == 4) laserCol = ColorAccent.rgb;

        col += laserCol * intensity * 0.95 * LaserIntensity * (0.85 + AudioKick * 0.7);
    }

    return col;
}

// -----------------------------------------------------------------------------
// 5. Audio Particle Swarm & Firefly Sparks
// -----------------------------------------------------------------------------
float3 RenderParticleSwarm(float2 uv, float3 ro, float3 uu, float3 vv, float3 ww, float hitShapeT)
{
    if (ParticleIntensity <= 0.01) return float3(0.0, 0.0, 0.0);

    float3 col = float3(0.0, 0.0, 0.0);
    int numParticles = 24;

    for (int i = 0; i < numParticles; ++i)
    {
        float pId = float(i);
        float seed = Hash11(pId * 13.37);
        float speed = 0.35 + seed * 0.6;

        float theta = Time * speed * CustomParam1 + seed * 6.28;
        float phi = sin(Time * 0.4 * speed + seed * 3.14) * 1.4;
        float radius = 1.3 + seed * 1.6 + AudioTreble * 0.6;

        float3 pPos = float3(
            cos(theta) * cos(phi) * radius,
            sin(phi) * radius + sin(Time * 1.2 + seed * 3.0) * 0.25,
            sin(theta) * cos(phi) * radius
        );

        float3 toP = pPos - ro;
        float zDepth = dot(toP, ww);

        if (zDepth > 0.2 && zDepth < hitShapeT)
        {
            float2 projUV = float2(dot(toP, uu), dot(toP, vv)) * (1.8 / max(zDepth, 0.1));
            float d = length(uv - projUV);
            float sparkSize = (0.010 + AudioTreble * 0.012) / max(zDepth * 0.5, 0.5);
            float spark = 1.0 - smoothstep(0.0, sparkSize, d);

            uint m = uint(i) % 3u;
            float3 pCol = (m == 0u) ? ColorAccent.rgb : ((m == 1u) ? ColorSecondary.rgb : ColorPrimary.rgb);
            col += pCol * spark * (0.8 + BeatPulse * 1.5) * ParticleIntensity;
        }
    }
    return col;
}

// -----------------------------------------------------------------------------
// Master Pixel Shader Entry Point
// -----------------------------------------------------------------------------
float4 PSMain(VSOutput input) : SV_Target
{
    float2 uv = input.UV * 2.0 - 1.0;
    uv.x *= AspectRatio;

    // 1. Camera setup with subtle orbital sway via CustomParam4
    float zoom = clamp(CustomParam0, 0.4, 3.0);
    float camDist = 4.2 / zoom;
    float camDriftY = (CameraDrift > 0.5) ? sin(Time * 0.3) * 0.15 : 0.0;
    float camOrbitAngle = CustomParam4 * 0.35;
    float3 ro = float3(sin(camOrbitAngle) * camDist * 0.3, 0.8 + camDriftY, -cos(camOrbitAngle) * camDist);
    float3 ta = float3(0.0, 0.2, 0.0);

    float3 ww = normalize(ta - ro);
    float3 uu = normalize(cross(ww, float3(0.0, 1.0, 0.0)));
    float3 vv = cross(uu, ww);
    float3 rd = normalize(uv.x * uu + uv.y * vv + 1.8 * ww);

    // 2. Composable 3D Environment Layer Weights (with legacy fallback)
    float skyW   = SkyLayerWeight;
    float warpW  = WarpLayerWeight;
    float floorW = FloorLayerWeight;
    float waveW  = SoundwaveLayerWeight;
    float trussW = StageTrussWeight;
    float hudW   = HudLayerWeight;

    float totalEnvWeight = skyW + warpW + floorW + waveW + trussW;
    if (totalEnvWeight < 0.001)
    {
        skyW   = (BackgroundType == 0) ? 1.0 : 0.0;
        warpW  = (BackgroundType == 1) ? 1.0 : 0.0;
        waveW  = (BackgroundType == 2) ? 1.0 : 0.0;
        trussW = (BackgroundType == 3) ? 1.0 : 0.0;
        floorW = (BackgroundType == 2 || BackgroundType == 3 || BackgroundType == 4) ? 1.0 : 0.0;
    }

    // Composable Sky & Horizon Dome
    float3 col = float3(0.001, 0.0015, 0.003);
    if (skyW > 0.001)   col += RenderStarfieldBackground(uv, rd) * skyW;
    if (warpW > 0.001)  col += RenderWarpTunnel(uv, rd) * warpW;
    if (waveW > 0.001)  col += RenderSoundwaveHorizon(uv, rd) * waveW;
    if (trussW > 0.001) col += RenderStageTruss(uv, rd) * trussW;

    // 3. Raymarch Hero Shape First to obtain hitShape, hitShapeT, shapeColor, totalGlow
    float t = 0.0;
    bool hitShape = false;
    int hitTintMode = 0;
    float totalGlow = 0.0;
    float3 nAccum = float3(0.0, 0.0, 0.0);
    int normalTap = 0;
    float hitShapeT = 1000.0;

    if (ShapeType != 8) // Not None
    {
        float maxMarchDist = (DuplicationMode == 4) ? 22.0 : 9.5;
        [loop]
        for (int i = 0; i < 60; ++i)
        {
            float3 e = float3(0.0, 0.0, 0.0);
            float3 pQuery = ro + rd * t;
            if (hitShape)
            {
                e = 0.57735027 * (2.0 * float3(((normalTap + 3) >> 1) & 1, (normalTap >> 1) & 1, normalTap & 1) - 1.0);
                pQuery += e * 0.003;
            }

            int tintTemp;
            float gTemp;
            float d = MapHeroShape(pQuery, tintTemp, gTemp);

            if (!hitShape)
            {
                totalGlow += gTemp * 0.015;

                if (d < 0.0025)
                {
                    hitShape = true;
                    hitShapeT = t;
                    hitTintMode = tintTemp;
                    normalTap = 0;
                }
                else
                {
                    t += d * 0.82;
                    if (t > maxMarchDist || i >= 55) break;
                }
            }
            else
            {
                nAccum += e * d;
                normalTap++;
                if (normalTap >= 4) break;
            }
        }
    }

    // 4. True 3D Ray-Plane Floor (depth-occluded against Hero Shape)
    if (floorW > 0.01 && rd.y < -0.001)
    {
        float tFloor = (-1.35 - ro.y) / rd.y;
        if (tFloor > 0.0 && tFloor < 45.0 && tFloor < hitShapeT)
        {
            float3 pFloor = ro + rd * tFloor;
            float2 worldXZ = pFloor.xz + float2(0.0, EnergyAccumulator * 1.5 * CustomParam1);

            // Optional liquid obsidian ripple
            if (FluidityAmount > 0.05)
            {
                float ripple = sin(worldXZ.x * 1.5 + worldXZ.y * 1.2 + Time * 2.5) * (0.04 + AudioSubBass * 0.05) * FluidityAmount;
                pFloor.y += ripple;
            }

            // Razor-sharp TRON perspective grid on pFloor.xz using fwidth
            float2 gridCoord = worldXZ * 0.65 * WireframeDensity;
            float2 gridDist = abs(frac(gridCoord - 0.5) - 0.5) / max(fwidth(gridCoord), 0.0001);
            float gridLine = 1.0 - min(min(gridDist.x, gridDist.y), 1.0);

            float3 gridCol = lerp(ColorSecondary.rgb, ColorPrimary.rgb, sin(worldXZ.y * 0.15) * 0.5 + 0.5) * 0.6;
            float3 floorBase = float3(0.002, 0.002, 0.005);
            float3 floorColor = floorBase + gridCol * gridLine * (0.7 + AudioKick * 0.8);

            // 3D FFT equalizer bar glow when waveW > 0.01
            if (waveW > 0.01)
            {
                float waveFreq = clamp(abs(worldXZ.x) * 0.16, 0.02, 0.95);
                float fftSample = GetAudioFFT(waveFreq);
                float soundBar = 1.0 - smoothstep(0.0, 0.15, abs(worldXZ.y * 0.4 - fftSample * 3.0));
                floorColor += ColorAccent.rgb * soundBar * (0.8 + AudioSubBass * 1.5) * waveW;
            }

            // Smoked Obsidian Mirror Reflection
            float3 refRd = float3(rd.x, -rd.y, rd.z);
            float fresnelFloor = pow(1.0 - saturate(-rd.y), 4.0);

            // Sample analytical reflection of Hero Shape at closest approach to origin
            float sc = max(0.0, dot(-pFloor, refRd));
            float3 pClosest = pFloor + refRd * sc;
            bool dummyCore;
            float refGlow;
            float dRef = EvaluateSingleShape(pClosest, dummyCore, refGlow);
            float shapeRefl = 0.025 / (0.05 + abs(dRef) * 4.0);
            float3 refColor = ColorPrimary.rgb * shapeRefl * (1.0 + AudioKick * 0.8);

            if (skyW > 0.01)  refColor += RenderStarfieldBackground(uv * float2(1.0, -1.0), refRd) * 0.3 * skyW;
            if (warpW > 0.01) refColor += RenderWarpTunnel(uv * float2(1.0, -1.0), refRd) * 0.3 * warpW;

            floorColor += refColor * fresnelFloor * 0.85;

            float fogFloor = exp(-tFloor * 0.055);
            float3 finalFloorCol = lerp(col, floorColor, fogFloor);
            col = lerp(col, finalFloorCol, floorW);
        }
    }

    // 5. Four Surface Material Modes when hitShape is true
    if (hitShape)
    {
        float3 pHit = ro + rd * t;
        float3 shapeColor = EvaluateHeroShapeColor(pHit, hitTintMode);
        float3 n = normalize(nAccum);

        float3 lightDir = normalize(float3(0.6, 1.0, -0.5));
        float diff = max(0.0, dot(n, lightDir));
        float fresnel = pow(1.0 - max(0.0, dot(-rd, n)), 3.0);
        float3 refDir = reflect(rd, n);
        float spec = pow(max(0.0, dot(refDir, lightDir)), 24.0);

        float3 litCol = shapeColor;

        if (MaterialMode == 0) // 0: Smoked Obsidian & Neon (TRON)
        {
            float3 body = shapeColor * 0.04;
            float3 rim = ColorSecondary.rgb * fresnel * (1.2 + BeatPulse * 0.8);
            float3 specular = ColorAccent.rgb * spec * (1.0 + AudioKick * 1.5);
            float3 envRefl = lerp(ColorPrimary.rgb * 0.15, ColorSecondary.rgb * 0.25, refDir.y * 0.5 + 0.5);

            float3 dndx = ddx(n);
            float3 dndy = ddy(n);
            float edgeCurv = length(dndx) + length(dndy);
            float neonEdge = smoothstep(0.08, 0.45, edgeCurv);
            float3 neonGlow = ColorAccent.rgb * neonEdge * (1.4 + AudioKick * 1.8) + ColorPrimary.rgb * clamp(totalGlow, 0.0, 0.4);

            litCol = body + rim + specular + envRefl + neonGlow;
        }
        else if (MaterialMode == 1) // 1: Alive LED Matrix (Daft Punk)
        {
            float3 darkFacet = shapeColor * (0.08 + diff * 0.15);
            float ledStripH = sin(pHit.y * 28.0 * WireframeDensity + EnergyAccumulator * 3.0);
            float ledStripV = sin(pHit.x * 24.0 * WireframeDensity);
            float stripMask = smoothstep(0.5, 0.85, ledStripH) * smoothstep(-0.2, 0.8, ledStripV);
            float fftDrive = GetAudioFFT(clamp(abs(pHit.y) * 0.25, 0.02, 0.95));
            float3 ledColor = lerp(ColorPrimary.rgb, ColorAccent.rgb, stripMask) * (1.4 + AudioKick * 2.0 + fftDrive * 2.5);

            litCol = darkFacet + ledColor * stripMask + ColorSecondary.rgb * fresnel * 0.6;
        }
        else if (MaterialMode == 2) // 2: Holographic Hard-Light HUD
        {
            float3 bgBehind = col;
            float scanline = sin(pHit.y * 65.0 - EnergyAccumulator * 6.0) * 0.5 + 0.5;
            float3 grid3D = abs(frac(pHit * 6.0 * WireframeDensity) - 0.5);
            float wireLattice = 1.0 - smoothstep(0.0, 0.06, min(grid3D.x, min(grid3D.y, grid3D.z)));
            float azimuth = atan2(pHit.z, pHit.x);
            float ticks = 1.0 - smoothstep(0.0, 0.04, abs(frac(azimuth * (24.0 / 6.2831853)) - 0.5));

            float3 holoColor = lerp(ColorPrimary.rgb, ColorAccent.rgb, scanline * 0.5 + wireLattice * 0.5);
            float holoIntensity = (fresnel * 1.5 + scanline * 0.35 + wireLattice * 0.7 + ticks * 0.4) * (1.0 + AudioTreble * 1.2);

            litCol = lerp(bgBehind * 0.35, holoColor * holoIntensity, 0.82);
        }
        else // 3: Liquid Chrome Mirror
        {
            float3 specColor = float3(1.0, 1.0, 1.0) * spec * (1.8 + AudioKick * 1.5);
            float3 chrPrimary   = ColorPrimary.rgb * (sin(refDir.y * 4.0 + EnergyAccumulator) * 0.4 + 0.6);
            float3 chrSecondary = ColorSecondary.rgb * (cos(refDir.x * 4.0 + EnergyAccumulator * 1.2) * 0.4 + 0.6);
            float3 chrAccent    = ColorAccent.rgb * (sin(refDir.z * 4.0 - EnergyAccumulator * 0.8) * 0.4 + 0.6);
            float3 chromRefl    = (chrPrimary + chrSecondary + chrAccent) * 0.45;

            litCol = chromRefl + specColor + ColorAccent.rgb * fresnel * 0.9;
        }

        // Specular laser bounce onto the hero shape
        if (LaserIntensity > 0.01)
        {
            float laserBounce = pow(max(0.0, dot(n, normalize(float3(0.0, 1.0, 0.5)))), 8.0);
            litCol += ColorAccent.rgb * laserBounce * LaserIntensity * (0.4 + AudioKick * 0.6);
        }

        float fog = exp(-t * 0.05);
        float crossfade = clamp(ShapeCrossfade, 0.0, 1.0);
        col = lerp(col, litCol, fog * crossfade);
    }

    // Hero shape ambient edge glow
    col += ColorPrimary.rgb * clamp(totalGlow, 0.0, 0.35) * (0.6 + AudioKick * 0.4);

    // 6. Volumetric Laser Matrix (unlocked in all scenes with depth occlusion)
    if (LaserIntensity > 0.01)
    {
        float floorDepth = (floorW > 0.01 && rd.y < -0.001) ? (-1.35 - ro.y) / rd.y : 1000.0;
        float maxDepth = min(hitShapeT, floorDepth);
        col += RenderVolumetricLasers(ro, rd, maxDepth);
    }

    // 7. 3D Camera Particles (depth-occluded against Hero Shape)
    col += RenderParticleSwarm(uv, ro, uu, vv, ww, hitShapeT);

    // 8. Sci-Fi HUD Telemetry Rings & Framing
    if (hudW > 0.001)
    {
        col += RenderHudTelemetry(uv, rd);
    }

    // 9. Stage Edge Border Vignette Pulse on heavy drops / beats
    if (DropIntensity > 0.2 || BeatPulse > 0.7)
    {
        float blinder = (DropIntensity * 0.5 + BeatPulse * 0.3);
        float edgeDist = length((input.UV - 0.5) * 2.0);
        float borderVignette = smoothstep(0.65, 1.35, edgeDist);
        col += ColorAccent.rgb * (blinder * borderVignette * 0.35);
    }

    // 10. Strobe Flash Shutter
    col = ApplyStrobe(col, StrobeFlash, Time);

    return float4(col, 1.0);
}

