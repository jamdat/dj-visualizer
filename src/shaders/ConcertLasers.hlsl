#include "Common.hlsl"

// =============================================================================
// ConcertLasers.hlsl - Volumetric Laser Matrix, Moving Heads & Stage Blinders
// =============================================================================

float LaserBeam(float3 ro, float3 rd, float3 laserOrigin, float3 laserDir, float beamWidth)
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

    if (sc < 0.0 || tc < 0.0) return 0.0;

    float3 dP = w0 + sc * rd - tc * laserDir;
    float dist = length(dP);

    // Atmospheric haze volumetric glow falloff
    return 1.0 / (1.0 + (dist / beamWidth) * (dist / beamWidth));
}

float4 PSMain(VSOutput input) : SV_Target
{
    float2 uv = input.UV * 2.0 - 1.0;
    uv.x *= AspectRatio;

    // Concert stage perspective camera
    float3 ro = float3(0.0, 0.5, -5.0);
    float3 ta = float3(0.0, 1.0, 5.0);

    float3 ww = normalize(ta - ro);
    float3 uu = normalize(cross(ww, float3(0.0, 1.0, 0.0)));
    float3 vv = cross(uu, ww);
    float3 rd = normalize(uv.x * uu + uv.y * vv + 1.5 * ww);

    float3 col = float3(0.01, 0.01, 0.03);

    // Stage Truss Rig / Laser Emitter origins (8 Moving Head Laser Fixtures)
    int numLasers = 8;
    float spread = 3.5 * (1.0 + AudioSubBass * 0.6);
    float fanAngle = (AudioHighMid * 1.2 + BeatPulse * 0.8) * 0.6 + CustomParam6;

    for (int i = 0; i < numLasers; ++i)
    {
        float normIdx = (float(i) / float(numLasers - 1)) * 2.0 - 1.0; // -1 to 1

        // Emitter on overhead stage truss
        float3 emitterPos = float3(normIdx * spread, 3.2, 8.0);

        // Sweeping beam orientation
        float sweep = sin(Time * 1.5 * CustomParam1 + normIdx * 2.0 + CustomParam4) * 0.8;
        float pitch = -0.4 + cos(Time * 2.0 + normIdx * 1.5) * 0.3 * fanAngle;

        float3 laserDir = normalize(float3(normIdx * 0.5 + sweep, pitch, -1.0));

        // Beam width and intensity
        float beamWidth = 0.012 * (1.0 + AudioKick * 1.5);
        float beamIntensity = LaserBeam(ro, rd, emitterPos, laserDir, beamWidth);

        // Color alternation (Primary / Secondary / Accent)
        float3 laserCol = (i % 2 == 0) ? ColorPrimary.rgb : ColorSecondary.rgb;
        if (i == 3 || i == 4) laserCol = ColorAccent.rgb;

        col += laserCol * beamIntensity * 0.45 * BloomIntensity * (0.8 + AudioKick * 1.2);

        // Moving Head Lens Flare Halo
        float3 toEmitter = emitterPos - ro;
        float flareDot = max(0.0, dot(rd, normalize(toEmitter)));
        col += laserCol * pow(flareDot, 64.0) * (0.5 + BeatPulse * 1.5);
    }

    // Stage Truss Blinder Lighting (illuminates top truss & atmosphere on heavy drops)
    float blinderFlash = DropIntensity * 0.8 + BeatPulse * 0.3;
    float trussGlow = exp(-abs(uv.y - 0.7) * 3.5);
    col += ColorAccent.rgb * trussGlow * blinderFlash * 0.4;

    // Atmospheric Ground Haze
    float groundHaze = exp(-abs(uv.y + 0.6) * 4.0);
    col += ColorSecondary.rgb * groundHaze * 0.15 * (1.0 + AudioLowMid * 1.0);

    // Strobe Flash
    col = ApplyStrobe(col, StrobeFlash, Time);

    return float4(col, 1.0);
}
