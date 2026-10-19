// =============================================================================
// CustomNode_PostProcessGlitch.hlsl - UE5 Post Process Material Custom Node
// Inputs:
//   float2 UV
//   float Time
//   float AudioKick
//   float BeatPulse
//   Texture2D SceneTexture (PostProcessInput0)
//   SamplerState SceneSampler
// Output:
//   float3 (Final Post Process RGB)
// =============================================================================

float2 uv = UV;
float split = (0.002 + AudioKick * 0.006 + BeatPulse * 0.004);

float r = SceneTexture.Sample(SceneSampler, uv + float2(split, 0.0)).r;
float g = SceneTexture.Sample(SceneSampler, uv).g;
float b = SceneTexture.Sample(SceneSampler, uv - float2(split, 0.0)).b;

float3 col = float3(r, g, b);

// CRT scanline
float scanline = sin(uv.y * 1080.0 * 1.5) * 0.03;
col -= scanline;

return col;
