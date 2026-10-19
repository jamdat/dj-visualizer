#include "Common.hlsl"

// =============================================================================
// ModularMixer.hlsl - Dad Punk Alive 2007 Stage Engine
// 4-Sided LED Pyramid, TRON Grid Floor, Daft LED Dot-Matrix Logo,
// Volumetric Apex Lasers, Halloween Spooky Moon/Bats/Pumpkins, Kid-Safe Limiter
// =============================================================================

// -----------------------------------------------------------------------------
// 1. Procedural 5x7 Dot-Matrix Font Renderer (Daft Punk LED Style)
// -----------------------------------------------------------------------------
uint GetRowBits(uint ch, int row)
{
    if (row < 0 || row > 6) return 0u;

    // Convert lowercase to uppercase
    if (ch >= 97u && ch <= 122u) ch -= 32u;

    if (ch == 32u) return 0u; // Space

    // Letters A-Z
    if (ch == 65u) { // A
        if (row == 0) return 14u;
        if (row == 3) return 31u;
        return 17u;
    }
    if (ch == 66u) { // B
        if (row == 0 || row == 3 || row == 6) return 30u;
        return 17u;
    }
    if (ch == 67u) { // C
        if (row == 0 || row == 6) return 14u;
        return 16u;
    }
    if (ch == 68u) { // D
        if (row == 0 || row == 6) return 30u;
        return 17u;
    }
    if (ch == 69u) { // E
        if (row == 0 || row == 6) return 31u;
        if (row == 3) return 30u;
        return 16u;
    }
    if (ch == 70u) { // F
        if (row == 0) return 31u;
        if (row == 3) return 30u;
        return 16u;
    }
    if (ch == 71u) { // G
        if (row == 0 || row == 6) return 14u;
        if (row == 3) return 19u;
        if (row >= 4) return 17u;
        return 16u;
    }
    if (ch == 72u) { // H
        if (row == 3) return 31u;
        return 17u;
    }
    if (ch == 73u) { // I
        if (row == 0 || row == 6) return 31u;
        return 4u;
    }
    if (ch == 74u) { // J
        if (row == 0) return 31u;
        if (row == 6) return 14u;
        if (row >= 4) return 17u;
        return 1u;
    }
    if (ch == 75u) { // K
        if (row == 0 || row == 6) return 17u;
        if (row == 1 || row == 5) return 18u;
        if (row == 2 || row == 4) return 20u;
        return 24u;
    }
    if (ch == 76u) { // L
        if (row == 6) return 31u;
        return 16u;
    }
    if (ch == 77u) { // M
        if (row == 0) return 17u;
        if (row == 1) return 27u;
        if (row == 2) return 21u;
        return 17u;
    }
    if (ch == 78u) { // N
        if (row == 1) return 19u;
        if (row == 2) return 21u;
        if (row == 3) return 25u;
        return 17u;
    }
    if (ch == 79u) { // O
        if (row == 0 || row == 6) return 14u;
        return 17u;
    }
    if (ch == 80u) { // P
        if (row == 0 || row == 3) return 30u;
        if (row == 1 || row == 2) return 17u;
        return 16u;
    }
    if (ch == 81u) { // Q
        if (row == 0) return 14u;
        if (row == 5) return 19u;
        if (row == 6) return 14u;
        return 17u;
    }
    if (ch == 82u) { // R
        if (row == 0 || row == 3) return 30u;
        if (row == 1 || row == 2) return 17u;
        if (row == 4) return 20u;
        if (row == 5) return 18u;
        return 17u;
    }
    if (ch == 83u) { // S
        if (row == 0 || row == 3 || row == 6) return 14u;
        if (row < 3) return 16u;
        return 1u;
    }
    if (ch == 84u) { // T
        if (row == 0) return 31u;
        return 4u;
    }
    if (ch == 85u) { // U
        if (row == 6) return 14u;
        return 17u;
    }
    if (ch == 86u) { // V
        if (row < 4) return 17u;
        if (row == 4) return 17u;
        if (row == 5) return 10u;
        return 4u;
    }
    if (ch == 87u) { // W
        if (row == 4) return 21u;
        if (row == 5) return 27u;
        return 17u;
    }
    if (ch == 88u) { // X
        if (row == 0 || row == 6) return 17u;
        if (row == 1 || row == 5) return 10u;
        if (row == 2 || row == 4) return 4u;
        return 4u;
    }
    if (ch == 89u) { // Y
        if (row == 0) return 17u;
        if (row == 1) return 10u;
        if (row == 2) return 4u;
        return 4u;
    }
    if (ch == 90u) { // Z
        if (row == 0 || row == 6) return 31u;
        if (row == 1) return 1u;
        if (row == 2) return 2u;
        if (row == 3) return 4u;
        if (row == 4) return 8u;
        return 16u;
    }

    // Digits 0-9
    if (ch == 48u) { if (row == 0 || row == 6) return 14u; if (row == 3) return 21u; return 17u; } // 0
    if (ch == 49u) { if (row == 0) return 4u; if (row == 1) return 12u; if (row == 6) return 31u; return 4u; } // 1
    if (ch == 50u) { if (row == 0 || row == 3 || row == 6) return 31u; if (row < 3) return 1u; return 16u; } // 2
    if (ch == 51u) { if (row == 0 || row == 3 || row == 6) return 31u; return 1u; } // 3
    if (ch == 52u) { if (row == 3) return 31u; if (row < 3) return 17u; return 1u; } // 4
    if (ch == 53u) { if (row == 0 || row == 3 || row == 6) return 31u; if (row < 3) return 16u; return 1u; } // 5
    if (ch == 54u) { if (row == 0 || row == 3 || row == 6) return 31u; if (row < 3) return 16u; return 17u; } // 6
    if (ch == 55u) { if (row == 0) return 31u; return 1u; } // 7
    if (ch == 56u) { if (row == 0 || row == 3 || row == 6) return 14u; return 17u; } // 8
    if (ch == 57u) { if (row == 0 || row == 3 || row == 6) return 31u; if (row < 3) return 17u; return 1u; } // 9

    // Symbols
    if (ch == 33u) { if (row >= 0 && row <= 4) return 4u; if (row == 6) return 4u; return 0u; } // !
    if (ch == 45u) { if (row == 3) return 14u; return 0u; } // -
    if (ch == 58u) { if (row == 2 || row == 4) return 4u; return 0u; } // :

    return 0u;
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

// Renders the 5x7 LED dot matrix text string
float3 RenderDaftLogo(float2 uv, float scale, float2 pos, float3 ledColor)
{
    float2 localUV = (uv - pos) / scale;

    // Center 16 characters: 6 columns per character (5 dots + 1 space)
    int numChars = 0;
    for (int c = 0; c < 16; ++c) {
        if (GetCustomChar(c) != 0u) numChars = c + 1;
    }
    if (numChars == 0) return float3(0.0, 0.0, 0.0);

    float totalW = float(numChars) * 6.0 - 1.0;
    float startX = -totalW * 0.5;

    localUV.x -= startX;
    localUV.y = 3.5 - localUV.y; // 0 at top, 6 at bottom

    int colIdx = int(floor(localUV.x));
    int rowIdx = int(floor(localUV.y));

    if (rowIdx < 0 || rowIdx > 6 || colIdx < 0 || colIdx >= int(totalW))
    {
        return float3(0.0, 0.0, 0.0);
    }

    int charIdx = colIdx / 6;
    int charCol = colIdx % 6;

    if (charCol >= 5 || charIdx >= numChars)
    {
        return float3(0.0, 0.0, 0.0); // Space between characters
    }

    uint ascii = GetCustomChar(charIdx);
    uint rowBits = GetRowBits(ascii, rowIdx);
    bool isLit = ((rowBits >> (4 - charCol)) & 1u) != 0u;

    float2 cellUV = frac(localUV);
    float dBulb = length(cellUV - float2(0.5, 0.5));

    // Circular LED bulb element
    float bulbCore = 1.0 - smoothstep(0.26, 0.38, dBulb);
    float bulbBloom = exp(-dBulb * 4.5) * 0.45;

    float3 col = float3(0.0, 0.0, 0.0);
    if (isLit)
    {
        float pulse = 1.0 + AudioKick * 0.4 + BeatPulse * 0.3;
        col = ledColor * (bulbCore * 2.2 + bulbBloom * 1.5) * pulse;
    }
    else
    {
        // Subtle unlit LED bulb socket
        col = float3(0.03, 0.03, 0.05) * bulbCore;
    }

    return col;
}

// -----------------------------------------------------------------------------
// 2. Layer 1: Sky Dome & Halloween Moon
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

    // Halloween Full Moon (When SpookyWeight > 0.01)
    if (SpookyWeight > 0.01)
    {
        float2 moonPos = float2(0.85, 0.52);
        float2 dMoonVec = uv - moonPos;
        float dMoon = length(dMoonVec);
        float moonR = 0.16;

        if (dMoon < moonR)
        {
            // Soft crater texture
            float crater = Noise2D(dMoonVec * 24.0 + float2(1.2, 3.4)) * 0.25;
            float3 moonCol = float3(1.0, 0.88, 0.45) * (0.85 - crater);
            // Limb darkening
            moonCol *= smoothstep(moonR, moonR * 0.8, dMoon);
            col += moonCol * SpookyWeight * (1.0 + AudioKick * 0.2);
        }

        // Moon eerie atmospheric glow halo
        float moonGlow = exp(-dMoon * 4.2) * 0.55;
        col += float3(1.0, 0.65, 0.15) * moonGlow * SpookyWeight;
    }

    return col;
}

// -----------------------------------------------------------------------------
// 3. Layer 2: TRON Grid Floor & Light-Cycle Trails
// -----------------------------------------------------------------------------
float3 RenderTronFloor(float3 ro, float3 rd, out float outFloorT)
{
    outFloorT = 1000.0;
    if (rd.y >= -0.001) return float3(0.0, 0.0, 0.0);

    float tFloor = (-1.35 - ro.y) / rd.y;
    if (tFloor <= 0.0 || tFloor > 50.0) return float3(0.0, 0.0, 0.0);

    outFloorT = tFloor;
    float3 pFloor = ro + rd * tFloor;
    float2 worldXZ = pFloor.xz + float2(0.0, EnergyAccumulator * 1.8 + Time * 0.8);

    // Anti-aliased TRON grid using fwidth
    float2 gridCoord = worldXZ * 0.6;
    float2 gridDist = abs(frac(gridCoord - 0.5) - 0.5) / max(fwidth(gridCoord), 0.0001);
    float gridLine = 1.0 - min(min(gridDist.x, gridDist.y), 1.0);

    float3 gridColor = lerp(ColorSecondary.rgb, ColorPrimary.rgb, sin(worldXZ.y * 0.1) * 0.5 + 0.5);
    float3 floorBase = float3(0.001, 0.0015, 0.003);
    float3 col = floorBase + gridColor * gridLine * (0.8 + AudioKick * 0.6);

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
            float trailZ = frac((pFloor.z + Time * speed + float(tr) * 12.3) * 0.06);
            float head = smoothstep(0.0, 0.05, trailZ) * (1.0 - smoothstep(0.05, 0.45, trailZ));

            float3 trailCol = ((tr & 1) == 0) ? ColorSecondary.rgb : ColorPrimary.rgb;
            if (tr == 2) trailCol = ColorAccent.rgb;

            col += trailCol * laneMask * head * 3.5 * TrailsWeight * (1.0 + AudioKick * 0.5);
        }
    }

    // Smoked obsidian mirror reflection of apex/lasers
    float fresnel = pow(1.0 - saturate(-rd.y), 4.0);
    float fog = exp(-tFloor * 0.045);

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

    // Distance to 4 corner slant edges
    float distToCornerX = abs(abs(q.x) - curW);
    float distToCornerZ = abs(abs(q.z) - curW);
    float cornerEdge = max(distToCornerX, distToCornerZ);

    // Distance to base edge perimeter
    float baseEdge = abs(q.y);

    float edgeDist = min(cornerEdge, baseEdge);
    outEdgeGlow = 1.0 / (1.0 + edgeDist * 32.0 * WireframeDensity);

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
// 5. Layer 4: Volumetric Apex Lasers
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

float3 RenderApexLasers(float3 ro, float3 rd, float maxDepth)
{
    if (LaserIntensity <= 0.01) return float3(0.0, 0.0, 0.0);

    float3 apexPos = float3(0.0, 0.9, 0.0);
    float3 col = float3(0.0, 0.0, 0.0);
    int numLasers = 8;

    float bpmSpeed = (Bpm / 120.0);
    float sweepBase = sin(Time * 1.2 * bpmSpeed) * 0.85;

    for (int i = 0; i < numLasers; ++i)
    {
        float normIdx = (float(i) / float(numLasers - 1)) * 2.0 - 1.0;
        float fanAngle = normIdx * 1.1 + sweepBase * 0.4;
        float pitch = -0.25 + cos(Time * 1.5 + normIdx * 1.5) * 0.15;

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
float3 RenderParticles(float2 uv, float3 ro, float3 uu, float3 vv, float3 ww, float hitT)
{
    if (ParticleIntensity <= 0.01) return float3(0.0, 0.0, 0.0);

    float3 col = float3(0.0, 0.0, 0.0);

    if (SpookyWeight > 0.1)
    {
        // 🦇 Halloween Bats flapping around
        int numBats = 12;
        for (int b = 0; b < numBats; ++b)
        {
            float seed = Hash11(float(b) * 17.31);
            float speed = 0.4 + seed * 0.6;
            float tBat = Time * speed + seed * 10.0;

            float batX = sin(tBat * 0.8) * 2.2 + cos(tBat * 0.3) * 0.8;
            float batY = 0.2 + sin(tBat * 1.2) * 0.6;
            float batZ = -0.5 + cos(tBat * 0.7) * 1.5;

            float3 pBat = float3(batX, batY, batZ);
            float3 toBat = pBat - ro;
            float zDepth = dot(toBat, ww);

            if (zDepth > 0.3 && zDepth < hitT)
            {
                float2 projUV = float2(dot(toBat, uu), dot(toBat, vv)) * (1.8 / zDepth);
                float2 dVec = uv - projUV;
                float batSize = 0.05 / max(zDepth * 0.6, 0.4);

                // Silhouette: body + flapping wings
                float flap = sin(Time * 14.0 + seed * 6.28) * 0.4;
                float wingDist = abs(dVec.x) - abs(dVec.y + flap * abs(dVec.x)) * 0.6;

                if (length(dVec) < batSize)
                {
                    // Dark silhouette with glowing orange eyes
                    float3 batCol = float3(0.01, 0.01, 0.02);
                    if (length(dVec - float2(0.005, 0.0)) < 0.004) {
                        batCol = ColorAccent.rgb * 2.0; // Glowing eye
                    }
                    col = lerp(col, batCol, 0.95);
                }
            }
        }
    }
    else
    {
        // Electric embers/sparks rising from floor
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
    }

    return col;
}

// -----------------------------------------------------------------------------
// 7. Layer 7: Jack-o'-Lanterns (SpookyWeight > 0.2)
// -----------------------------------------------------------------------------
float3 RenderJackOLanterns(float2 uv)
{
    if (SpookyWeight <= 0.2) return float3(0.0, 0.0, 0.0);

    float3 col = float3(0.0, 0.0, 0.0);

    for (int p = 0; p < 2; ++p)
    {
        float2 pCenter = (p == 0) ? float2(-1.25, -0.42) : float2(1.25, -0.42);
        float2 pUV = uv - pCenter;
        float r = length(pUV * float2(1.0, 1.25)); // Slightly squashed sphere

        float pumpR = 0.13;
        if (r < pumpR)
        {
            // Pumpkin body orange
            float ridges = sin(atan2(pUV.y, pUV.x) * 10.0) * 0.15;
            float3 bodyCol = float3(0.85, 0.35, 0.02) * (0.8 + ridges);

            // Carved face: glowing kick-reactive eyes & smile
            bool isEye = false;
            // Left eye
            float2 eyeL = pUV - float2(-0.04, 0.025);
            if (eyeL.y > 0.0 && abs(eyeL.x) < 0.025 - eyeL.y * 0.7) isEye = true;
            // Right eye
            float2 eyeR = pUV - float2(0.04, 0.025);
            if (eyeR.y > 0.0 && abs(eyeR.x) < 0.025 - eyeR.y * 0.7) isEye = true;

            // Jagged smile
            float2 mouth = pUV - float2(0.0, -0.04);
            bool isMouth = (abs(mouth.x) < 0.065 && mouth.y > -0.025 && mouth.y < 0.015 &&
                            frac(mouth.x * 60.0) > 0.35);

            if (isEye || isMouth)
            {
                float glow = (1.5 + AudioKick * 2.2 + BeatPulse * 1.5);
                bodyCol = float3(1.0, 0.9, 0.1) * glow;
            }

            col += bodyCol * SpookyWeight;
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
        col += floorCol * FloorLayerWeight;
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

        // Pyramid neon LED edges (always visible)
        float3 edgeCol = ColorAccent.rgb * edgeGlow * (1.5 + AudioKick * 1.5 + BeatPulse * 1.0);
        col += edgeCol * PyramidWeight;

        // Pyramid face patterns
        if (hit && hitPyramidT < floorT)
        {
            float3 faceCol = EvaluatePyramidPattern(faceUV);
            float fog = exp(-hitPyramidT * 0.04);
            col = lerp(col, col + faceCol, fog * PyramidWeight);
        }

        // Inner glowing sub-bass octahedron core
        float3 pCore = float3(0.0, -0.2, 0.0);
        float dCore = SDFOctahedron(ro + rd * 3.8 - pCore, 0.35 + AudioSubBass * 0.4);
        float coreGlow = 1.0 / (1.0 + abs(dCore) * 28.0);
        col += ColorPrimary.rgb * coreGlow * (1.2 + AudioSubBass * 2.0) * PyramidWeight;
    }

    // Minimum depth for laser/particle occlusion
    float minHitT = min(floorT, hitPyramidT);

    // 5. Volumetric Apex Lasers
    if (LaserIntensity > 0.01)
    {
        col += RenderApexLasers(ro, rd, minHitT);
    }

    // 6. Particles (Bats or Sparks)
    if (ParticleIntensity > 0.01)
    {
        col += RenderParticles(uv, ro, uu, vv, ww, minHitT);
    }

    // 7. Jack-o'-Lanterns (when SpookyWeight > 0.2)
    col += RenderJackOLanterns(uv);

    // 8. Daft Punk Style Dot-Matrix LED Logo ("DAD PUNK")
    if (LogoWeight > 0.01)
    {
        // Position: In Hero scene, center of screen. Otherwise, above pyramid / upper horizon
        float logoScale = (LogoWeight > 1.1) ? 0.042 : 0.026;
        float2 logoPos = (LogoWeight > 1.1) ? float2(0.0, 0.05) : float2(0.0, 0.72);

        float3 logoCol = RenderDaftLogo(uv, logoScale, logoPos, ColorAccent.rgb);
        col += logoCol * LogoWeight;
    }

    // 9. Kid-Safe Photosensitivity Limiter
    // Cap overall luminance by MasterBrightness (default 0.85)
    col *= MasterBrightness;

    // Outer border vignette pulse for drops (r > 0.7) - keeping central visual dark and crisp
    if (DropIntensity > 0.1 || BeatPulse > 0.7)
    {
        float edgeDist = length(input.UV - 0.5) * 2.0;
        float dropBorder = smoothstep(0.7, 1.4, edgeDist) * (DropIntensity * 0.3 + BeatPulse * 0.2);
        col += ColorAccent.rgb * dropBorder * 0.35 * SafetyDamping;
    }

    return float4(saturate(col), 1.0);
}
