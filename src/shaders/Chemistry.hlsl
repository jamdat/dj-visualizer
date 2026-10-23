#include "Common.hlsl"

// =============================================================================
// Chemistry.hlsl - Chemistry Layout ("Molecular World")
// Glossy ball-and-stick molecules dancing to the music. A hero structure in the
// center changes every 24 s:
//   0 DNA Double Helix   1 Benzene Ring   2 Salt Crystal   3 Carbon Atom
// Around it a swarm of water, CO2, methane and nitrogen molecules each pulses to
// its own slice of the spectrum, hops on the beat and bursts outward on drops.
// Everything is ray-traced analytically (spheres + capsules), no raymarching.
// =============================================================================

struct AtomHit
{
    float t;
    float3 n;
    float3 albedo;
    float glow;
};

// -----------------------------------------------------------------------------
// 1. Intersection & Transform Helpers
// -----------------------------------------------------------------------------
float3 Hash33(float3 p)
{
    p = frac(p * float3(0.1031, 0.1030, 0.0973));
    p += dot(p, p.yxz + 33.33);
    return frac((p.xxy + p.yxx) * p.zyx);
}

float3 CameraRay(float3 ro, float3 ta, float2 uv, float focal)
{
    float3 ww = normalize(ta - ro);
    float3 uu = normalize(cross(float3(0.0, 1.0, 0.0), ww));
    float3 vv = cross(ww, uu);
    return normalize(uv.x * uu + uv.y * vv + focal * ww);
}

float SphereT(float3 ro, float3 rd, float3 c, float r)
{
    float3 oc = ro - c;
    float b = dot(oc, rd);
    float h = b * b - (dot(oc, oc) - r * r);
    if (h < 0.0) return -1.0;
    return -b - sqrt(h);
}

// True when the ray passes through the sphere somewhere in [0, tMax]
bool BoundHit(float3 ro, float3 rd, float3 c, float r, float tMax)
{
    float3 oc = ro - c;
    float b = dot(oc, rd);
    float h = b * b - (dot(oc, oc) - r * r);
    if (h < 0.0) return false;
    h = sqrt(h);
    return (-b + h > 0.0) && (-b - h < tMax);
}

// Ray-capsule intersection (bond stick)
float CapsuleT(float3 ro, float3 rd, float3 pa, float3 pb, float r)
{
    float3 ba = pb - pa;
    float3 oa = ro - pa;
    float baba = dot(ba, ba);
    float bard = dot(ba, rd);
    float baoa = dot(ba, oa);
    float a = baba - bard * bard;
    float b = baba * dot(rd, oa) - baoa * bard;
    float c = baba * dot(oa, oa) - baoa * baoa - r * r * baba;
    float h = b * b - a * c;
    if (h < 0.0 || a < 0.000001) return -1.0;

    float t = (-b - sqrt(h)) / a;
    float y = baoa + t * bard;
    if (y > 0.0 && y < baba) return t;

    float3 oc = (y <= 0.0) ? oa : ro - pb;
    b = dot(rd, oc);
    h = b * b - (dot(oc, oc) - r * r);
    return (h > 0.0) ? -b - sqrt(h) : -1.0;
}

void TestAtom(float3 ro, float3 rd, float3 c, float r, float3 albedo, float glow, inout AtomHit hit)
{
    float t = SphereT(ro, rd, c, r);
    if (t > 0.0 && t < hit.t)
    {
        hit.t = t;
        hit.n = normalize(ro + rd * t - c);
        hit.albedo = albedo;
        hit.glow = glow;
    }
}

// Bond stick, colored half-and-half by the two atoms it joins
void TestBond(float3 ro, float3 rd, float3 a, float3 b, float r, float3 colA, float3 colB, float glow, inout AtomHit hit)
{
    float t = CapsuleT(ro, rd, a, b, r);
    if (t > 0.0 && t < hit.t)
    {
        float3 p = ro + rd * t;
        float3 ba = b - a;
        float h = saturate(dot(p - a, ba) / dot(ba, ba));
        hit.t = t;
        hit.n = normalize(p - (a + ba * h));
        hit.albedo = (h < 0.5) ? colA : colB;
        hit.glow = glow;
    }
}

float3 RotY(float3 p, float a)
{
    float c = cos(a);
    float s = sin(a);
    return float3(c * p.x + s * p.z, p.y, -s * p.x + c * p.z);
}

float3 RotZ(float3 p, float a)
{
    float c = cos(a);
    float s = sin(a);
    return float3(c * p.x - s * p.y, s * p.x + c * p.y, p.z);
}

// Object frame = tilt about Z, then spin about Y
float3 FrameToWorld(float3 v, float tilt, float spin)
{
    return RotY(RotZ(v, tilt), spin);
}

float3 WorldToFrame(float3 v, float tilt, float spin)
{
    return RotZ(RotY(v, -spin), -tilt);
}

// Folds a hit traced in a scaled/rotated object frame back into the world hit
void MergeHit(AtomHit local, float scale, float tilt, float spin, inout AtomHit hit)
{
    float tw = local.t * scale;
    if (local.t < 100000.0 && tw < hit.t)
    {
        hit.t = tw;
        hit.n = FrameToWorld(local.n, tilt, spin);
        hit.albedo = local.albedo;
        hit.glow = local.glow;
    }
}

// CPK element colors: 0 H, 1 C, 2 N, 3 O, 4 Na, 5 Cl
float3 CpkColor(int element)
{
    if (element == 0) return float3(0.92, 0.93, 0.96);
    if (element == 1) return float3(0.22, 0.23, 0.26);
    if (element == 2) return float3(0.2, 0.38, 1.0);
    if (element == 3) return float3(1.0, 0.16, 0.12);
    if (element == 4) return float3(0.62, 0.32, 0.95);
    return float3(0.25, 0.95, 0.35);
}

// -----------------------------------------------------------------------------
// 2. Hero 0: DNA Double Helix (base pairs light up as a spectrum EQ)
// -----------------------------------------------------------------------------
void DnaStrandPoints(float k, float twist, out float3 pA, out float3 pB)
{
    float y = (k - 9.5) * 0.55;
    float sway = sin(y * 0.6 - Time * 1.8) * (0.15 + AudioSubBass * 0.35);
    float ang = k * 0.6283 + twist;
    pA = float3(cos(ang) * 1.6 + sway, y, sin(ang) * 1.6);
    pB = float3(cos(ang + 2.4) * 1.6 + sway, y, sin(ang + 2.4) * 1.6);
}

void TraceDNA(float3 ro, float3 rd, inout AtomHit hit)
{
    float twist = Time * 0.35 + EnergyAccumulator * 0.03;
    float3 sugar = float3(0.85, 0.8, 0.7);
    float3 phosphate = float3(1.0, 0.55, 0.1);

    for (int k = 0; k < 20; ++k)
    {
        float fk = float(k);
        float3 pA, pB, nA, nB;
        DnaStrandPoints(fk, twist, pA, pB);
        DnaStrandPoints(fk + 1.0, twist, nA, nB);

        TestAtom(ro, rd, pA, 0.32, phosphate, 0.15, hit);
        TestAtom(ro, rd, pB, 0.32, phosphate, 0.15, hit);
        if (k < 19)
        {
            TestBond(ro, rd, pA, nA, 0.12, sugar, sugar, 0.0, hit);
            TestBond(ro, rd, pB, nB, 0.12, sugar, sugar, 0.0, hit);
        }

        // Base pair rung: bottom of the helix = bass, top = highs
        float spec = GetAudioFFT(0.02 + fk / 19.0 * 0.5);
        bool gc = Hash11(fk * 7.13 + 1.9) > 0.5;
        float3 c1 = gc ? float3(0.25, 0.5, 1.0) : float3(0.2, 0.9, 0.35);  // G / A
        float3 c2 = gc ? float3(1.0, 0.85, 0.2) : float3(1.0, 0.3, 0.25);  // C / T
        if (Hash11(fk * 3.71 + 0.3) > 0.5)
        {
            float3 tmp = c1;
            c1 = c2;
            c2 = tmp;
        }
        float3 mid = (pA + pB) * 0.5;
        float glow = spec * spec * 2.5;
        TestBond(ro, rd, pA, mid, 0.14, c1, c1, glow, hit);
        TestBond(ro, rd, mid, pB, 0.14, c2, c2, glow, hit);
    }
}

// -----------------------------------------------------------------------------
// 3. Hero 1: Benzene Ring (breathing mode on the kick, circulating pi electrons)
// -----------------------------------------------------------------------------
void TraceBenzene(float3 ro, float3 rd, inout AtomHit hit)
{
    float breathe = 1.0 + AudioKick * 0.1;
    float3 carbon = CpkColor(1);
    float3 hydrogen = CpkColor(0);

    for (int i = 0; i < 6; ++i)
    {
        float a0 = float(i) * 1.0471976;
        float a1 = a0 + 1.0471976;
        float3 c0 = float3(cos(a0), 0.0, sin(a0)) * 1.4 * breathe;
        float3 c1 = float3(cos(a1), 0.0, sin(a1)) * 1.4 * breathe;
        float3 h0 = float3(cos(a0), 0.0, sin(a0)) * (2.48 + AudioTreble * 0.12 * sin(Time * 25.0 + float(i)));

        TestAtom(ro, rd, c0, 0.42, carbon, 0.0, hit);
        TestAtom(ro, rd, h0, 0.27, hydrogen, 0.0, hit);
        TestBond(ro, rd, c0, c1, 0.13, carbon, carbon, 0.0, hit);
        TestBond(ro, rd, c0, h0, 0.1, carbon, hydrogen, 0.0, hit);
    }
}

float3 BenzenePiGlow(float3 ro, float3 rd, float tMax)
{
    float3 col = float3(0.0, 0.0, 0.0);
    if (abs(rd.y) < 0.0001) return col;

    float ringR = 1.4 * (1.0 + AudioKick * 0.1);
    float energy = 0.35 + BeatPulse * 0.8 * SafetyDamping + AudioLowMid * 0.5;
    for (int s = 0; s < 2; ++s)
    {
        float yPlane = (s == 0) ? 0.6 : -0.6;
        float t = (yPlane - ro.y) / rd.y;
        if (t < 0.0 || t > tMax) continue;
        float3 q = ro + rd * t;
        float d = abs(length(q.xz) - ringR);
        float flow = 0.6 + 0.4 * sin(atan2(q.z, q.x) * 3.0 - Time * 4.0);
        col += ColorSecondary.rgb * exp(-d * d * 10.0) * flow * energy * min(0.4 / abs(rd.y), 1.2);
    }
    return col;
}

// -----------------------------------------------------------------------------
// 4. Hero 2: Salt Crystal (NaCl lattice: thermal jiggle with the highs)
// -----------------------------------------------------------------------------
float3 SaltIonPos(int ix, int iy, int iz, float spacing, float jig)
{
    float3 g = float3(float(ix), float(iy), float(iz)) - 1.0;
    float3 h = Hash33(g * 3.1 + 7.7);
    return g * spacing + sin(Time * (5.0 + h * 4.0) + h * 30.0) * jig;
}

void TraceSalt(float3 ro, float3 rd, inout AtomHit hit)
{
    float spacing = 1.3 * (1.0 + AudioKick * 0.06);
    float jig = 0.03 + (AudioHighMid + AudioTreble) * 0.05;
    float3 sodium = CpkColor(4);
    float3 chlorine = CpkColor(5);
    float naGlow = 0.15 + BeatPulse * 0.5 * SafetyDamping;

    for (int i = 0; i < 27; ++i)
    {
        int ix = i % 3;
        int iy = (i / 3) % 3;
        int iz = i / 9;
        float3 c = SaltIonPos(ix, iy, iz, spacing, jig);
        bool isNa = ((ix + iy + iz) & 1) == 0;
        float3 col = isNa ? sodium : chlorine;
        float3 other = isNa ? chlorine : sodium;

        TestAtom(ro, rd, c, isNa ? 0.34 : 0.55, col, isNa ? naGlow : 0.08, hit);
        if (ix < 2) TestBond(ro, rd, c, SaltIonPos(ix + 1, iy, iz, spacing, jig), 0.07, col, other, 0.0, hit);
        if (iy < 2) TestBond(ro, rd, c, SaltIonPos(ix, iy + 1, iz, spacing, jig), 0.07, col, other, 0.0, hit);
        if (iz < 2) TestBond(ro, rd, c, SaltIonPos(ix, iy, iz + 1, spacing, jig), 0.07, col, other, 0.0, hit);
    }
}

// -----------------------------------------------------------------------------
// 5. Hero 3: Carbon Atom (6 protons + 6 neutrons, 6 electrons streaking on orbits)
// -----------------------------------------------------------------------------
float3 IcosaVertex(int i)
{
    float phi = 1.618034;
    float s1 = ((i & 1) == 0) ? 1.0 : -1.0;
    float s2 = ((i & 2) == 0) ? 1.0 : -1.0;
    int g = i / 4;
    float3 v = (g == 0) ? float3(0.0, s1, s2 * phi) : ((g == 1) ? float3(s1, s2 * phi, 0.0) : float3(s2 * phi, 0.0, s1));
    return v / 1.902113;
}

// Orbit 0 = inner shell, 1..2 = outer shells
void OrbitFrame(int orbit, out float radius, out float tilt, out float yaw, out float speed)
{
    radius = (orbit == 0) ? 1.9 : 3.3;
    tilt = (orbit == 0) ? 0.35 : ((orbit == 1) ? 1.15 : -1.15);
    yaw = (orbit == 0) ? 0.0 : ((orbit == 1) ? 0.6 : -0.6);
    speed = (orbit == 0) ? 2.6 : 1.7;
}

float ElectronAngle(int orbit, int e, float speed)
{
    return Time * speed + EnergyAccumulator * 0.15 + float(e) * 3.14159265 + float(orbit) * 1.3;
}

void TraceCarbonAtom(float3 ro, float3 rd, inout AtomHit hit)
{
    float pulse = 1.0 + AudioKick * 0.12;
    float protonGlow = 0.25 + BeatPulse * 0.5 * SafetyDamping;
    for (int i = 0; i < 12; ++i)
    {
        float3 h = Hash33(float3(float(i), 1.0, 2.0));
        float3 c = IcosaVertex(i) * 0.62 * pulse + sin(Time * (9.0 + h * 5.0) + h * 20.0) * 0.03 * (1.0 + AudioKick * 2.0);
        bool proton = (i % 2) == 0;
        TestAtom(ro, rd, c, 0.42, proton ? float3(1.0, 0.28, 0.22) : float3(0.55, 0.62, 0.75), proton ? protonGlow : 0.05, hit);
    }

    for (int j = 0; j < 6; ++j)
    {
        int orbit = j / 2;
        float radius, tilt, yaw, speed;
        OrbitFrame(orbit, radius, tilt, yaw, speed);
        float theta = ElectronAngle(orbit, j % 2, speed);
        float3 e = FrameToWorld(float3(cos(theta), 0.0, sin(theta)) * radius, tilt, yaw);
        TestAtom(ro, rd, e, 0.16, ColorSecondary.rgb, 3.0, hit);
    }
}

// Orbit paths with comet trails behind each electron
float3 OrbitGlow(float3 ro, float3 rd, float tMax)
{
    float3 col = float3(0.0, 0.0, 0.0);
    for (int orbit = 0; orbit < 3; ++orbit)
    {
        float radius, tilt, yaw, speed;
        OrbitFrame(orbit, radius, tilt, yaw, speed);
        float3 o = WorldToFrame(ro, tilt, yaw);
        float3 d = WorldToFrame(rd, tilt, yaw);
        if (abs(d.y) < 0.0001) continue;
        float t = -o.y / d.y;
        if (t < 0.0 || t > tMax) continue;

        float3 q = o + d * t;
        float dist = abs(length(q.xz) - radius);
        float phiQ = atan2(q.z, q.x);
        float trail = 0.0;
        for (int e = 0; e < 2; ++e)
        {
            float behind = frac((ElectronAngle(orbit, e, speed) - phiQ) / 6.2831853);
            trail += exp(-behind * 10.0);
        }
        col += ColorSecondary.rgb * exp(-dist * dist * 120.0) * (0.12 + trail * 0.9) * (0.7 + AudioHighMid * 0.6);
    }
    return col;
}

// -----------------------------------------------------------------------------
// 6. Swarm: small molecules dancing around the hero
// -----------------------------------------------------------------------------
// type 0 = H2O, 1 = CO2, 2 = CH4, 3 = N2 (local frame, bonds vibrate with the treble)
void TraceSmallMolecule(int type, float3 ro, float3 rd, float glow, float vib, inout AtomHit hit)
{
    float3 hydrogen = CpkColor(0);
    float3 carbon = CpkColor(1);
    float3 nitrogen = CpkColor(2);
    float3 oxygen = CpkColor(3);
    float3 origin = float3(0.0, 0.0, 0.0);

    if (type == 0)
    {
        float3 h1 = float3(0.76, -0.59, 0.0) * vib;
        float3 h2 = float3(-0.76, -0.59, 0.0) * vib;
        TestAtom(ro, rd, origin, 0.42, oxygen, glow, hit);
        TestAtom(ro, rd, h1, 0.27, hydrogen, glow, hit);
        TestAtom(ro, rd, h2, 0.27, hydrogen, glow, hit);
        TestBond(ro, rd, origin, h1, 0.1, oxygen, hydrogen, glow, hit);
        TestBond(ro, rd, origin, h2, 0.1, oxygen, hydrogen, glow, hit);
    }
    else if (type == 1)
    {
        float3 o1 = float3(1.16 * vib, 0.0, 0.0);
        TestAtom(ro, rd, origin, 0.38, carbon, glow, hit);
        TestAtom(ro, rd, o1, 0.42, oxygen, glow, hit);
        TestAtom(ro, rd, -o1, 0.42, oxygen, glow, hit);
        for (int s = 0; s < 2; ++s)
        {
            float3 off = float3(0.0, 0.0, (s == 0) ? 0.1 : -0.1); // double bonds
            TestBond(ro, rd, off, o1 + off, 0.055, carbon, oxygen, glow, hit);
            TestBond(ro, rd, off, -o1 + off, 0.055, carbon, oxygen, glow, hit);
        }
    }
    else if (type == 2)
    {
        TestAtom(ro, rd, origin, 0.4, carbon, glow, hit);
        for (int i = 0; i < 4; ++i)
        {
            float3 dir = (i == 0) ? float3(1.0, 1.0, 1.0) : ((i == 1) ? float3(1.0, -1.0, -1.0) : ((i == 2) ? float3(-1.0, 1.0, -1.0) : float3(-1.0, -1.0, 1.0)));
            float3 h = dir * (0.629 * vib);
            TestAtom(ro, rd, h, 0.26, hydrogen, glow, hit);
            TestBond(ro, rd, origin, h, 0.09, carbon, hydrogen, glow, hit);
        }
    }
    else
    {
        float3 n1 = float3(0.55 * vib, 0.0, 0.0);
        TestAtom(ro, rd, n1, 0.4, nitrogen, glow, hit);
        TestAtom(ro, rd, -n1, 0.4, nitrogen, glow, hit);
        for (int s = 0; s < 3; ++s)
        {
            float a = float(s) * 2.0943951;
            float3 off = float3(0.0, cos(a), sin(a)) * 0.1; // triple bond
            TestBond(ro, rd, n1 + off, -n1 + off, 0.045, nitrogen, nitrogen, glow, hit);
        }
    }
}

void TraceSwarm(float3 ro, float3 rd, inout AtomHit hit)
{
    for (int k = 0; k < 14; ++k)
    {
        float fk = float(k);
        float3 h = Hash33(float3(fk * 1.3, fk * 7.1, 2.9));

        // Orbit the hero; drops blast the swarm outward, beats make each molecule hop
        float orbitR = 8.5 + h.x * 4.0 + DropIntensity * 2.5 * SafetyDamping;
        float ang = h.y * 6.2831853 + Time * (0.04 + h.z * 0.06) + EnergyAccumulator * 0.004;
        float y = (h.z - 0.5) * 9.0 + sin(Time * (0.5 + h.x) + fk) * 0.6;
        y += BeatPulse * 0.35 * SafetyDamping * sin(fk * 2.3 + 1.0);
        float3 center = float3(cos(ang) * orbitR, y, sin(ang) * orbitR);

        // Each molecule pulses with its own slice of the spectrum
        float spec = GetAudioFFT(0.02 + fk / 13.0 * 0.5);
        float scale = 1.0 + spec * 0.3;
        if (!BoundHit(ro, rd, center, 1.6 * scale, hit.t)) continue;

        float tumble = Time * (0.4 + h.x * 0.8) + fk;
        float spin = Time * (0.3 + h.y * 0.7) + EnergyAccumulator * (0.02 + h.y * 0.03);
        float vib = 1.0 + AudioTreble * 0.1 * sin(Time * 30.0 + fk * 5.0);

        AtomHit local;
        local.t = hit.t / scale;
        local.n = float3(0.0, 1.0, 0.0);
        local.albedo = float3(0.0, 0.0, 0.0);
        local.glow = 0.0;
        TraceSmallMolecule(k % 4, WorldToFrame(ro - center, tumble, spin) / scale, WorldToFrame(rd, tumble, spin), spec * 1.2, vib, local);
        if (local.t < hit.t / scale)
        {
            MergeHit(local, scale, tumble, spin, hit);
        }
    }
}

// -----------------------------------------------------------------------------
// 7. Lab Backdrop: deep gradient, drifting honeycomb lattice, bokeh particles
// -----------------------------------------------------------------------------
float HexEdgeDist(float2 p)
{
    float2 s = float2(1.0, 1.7320508);
    float2 a = (frac(p / s) - 0.5) * s;
    float2 b = (frac((p - s * 0.5) / s) - 0.5) * s;
    float2 gv = (dot(a, a) < dot(b, b)) ? a : b;
    float2 q = abs(gv);
    return 0.5 - max(dot(q, float2(0.5, 0.8660254)), q.x);
}

float3 LabBackground(float2 uv, float3 rd)
{
    float3 col = lerp(float3(0.008, 0.012, 0.025), float3(0.02, 0.045, 0.07), saturate(rd.y * 0.5 + 0.5));
    col += float3(0.04, 0.07, 0.12) * exp(-length(uv) * 1.2) * (1.0 + AudioSubBass * 0.5);

    // Honeycomb lattice (benzene rings) drifting behind everything
    float2 hp = uv * 5.0 + float2(Time * 0.05, Time * 0.03);
    float edge = HexEdgeDist(hp);
    float hexLine = 1.0 - smoothstep(0.0, 0.05, edge);
    col += ColorSecondary.rgb * hexLine * 0.05 * (0.6 + BeatPulse * 0.6 * SafetyDamping);

    // Out-of-focus particles drifting up through the solution
    for (int layer = 0; layer < 2; ++layer)
    {
        float sc = 3.0 + float(layer) * 2.5;
        float2 bp = uv * sc + float2(0.0, Time * (0.05 + float(layer) * 0.03));
        float2 id = floor(bp);
        float2 f = frac(bp) - 0.5;
        float2 h = Hash22(id + float(layer) * 7.0);
        if (h.x > 0.55) continue;
        float r = 0.12 + h.y * 0.18;
        float d = length(f - (h - 0.5) * 0.5);
        float disk = smoothstep(r, r - 0.04, d) * (0.6 + 0.4 * smoothstep(r - 0.06, r, d));
        col += lerp(ColorSecondary.rgb, ColorPrimary.rgb, h.y) * disk * 0.05 * (0.7 + AudioTreble * 0.6);
    }
    return col;
}

float3 ShadeAtom(float3 n, float3 rd, float3 albedo, float glow)
{
    float3 keyDir = normalize(float3(0.45, 0.85, 0.3));
    float3 fillDir = normalize(float3(-0.6, -0.2, -0.5));
    float diff = saturate(dot(n, keyDir));
    float fill = saturate(dot(n, fillDir)) * 0.25;
    float spec = pow(saturate(dot(n, normalize(keyDir - rd))), 60.0) * 0.8;
    float fres = pow(1.0 - saturate(dot(n, -rd)), 3.0);

    float3 col = albedo * (0.12 + diff * 0.85 + fill) + float3(1.0, 1.0, 1.0) * spec;
    col += ColorSecondary.rgb * fres * (0.45 + AudioHighMid * 0.5);
    col += albedo * glow;
    return col;
}

// -----------------------------------------------------------------------------
// 8. Main Pixel Shader
// -----------------------------------------------------------------------------
float4 PSMain(VSOutput input) : SV_Target
{
    float2 uv = input.UV * 2.0 - 1.0;
    uv.x *= AspectRatio;

    float camA = Time * 0.04 + 0.6;
    float3 ro = float3(sin(camA) * 20.0, 3.0 + sin(Time * 0.07) * 2.0, cos(camA) * 20.0);
    float3 rd = CameraRay(ro, float3(0.0, 0.2, 0.0), uv, 2.2);

    AtomHit hit;
    hit.t = 100000.0;
    hit.n = float3(0.0, 1.0, 0.0);
    hit.albedo = float3(0.0, 0.0, 0.0);
    hit.glow = 0.0;

    // Hero structure: shrinks away and the next one grows in every 24 s
    float seg = floor(Time / 24.0);
    float hp = Time - seg * 24.0;
    int hero = int(fmod(seg, 4.0) + 0.5);
    float env = smoothstep(0.0, 1.5, hp) * (1.0 - smoothstep(22.5, 24.0, hp));
    float baseScale = (hero == 0) ? 1.25 : ((hero == 1) ? 2.0 : ((hero == 2) ? 1.7 : 1.6));
    float boundR = (hero == 0) ? 6.0 : ((hero == 1) ? 2.9 : ((hero == 2) ? 3.1 : 3.6));
    float heroScale = env * baseScale * (1.0 + AudioKick * 0.04);
    float spin = Time * 0.15 + EnergyAccumulator * 0.01 + (1.0 - env) * (1.0 - env) * 3.0;
    float tilt = (hero == 0) ? (0.3 + 0.12 * sin(Time * 0.2)) : (0.35 * sin(Time * 0.17));

    bool heroVisible = heroScale > 0.01 && BoundHit(ro, rd, float3(0.0, 0.0, 0.0), boundR * heroScale, hit.t);
    float3 roL = WorldToFrame(ro, tilt, spin) / max(heroScale, 0.01);
    float3 rdL = WorldToFrame(rd, tilt, spin);
    if (heroVisible)
    {
        AtomHit local;
        local.t = 100000.0;
        local.n = float3(0.0, 1.0, 0.0);
        local.albedo = float3(0.0, 0.0, 0.0);
        local.glow = 0.0;
        if (hero == 0) TraceDNA(roL, rdL, local);
        else if (hero == 1) TraceBenzene(roL, rdL, local);
        else if (hero == 2) TraceSalt(roL, rdL, local);
        else TraceCarbonAtom(roL, rdL, local);
        MergeHit(local, heroScale, tilt, spin, hit);
    }

    TraceSwarm(ro, rd, hit);

    float3 col = LabBackground(uv, rd);
    if (hit.t < 100000.0)
    {
        float3 shaded = ShadeAtom(hit.n, rd, hit.albedo, hit.glow);
        col = lerp(shaded, col, (1.0 - exp(-hit.t * 0.02)) * 0.5);
    }

    // Hero glow effects (pi electron rings / electron orbits), hidden behind closer atoms
    if (heroVisible && hero == 1) col += BenzenePiGlow(roL, rdL, hit.t / heroScale) * env;
    if (heroVisible && hero == 3) col += OrbitGlow(roL, rdL, hit.t / heroScale) * env;

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
