// =============================================================================
// CustomNode_RaymarchedCrystals.hlsl - UE5 Material Custom Node Snippet
// Inputs:
//   float2 UV
//   float Time
//   float AudioSubBass
//   float AudioKick
//   float3 ColorPrimary
//   float3 ColorSecondary
//   float3 ColorAccent
// Output:
//   float3 (Emissive Color)
// =============================================================================

float2 uv = UV * 2.0 - 1.0;
float3 ro = float3(sin(Time * 0.3) * 3.5, 1.0, cos(Time * 0.3) * 3.5);
float3 ta = float3(0.0, 0.0, 0.0);

float3 ww = normalize(ta - ro);
float3 uu = normalize(cross(ww, float3(0.0, 1.0, 0.0)));
float3 vv = cross(uu, ww);
float3 rd = normalize(uv.x * uu + uv.y * vv + 1.5 * ww);

float t = 0.0;
float glow = 0.0;
float3 col = float3(0.01, 0.01, 0.02);

for (int i = 0; i < 40; ++i) {
    float3 p = ro + rd * t;
    float3 q = abs(p);
    float dOct = (q.x + q.y + q.z - 1.0) * 0.577;
    glow += 0.01 / (0.05 + abs(dOct) * 6.0);
    if (dOct < 0.005) {
        col = lerp(ColorPrimary, ColorSecondary, p.y * 0.5 + 0.5) * (1.2 + AudioKick * 2.0);
        break;
    }
    if (t > 15.0) break;
    t += dOct * 0.8;
}

col += ColorAccent * glow * (1.0 + AudioSubBass * 2.5);
return col;
