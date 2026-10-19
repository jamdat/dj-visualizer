// =============================================================================
// CustomNode_AlivePyramid.hlsl - UE5 Material Custom Node Snippet
// Inputs:
//   float2 UV (Viewport UV coordinates)
//   float Time (Material Time or Engine Game Time)
//   float AudioSubBass (0..2)
//   float AudioKick (0..2)
//   float BeatPulse (0..1)
//   float3 ColorPrimary (RGB)
//   float3 ColorAccent (RGB)
// Output:
//   float3 (Emissive Color)
// =============================================================================

float2 uv = UV * 2.0 - 1.0;
uv.x *= 1.7777;

// Pyramid shape math
float2 pPyr = uv;
pPyr.y += 0.2;
float halfW = max(0.0, (1.0 - pPyr.y * 1.5)) * 0.6;
bool inside = (abs(pPyr.x) <= halfW) && (pPyr.y >= -0.6 && pPyr.y <= 0.6);
float edge = abs(abs(pPyr.x) - halfW);

float wire = smoothstep(0.03, 0.005, edge);
float ledStrips = sin(pPyr.y * 30.0 + Time * 3.0);
float stripMask = smoothstep(0.7, 0.95, ledStrips);

float3 col = float3(0.01, 0.01, 0.02);
if (inside) {
    col = lerp(ColorPrimary, ColorAccent, stripMask) * (1.2 + AudioKick * 2.0);
    col += ColorAccent * (wire * 2.0);
}

// Sub-bass pulsing core
float coreDist = length(uv - float2(0.0, -0.1));
col += ColorAccent * (0.05 / (0.05 + coreDist * coreDist * 8.0)) * (1.0 + AudioSubBass * 3.0);

return col;
