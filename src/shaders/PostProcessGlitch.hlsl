#include "Common.hlsl"

// =============================================================================
// PostProcessGlitch.hlsl - Audio-Reactive Chromatic Aberration, Glitch & Bloom
// =============================================================================

float4 PSMain(VSOutput input) : SV_Target
{
    float2 uv = input.UV;

    // 1. Digital Glitch Block Displacement (triggered by transients or high GlitchRate)
    float glitchTrigger = (BeatPulse > 0.7 ? 1.0 : 0.0) * GlitchRate + CustomParam7;
    if (glitchTrigger > 0.05)
    {
        float2 block = floor(uv * float2(24.0, 60.0));
        float blockRand = Hash21(block + floor(Time * 20.0));

        if (blockRand < 0.15 * glitchTrigger)
        {
            float shift = (Hash11(blockRand * 10.0) - 0.5) * 0.08 * glitchTrigger;
            uv.x += shift;
        }
    }

    // 2. Audio-Reactive Chromatic Aberration (RGB Split)
    float splitAmount = (0.003 + AudioKick * 0.008 + BeatPulse * 0.006) * (1.0 + GlitchRate);
    float2 redUV   = uv + float2(splitAmount, 0.0);
    float2 greenUV = uv;
    float2 blueUV  = uv - float2(splitAmount, 0.0);

    float r = SceneTexture.Sample(SamplerLinearClamp, redUV).r;
    float g = SceneTexture.Sample(SamplerLinearClamp, greenUV).g;
    float b = SceneTexture.Sample(SamplerLinearClamp, blueUV).b;
    float3 col = float3(r, g, b);

    // 3. Subtle VHS Scanlines
    float scanline = sin(uv.y * Resolution.y * 1.5) * 0.04;
    col -= scanline;

    // 4. Optical Lens Vignette
    float2 vigUV = saturate(uv) * (1.0 - saturate(uv.yx));
    float vig = max(vigUV.x * vigUV.y * 16.0, 0.0);
    vig = pow(vig, 0.25);
    col *= vig;

    // 5. ACES Filmic Tone Mapping Curve
    float a = 2.51f;
    float b_val = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    col = saturate((col * (a * col + b_val)) / (col * (c * col + d) + e));

    return float4(col, 1.0);
}
