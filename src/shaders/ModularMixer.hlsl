#include "Common.hlsl"

// =============================================================================
// ModularMixer.hlsl - Dad Punk Alive 2007 Stage Engine
// 4-Sided LED Pyramid, TRON Grid Floor, Daft LED Dot-Matrix Logo,
// Volumetric Apex Lasers, Halloween Spooky Moon/Bats/Pumpkins, Kid-Safe Limiter
// =============================================================================

// -----------------------------------------------------------------------------
// 1. Daft Punk Alive 2007 Analytical Vector Font Engine
// Supports 5 Rendering Styles:
// 0: Neon Wireframe Outline   1: Full-Faced Geometric Mesh
// 2: Scrolling Digital Banner  3: Digital Scanlines / Cyber Slices
// 4: High-Density LED Matrix
// -----------------------------------------------------------------------------
void GetAliveSegmentEnds(int segIdx, out float2 a, out float2 b)
{
    // Segment coordinates in local char space [-0.45, 0.45] x [-0.65, 0.65]
    if (segIdx == 0)  { a = float2(-0.38,  0.58); b = float2( 0.00,  0.58); return; } // Top-left horiz
    if (segIdx == 1)  { a = float2( 0.00,  0.58); b = float2( 0.38,  0.58); return; } // Top-right horiz
    if (segIdx == 2)  { a = float2(-0.38, -0.58); b = float2( 0.00, -0.58); return; } // Bottom-left horiz
    if (segIdx == 3)  { a = float2( 0.00, -0.58); b = float2( 0.38, -0.58); return; } // Bottom-right horiz
    if (segIdx == 4)  { a = float2(-0.38,  0.00); b = float2(-0.38,  0.58); return; } // Upper-left vert
    if (segIdx == 5)  { a = float2(-0.38, -0.58); b = float2(-0.38,  0.00); return; } // Lower-left vert
    if (segIdx == 6)  { a = float2( 0.38,  0.00); b = float2( 0.38,  0.58); return; } // Upper-right vert
    if (segIdx == 7)  { a = float2( 0.38, -0.58); b = float2( 0.38,  0.00); return; } // Lower-right vert
    if (segIdx == 8)  { a = float2(-0.38,  0.00); b = float2( 0.00,  0.00); return; } // Mid-left horiz
    if (segIdx == 9)  { a = float2( 0.00,  0.00); b = float2( 0.38,  0.00); return; } // Mid-right horiz
    if (segIdx == 10) { a = float2( 0.00,  0.00); b = float2( 0.00,  0.58); return; } // Upper-center vert
    if (segIdx == 11) { a = float2( 0.00, -0.58); b = float2( 0.00,  0.00); return; } // Lower-center vert
    if (segIdx == 12) { a = float2(-0.38,  0.58); b = float2( 0.00,  0.00); return; } // Diag up-left
    if (segIdx == 13) { a = float2( 0.38,  0.58); b = float2( 0.00,  0.00); return; } // Diag up-right
    if (segIdx == 14) { a = float2(-0.38, -0.58); b = float2( 0.00,  0.00); return; } // Diag low-left
    /* segIdx == 15 */{ a = float2( 0.38, -0.58); b = float2( 0.00,  0.00); return; } // Diag low-right
}

uint GetAliveSegmentMask(uint ch)
{
    // Convert lowercase to uppercase
    if (ch >= 97u && ch <= 122u) ch -= 32u;

    if (ch == 32u) return 0x0000u; // Space

    // Letters A-Z
    if (ch == 65u) return 0x03F3u; // A
    if (ch == 66u) return 0x0ECEu; // B
    if (ch == 67u) return 0x003Fu; // C
    if (ch == 68u) return 0x00FFu; // D
    if (ch == 69u) return 0x013Fu; // E
    if (ch == 70u) return 0x0133u; // F
    if (ch == 71u) return 0x02BFu; // G
    if (ch == 72u) return 0x03F0u; // H
    if (ch == 73u) return 0x0C0Fu; // I
    if (ch == 74u) return 0x00ECu; // J
    if (ch == 75u) return 0xA130u; // K
    if (ch == 76u) return 0x003Cu; // L
    if (ch == 77u) return 0x30F0u; // M
    if (ch == 78u) return 0x90F0u; // N
    if (ch == 79u) return 0x00FFu; // O
    if (ch == 80u) return 0x0373u; // P
    if (ch == 81u) return 0x80FFu; // Q
    if (ch == 82u) return 0x8373u; // R
    if (ch == 83u) return 0x039Fu; // S
    if (ch == 84u) return 0x0C03u; // T
    if (ch == 85u) return 0x00FCu; // U
    if (ch == 86u) return 0x6030u; // V
    if (ch == 87u) return 0xC0F0u; // W
    if (ch == 88u) return 0xF000u; // X
    if (ch == 89u) return 0x3800u; // Y
    if (ch == 90u) return 0x600Fu; // Z

    // Digits 0-9
    if (ch == 48u) return 0x00FFu; // 0
    if (ch == 49u) return 0x00C0u; // 1
    if (ch == 50u) return 0x036Fu; // 2
    if (ch == 51u) return 0x02CFu; // 3
    if (ch == 52u) return 0x03D0u; // 4
    if (ch == 53u) return 0x039Fu; // 5
    if (ch == 54u) return 0x03BFu; // 6
    if (ch == 55u) return 0x00C3u; // 7
    if (ch == 56u) return 0x03FFu; // 8
    if (ch == 57u) return 0x03DFu; // 9

    // Symbols
    if (ch == 45u) return 0x0300u; // -
    if (ch == 33u) return 0x0400u; // !

    return 0x0000u;
}

float SegmentDist2D(float2 p, float2 a, float2 b)
{
    float2 pa = p - a;
    float2 ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}

float EvaluateAliveCharDist(float2 p, uint ascii)
{
    uint mask = GetAliveSegmentMask(ascii);
    if (mask == 0u) return 10.0;

    float minDist = 10.0;
    [unroll]
    for (int i = 0; i < 16; ++i)
    {
        if (((mask >> uint(i)) & 1u) != 0u)
        {
            float2 a, b;
            GetAliveSegmentEnds(i, a, b);
            float d = SegmentDist2D(p, a, b);
            minDist = min(minDist, d);
        }
    }
    return minDist;
}

uint GetCustomChar(int charIdx)
{
    if (charIdx < 0 || charIdx >= 16) return 0u;
    int vecIdx = charIdx / 4;
    int comp = charIdx % 4;
    if (comp == 0) return CustomText[vecIdx].x;
    if (comp == 1) return CustomText[vecIdx].y;
    if (comp == 2) return CustomText[vecIdx].z;
    return CustomText[vecIdx].w;
}

// Renders the Daft Punk Alive 2007 Hero Typography
float3 RenderDaftLogo(float2 uv, float scale, float2 pos, float3 ledColor, int style)
{
    if (CustomParam5 > 0.5)
    {
        // ---------------------------------------------------------------------
        // A. Authentic Daft Punk Brush Typography Engine (References 1-3)
        // Texture is 1024x256 (aspect ratio 4:1) with precomputed GPU normal-mapped SDF
        // ---------------------------------------------------------------------
        float2 localUV = (uv - pos) / scale;

        // Aspect ratio conversion: text quad spans [-2.0, 2.0] x [-0.5, 0.5]
        float2 texUV = float2(localUV.x * 0.25 + 0.5, 0.5 - localUV.y);

        // Outside bounds check with soft fade margin
        if (texUV.x < 0.0 || texUV.x > 1.0 || texUV.y < 0.0 || texUV.y > 1.0)
        {
            return float3(0.0, 0.0, 0.0);
        }

        float4 logoSample = LogoTexture.Sample(SamplerLinearClamp, texUV);
        float sdf = logoSample.r;       // 0.501 (128/255) is edge, > 0.5 is inside, < 0.5 is outside
        float bevel = logoSample.g;     // Bevel height from edge to crest (0.0 to 1.0)
        float rimMask = logoSample.b;   // Edge rim highlight
        float alpha = logoSample.a;     // Alpha mask

        if (alpha < 0.01 && sdf < 0.35)
        {
            return float3(0.0, 0.0, 0.0);
        }

        // Normal estimation via central differences of SDF
        float2 eps = float2(2.5 / 1024.0, 2.5 / 256.0);
        float dR = LogoTexture.Sample(SamplerLinearClamp, texUV + float2(eps.x, 0.0)).r;
        float dL = LogoTexture.Sample(SamplerLinearClamp, texUV - float2(eps.x, 0.0)).r;
        float dD = LogoTexture.Sample(SamplerLinearClamp, texUV + float2(0.0, eps.y)).r; // down in texture UV (+Y)
        float dU = LogoTexture.Sample(SamplerLinearClamp, texUV - float2(0.0, eps.y)).r; // up in texture UV (-Y)

        // In screen space (Y is up):
        float2 grad = float2(dR - dL, -(dD - dU)) * 24.0;
        float lenGrad = length(grad);
        float3 N = normalize(float3(-grad.x, -grad.y, max(0.18, 1.0 - lenGrad * 0.4)));

        float3 col = float3(0.0, 0.0, 0.0);

        if (style == 0) // 0: Discovery 3D Liquid Chrome (Reference Image 1)
        {
            float3 V = float3(0.0, 0.0, 1.0);
            float3 R = reflect(-V, N);

            // High-contrast liquid chrome horizon reflection
            float chromeHorizon = smoothstep(-0.06, 0.06, R.y);
            float3 skyReflect = lerp(float3(0.12, 0.16, 0.28), float3(0.95, 0.98, 1.0), pow(max(R.y, 0.0), 0.35));
            float3 groundReflect = lerp(float3(0.03, 0.03, 0.05), float3(0.40, 0.40, 0.45), pow(max(-R.y, 0.0), 0.5));
            float3 chromeBody = lerp(groundReflect, skyReflect, chromeHorizon);

            // Dual specular lights (apex laser + stage fill)
            float3 lightDir1 = normalize(float3(0.35, 0.75, 0.5));
            float spec1 = pow(max(dot(R, lightDir1), 0.0), 42.0) * (3.0 + AudioKick * 1.5);
            float3 lightDir2 = normalize(float3(-0.4, 0.35, 0.6));
            float spec2 = pow(max(dot(R, lightDir2), 0.0), 18.0) * 1.2;

            // Authentic Discovery 2001 prismatic rainbow chromatic dispersion along all bevel edges
            float angle = atan2(N.y, N.x);
            float rainbowPhase = frac(angle / 6.28318 + bevel * 0.8 - EnergyAccumulator * 0.15);
            float3 rainbow = HSVtoRGB(float3(rainbowPhase, 0.95, 1.0));

            // Bevel rim caustic intensity
            float causticStrength = rimMask * (2.4 + AudioKick * 1.6 + BeatPulse * 1.0);

            // Solid chrome body with crisp anti-aliased edge
            float edgeAlpha = smoothstep(0.48, 0.52, sdf);
            float3 chromePixel = (chromeBody + spec1 + spec2) * (0.85 + bevel * 0.4) + rainbow * causticStrength;

            // Ambient dark drop shadow and iridescent outer rim glow
            float halo = exp(-max(0.50 - sdf, 0.0) * 18.0) * (1.0 - edgeAlpha);
            float3 haloCol = (rainbow * 0.8 + ColorPrimary.rgb * 0.4) * halo;

            col = chromePixel * edgeAlpha + haloCol;
        }
        else if (style == 1) // 1: Faceted 3D Crystal Mesh (Reference Image 2)
        {
            float edgeAlpha = smoothstep(0.48, 0.52, sdf);

            // Faceted triangular wireframe mesh mapped over brush letterforms
            float2 meshUV = texUV * float2(48.0, 12.0);
            float tri1 = abs(frac(meshUV.x * 0.866 + meshUV.y * 0.5) - 0.5);
            float tri2 = abs(frac(meshUV.x * 0.866 - meshUV.y * 0.5) - 0.5);
            float tri3 = abs(frac(meshUV.y) - 0.5);
            float triWire = 1.0 - smoothstep(0.03, 0.12, min(tri1, min(tri2, tri3)));

            // Facet specular sparkle
            float2 fId = floor(meshUV);
            float facetRnd = Hash21(fId);
            float sparkle = pow(sin(facetRnd * 6.28 + EnergyAccumulator * 4.0) * 0.5 + 0.5, 4.0);

            float3 litMesh = ColorAccent.rgb * (triWire * 2.5 * (1.0 + AudioTreble * 1.5) + sparkle * 2.0);
            float3 crystalBody = lerp(float3(0.01, 0.02, 0.04), ColorPrimary.rgb * 0.4, bevel);
            crystalBody += ColorSecondary.rgb * max(dot(N, normalize(float3(0.0, 1.0, 0.5))), 0.0) * 0.8;

            col = (crystalBody + litMesh) * edgeAlpha;
        }
        else if (style == 2) // 2: Neon Laser Outline
        {
            // Multi-layer laser beam outline centered on SDF zero-crossing
            float distToEdge = abs(sdf - 0.501);
            float razorCore = 1.0 - smoothstep(0.001, 0.016, distToEdge);
            float innerGlow = exp(-distToEdge * 28.0) * 1.8;
            float outerBloom = exp(-distToEdge * 8.0) * 0.8;

            float pulse = 1.0 + AudioKick * 0.8 + BeatPulse * 0.5;
            col = ledColor * (razorCore * 3.5 + innerGlow * 1.8 + outerBloom) * pulse;
        }
        else if (style == 3) // 3: Classic Red Brush Punk (Reference Image 2)
        {
            // Raw authentic Daft Punk Alive 2007 crimson / blood-red brush typography
            float edgeAlpha = smoothstep(0.48, 0.52, sdf);
            float brushTexture = frac(sin(dot(texUV, float2(12.9898, 78.233))) * 43758.5453);

            float3 punkRed = float3(1.0, 0.05, 0.02) * (1.8 + AudioKick * 1.2);
            float3 punkOrange = float3(1.0, 0.48, 0.0) * bevel;
            float3 inkBody = lerp(punkRed, punkOrange, bevel * 0.6) * (0.88 + brushTexture * 0.2);

            // Bleeding neon edge glow
            float edgeBleed = exp(-max(0.50 - sdf, 0.0) * 14.0) * 0.75;
            col = inkBody * edgeAlpha + float3(1.0, 0.15, 0.0) * edgeBleed;
        }
        else if (style == 4) // 4: Digital Cyber Scanlines
        {
            float edgeAlpha = smoothstep(0.485, 0.515, sdf);
            float scanline = sin(texUV.y * 120.0 - EnergyAccumulator * 8.0) * 0.5 + 0.5;
            float sweep = exp(-abs(texUV.y - (sin(Time * 3.0) * 0.4 + 0.5)) * 14.0) * 2.5;

            float3 beamCol = lerp(ColorPrimary.rgb, ColorAccent.rgb, scanline);
            col = beamCol * edgeAlpha * (pow(scanline, 3.0) * 2.2 + sweep + 0.35) * (1.1 + AudioKick * 0.6);
        }
        else // 5: High-Density LED Matrix
        {
            float edgeAlpha = smoothstep(0.49, 0.51, sdf);
            float2 ledUV = frac(texUV * float2(96.0, 24.0)) - 0.5;
            float dBulb = length(ledUV);
            float bulbCore = 1.0 - smoothstep(0.24, 0.38, dBulb);
            float bulbBloom = exp(-dBulb * 3.8) * 0.4;

            float pulse = 1.0 + AudioKick * 0.6 + BeatPulse * 0.4;
            col = ledColor * (bulbCore * 2.8 + bulbBloom * 1.5) * pulse * edgeAlpha;
        }

        return col;
    }

    // -------------------------------------------------------------------------
    // B. Multi-Font Concert Atlas Typography Engine (Custom Text Mode)
    // Supports 4 retro fonts (Neon Sans, Ryga, Raster Forge, Atari Classic)
    // with 5 finishes: Chrome, Crystal Mesh, Neon Outline, Classic Punk, Cyber Scanlines
    // -------------------------------------------------------------------------
    float2 localUV = (uv - pos) / scale;

    // Count characters in CustomText (up to 16)
    int numChars = 0;
    for (int c = 0; c < 16; ++c) {
        if (GetCustomChar(c) != 0u) numChars = c + 1;
    }
    if (numChars == 0) return float3(0.0, 0.0, 0.0);

    float charPitch = 1.05;
    float totalW = float(numChars) * charPitch;

    // Optional Scrolling Banner (Style == 2 when beat pulse is active)
    if (style == 2 && BeatPulse > 0.95)
    {
        float scrollSpeed = 1.6;
        float scrollOffset = frac(Time * 0.12) * (totalW + 6.0) - 3.0;
        localUV.x += scrollOffset;
    }

    // Centered string offset
    float startX = -totalW * 0.5 + charPitch * 0.5;
    float relX = localUV.x - startX;

    int charIdx = int(floor((relX + charPitch * 0.5) / charPitch));
    if (charIdx < 0 || charIdx >= numChars) return float3(0.0, 0.0, 0.0);

    // Local coordinates inside the character box [-0.5, 0.5] x [-0.5, 0.5]
    float2 charUV = float2(relX - float(charIdx) * charPitch, localUV.y);
    if (abs(charUV.x) > 0.5 || abs(charUV.y) > 0.5) return float3(0.0, 0.0, 0.0);

    uint ascii = GetCustomChar(charIdx);
    if (ascii < 32u || ascii > 127u || ascii == 32u) return float3(0.0, 0.0, 0.0);

    // Compute UV coordinates into FontAtlasTexture
    // Atlas layout: 16 columns x 6 rows per font = 96 characters.
    // 4 fonts stacked vertically (CustomParam7 = 0..3):
    // Font 0 = Neon Sans, Font 1 = Ryga, Font 2 = Raster Forge, Font 3 = Atari Classic
    int fontIndex = clamp(int(CustomParam7 + 0.5), 0, 3);
    int charSubIdx = int(ascii - 32u);
    int colIdx = charSubIdx % 16;
    int rowIdx = charSubIdx / 16;

    float cellNormU = (charUV.x + 0.5);
    float cellNormV = (0.5 - charUV.y); // Y is up

    float atlasU = (float(colIdx) + cellNormU) / 16.0;
    float atlasV = (float(fontIndex * 6 + rowIdx) + cellNormV) / 24.0;

    float4 atlasSample = FontAtlasTexture.Sample(SamplerLinearClamp, float2(atlasU, atlasV));
    float sdf = atlasSample.r;       // 0.501 (128/255) is edge, > 0.5 is inside, < 0.5 is outside
    float bevel = atlasSample.g;     // Bevel height from edge to crest (0.0 to 1.0)
    float rimMask = atlasSample.b;   // Edge rim highlight
    float alpha = atlasSample.a;     // Alpha mask

    if (alpha < 0.01 && sdf < 0.35)
    {
        return float3(0.0, 0.0, 0.0);
    }

    // Normal estimation via central differences of font atlas SDF
    float2 eps = float2(1.5 / 1024.0, 1.5 / 1536.0);
    float dR = FontAtlasTexture.Sample(SamplerLinearClamp, float2(atlasU + eps.x, atlasV)).r;
    float dL = FontAtlasTexture.Sample(SamplerLinearClamp, float2(atlasU - eps.x, atlasV)).r;
    float dD = FontAtlasTexture.Sample(SamplerLinearClamp, float2(atlasU, atlasV + eps.y)).r;
    float dU = FontAtlasTexture.Sample(SamplerLinearClamp, float2(atlasU, atlasV - eps.y)).r;

    float2 grad = float2(dR - dL, -(dD - dU)) * 24.0;
    float lenGrad = length(grad);
    float3 N = normalize(float3(-grad.x, -grad.y, max(0.18, 1.0 - lenGrad * 0.4)));

    float3 col = float3(0.0, 0.0, 0.0);

    if (style == 0) // 0: Discovery 3D Liquid Chrome
    {
        float3 V = float3(0.0, 0.0, 1.0);
        float3 R = reflect(-V, N);

        float chromeHorizon = smoothstep(-0.06, 0.06, R.y);
        float3 skyReflect = lerp(float3(0.12, 0.16, 0.28), float3(0.95, 0.98, 1.0), pow(max(R.y, 0.0), 0.35));
        float3 groundReflect = lerp(float3(0.03, 0.03, 0.05), float3(0.40, 0.40, 0.45), pow(max(-R.y, 0.0), 0.5));
        float3 chromeBody = lerp(groundReflect, skyReflect, chromeHorizon);

        float3 lightDir1 = normalize(float3(0.35, 0.75, 0.5));
        float spec1 = pow(max(dot(R, lightDir1), 0.0), 42.0) * (3.0 + AudioKick * 1.5);
        float3 lightDir2 = normalize(float3(-0.4, 0.35, 0.6));
        float spec2 = pow(max(dot(R, lightDir2), 0.0), 18.0) * 1.2;

        float angle = atan2(N.y, N.x);
        float rainbowPhase = frac(angle / 6.28318 + bevel * 0.8 - EnergyAccumulator * 0.15);
        float3 rainbow = HSVtoRGB(float3(rainbowPhase, 0.95, 1.0));

        float causticStrength = rimMask * (2.4 + AudioKick * 1.6 + BeatPulse * 1.0);
        float edgeAlpha = smoothstep(0.48, 0.52, sdf);
        float3 chromePixel = (chromeBody + spec1 + spec2) * (0.85 + bevel * 0.4) + rainbow * causticStrength;

        float halo = exp(-max(0.50 - sdf, 0.0) * 18.0) * (1.0 - edgeAlpha);
        float3 haloCol = (rainbow * 0.8 + ColorPrimary.rgb * 0.4) * halo;

        col = chromePixel * edgeAlpha + haloCol;
    }
    else if (style == 1) // 1: Faceted 3D Crystal Mesh
    {
        float edgeAlpha = smoothstep(0.48, 0.52, sdf);

        float2 meshUV = charUV * 16.0;
        float tri1 = abs(frac(meshUV.x * 0.866 + meshUV.y * 0.5) - 0.5);
        float tri2 = abs(frac(meshUV.x * 0.866 - meshUV.y * 0.5) - 0.5);
        float tri3 = abs(frac(meshUV.y) - 0.5);
        float triWire = 1.0 - smoothstep(0.03, 0.12, min(tri1, min(tri2, tri3)));

        float2 fId = floor(meshUV);
        float facetRnd = Hash21(fId + float2(float(charIdx) * 7.1, 0.0));
        float sparkle = pow(sin(facetRnd * 6.28 + EnergyAccumulator * 4.0) * 0.5 + 0.5, 4.0);

        float3 litMesh = ColorAccent.rgb * (triWire * 2.5 * (1.0 + AudioTreble * 1.5) + sparkle * 2.0);
        float3 crystalBody = lerp(float3(0.01, 0.02, 0.04), ColorPrimary.rgb * 0.4, bevel);
        crystalBody += ColorSecondary.rgb * max(dot(N, normalize(float3(0.0, 1.0, 0.5))), 0.0) * 0.8;

        col = (crystalBody + litMesh) * edgeAlpha;
    }
    else if (style == 2) // 2: Neon Laser Outline
    {
        float distToEdge = abs(sdf - 0.501);
        float razorCore = 1.0 - smoothstep(0.001, 0.018, distToEdge);
        float innerGlow = exp(-distToEdge * 28.0) * 1.8;
        float outerBloom = exp(-distToEdge * 8.0) * 0.8;

        float pulse = 1.0 + AudioKick * 0.8 + BeatPulse * 0.5;
        col = ledColor * (razorCore * 3.5 + innerGlow * 1.8 + outerBloom) * pulse;
    }
    else if (style == 3) // 3: Classic Red Brush / Punk Solid
    {
        float edgeAlpha = smoothstep(0.48, 0.52, sdf);
        float3 punkRed = float3(1.0, 0.05, 0.02) * (1.8 + AudioKick * 1.2);
        float3 punkOrange = float3(1.0, 0.48, 0.0) * bevel;
        float3 inkBody = lerp(punkRed, punkOrange, bevel * 0.6) * 0.95;

        float edgeBleed = exp(-max(0.50 - sdf, 0.0) * 14.0) * 0.75;
        col = inkBody * edgeAlpha + float3(1.0, 0.15, 0.0) * edgeBleed;
    }
    else if (style == 4) // 4: Digital Cyber Scanlines
    {
        float edgeAlpha = smoothstep(0.485, 0.515, sdf);
        float scanline = sin(charUV.y * 60.0 - EnergyAccumulator * 8.0) * 0.5 + 0.5;
        float sweep = exp(-abs(charUV.y - (sin(Time * 3.0) * 0.4 + 0.5)) * 14.0) * 2.5;

        float3 beamCol = lerp(ColorPrimary.rgb, ColorAccent.rgb, scanline);
        col = beamCol * edgeAlpha * (pow(scanline, 3.0) * 2.2 + sweep + 0.35) * (1.1 + AudioKick * 0.6);
    }
    else // 5: High-Density LED Matrix
    {
        float edgeAlpha = smoothstep(0.49, 0.51, sdf);
        float2 ledUV = frac(charUV * float2(24.0, 24.0)) - 0.5;
        float dBulb = length(ledUV);
        float bulbCore = 1.0 - smoothstep(0.24, 0.38, dBulb);
        float bulbBloom = exp(-dBulb * 3.8) * 0.4;

        float pulse = 1.0 + AudioKick * 0.6 + BeatPulse * 0.4;
        col = ledColor * (bulbCore * 2.8 + bulbBloom * 1.5) * pulse * edgeAlpha;
    }

    return col;
}

// -----------------------------------------------------------------------------
// 2. Layer 1: Sky Dome & 3-Way Celestial Centerpiece
// Modes (CustomParam1):
// 0: Black Hole Singularity Eclipse (Inspiration Image 1)
// 1: Glowing Neon Portal Ring (Inspiration Image 2)
// 2: Digital Wireframe Cyber Moon
// -----------------------------------------------------------------------------
float3 RenderSkyDome(float2 uv, float3 rd)
{
    float3 col = float3(0.001, 0.0015, 0.0035);

    // Deep space stars
    float2 starGrid = uv * 32.0;
    float2 id = floor(starGrid);
    float2 gUV = frac(starGrid) - 0.5;
    float rnd = Hash21(id);
    if (rnd > 0.65)
    {
        float2 pos = (Hash22(id) - 0.5) * 0.7;
        float dStar = length(gUV - pos);
        float star = pow(saturate(1.0 - dStar / 0.06), 18.0);
        float twinkle = 0.5 + 0.5 * sin(Time * 3.0 + rnd * 20.0);
        col += float3(0.9, 0.95, 1.0) * star * twinkle * (0.8 + AudioTreble * 0.8);
    }

    int celestialMode = int(CustomParam1);

    // Centered Celestial Feature: Anchored directly on the horizon line (0.0, 0.15)
    float2 centerPos = float2(0.0, 0.15);
    float2 dVec = uv - centerPos;
    float dCenter = length(dVec);

    if (celestialMode == 0) // 0: Black Hole Singularity Eclipse (Inspiration Image 1)
    {
        float rEvent = 0.17;
        if (dCenter < rEvent)
        {
            // Pitch black singularity core
            col = float3(0.0, 0.0, 0.0);
        }
        else
        {
            // Accretion disk & intense gravitational lensing corona
            float distCorona = dCenter - rEvent;
            float photonRing = 1.0 - smoothstep(0.0, 0.016, distCorona);
            float innerGlow = exp(-distCorona * 14.0) * 3.5;
            float outerHalo = exp(-distCorona * 3.2) * 1.8;

            float3 deepRed = float3(1.0, 0.08, 0.05);
            float3 hotMagenta = float3(1.0, 0.15, 0.45);
            float3 photonRingCol = float3(1.0, 0.88, 0.45);

            float3 coronaCol = lerp(deepRed, hotMagenta, exp(-distCorona * 6.0));
            col += (coronaCol * (innerGlow + outerHalo) + photonRingCol * photonRing * 3.2) * (1.0 + AudioKick * 0.4 + BeatPulse * 0.3);
        }
    }
    else if (celestialMode == 1) // 1: Glowing Neon Portal Ring (Inspiration Image 2)
    {
        float portalR = 0.48;
        float distRing = abs(dCenter - portalR);
        float ringCore = 1.0 - smoothstep(0.004, 0.022, distRing);
        float ringBloom = exp(-distRing * 12.0) * 2.2;
        float ringAura = exp(-distRing * 3.8) * 0.65;

        // Angle for two-tone Magenta-to-Cyan concert neon gradient (matching Image 2)
        float angle = atan2(dVec.y, dVec.x);
        float grad = sin(angle) * 0.5 + 0.5;
        float3 portalCol = lerp(float3(1.0, 0.08, 0.65), float3(0.05, 0.85, 1.0), grad);

        col += portalCol * (ringCore * 3.8 + ringBloom + ringAura) * (1.1 + AudioKick * 0.5 + BeatPulse * 0.4);

        // Subtle starry gateway inside portal
        if (dCenter < portalR)
        {
            float gatewayGlow = exp(-(portalR - dCenter) * 2.5) * 0.25;
            col += portalCol * gatewayGlow;
        }
    }
    else // 2: Digital Wireframe Cyber Moon
    {
        float moonR = 0.22;
        if (dCenter < moonR)
        {
            float2 sphereUV = dVec / moonR;
            float z = sqrt(max(0.0, 1.0 - dot(sphereUV, sphereUV)));

            // Latitude / Longitude digital wireframe rings
            float lat = abs(frac(sphereUV.y * 8.0) - 0.5);
            float lon = abs(frac((atan2(sphereUV.y, sphereUV.x) / 6.28318) * 16.0) - 0.5);
            float cyberGrid = 1.0 - smoothstep(0.06, 0.16, min(lat, lon));

            // Concentric radar telemetry circles
            float radar = abs(frac(dCenter * 14.0 - Time * 0.8) - 0.5);
            float radarRing = 1.0 - smoothstep(0.05, 0.12, radar);

            float3 moonBase = float3(0.02, 0.04, 0.08) * z;
            float3 cyberCol = lerp(ColorSecondary.rgb, ColorAccent.rgb, cyberGrid) * (cyberGrid * 2.2 + radarRing * 1.2);
            col += (moonBase + cyberCol) * (1.0 + AudioKick * 0.4);
        }

        float moonAura = exp(-abs(dCenter - moonR) * 6.0) * 0.8;
        col += ColorSecondary.rgb * moonAura;
    }

    return col;
}

// -----------------------------------------------------------------------------
// 3. Layer 2: TRON Grid Floor & Mountain Wave Terrain (Inspiration Image 1)
// -----------------------------------------------------------------------------
float GetMountainWaveHeight(float3 p)
{
    float waveAmp = CustomParam0; // 0.0 = flat plane, >0.0 = undulating mountain waves
    if (waveAmp <= 0.001) return 0.0;

    // Moving towards camera (+Z to -Z direction, in sync with TRON grid)
    float waveZ = p.z * 0.38 + EnergyAccumulator * 1.5 + Time * 0.8;

    // Primary undulating wave + cross ripples
    float w1 = sin(waveZ) * 0.42;
    float w2 = sin(waveZ * 2.1 + p.x * 0.75) * 0.22;
    float w3 = cos(p.x * 0.6) * 0.28;

    // Mountain swell higher on left/right flanks
    float flankSwell = 0.6 + smoothstep(0.8, 3.2, abs(p.x)) * 1.8;

    float height = (w1 + w2 + w3) * flankSwell * waveAmp * (1.0 + AudioSubBass * 0.6);
    return height;
}

float3 RenderTronFloor(float3 ro, float3 rd, out float outFloorT)
{
    outFloorT = 1000.0;
    if (rd.y >= -0.001) return float3(0.0, 0.0, 0.0);

    float baseY = -1.35;
    float tBase = (baseY - ro.y) / rd.y;
    if (tBase <= 0.0 || tBase > 50.0) return float3(0.0, 0.0, 0.0);

    float tHit = tBase;
    float waveAmp = CustomParam0;

    // Raymarch displaced heightfield when wave mountain terrain is active
    if (waveAmp > 0.01)
    {
        float tStart = (baseY + waveAmp * 1.8 - ro.y) / rd.y;
        tStart = max(tStart, 0.5);
        float tEnd = min(tBase + 4.0, 45.0);
        float tStep = (tEnd - tStart) / 16.0;

        float tCur = tStart;
        for (int i = 0; i < 16; ++i)
        {
            float3 pTest = ro + rd * tCur;
            float h = GetMountainWaveHeight(pTest);
            float dY = pTest.y - (baseY + h);
            if (dY <= 0.0)
            {
                tHit = tCur;
                break;
            }
            tCur += tStep;
        }
    }

    outFloorT = tHit;
    float3 pFloor = ro + rd * tHit;
    float hAtHit = GetMountainWaveHeight(pFloor);

    float2 worldXZ = pFloor.xz + float2(0.0, EnergyAccumulator * 1.8 + Time * 0.8);

    // 1. Perspective longitudinal grid lines
    float2 gridCoord = worldXZ * 0.55;
    float2 gridDist = abs(frac(gridCoord - 0.5) - 0.5) / max(fwidth(gridCoord), 0.0001);
    float gridLine = 1.0 - min(min(gridDist.x, gridDist.y), 1.0);

    // 2. Diagonal wireframe mesh lines (polygonal terrain wireframe as in Image 1)
    float diagCoord = (pFloor.x + worldXZ.y) * 0.55;
    float diagDist = abs(frac(diagCoord - 0.5) - 0.5) / max(fwidth(diagCoord), 0.0001);
    float diagLine = 1.0 - min(diagDist, 1.0);

    float wire = max(gridLine, diagLine * 0.75);

    // Wave crest illumination: peaks glow in vibrant crimson / neon red
    float crestIntensity = smoothstep(0.1, 0.8, hAtHit / max(waveAmp * 1.5, 0.01));

    float3 gridColor = lerp(ColorSecondary.rgb, ColorPrimary.rgb, sin(worldXZ.y * 0.1) * 0.5 + 0.5);
    if (waveAmp > 0.01)
    {
        float3 crestColor = lerp(ColorPrimary.rgb * 1.5, ColorAccent.rgb * 2.0, crestIntensity);
        gridColor = lerp(gridColor, crestColor, crestIntensity);
    }

    float3 floorBase = float3(0.001, 0.0015, 0.003);
    float3 col = floorBase + gridColor * wire * (0.8 + AudioKick * 0.6 + crestIntensity * 1.5);

    // TRON Light-Cycle Trails (TrailsWeight > 0.01)
    if (TrailsWeight > 0.01)
    {
        for (int tr = 0; tr < 4; ++tr)
        {
            float laneX = (tr == 0) ? -2.5 : ((tr == 1) ? -0.83 : ((tr == 2) ? 0.83 : 2.5));
            float distToLane = abs(pFloor.x - laneX);
            float trailW = 0.045;
            float laneMask = 1.0 - smoothstep(0.0, trailW, distToLane);

            float speed = (tr == 0) ? 4.5 : ((tr == 1) ? 3.8 : ((tr == 2) ? 5.2 : 4.1));
            // Reversed direction: shooting towards horizon
            float trailZ = frac((pFloor.z - Time * speed + float(tr) * 12.3) * 0.06);
            float head = smoothstep(0.0, 0.05, trailZ) * (1.0 - smoothstep(0.05, 0.45, trailZ));

            float3 trailCol = ((tr & 1) == 0) ? ColorSecondary.rgb : ColorPrimary.rgb;
            if (tr == 2) trailCol = ColorAccent.rgb;

            col += trailCol * laneMask * head * 3.5 * TrailsWeight * (1.0 + AudioKick * 0.5);
        }
    }

    float fog = exp(-tHit * 0.04);
    return col * fog;
}

// -----------------------------------------------------------------------------
// 4. Layer 3: 4-Sided Alive 2007 Stage Pyramid
// -----------------------------------------------------------------------------
float EvaluateAlivePyramid(float3 p, out float outEdgeGlow, out float2 outFaceUV)
{
    outEdgeGlow = 0.0;
    outFaceUV = float2(0.5, 0.5);

    // Pyramid dimensions matching PVC physical stage:
    // Apex at (0, 0.9, 0), Base at y = -1.35, half-width = 1.6
    float apexY = 0.9;
    float baseY = -1.35;
    float pyrHeight = apexY - baseY;
    float pyrHalfW = 1.6;

    // Shift coordinate system so base is at y = 0
    float3 q = p - float3(0.0, baseY, 0.0);

    // Outside vertical bounds
    if (q.y < 0.0 || q.y > pyrHeight)
    {
        float dBelow = -q.y;
        float dAbove = q.y - pyrHeight;
        return max(dBelow, dAbove);
    }

    // Normalized height: 0 at base, 1 at apex
    float hNorm = q.y / pyrHeight;
    float curW = (1.0 - hNorm) * pyrHalfW;

    // Planar 4-sided distance to square cross-section
    float dx = abs(q.x) - curW;
    float dz = abs(q.z) - curW;
    float dHoriz = max(dx, dz);

    // Distance to face planes (scaled by slope normal)
    float slope = pyrHeight / sqrt(pyrHeight * pyrHeight + pyrHalfW * pyrHalfW);
    float dFace = dHoriz * slope;

    // Face UV mapping for patterns
    float uFace = 0.5;
    if (abs(q.z) > abs(q.x)) {
        uFace = (q.x / max(curW, 0.001)) * 0.5 + 0.5;
    } else {
        uFace = (q.z / max(curW, 0.001)) * 0.5 + 0.5;
    }
    outFaceUV = float2(clamp(uFace, 0.0, 1.0), hNorm);

    // Razor-sharp structural strut edges: ONLY evaluated right on the pyramid surface
    if (abs(dFace) < 0.035)
    {
        // Distance to 4 corner slant struts (uFace = 0 or 1)
        float distCorner = min(uFace, 1.0 - uFace) * curW;
        // Distance to bottom perimeter frame
        float distBase = hNorm * pyrHeight;
        float edgeDist = min(distCorner, distBase);

        outEdgeGlow = 1.0 - smoothstep(0.004, 0.022 * WireframeDensity, edgeDist);
    }

    return dFace;
}

float3 EvaluatePyramidPattern(float2 faceUV)
{
    float u = faceUV.x;
    float h = faceUV.y; // 0 = base, 1 = apex

    float3 col = float3(0.0, 0.0, 0.0);

    if (PyramidPattern == 0) // 0: Horizontal VU Strips
    {
        float fftVal = GetAudioFFT(clamp(h * 0.75 + 0.05, 0.02, 0.95));
        float numStrips = 24.0;
        float stripFrac = frac(h * numStrips);
        float stripMask = step(stripFrac, 0.72) * step(h, fftVal * 1.35 + 0.08);

        float3 stripCol = lerp(ColorPrimary.rgb, ColorAccent.rgb, h);
        col = stripCol * stripMask * (1.2 + AudioKick * 1.5 + BeatPulse * 0.8);
    }
    else if (PyramidPattern == 1) // 1: Rising Chevrons
    {
        float chev = frac((h - abs(u - 0.5) * 0.9) * 6.0 - EnergyAccumulator * 2.5);
        float chevMask = smoothstep(0.65, 0.92, chev);
        col = lerp(ColorPrimary.rgb, ColorAccent.rgb, chevMask) * chevMask * (1.4 + AudioSubBass * 1.8);
    }
    else if (PyramidPattern == 2) // 2: Scanner Sweep
    {
        float sweepPos = sin(Time * 2.8) * 0.45 + 0.5;
        float distToSweep = abs(h - sweepPos);
        float sweepGlow = exp(-distToSweep * 22.0);
        col = ColorAccent.rgb * sweepGlow * (1.6 + AudioKick * 1.2);
    }
    else if (PyramidPattern == 3) // 3: Solid Neon Glow
    {
        float pulse = 0.4 + BeatPulse * 0.8 + AudioSubBass * 0.6;
        col = ColorPrimary.rgb * pulse * (1.0 - h * 0.4);
    }
    else // 4: Rainbow Spectrum Cycle
    {
        float3 rainbow = HSVtoRGB(float3(h * 0.8 - EnergyAccumulator * 0.25, 0.85, 1.0));
        col = rainbow * (1.0 + AudioKick * 0.8);
    }

    return col;
}

// -----------------------------------------------------------------------------
// 5. Layer 4: Volumetric Apex Lasers (Strictly Bounded & Phase-Smooth)
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
    // Guard against near-parallel ray singularities that cause stretched lines near apex/top
    if (denom < 0.0005) return 0.0;

    float sc = (b * e - c * d) / denom;
    float tc = (a * e - b * d) / denom;

    // Strict bounds: laser length max 12.0 units, must be in front of camera (sc > 0.1) and before surfaces (sc < maxDepth)
    float maxLaserLen = 12.0;
    if (sc < 0.1 || tc < 0.05 || tc > maxLaserLen || sc > maxDepth) return 0.0;

    float3 dP = w0 + sc * rd - tc * laserDir;
    float dist = length(dP);

    float core = 1.0 - smoothstep(beamWidth * 0.2, beamWidth, dist);
    // Smooth fade near laser tip and smooth fade near origin
    float tipFade = 1.0 - smoothstep(maxLaserLen * 0.75, maxLaserLen, tc);
    float originFade = smoothstep(0.05, 0.25, tc);

    return core * tipFade * originFade;
}

float3 RenderApexLasers(float3 ro, float3 rd, float maxDepth)
{
    if (LaserIntensity <= 0.01) return float3(0.0, 0.0, 0.0);

    float3 apexPos = float3(0.0, 0.9, 0.0);
    float3 col = float3(0.0, 0.0, 0.0);
    int numLasers = 8;

    // Use continuous EnergyAccumulator for smooth musical sweep (NO instantaneous beat-snapping phase jumps)
    float sweepBase = sin(EnergyAccumulator * 0.55) * 0.85;

    for (int i = 0; i < numLasers; ++i)
    {
        float normIdx = (float(i) / float(numLasers - 1)) * 2.0 - 1.0;
        float fanAngle = normIdx * 1.1 + sweepBase * 0.4;
        float pitch = -0.25 + cos(EnergyAccumulator * 0.75 + normIdx * 1.2) * 0.15;

        float3 laserDir = normalize(float3(sin(fanAngle), pitch, -cos(fanAngle)));

        float beamWidth = 0.012 * (1.0 + AudioKick * 0.5);
        float intensity = LaserBeamSegment(ro, rd, apexPos, laserDir, beamWidth, maxDepth);

        float3 laserCol = ((uint(i) & 1u) == 0u) ? ColorPrimary.rgb : ColorSecondary.rgb;
        if (i == 3 || i == 4) laserCol = ColorAccent.rgb;

        col += laserCol * intensity * LaserIntensity * (1.0 + AudioHighMid * 0.8);
    }

    return col;
}

// -----------------------------------------------------------------------------
// 6. Layer 5: Particles (Flapping Bats or Stage Sparks)
// -----------------------------------------------------------------------------
// 6. Layer 5: Particles (Electric Stage Sparks)
// -----------------------------------------------------------------------------
float3 RenderParticles(float2 uv, float3 ro, float3 uu, float3 vv, float3 ww, float hitT)
{
    if (ParticleIntensity <= 0.01) return float3(0.0, 0.0, 0.0);

    float3 col = float3(0.0, 0.0, 0.0);

    // Electric stage embers/sparks rising from floor
    int numSparks = 20;
    for (int s = 0; s < numSparks; ++s)
    {
        float seed = Hash11(float(s) * 23.17);
        float speed = 0.5 + seed * 0.7;
        float yPos = -1.35 + frac(Time * 0.35 * speed + seed) * 2.6;
        float xPos = sin(Time * 0.5 + seed * 6.28) * 2.4;
        float zPos = -1.0 + cos(Time * 0.4 + seed * 3.14) * 2.0;

        float3 pSpark = float3(xPos, yPos, zPos);
        float3 toSpark = pSpark - ro;
        float zDepth = dot(toSpark, ww);

        if (zDepth > 0.3 && zDepth < hitT)
        {
            float2 projUV = float2(dot(toSpark, uu), dot(toSpark, vv)) * (1.8 / zDepth);
            float d = length(uv - projUV);
            float sparkR = 0.012 / max(zDepth * 0.5, 0.4);
            float spark = 1.0 - smoothstep(0.0, sparkR, d);

            float3 sCol = ((s & 1) == 0) ? ColorAccent.rgb : ColorPrimary.rgb;
            col += sCol * spark * ParticleIntensity * (1.0 + BeatPulse * 1.2);
        }
    }

    return col;
}

// -----------------------------------------------------------------------------
// 8. Full LED Wall Backdrop (LedWallWeight > 0.01)
// -----------------------------------------------------------------------------
float3 RenderLedWall(float2 uv)
{
    if (LedWallWeight <= 0.01) return float3(0.0, 0.0, 0.0);

    float2 ledGrid = frac((uv + float2(0.0, 0.2)) * 28.0);
    float dLed = length(ledGrid - 0.5);
    float ledMask = 1.0 - smoothstep(0.28, 0.38, dLed);

    float fftSample = GetAudioFFT(clamp(abs(uv.x) * 0.35, 0.01, 0.98));
    float barY = uv.y * 0.5 + 0.5;
    float lit = step(barY, fftSample * 1.25);

    float3 ledCol = lerp(ColorSecondary.rgb, ColorAccent.rgb, barY) * lit * ledMask;
    return ledCol * LedWallWeight * (0.8 + AudioKick * 0.6);
}

// -----------------------------------------------------------------------------
// Master Pixel Shader Entry Point
// -----------------------------------------------------------------------------
float4 PSMain(VSOutput input) : SV_Target
{
    float2 uv = input.UV * 2.0 - 1.0;
    uv.x *= AspectRatio;

    // Fixed 3D camera: Apex anchored top-center, horizon at 40% height
    float3 ro = float3(0.0, 0.45, -3.8);
    float3 ta = float3(0.0, -0.08, 0.0);

    float3 ww = normalize(ta - ro);
    float3 uu = normalize(cross(ww, float3(0.0, 1.0, 0.0)));
    float3 vv = cross(uu, ww);
    float3 rd = normalize(uv.x * uu + uv.y * vv + 1.85 * ww);

    float3 col = float3(0.0, 0.0, 0.0);

    // 1. Sky Dome & Halloween Moon
    if (SkyLayerWeight > 0.01)
    {
        col += RenderSkyDome(uv, rd) * SkyLayerWeight;
    }

    // 2. Full LED Wall Backdrop (when enabled)
    if (LedWallWeight > 0.01)
    {
        col += RenderLedWall(uv);
    }

    // 3. TRON Grid Floor
    float floorT = 1000.0;
    if (FloorLayerWeight > 0.01)
    {
        float3 floorCol = RenderTronFloor(ro, rd, floorT);
        if (floorT < 100.0)
        {
            // Floor is solid obsidian ground that culls whatever part of the sky/sun is behind it
            float floorAlpha = saturate(FloorLayerWeight);
            col = lerp(col, floorCol, floorAlpha);
        }
    }

    // 4. Alive 2007 Stage Pyramid
    float hitPyramidT = 1000.0;
    if (PyramidWeight > 0.01)
    {
        float t = 0.0;
        bool hit = false;
        float edgeGlow = 0.0;
        float2 faceUV = float2(0.5, 0.5);

        [loop]
        for (int i = 0; i < 50; ++i)
        {
            float3 p = ro + rd * t;
            float gTemp;
            float2 uvTemp;
            float d = EvaluateAlivePyramid(p, gTemp, uvTemp);

            edgeGlow = max(edgeGlow, gTemp);

            if (d < 0.003)
            {
                hit = true;
                hitPyramidT = t;
                faceUV = uvTemp;
                break;
            }

            t += d * 0.85;
            if (t > 12.0 || t > floorT) break;
        }

        // Pyramid neon LED struts and reactive face patterns (strictly on surface hit)
        if (hit && hitPyramidT < floorT)
        {
            float fog = exp(-hitPyramidT * 0.04);

            // Clean razor-sharp neon strut frame
            float3 edgeCol = ColorAccent.rgb * edgeGlow * (2.2 + AudioKick * 1.5 + BeatPulse * 1.0);

            // Pyramid face reactive pattern
            float3 faceCol = EvaluatePyramidPattern(faceUV);

            col = lerp(col, col + edgeCol + faceCol, fog * PyramidWeight);

            // Inner glowing sub-bass octahedron core (enabled when CustomParam3 > 0.5)
            if (CustomParam3 > 0.5)
            {
                float3 pCore = float3(0.0, -0.2, 0.0);
                float dCore = SDFOctahedron(ro + rd * hitPyramidT - pCore, 0.35 + AudioSubBass * 0.4);
                float coreGlow = 1.0 / (1.0 + abs(dCore) * 32.0);
                col += ColorPrimary.rgb * coreGlow * (1.2 + AudioSubBass * 2.0) * PyramidWeight;
            }
        }
    }

    // Minimum depth for laser/particle occlusion
    float minHitT = min(floorT, hitPyramidT);

    // 5. Volumetric Apex Lasers
    if (LaserIntensity > 0.01)
    {
        col += RenderApexLasers(ro, rd, minHitT);
    }

    // 6. Particles (Electric Sparks)
    if (ParticleIntensity > 0.01)
    {
        col += RenderParticles(uv, ro, uu, vv, ww, minHitT);
    }

    // 7. Daft Punk Style Hero Typography (2D Fallback only when not in 3D Mesh Mode)
    // Foremost depth: blends cleanly over grid lines and lasers
    if (LogoWeight > 0.01 && CustomParam5 <= 0.5)
    {
        float baseScale = 0.06875;
        float sizeT = saturate((CustomParam2 - 0.5) / 3.0);
        float logoScale = lerp(1.5, 3.5, sizeT) * baseScale;
        float2 logoPos = float2(0.0, CustomParam4);

        float3 logoCol = RenderDaftLogo(uv, logoScale, logoPos, ColorAccent.rgb, LogoStyle);
        float logoIntensity = max(max(logoCol.r, logoCol.g), logoCol.b);
        float logoAlpha = smoothstep(0.02, 0.25, logoIntensity);
        col = lerp(col, col + logoCol, logoAlpha * saturate(LogoWeight));
    }

    // 8. Kid-Safe Photosensitivity Limiter
    // Cap overall luminance by MasterBrightness (default 0.85)
    col *= MasterBrightness;

    // 9. Vignette & Drop Flash Border (Over everything)
    if (SpookyWeight > 0.01)
    {
        float edgeDist = length(input.UV - 0.5) * 2.0;
        float darkVignette = clamp(1.0 - smoothstep(0.7, 1.4, edgeDist) * 0.45 * SpookyWeight, 0.0, 1.0);
        col *= darkVignette;

        if (DropIntensity > 0.1 || BeatPulse > 0.7)
        {
            float dropBorder = smoothstep(0.7, 1.4, edgeDist) * (DropIntensity * 0.3 + BeatPulse * 0.2);
            col += ColorAccent.rgb * dropBorder * 0.35 * SafetyDamping * SpookyWeight;
        }
    }

    return float4(saturate(col), 1.0);
}
