#include "Common.hlsl"

// =============================================================================
// Quantum.hlsl - Quantum Layout
// Four quantum phenomena crossfading through a "wavefunction collapse" (2-minute cycle):
//   0 Electron Orbitals (20 s)   1 Bubble Chamber (20 s)   2 Double Slit (20 s)   3 Quantum Field (60 s)
// main.cpp sets CustomParam7 = beat phase (beats, wraps at 64) for tempo-locked events.
// =============================================================================

// -----------------------------------------------------------------------------
// 1. Shared Helpers
// -----------------------------------------------------------------------------
float Sq(float x)
{
    return x * x;
}

float3 Hash33(float3 p)
{
    p = frac(p * float3(0.1031, 0.1030, 0.0973));
    p += dot(p, p.yxz + 33.33);
    return frac((p.xxy + p.yxx) * p.zyx);
}

// 3D value noise from the 256x256 random NoiseTexture (two z-slice lookups)
float Noise3(float3 p)
{
    float3 i = floor(p);
    float3 f = frac(p);
    f = f * f * (3.0 - 2.0 * f);
    float2 uv = i.xy + float2(37.0, 17.0) * i.z + f.xy;
    float2 uvA = (uv + 0.5) / 256.0;
    float2 uvB = (uv + float2(37.5, 17.5)) / 256.0;
    float a = NoiseTexture.SampleLevel(SamplerLinearWrap, uvA, 0).r;
    float b = NoiseTexture.SampleLevel(SamplerLinearWrap, uvB, 0).r;
    return lerp(a, b, f.z);
}

float3 CameraRay(float3 ro, float3 ta, float2 uv, float focal)
{
    float3 ww = normalize(ta - ro);
    float3 uu = normalize(cross(float3(0.0, 1.0, 0.0), ww));
    float3 vv = cross(ww, uu);
    return normalize(uv.x * uu + uv.y * vv + focal * ww);
}

float2 SphereHit(float3 ro, float3 rd, float3 c, float r)
{
    float3 oc = ro - c;
    float b = dot(oc, rd);
    float h = b * b - (dot(oc, oc) - r * r);
    if (h < 0.0) return float2(-1.0, -1.0);
    h = sqrt(h);
    return float2(-b - h, -b + h);
}

float SegDist(float2 p, float2 a, float2 b)
{
    float2 pa = p - a;
    float2 ba = b - a;
    float h = saturate(dot(pa, ba) / max(dot(ba, ba), 0.000001));
    return length(pa - ba * h);
}

float3 RotY(float3 p, float a)
{
    float c = cos(a);
    float s = sin(a);
    return float3(c * p.x + s * p.z, p.y, -s * p.x + c * p.z);
}

float3 RotX(float3 p, float a)
{
    float c = cos(a);
    float s = sin(a);
    return float3(p.x, c * p.y - s * p.z, s * p.y + c * p.z);
}

// Faint quantum "probability grid" backdrop shared by the 2D phenomena
float3 QuantumBackdrop(float2 uv)
{
    float3 col = float3(0.006, 0.008, 0.02) + float3(0.02, 0.03, 0.06) * exp(-length(uv) * 1.2);
    float2 g = abs(frac(uv * 6.0) - 0.5);
    float gridLine = 1.0 - smoothstep(0.0, 0.03, min(g.x, g.y));
    col += ColorSecondary.rgb * gridLine * 0.025;
    return col;
}

// -----------------------------------------------------------------------------
// 2. Phenomenon 0: Electron Orbitals (hydrogen superposition, colored by phase)
// -----------------------------------------------------------------------------
// Real hydrogen wavefunctions (Bohr radius = 1), scaled so each peaks near 1
float OrbitalPsi(int id, float3 p)
{
    float r = length(p);
    if (id == 0) return p.z * exp(-r * 0.5) / 0.736;                          // 2p_z
    if (id == 1) return (3.0 * p.z * p.z - r * r) * exp(-r / 3.0) / 9.74;    // 3d_z2
    if (id == 2) return p.x * p.y * exp(-r / 3.0) / 2.44;                    // 3d_xy
    return (6.0 - r) * p.x * exp(-r / 3.0) / 4.1;                            // 3p_x
}

float3 RenderOrbitals(float2 uv, float visit, float2 fragCoord)
{
    float camA = Time * 0.08;
    float3 ro = float3(sin(camA) * 34.0, 9.0 + sin(Time * 0.1) * 6.0, cos(camA) * 34.0);
    float3 rd = CameraRay(ro, float3(0.0, 0.0, 0.0), uv, 1.8);

    // Each visit shows a different pair of states in superposition
    int idA = int(fmod(visit, 4.0) + 0.5);
    int idB = int(fmod(visit + 1.0, 4.0) + 0.5);
    float mixAngle = 0.62 + 0.3 * sin(Time * 0.21);
    float ca = cos(mixAngle);
    float sa = sin(mixAngle);
    // Relative quantum phase: the cloud sloshes between the two shapes, faster when it's loud
    float relPhase = Time * 1.1 + EnergyAccumulator * 0.06;
    float cp = cos(relPhase);
    float sp = sin(relPhase);
    float tumble = Time * 0.05;

    float3 col = QuantumBackdrop(uv) * 0.7;
    float glow = 0.75 + AudioKick * 0.7 + AudioSubBass * 0.4;
    float isoLevel = 0.18 + BeatPulse * 0.1 * SafetyDamping;

    float2 hb = SphereHit(ro, rd, float3(0.0, 0.0, 0.0), 17.0);
    if (hb.y > 0.0)
    {
        float t0 = max(hb.x, 0.0);
        float dt = (hb.y - t0) / 64.0;
        float t = t0 + Hash21(fragCoord) * dt;
        for (int i = 0; i < 64; ++i)
        {
            float3 q = RotX(RotY(ro + rd * t, tumble), 0.3);
            float a = OrbitalPsi(idA, q);
            float b = OrbitalPsi(idB, q);
            float re = ca * a + sa * b * cp;
            float im = sa * b * sp;
            float density = re * re + im * im;

            // Domain coloring: hue = complex phase of the wavefunction
            float hue = atan2(im, re) / 6.2831853 + 0.5 + AudioLowMid * 0.1;
            float3 c = HSVtoRGB(float3(frac(hue), 0.75, 1.0));
            col += c * density * dt * 0.22 * glow;

            // Glowing probability isosurface that flares on the beat
            float shell = exp(-Sq((density - isoLevel) / 0.03));
            col += lerp(c, float3(1.0, 1.0, 1.0), 0.5) * shell * dt * 0.35;
            t += dt;
        }
    }

    // Proton at the center
    float tc = max(dot(-ro, rd), 0.0);
    float dm = length(-ro - rd * tc);
    col += float3(1.0, 0.85, 0.7) * (0.02 / (dm * dm + 0.02)) * (0.6 + AudioKick * 0.6);
    return col;
}

// -----------------------------------------------------------------------------
// 3. Phenomenon 1: Bubble Chamber (a collision fires on every beat)
// -----------------------------------------------------------------------------
float3 RenderBubbleChamber(float2 uv)
{
    float px = 2.0 / Resolution.y;
    float3 col = float3(0.008, 0.025, 0.03) + float3(0.0, 0.03, 0.035) * exp(-length(uv) * 0.9);
    col += float3(0.05, 0.08, 0.08) * (Noise3(float3(uv * 90.0, Time * 0.5)) - 0.5) * 0.4; // liquid grain

    // Straight beam tracks crossing the chamber
    for (int bt = 0; bt < 3; ++bt)
    {
        float y = -0.55 + float(bt) * 0.5 + 0.03 * sin(uv.x * 2.0 + float(bt));
        col += float3(0.5, 0.7, 0.7) * exp(-Sq((uv.y - y) / (px * 1.2))) * 0.12;
    }

    float beat = CustomParam7;
    float slot = floor(beat);
    for (int j = 0; j < 6; ++j)
    {
        float k = slot - float(j);
        float age = beat - k;                      // beats since this collision fired
        float kk = fmod(k + 640.0, 64.0);          // wraps with the 64-beat phase
        float3 h = Hash33(float3(kk, 3.7, 1.9));
        float2 vtx = (h.xy - 0.5) * float2(2.6, 1.4);
        float grow = smoothstep(0.0, 0.7, age);
        float fade = exp(-age * 0.55) * (1.0 - smoothstep(5.0, 6.0, age));
        int nTracks = 2 + int(h.z * 2.99);

        for (int tr = 0; tr < 4; ++tr)
        {
            if (tr >= nTracks) break;
            float3 g = Hash33(float3(kk, float(tr), 7.7));
            float charge = (g.x > 0.5) ? 1.0 : -1.0;
            float len = (0.6 + g.z * 1.8) * grow;
            if (length(uv - vtx) > len + 0.06) continue;

            // Charged particles curl in the magnetic field and spiral inward as they lose energy
            float theta = g.y * 6.2831853;
            float radius0 = 0.25 + g.z * 1.2;
            float loss = 0.8 + h.x * 1.5;
            float ds = len / 40.0;
            float2 pos = vtx;
            float dmin = 10.0;
            float sAt = 0.0;
            for (int s = 0; s < 40; ++s)
            {
                float sf = float(s) * ds;
                float2 nxt = pos + float2(cos(theta), sin(theta)) * ds;
                float d = SegDist(uv, pos, nxt);
                if (d < dmin)
                {
                    dmin = d;
                    sAt = sf;
                }
                theta += charge * min(exp(sf * loss) / radius0, 0.3 / max(ds, 0.0001)) * ds;
                pos = nxt;
            }

            float bubbles = 0.55 + 0.45 * step(0.35, Hash11(floor(sAt * 90.0) + kk * 13.0));
            float w = 0.003 + px;
            float core = exp(-Sq(dmin / w));
            float halo = exp(-dmin / (w * 4.0)) * 0.3;
            float3 tc = (charge > 0.0) ? ColorPrimary.rgb : ColorSecondary.rgb;
            col += (tc * (core * 1.4 + halo) + float3(1.0, 1.0, 1.0) * core * 0.5) * fade * bubbles * (0.8 + AudioHighMid * 0.6);
        }

        // Flash at the collision vertex as it fires
        col += float3(0.9, 0.95, 1.0) * exp(-length(uv - vtx) * 30.0) * exp(-age * 4.0) * (0.6 + AudioKick * 0.6) * SafetyDamping;
    }
    return col;
}

// -----------------------------------------------------------------------------
// 4. Phenomenon 2: Double Slit (the mid-range sets the wavelength)
// -----------------------------------------------------------------------------
float SlitIntensity(float y, float k, float d, float xb, float xs)
{
    float r1 = length(float2(xs - xb, y - d * 0.5));
    float r2 = length(float2(xs - xb, y + d * 0.5));
    float fringe = Sq(cos(k * (r1 - r2) * 0.5));
    return fringe * exp(-y * y * 1.6);
}

float3 RenderDoubleSlit(float2 uv)
{
    float px = 2.0 / Resolution.y;
    float3 col = QuantumBackdrop(uv);

    float k = 30.0 + AudioLowMid * 12.0;
    float d = 0.32 + 0.06 * sin(Time * 0.15);
    float xb = -0.45;
    float xs = 1.35;
    float2 src = float2(-1.6, 0.0);
    float phase = Time * 7.0;
    float2 s1 = float2(xb, d * 0.5);
    float2 s2 = float2(xb, -d * 0.5);
    float rs = length(s1 - src);

    // Probability wave: one spherical wave in, two coherent waves out of the slits
    float amp;
    if (uv.x < xb)
    {
        float r = length(uv - src);
        amp = cos(k * r - phase) / sqrt(r * 3.0 + 0.3);
    }
    else
    {
        float r1 = length(uv - s1);
        float r2 = length(uv - s2);
        amp = (cos(k * (rs + r1) - phase) / sqrt(r1 * 3.0 + 0.3) + cos(k * (rs + r2) - phase) / sqrt(r2 * 3.0 + 0.3)) * 0.55;
    }
    if (uv.x < xs)
    {
        float3 waveCol = (amp > 0.0) ? ColorSecondary.rgb : ColorPrimary.rgb;
        col += waveCol * abs(amp) * (0.35 + AudioKick * 0.3);
        col += float3(1.0, 1.0, 1.0) * pow(saturate(abs(amp)), 6.0) * 0.25;
    }

    // Barrier with two slits
    float slitHalf = 0.035;
    bool inBarrier = abs(uv.x - xb) < 0.018 && abs(uv.y - d * 0.5) > slitHalf && abs(uv.y + d * 0.5) > slitHalf;
    if (inBarrier) col = float3(0.08, 0.1, 0.16) + ColorSecondary.rgb * 0.15;

    // Electron gun pulsing on the beat
    col += float3(0.8, 0.9, 1.0) * exp(-length(uv - src) * 25.0) * (1.0 + BeatPulse * SafetyDamping);

    // Detector screen: single particle hits pile up into interference fringes
    if (uv.x > xs)
    {
        col = float3(0.01, 0.012, 0.025) + ColorSecondary.rgb * SlitIntensity(uv.y, k, d, xb, xs) * 0.12 * (0.6 + AudioSubBass * 0.6);
        float rate = 30.0;
        float now = Time * rate;
        float slot = floor(now);
        for (int j = 0; j < 90; ++j)
        {
            float sl = slot - float(j);
            for (int c = 0; c < 4; ++c)
            {
                float3 h = Hash33(float3(fmod(sl, 4096.0), float(c), 1.3));
                float y = (h.x * 2.0 - 1.0) * 0.95;
                if (h.y > SlitIntensity(y, k, d, xb, xs)) continue;
                float2 hit = float2(xs + 0.03 + h.z * 0.3, y);
                float age = (now - sl) / rate;
                float dd = length(uv - hit);
                col += float3(0.7, 0.95, 1.0) * exp(-dd * dd / Sq(0.005 + px)) * (exp(-age * 0.35) * 0.8 + exp(-age * 12.0) * 2.0);
            }
        }
        col += float3(0.3, 0.6, 0.8) * exp(-abs(uv.x - xs) / (px * 2.0)) * 0.4;
    }
    return col;
}

// -----------------------------------------------------------------------------
// 5. Phenomenon 3: Quantum Field (fluctuating fabric with wave-packet particles)
// -----------------------------------------------------------------------------
float QuantumField(float2 x)
{
    // Vacuum fluctuations get stronger with the highs
    float f = (Noise3(float3(x * 0.45, Time * 0.6)) - 0.5) * (0.25 + AudioTreble * 0.4);
    float omega = Time * 6.0;

    // Long-lived particles drifting through the field as oscillating wave packets
    for (int i = 0; i < 4; ++i)
    {
        float3 h = Hash33(float3(float(i), 4.2, 9.1));
        float2 c = (frac(h.xy + float2(Time, Time * 0.7) * (0.01 + h.z * 0.02)) - 0.5) * 26.0;
        float2 dv = x - c;
        float env = exp(-dot(dv, dv) * 0.35);
        f += env * cos(dot(dv, float2(cos(h.z * 6.28), sin(h.z * 6.28))) * 3.0 - omega) * (0.8 + AudioKick * 0.6);
    }

    // A particle-antiparticle pair pops out on every beat and races apart
    float beat = CustomParam7;
    float slot = floor(beat);
    for (int j = 0; j < 3; ++j)
    {
        float k = slot - float(j);
        float age = beat - k;
        float3 h = Hash33(float3(fmod(k + 640.0, 64.0), 1.1, 5.5));
        float2 vtx = (h.xy - 0.5) * 18.0;
        float2 dir = float2(cos(h.z * 6.28), sin(h.z * 6.28));
        float amp = exp(-age * 0.8) * SafetyDamping;
        for (int s = 0; s < 2; ++s)
        {
            float2 c = vtx + dir * age * 3.0 * ((s == 0) ? 1.0 : -1.0);
            float2 dv = x - c;
            f += exp(-dot(dv, dv) * 0.6) * cos(length(dv) * 4.0 - omega) * amp * ((s == 0) ? 1.0 : -1.0) * 1.2;
        }
    }
    return f;
}

float3 RenderQuantumField(float2 uv, float pixelAngle)
{
    float camA = Time * 0.05;
    float3 ro = float3(sin(camA) * 22.0, 11.0, cos(camA) * 22.0);
    float3 rd = CameraRay(ro, float3(0.0, -1.0, 0.0), uv, 1.6);

    float3 col = QuantumBackdrop(uv) * 0.6;
    if (rd.y >= -0.01) return col;

    // Heightfield march between y = +2.4 and y = -2.4
    float tTop = (2.4 - ro.y) / rd.y;
    float tBot = (-2.4 - ro.y) / rd.y;
    float t = max(tTop, 0.0);
    float tHit = -1.0;
    float stepLen = (tBot - t) / 56.0;
    float prevD = 1.0;
    for (int i = 0; i < 56; ++i)
    {
        float3 p = ro + rd * t;
        float d = p.y - QuantumField(p.xz) * 1.4;
        if (d < 0.0)
        {
            tHit = t - stepLen * d / (d - prevD); // linear refine between the last two samples
            break;
        }
        prevD = d;
        t += stepLen;
    }
    if (tHit < 0.0) return col;

    float3 p = ro + rd * tHit;
    float hgt = QuantumField(p.xz);
    float fw = tHit * pixelAngle;

    // Glowing lattice lines on the field, colored by the sign of the field
    float2 g = abs(frac(p.xz * 0.9) - 0.5) / 0.9;
    float lattice = 1.0 - smoothstep(0.0, 0.02 + fw * 1.5, min(g.x, g.y));
    float3 fieldCol = (hgt > 0.0) ? ColorPrimary.rgb : ColorSecondary.rgb;
    col = float3(0.01, 0.012, 0.03) + fieldCol * (lattice * 1.2 + abs(hgt) * 0.9) * (0.7 + AudioLowMid * 0.5);
    col += float3(1.0, 1.0, 1.0) * pow(saturate(abs(hgt) - 0.4), 2.0) * 1.5; // hot particle peaks
    col *= exp(-max(tHit - 15.0, 0.0) * 0.04);
    return col;
}

// -----------------------------------------------------------------------------
// 6. Main Pixel Shader
// -----------------------------------------------------------------------------
float3 RenderPhenomenon(int id, float2 uv, float visit, float pixelAngle, float2 fragCoord)
{
    if (id == 0) return RenderOrbitals(uv, visit, fragCoord);
    if (id == 1) return RenderBubbleChamber(uv);
    if (id == 2) return RenderDoubleSlit(uv);
    return RenderQuantumField(uv, pixelAngle);
}

float4 PSMain(VSOutput input) : SV_Target
{
    float2 uv = input.UV * 2.0 - 1.0;
    uv.x *= AspectRatio;
    float pixelAngle = 2.0 / (Resolution.y * 1.6);

    // Schedule: Orbitals, Bubble Chamber and Double Slit get 20 s each, then the Quantum Field holds for 60 s
    const float kCycle = 120.0;
    float visit = floor(Time / kCycle);
    float tc = Time - visit * kCycle;
    int cur = 3;
    float segStart = 60.0;
    float segLen = 60.0;
    if (tc < 60.0)
    {
        cur = int(tc / 20.0);
        segStart = float(cur) * 20.0;
        segLen = 20.0;
    }
    float p = tc - segStart;
    int nxt = (cur + 1) % 4;
    float nextVisit = (cur == 3) ? visit + 1.0 : visit;

    float3 col = RenderPhenomenon(cur, uv, visit, pixelAngle, input.Position.xy);

    // Crossfade into the next phenomenon over the last second
    float w = smoothstep(segLen - 1.0, segLen, p);
    if (w > 0.001)
    {
        col = lerp(col, RenderPhenomenon(nxt, uv, nextVisit, pixelAngle, input.Position.xy), w);
    }

    // "Wavefunction collapse" ripple racing out from the center at each switch
    float since = (p < 2.0) ? p : 100.0;
    col += ColorSecondary.rgb * exp(-Sq((length(uv) - since * 1.6) / 0.06)) * exp(-since * 1.5) * 0.8 * SafetyDamping;

    // Soft tone curve, then kid-safe brightness cap
    col = 1.0 - exp(-col * 1.25);
    col *= MasterBrightness;

    // Shared vignette layer (mixer "Vignette & Drop Pulse")
    if (SpookyWeight > 0.01)
    {
        float edgeDist = length(input.UV - 0.5) * 2.0;
        col *= clamp(1.0 - smoothstep(0.7, 1.4, edgeDist) * 0.45 * SpookyWeight, 0.0, 1.0);
    }

    return float4(saturate(col), 1.0);
}
