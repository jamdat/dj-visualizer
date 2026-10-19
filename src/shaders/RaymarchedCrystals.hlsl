#include "Common.hlsl"

// =============================================================================
// RaymarchedCrystals.hlsl - 3D Morphing Geometric Monoliths & Fractal Crystals
// =============================================================================

float MapCrystalScene(float3 p, out float3 outColor)
{
    // Space repetition for infinite fractal hall
    float3 q = p;
    float repDist = 4.0 / max(CustomParam2, 0.2);
    q.xz = (frac(q.xz / repDist + 0.5) - 0.5) * repDist;

    // Audio-reactive twist along Y axis
    float twist = (AudioSubBass * 1.5 + BeatPulse * 0.8) * 0.8;
    q.xz = Rotate2D(q.xz, q.y * twist + Time * 0.2 * CustomParam1 + CustomParam4);

    // Morph between Octahedron crystal and Box monolith based on AudioLowMid
    float sOct = SDFOctahedron(q, 1.2 * CustomParam0);
    float sBox = SDFBox(q, float3(0.8, 1.6, 0.8) * CustomParam0) - 0.1;
    float morphFactor = clamp(AudioLowMid * 1.5, 0.0, 1.0);
    float dCrystal = lerp(sOct, sBox, morphFactor);

    // Inner glowing core
    float sCore = SDFSphere(q, 0.5 + AudioKick * 0.4);

    float d = min(dCrystal, sCore);

    if (d == sCore) {
        outColor = ColorAccent.rgb * (2.5 + AudioSubBass * 3.0);
    } else {
        float3 gradCol = lerp(ColorPrimary.rgb, ColorSecondary.rgb, clamp(q.y * 0.5 + 0.5, 0.0, 1.0));
        outColor = gradCol;
    }

    return d;
}

float3 GetCrystalNormal(float3 p)
{
    float2 e = float2(0.002, 0.0);
    float3 dummyCol;
    return normalize(float3(
        MapCrystalScene(p + e.xyy, dummyCol) - MapCrystalScene(p - e.xyy, dummyCol),
        MapCrystalScene(p + e.yxy, dummyCol) - MapCrystalScene(p - e.yxy, dummyCol),
        MapCrystalScene(p + e.yyx, dummyCol) - MapCrystalScene(p - e.yyx, dummyCol)
    ));
}

float4 PSMain(VSOutput input) : SV_Target
{
    float2 uv = input.UV * 2.0 - 1.0;
    uv.x *= AspectRatio;

    // Orbiting Camera
    float camRadius = 4.0;
    float camAngle = Time * 0.15 * CustomParam1 + CustomParam4;
    float3 ro = float3(sin(camAngle) * camRadius, 1.2 + sin(Time * 0.3) * 0.5, cos(camAngle) * camRadius);
    float3 ta = float3(0.0, 0.0, 0.0);

    float3 ww = normalize(ta - ro);
    float3 uu = normalize(cross(ww, float3(0.0, 1.0, 0.0)));
    float3 vv = cross(uu, ww);
    float3 rd = normalize(uv.x * uu + uv.y * vv + 1.6 * ww);

    float t = 0.0;
    float glow = 0.0;
    float3 hitCol = float3(0.0, 0.0, 0.0);
    bool hit = false;

    for (int i = 0; i < 90; ++i)
    {
        float3 p = ro + rd * t;
        float3 colTemp;
        float d = MapCrystalScene(p, colTemp);

        // Volumetric edge & core glow
        glow += 0.015 / (0.05 + abs(d) * 8.0);

        if (d < 0.002)
        {
            hit = true;
            hitCol = colTemp;
            break;
        }
        if (t > 30.0) break;
        t += d * 0.7;
    }

    float3 col = float3(0.01, 0.01, 0.02);

    if (hit)
    {
        float3 p = ro + rd * t;
        float3 n = GetCrystalNormal(p);

        // Specular & Fresnel Reflections
        float3 lightDir = normalize(float3(0.6, 1.0, -0.4));
        float diff = max(0.0, dot(n, lightDir));
        float fresnel = pow(1.0 - max(0.0, dot(-rd, n)), 3.5);

        // Chromatic dispersion reflection
        float3 refDir = reflect(rd, n);
        float3 refR = ColorPrimary.rgb * pow(max(0.0, dot(refDir + float3(0.02, 0.0, 0.0), lightDir)), 16.0);
        float3 refG = ColorSecondary.rgb * pow(max(0.0, dot(refDir, lightDir)), 16.0);
        float3 refB = ColorAccent.rgb * pow(max(0.0, dot(refDir - float3(0.02, 0.0, 0.0), lightDir)), 16.0);

        col = hitCol * (0.3 + diff * 0.7) + (refR + refG + refB) * (1.5 + AudioKick * 2.0);
        col += ColorSecondary.rgb * fresnel * (1.0 + BeatPulse * 1.5);

        // Distance fog
        col = lerp(col, float3(0.01, 0.01, 0.03), 1.0 - exp(-t * 0.08));
    }

    // Glow accumulation
    col += ColorPrimary.rgb * glow * 0.08 * BloomIntensity * (1.0 + AudioKick * 1.5);

    // Strobe Flash
    col = ApplyStrobe(col, StrobeFlash, Time);

    return float4(col, 1.0);
}
