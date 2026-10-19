#include "Common.hlsl"

// =============================================================================
// SynthwaveGrid.hlsl - Outrun / Tron Neon Horizon & Audio-Reactive Terrain Grid
// =============================================================================

float GetTerrainHeight(float2 p)
{
    // Highway channel in center
    float roadMask = smoothstep(0.4, 2.5, abs(p.x));

    // Audio-reactive mountain terrain based on FFT spectrum
    float fftX = abs(p.x) * 0.15;
    float audioPeak = GetAudioFFT(clamp(fftX, 0.05, 0.95));

    float n1 = Noise2D(p * 0.35 + float2(0.0, -EnergyAccumulator * 0.5));
    float n2 = Noise2D(p * 0.8 + float2(0.0, -EnergyAccumulator * 0.8)) * 0.5;

    float height = (n1 + n2) * (1.2 + audioPeak * 2.5) * roadMask;
    return height;
}

float4 PSMain(VSOutput input) : SV_Target
{
    float2 uv = input.UV * 2.0 - 1.0;
    uv.x *= AspectRatio;

    float3 col = float3(0.02, 0.01, 0.05);

    // Horizon line
    float horizonY = -0.05 + CustomParam4 * 0.2;

    if (uv.y > horizonY) // Sky & Retro Neon Sun
    {
        float skyY = (uv.y - horizonY) / (1.0 - horizonY);

        // Sky Gradient (Dark Purple to Horizon Red)
        col = lerp(float3(0.01, 0.0, 0.04), ColorPrimary.rgb * 0.4, pow(1.0 - skyY, 2.5));

        // Audio-Reactive Starfield
        float2 starUV = uv * 35.0;
        float2 starCell = floor(starUV);
        float starRand = Hash21(starCell);
        if (starRand > 0.94) {
            float starDist = length(frac(starUV) - 0.5);
            float twinkle = sin(Time * 4.0 + starRand * 20.0) * 0.5 + 0.5;
            col += ColorSecondary.rgb * (1.0 - smoothstep(0.0, 0.12, starDist)) * twinkle * (0.8 + AudioTreble * 1.5);
        }

        // Retro Sun on Horizon
        float2 sunCenter = float2(0.0, horizonY + 0.35);
        float sunDist = length((uv - sunCenter) * float2(1.0, 1.25));
        float sunRadius = 0.42 * CustomParam0;

        if (sunDist < sunRadius)
        {
            // Horizontal raster blinds
            float blinds = sin(uv.y * 70.0 - Time * 0.8);
            float blindMask = smoothstep(-0.2, 0.6, blinds);

            // Sun color gradient (Yellow at top to Neon Red/Orange at bottom)
            float sunGrad = (uv.y - (sunCenter.y - sunRadius)) / (sunRadius * 2.0);
            float3 sunColor = lerp(ColorPrimary.rgb, ColorAccent.rgb, pow(sunGrad, 1.2));

            // Blend blinds toward bottom of sun
            float blindBlend = lerp(1.0, blindMask, 1.0 - smoothstep(0.0, 0.4, sunGrad));
            col = lerp(col, sunColor * (1.2 + BeatPulse * 0.5), blindBlend);
        }

        // Sun Glow Halo
        col += ColorPrimary.rgb * (0.05 / (0.05 + sunDist * 1.5)) * BloomIntensity * (1.0 + AudioLowMid * 1.2);
    }
    else // Endless Neon Ground Grid & Mountain Raymarch
    {
        // Camera ray for terrain
        float groundY = (horizonY - uv.y) / (1.0 + horizonY);
        float depth = 1.0 / max(groundY, 0.001);

        float3 worldPos;
        worldPos.x = uv.x * depth * 0.8;
        worldPos.z = depth * 2.0 + EnergyAccumulator * 2.5 * CustomParam1;
        worldPos.y = -1.2;

        // Terrain height displacement
        float tHeight = GetTerrainHeight(worldPos.xz);
        worldPos.y += tHeight;

        // Grid lines calculation
        float2 gridCoord = worldPos.xz * 0.8 * WireframeDensity;
        float2 gridDist = abs(frac(gridCoord - 0.5) - 0.5) / fwidth(gridCoord);
        float lineVal = min(gridDist.x, gridDist.y);
        float gridIntensity = 1.0 - min(lineVal, 1.0);

        // Center highway laser lines
        float centerRoad = (1.0 - smoothstep(0.02, 0.08, abs(worldPos.x))) + (1.0 - smoothstep(0.34, 0.4, abs(worldPos.x)));

        // Audio reactivity colors
        float3 gridColor = lerp(ColorSecondary.rgb, ColorPrimary.rgb, smoothstep(0.5, 3.0, abs(worldPos.x)));
        gridColor = lerp(gridColor, ColorAccent.rgb, centerRoad);

        // Depth fog / Atmospheric haze
        float fog = exp(-depth * 0.05);

        float3 groundCol = float3(0.01, 0.01, 0.03);
        groundCol += gridColor * gridIntensity * (1.0 + AudioKick * 2.0) * (0.6 + BeatPulse * 0.8);
        groundCol += ColorAccent.rgb * centerRoad * (1.5 + AudioSubBass * 2.5);

        col = lerp(float3(0.0, 0.0, 0.02), groundCol, fog);
    }

    // Volumetric Neon Fog on Horizon
    float horizonGlow = exp(-abs(uv.y - horizonY) * 12.0);
    col += ColorPrimary.rgb * horizonGlow * 0.4 * (1.0 + AudioKick * 1.5) * BloomIntensity;

    // Strobe Flash
    col = ApplyStrobe(col, StrobeFlash, Time);

    return float4(col, 1.0);
}
