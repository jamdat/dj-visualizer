#include "Common.hlsl"

// =============================================================================
// AlivePyramid.hlsl - Daft Punk "Alive 2007" Concert Stage Visualizer
// =============================================================================

float MapScene(float3 p, out int matId)
{
    matId = 0; // 0 = floor, 1 = pyramid wireframe, 2 = pyramid core, 3 = stage lights

    // 1. Reflective stage floor with audio undulation
    float floorDist = p.y + 1.2;

    // 2. Main Alive Stage Pyramid
    float3 pPyr = p - float3(0.0, 0.4, 0.0);
    pPyr.xz = Rotate2D(pPyr.xz, CustomParam4 + Time * 0.1 * CustomParam1);

    float pyrSize = 1.8 * CustomParam0;
    float pyrDist = SDFPyramid(pPyr / pyrSize, 1.2) * pyrSize;

    // Sub-bass pulsing core
    float coreDist = SDFOctahedron(pPyr, 0.45 + AudioSubBass * 0.4);

    // Combine
    float d = min(floorDist, pyrDist);
    if (d == floorDist) matId = 0;
    else if (d == pyrDist) matId = 1;

    if (coreDist < d) {
        d = coreDist;
        matId = 2;
    }

    return d;
}

float3 GetNormal(float3 p)
{
    float2 e = float2(0.002, 0.0);
    int matDummy;
    return normalize(float3(
        MapScene(p + e.xyy, matDummy) - MapScene(p - e.xyy, matDummy),
        MapScene(p + e.yxy, matDummy) - MapScene(p - e.yxy, matDummy),
        MapScene(p + e.yyx, matDummy) - MapScene(p - e.yyx, matDummy)
    ));
}

float4 PSMain(VSOutput input) : SV_Target
{
    float2 uv = input.UV * 2.0 - 1.0;
    uv.x *= AspectRatio;

    // Camera setup
    float camDist = 4.2 / max(CustomParam0, 0.1);
    float3 ro = float3(0.0, 0.8 + sin(Time * 0.3) * 0.2, -camDist);
    float3 ta = float3(0.0, 0.2, 0.0);

    float3 ww = normalize(ta - ro);
    float3 uu = normalize(cross(ww, float3(0.0, 1.0, 0.0)));
    float3 vv = cross(uu, ww);
    float3 rd = normalize(uv.x * uu + uv.y * vv + 1.8 * ww);

    // Raymarching
    float t = 0.0;
    int hitMat = -1;
    float glow = 0.0;
    float coreGlow = 0.0;

    for (int i = 0; i < 90; ++i)
    {
        float3 p = ro + rd * t;
        int mId;
        float d = MapScene(p, mId);

        // Volumetric wireframe edge glow accumulation
        float edgeDensity = 1.0 / (1.0 + abs(d) * 45.0 * WireframeDensity);
        glow += edgeDensity * (0.015 + BeatPulse * 0.03);

        // Core glow accumulation
        float coreDist = length(p - float3(0.0, 0.4, 0.0));
        coreGlow += 0.02 / (1.0 + coreDist * coreDist * 4.0);

        if (d < 0.001)
        {
            hitMat = mId;
            break;
        }
        if (t > 25.0) break;
        t += d * 0.75;
    }

    float3 col = float3(0.01, 0.01, 0.02);

    // Background LED Stadium Matrix
    float2 ledGrid = frac(uv * 28.0);
    float ledDist = length(ledGrid - 0.5);
    float ledMask = 1.0 - smoothstep(0.38, 0.42, ledDist);

    float fftVal = GetAudioFFT(abs(uv.x) * 0.3);
    float ledBar = step(uv.y * 1.5 + 0.3, fftVal * 1.6 - 0.2);
    float3 ledColor = lerp(ColorSecondary.rgb, ColorPrimary.rgb, abs(uv.x)) * ledBar * ledMask * 0.6;
    col += ledColor;

    // Lighting & Shading
    if (hitMat >= 0)
    {
        float3 p = ro + rd * t;
        float3 n = GetNormal(p);

        if (hitMat == 0) // Reflective Floor
        {
            float2 floorGrid = frac(p.xz * 1.5 * WireframeDensity);
            float gridLine = (1.0 - smoothstep(0.0, 0.05, abs(floorGrid.x - 0.5))) + (1.0 - smoothstep(0.0, 0.05, abs(floorGrid.y - 0.5)));

            float wave = GetAudioWave(frac(length(p.xz) * 0.1 - Time * 0.5));
            col = float3(0.02, 0.02, 0.04);
            col += ColorPrimary.rgb * gridLine * (0.4 + wave * 0.8) * exp(-p.z * 0.15);

            // Floor reflection of pyramid
            float3 refDir = reflect(rd, n);
            col += ColorAccent.rgb * pow(max(0.0, dot(refDir, float3(0.0, 1.0, 0.0))), 8.0) * (0.8 + AudioKick * 1.5);
        }
        else if (hitMat == 1) // Pyramid Wireframe
        {
            // Horizontal LED strips across the pyramid faces
            float ledStrip = sin(p.y * 24.0 + EnergyAccumulator * 2.0);
            float stripMask = smoothstep(0.7, 0.95, ledStrip);

            float3 pyrColor = lerp(ColorPrimary.rgb, ColorAccent.rgb, stripMask);
            col = pyrColor * (1.2 + AudioKick * 2.0 + BeatPulse * 1.5);
        }
        else if (hitMat == 2) // Inner Core
        {
            col = ColorAccent.rgb * (3.0 + AudioSubBass * 4.0);
        }
    }

    // Additive Volumetric Glow & Beams
    col += ColorPrimary.rgb * glow * BloomIntensity * (1.0 + AudioKick * 2.0);
    col += ColorAccent.rgb * coreGlow * BloomIntensity * (1.0 + AudioSubBass * 3.0);

    // Strobe Flash
    col = ApplyStrobe(col, StrobeFlash, Time);

    return float4(col, 1.0);
}
