// =============================================================================
// CustomNode_SynthwaveGrid.hlsl - UE5 Material Custom Node Snippet
// Inputs:
//   float2 UV
//   float Time
//   float AudioKick
//   float AudioLowMid
//   float3 ColorPrimary
//   float3 ColorSecondary
//   float3 ColorAccent
// Output:
//   float3 (Emissive Color)
// =============================================================================

float2 uv = UV * 2.0 - 1.0;
float3 col = float3(0.01, 0.0, 0.03);

if (uv.y > 0.0) { // Sky & Sun
    float sunDist = length(uv - float2(0.0, 0.35));
    if (sunDist < 0.35) {
        float blinds = sin(uv.y * 60.0 - Time * 2.0);
        float bMask = smoothstep(-0.2, 0.5, blinds);
        col = lerp(ColorPrimary, ColorAccent, uv.y * 2.0) * bMask * 1.5;
    }
    col += ColorPrimary * (0.04 / (0.04 + sunDist * 2.0));
} else { // Grid
    float depth = 1.0 / max(-uv.y, 0.01);
    float2 gridP = float2(uv.x * depth, depth * 2.0 + Time * 4.0);
    float2 gDist = abs(frac(gridP - 0.5) - 0.5);
    float line = min(gDist.x, gDist.y);
    float intensity = smoothstep(0.1, 0.0, line);

    float3 gCol = lerp(ColorSecondary, ColorPrimary, smoothstep(0.5, 2.5, abs(gridP.x)));
    col = gCol * intensity * (1.0 + AudioKick * 2.0) * exp(-depth * 0.05);
}

return col;
