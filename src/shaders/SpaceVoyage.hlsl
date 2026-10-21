#include "Common.hlsl"

// =============================================================================
// SpaceVoyage.hlsl - Space Voyage Layout
// A guided tour through seven destinations, each ending in a hyperspace jump:
//   0 Ringed Giant    1 Asteroid Belt    2 Nebula Drift    3 Pulsar
//   4 Aurora World    5 Spiral Galaxy    6 Event Horizon
// Driven by main.cpp while this layout is active:
//   CustomParam6 = voyage clock (seconds, 15 s per destination, wraps every 105 s)
//   CustomParam7 = beat phase (beats, integrated from the detected BPM)
// Keep the destination order and timing in sync with Control::SpaceVoyage.
// =============================================================================

// -----------------------------------------------------------------------------
// 1. Shared Helpers: noise, camera, stars
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

// 3D value noise from the 256x256 random NoiseTexture (two z-slice lookups, period 256)
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

float Fbm3(float3 p)
{
    float v = 0.0;
    float a = 0.5;
    for (int i = 0; i < 4; ++i)
    {
        v += a * Noise3(p);
        p = p * 2.0 + float3(1.7, 9.2, 3.1);
        a *= 0.5;
    }
    return v;
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

float3 CameraRay(float3 ro, float3 ta, float2 uv, float focal)
{
    float3 ww = normalize(ta - ro);
    float3 uu = normalize(cross(float3(0.0, 1.0, 0.0), ww));
    float3 vv = cross(ww, uu);
    return normalize(uv.x * uu + uv.y * vv + focal * ww);
}

// Ray-sphere intersection: (tNear, tFar), or (-1, -1) on a miss
float2 SphereHit(float3 ro, float3 rd, float3 c, float r)
{
    float3 oc = ro - c;
    float b = dot(oc, rd);
    float h = b * b - (dot(oc, oc) - r * r);
    if (h < 0.0) return float2(-1.0, -1.0);
    h = sqrt(h);
    return float2(-b - h, -b + h);
}

// Closest approach between the view ray and an axis segment: (distance, distance along axis)
float2 RayAxisClosest(float3 ro, float3 rd, float3 axisOrigin, float3 axisDir, float axisLen)
{
    float3 w0 = ro - axisOrigin;
    float b = dot(rd, axisDir);
    float d = dot(rd, w0);
    float e = dot(axisDir, w0);
    float denom = 1.0 - b * b;
    float s = (denom > 0.00001) ? (e - b * d) / denom : 0.0;
    s = clamp(s, 0.0, axisLen);
    float t = max(dot(axisOrigin + axisDir * s - ro, rd), 0.0);
    return float2(length(ro + rd * t - (axisOrigin + axisDir * s)), s);
}

float3 StarLayer(float3 rd, float scale, float density, float pixelAngle)
{
    float3 p = rd * scale;
    float3 id = floor(p);
    float3 h = Hash33(id);
    if (h.x > density) return float3(0.0, 0.0, 0.0);

    float3 h2 = Hash33(id + 17.31);
    float d = length(p - (id + 0.15 + h2 * 0.7));
    float core = saturate(1.0 - d / (pixelAngle * scale * 1.1));
    float mag = 0.25 + pow(h.y, 8.0) * 5.0;
    float twinkle = 0.75 + 0.25 * sin(Time * (2.0 + h.z * 5.0) + h.y * 40.0);
    float3 tint = lerp(lerp(float3(1.0, 0.72, 0.5), float3(0.62, 0.78, 1.0), h2.x), float3(1.0, 1.0, 1.0), 0.45);
    return tint * core * core * mag * twinkle;
}

// Deep-space backdrop: Milky Way band with dust lanes plus three star layers
float3 SpaceBackground(float3 rd, float pixelAngle)
{
    float bd = dot(rd, normalize(float3(0.35, 1.0, 0.25)));
    float band = exp(-bd * bd * 14.0);
    float glow = Fbm3(rd * 5.0 + 3.0);

    float3 col = float3(0.004, 0.005, 0.012);
    col += float3(0.32, 0.28, 0.42) * band * (0.1 + 0.55 * glow * glow);
    col *= 1.0 - smoothstep(0.45, 0.62, Fbm3(rd * 9.0 + 11.0)) * band * 0.8;

    col += StarLayer(rd, 70.0, 0.22 + band * 0.1, pixelAngle);
    col += StarLayer(rd, 130.0, 0.3 + band * 0.15, pixelAngle) * 0.7;
    col += StarLayer(rd, 220.0, 0.35 + band * 0.2, pixelAngle) * 0.5;
    return col * (0.85 + AudioTreble * 0.35);
}

float3 SunGlare(float3 rd, float3 sunDir, float3 sunCol)
{
    float c = saturate(dot(rd, sunDir));
    return sunCol * (pow(c, 3000.0) * 8.0 + pow(c, 120.0) * 0.5 + pow(c, 10.0) * 0.05);
}

// Glowing motes on world-anchored depth slices (z = const), so they stream past with parallax
// as the camera flies forward (+Z). Each mote wobbles with the kick, jumps outward on the beat,
// pulses with its own slice of the spectrum, and the whole field spirals faster when it's loud.
float3 DancingMotes(float3 ro, float3 rd, float tMax, float3 tintA, float3 tintB, float pixelAngle)
{
    float3 col = float3(0.0, 0.0, 0.0);
    if (rd.z < 0.05) return col;

    float spacing = 2.5;
    float z0 = floor(ro.z / spacing) * spacing;
    for (int i = 1; i <= 14; ++i)
    {
        float zPlane = z0 + float(i) * spacing;
        float t = (zPlane - ro.z) / rd.z;
        if (t > tMax) break;

        float swirl = zPlane * 0.04 + EnergyAccumulator * 0.01;
        float2 q = Rotate2D(ro.xy + rd.xy * t, swirl);
        float2 axis = Rotate2D(ro.xy, swirl);
        float2 cell = floor(q / 1.6);
        float3 h = Hash33(float3(cell, zPlane * 0.37 + 11.0));
        if (h.x > 0.45) continue;

        float2 base = (cell + 0.5) * 1.6;
        float2 offset = (h.yz - 0.5) * 0.4;
        offset += float2(sin(Time * (1.0 + h.y * 2.0) + h.z * 20.0), cos(Time * (1.3 + h.z * 1.5) + h.y * 20.0)) * 0.2 * (1.0 + AudioKick * 0.5);
        offset += normalize(base - axis + 0.0001) * BeatPulse * 0.3 * SafetyDamping;
        offset *= min(1.0, 0.68 / max(length(offset), 0.0001)); // stay inside the cell

        float spec = GetAudioFFT(0.02 + h.y * 0.5);
        float r = max((0.035 + h.z * 0.04) * (1.0 + spec * 0.8), t * pixelAngle * 1.2);
        float d = length(q - base - offset);
        float glow = exp(-d * d / (r * r)) + exp(-d / (r * 3.0)) * 0.25;
        float fade = smoothstep(0.5, 4.0, t) * exp(-t * 0.045);
        col += lerp(tintA, tintB, h.y) * glow * fade * (0.45 + spec * 1.8);
    }
    return col * 0.7;
}

// -----------------------------------------------------------------------------
// 2. Destination 0: Ringed Giant (the rings double as a live spectrum EQ)
// -----------------------------------------------------------------------------
float RingDensity(float r)
{
    float inside = smoothstep(4.1, 4.4, r) * (1.0 - smoothstep(7.4, 7.7, r));
    float bands = 0.45 + 0.55 * Noise3(float3(r * 7.0, 0.5, 0.5));
    bands *= 0.7 + 0.3 * Noise3(float3(r * 31.0, 3.5, 1.5));
    float cassini = 1.0 - exp(-Sq((r - 6.05) / 0.12)) * 0.92;
    float encke = 1.0 - exp(-Sq((r - 7.05) / 0.04)) * 0.8;
    float innerFaint = lerp(0.35, 1.0, smoothstep(4.4, 5.0, r));
    return inside * bands * cassini * encke * innerFaint;
}

// The whole ring system breathes outward with the kick, sub-bass and every beat, then settles back
float RingPulseScale()
{
    return 1.0 + AudioKick * 0.06 + AudioSubBass * 0.04 + BeatPulse * 0.05 * SafetyDamping;
}

float3 ShadeGiant(float3 pos, float3 rd, float3 ringN, float3 sunDir, float3 sunCol)
{
    float3 nrm = normalize(pos);
    float lat = dot(nrm, ringN);
    float turb = Fbm3(nrm * 4.0 + float3(0.0, 0.0, Time * 0.01));
    float bandCoord = lat * 9.0 + turb * 1.4;
    float b = 0.5 + 0.5 * sin(bandCoord * 3.14159);
    float3 albedo = lerp(float3(0.62, 0.45, 0.3), float3(0.86, 0.74, 0.55), b);
    albedo = lerp(albedo, float3(0.96, 0.9, 0.74), smoothstep(0.6, 1.0, Noise3(float3(bandCoord * 2.0, 0.5, 2.5))));

    // Great storm oval in the southern belt
    float3 stormDir = normalize(float3(0.6, -0.35, 0.72));
    float storm = smoothstep(0.985, 0.995, dot(nrm, stormDir) + turb * 0.01);
    albedo = lerp(albedo, float3(0.85, 0.42, 0.25), storm * 0.8);

    float ndl = dot(nrm, sunDir);
    float diff = smoothstep(-0.05, 0.6, ndl);

    // Ring shadow cast across the cloud tops
    float ringShadow = 1.0;
    float dn = dot(sunDir, ringN);
    if (abs(dn) > 0.0001)
    {
        float ts = -dot(pos, ringN) / dn;
        if (ts > 0.0) ringShadow = 1.0 - RingDensity(length(pos + sunDir * ts) / RingPulseScale()) * 0.75;
    }

    float3 col = albedo * sunCol * diff * ringShadow + albedo * 0.012;
    float rim = pow(1.0 - saturate(dot(nrm, -rd)), 4.0);
    col += float3(0.35, 0.55, 1.0) * rim * saturate(ndl + 0.3) * (0.5 + AudioSubBass * 0.6);
    return col;
}

float3 RenderRingedGiant(float2 uv, float p, float focal, float dolly, float pixelAngle)
{
    float az = 0.35 + p * 0.022;
    float dist = 15.5 - p * 0.1;
    float3 ro = float3(sin(az) * dist, 1.2 + p * 0.025, cos(az) * dist);
    float3 ta = float3(-1.2 + p * 0.025, -0.2, 0.0);
    ro += normalize(ta - ro) * dolly;
    float3 rd = CameraRay(ro, ta, uv, focal);

    float3 ringN = normalize(float3(0.18, 1.0, -0.12));
    float3 sunDir = normalize(float3(-0.75, 0.28, 0.6));
    float3 sunCol = float3(1.0, 0.94, 0.85);

    float3 col = SpaceBackground(rd, pixelAngle) + SunGlare(rd, sunDir, sunCol);
    float tOpaque = 100000.0;

    // Small cratered moon on a slow orbit
    float mAng = 2.1 + p * 0.035;
    float3 moonC = float3(cos(mAng) * 11.0, 0.9, sin(mAng) * 11.0);
    float2 hm = SphereHit(ro, rd, moonC, 0.45);
    if (hm.x > 0.0)
    {
        tOpaque = hm.x;
        float3 mn = normalize(ro + rd * hm.x - moonC);
        float alb = 0.35 + 0.35 * Fbm3(mn * 6.0);
        col = float3(0.9, 0.88, 0.85) * alb * saturate(dot(mn, sunDir));
    }

    float2 hp = SphereHit(ro, rd, float3(0.0, 0.0, 0.0), 3.0);
    if (hp.x > 0.0 && hp.x < tOpaque)
    {
        tOpaque = hp.x;
        col = ShadeGiant(ro + rd * hp.x, rd, ringN, sunDir, sunCol);
    }

    // Rings in front of the planet / moon
    float denom = dot(rd, ringN);
    if (abs(denom) > 0.0001)
    {
        float tr = -dot(ro, ringN) / denom;
        if (tr > 0.0 && tr < tOpaque)
        {
            float3 q = ro + rd * tr;
            float r = length(q) / RingPulseScale(); // ring-local radius, so the rings pulse in and out
            float dens = RingDensity(r);
            if (dens > 0.001)
            {
                float2 sh = SphereHit(q, sunDir, float3(0.0, 0.0, 0.0), 3.0);
                float lit = (sh.x > 0.0) ? 0.06 : 1.0;
                float scatter = 0.55 + 0.45 * pow(saturate(dot(rd, sunDir)), 3.0);
                float3 ringCol = lerp(float3(0.75, 0.66, 0.52), float3(0.95, 0.9, 0.8), Noise3(float3(r * 3.0, 1.5, 0.5)));
                ringCol *= lit * scatter * 1.2;

                // Spectrum EQ: ring radius maps to frequency (inner = bass, outer = highs)
                float spec = GetAudioFFT(0.02 + saturate((r - 4.1) / 3.6) * 0.55);
                ringCol += ColorSecondary.rgb * spec * spec * 0.9;

                // Bright ripple racing outward through the rings on every beat
                float ripple = exp(-Sq((r - 4.1 - (1.0 - BeatPulse) * 3.6) / 0.12)) * BeatPulse * SafetyDamping;
                ringCol += ColorSecondary.rgb * ripple * 1.2;

                col = lerp(col, ringCol, saturate(dens * 0.9));
            }
        }
    }
    return col;
}

// -----------------------------------------------------------------------------
// 3. Destination 1: Asteroid Belt (tumbling rocks that each dance to a spectrum band)
// -----------------------------------------------------------------------------

// One rock per 8x8x8 cell (about 55% of cells), kept clear of the flight corridor.
// Returns rock center (xyz) and radius (w); w = 0 when the cell is empty.
float4 AsteroidCell(float3 cell)
{
    float3 h = Hash33(cell * 1.17 + 17.0);
    if (h.x > 0.55) return float4(0.0, 0.0, 0.0, 0.0);

    float spec = GetAudioFFT(0.02 + h.z * 0.5);
    float radius = (0.7 + h.y * 0.8) * (1.0 + spec * 0.25 + AudioKick * 0.06);
    float3 h2 = Hash33(cell * 2.31 + 5.0);
    float3 center = (cell + 0.5) * 8.0 + (h2 - 0.5) * 1.2;
    center.y += BeatPulse * 0.4 * SafetyDamping * (h2.x - 0.5) * 2.0; // hop on the beat
    if (length(center.xy) < radius + 3.2) return float4(0.0, 0.0, 0.0, 0.0);
    return float4(center, radius);
}

// Lumpy potato SDF in the rock's tumbling frame (q = rock-local position)
float AsteroidRockSDF(float3 p, float3 cell, float4 rock, out float3 q)
{
    float3 h = Hash33(cell * 3.7 + 1.0);
    float a1 = Time * (0.15 + h.x * 0.35) + EnergyAccumulator * 0.01 * h.y + h.z * 6.0;
    float a2 = Time * (0.1 + h.y * 0.25) + h.x * 6.0;
    q = RotY(RotZ(p - rock.xyz, a1), a2);

    float d = length(q / float3(1.0, 0.72 + h.z * 0.2, 0.85)) - rock.w;
    float3 np = q / rock.w;
    d += (Noise3(np * 1.6 + h * 40.0) - 0.5) * 0.55 * rock.w;
    d += (Noise3(np * 4.3 + h * 17.0) - 0.5) * 0.18 * rock.w;
    return d * 0.7;
}

float TraceAsteroids(float3 ro, float3 rd, float tMax, out float3 hitCell, out float4 hitRock)
{
    hitCell = float3(0.0, 0.0, 0.0);
    hitRock = float4(0.0, 0.0, 0.0, 0.0);
    float3 invRd = 1.0 / float3(abs(rd.x) > 0.00001 ? rd.x : 0.00001, abs(rd.y) > 0.00001 ? rd.y : 0.00001, abs(rd.z) > 0.00001 ? rd.z : 0.00001);
    float3 cellStep = step(float3(0.0, 0.0, 0.0), rd) * 8.0;

    float t = 0.3;
    for (int i = 0; i < 90; ++i)
    {
        float3 p = ro + rd * t;
        float3 cell = floor(p / 8.0);
        float4 rock = AsteroidCell(cell);

        // Never step past the cell boundary, so a rock in the next cell can't be skipped
        float3 tb = (cell * 8.0 + cellStep - p) * invRd;
        float d = min(min(tb.x, tb.y), tb.z) + 0.02;
        if (rock.w > 0.0)
        {
            float bound = length(p - rock.xyz) - rock.w * 1.45;
            if (bound > 0.3)
            {
                d = min(d, bound);
            }
            else
            {
                float3 q;
                float dr = AsteroidRockSDF(p, cell, rock, q);
                if (dr < 0.002 * t + 0.001)
                {
                    hitCell = cell;
                    hitRock = rock;
                    return t;
                }
                d = min(d, dr);
            }
        }
        t += max(d, 0.01);
        if (t > tMax) break;
    }
    return tMax;
}

float3 ShadeAsteroid(float3 p, float3 rd, float3 cell, float4 rock, float3 sunDir, float3 sunCol)
{
    float3 q;
    float e = 0.01 * rock.w;
    float3 n = normalize(
        float3( 1.0, -1.0, -1.0) * AsteroidRockSDF(p + float3( e, -e, -e), cell, rock, q) +
        float3(-1.0, -1.0,  1.0) * AsteroidRockSDF(p + float3(-e, -e,  e), cell, rock, q) +
        float3(-1.0,  1.0, -1.0) * AsteroidRockSDF(p + float3(-e,  e, -e), cell, rock, q) +
        float3( 1.0,  1.0,  1.0) * AsteroidRockSDF(p + float3( e,  e,  e), cell, rock, q));
    AsteroidRockSDF(p, cell, rock, q);

    float3 h = Hash33(cell * 1.17 + 17.0);
    float spec = GetAudioFFT(0.02 + h.z * 0.5);
    float3 np = q / rock.w;
    float3 albedo = lerp(float3(0.3, 0.27, 0.24), float3(0.52, 0.46, 0.4), Noise3(np * 2.0 + 9.0));
    albedo *= 1.0 - smoothstep(0.55, 0.7, Noise3(np * 6.0 + 3.0)) * 0.45; // crater shading

    float diff = saturate(dot(n, sunDir));
    float rim = pow(1.0 - saturate(dot(n, -rd)), 3.0);
    float3 col = albedo * (sunCol * diff * 1.1 + 0.03);
    col += ColorSecondary.rgb * rim * (0.3 + AudioSubBass * 0.9 + BeatPulse * 0.3 * SafetyDamping);

    // Glowing veins pulse with the rock's own frequency band
    float veins = pow(saturate(1.0 - abs(Noise3(np * 3.0 + 21.0) * 2.0 - 1.0)), 14.0);
    col += ColorPrimary.rgb * veins * spec * spec * 1.8;
    return col;
}

float3 RenderAsteroidBelt(float2 uv, float p, float focal, float dolly, float pixelAngle)
{
    float z = p * 3.0 + EnergyAccumulator * 0.25 + dolly;
    float3 ro = float3(sin(p * 0.11) * 2.0, cos(p * 0.08) * 1.2, z);
    float3 ta = ro + float3(sin(p * 0.11 + 0.5) * 0.8, cos(p * 0.08 + 0.4) * 0.5 - 0.1, 6.0);
    float3 rd = CameraRay(ro, ta, uv, focal);

    // Sun off to the side and slightly behind, so the rocks we fly toward are lit
    float3 sunDir = normalize(float3(-0.75, 0.4, -0.35));
    float3 sunCol = float3(1.0, 0.92, 0.8);
    float3 col = SpaceBackground(rd, pixelAngle) + SunGlare(rd, sunDir, sunCol);
    col += float3(0.5, 0.4, 0.3) * exp(-abs(rd.y) * 6.0) * 0.04; // dusty glow along the belt plane

    // Banded gas giant hanging far behind the belt
    float3 giantC = ro + float3(-60.0, 12.0, 160.0);
    float2 hg = SphereHit(ro, rd, giantC, 38.0);
    if (hg.x > 0.0)
    {
        float3 pn = normalize(ro + rd * hg.x - giantC);
        float b = 0.5 + 0.5 * sin((pn.y * 8.0 + Fbm3(pn * 3.0) * 1.2) * 3.14159);
        float3 alb = lerp(float3(0.55, 0.42, 0.32), float3(0.85, 0.75, 0.6), b);
        col = alb * smoothstep(-0.05, 0.6, dot(pn, sunDir)) * sunCol + alb * 0.01;
        col += float3(0.35, 0.55, 1.0) * pow(1.0 - saturate(dot(pn, -rd)), 4.0) * 0.4;
    }

    float3 cell;
    float4 rock;
    float tHit = TraceAsteroids(ro, rd, 110.0, cell, rock);
    if (tHit < 110.0)
    {
        float3 rockCol = ShadeAsteroid(ro + rd * tHit, rd, cell, rock, sunDir, sunCol);
        col = lerp(rockCol, col, 1.0 - exp(-tHit * 0.025));
    }

    col += DancingMotes(ro, rd, min(tHit, 60.0), float3(1.0, 0.85, 0.6), ColorSecondary.rgb, pixelAngle);
    return col;
}

// -----------------------------------------------------------------------------
// 4. Destination 2: Nebula Drift (volumetric gas lit by embedded young stars)
// -----------------------------------------------------------------------------
// (glowing gas density, dark dust density)
float2 NebulaField(float3 q)
{
    float3 w = q * 0.25;
    w.xy += (float2(Noise3(w * 1.7), Noise3(w * 1.7 + 5.3)) - 0.5) * 1.2; // domain warp for wispy structure
    float gas = saturate((Fbm3(w) - 0.42) * 3.5);
    float dust = saturate((Noise3(w * 2.3 + 21.0) * 0.7 + Noise3(w * 5.1 + 3.0) * 0.3 - 0.58) * 5.0);
    return float2(gas * gas, dust);
}

float3 NebulaStarPos(float k)
{
    float3 h = Hash33(float3(k, k * 1.7, 3.3));
    return float3((h.x - 0.5) * 9.0, (h.y - 0.5) * 6.0, k * 26.0 + h.z * 10.0);
}

float3 RenderNebula(float2 uv, float p, float focal, float dolly, float pixelAngle, float2 fragCoord)
{
    // Travel = steady drift plus music energy (wrapped on the noise period so it never seams)
    float z = p * 1.5 + fmod(EnergyAccumulator * 0.12, 1024.0) + dolly;
    float3 ro = float3(sin(p * 0.05) * 3.0, cos(p * 0.037) * 2.0, z);
    float3 ta = ro + float3(sin(p * 0.05 + 0.6) * 0.6, cos(p * 0.037 + 0.4) * 0.4, 4.0);
    float3 rd = CameraRay(ro, ta, uv, focal);

    float kBase = floor(ro.z / 26.0);
    float3 s0 = NebulaStarPos(kBase);
    float3 s1 = NebulaStarPos(kBase + 1.0);
    float3 s2 = NebulaStarPos(kBase + 2.0);

    float3 hydrogen = lerp(float3(0.95, 0.15, 0.5), ColorPrimary.rgb, 0.35);
    float3 oxygen = lerp(float3(0.1, 0.6, 0.95), ColorSecondary.rgb, 0.35);
    float3 starCol = float3(0.75, 0.85, 1.0);
    float pulse = 0.65 + AudioSubBass * 0.5 + AudioKick * 0.35;

    float3 col = float3(0.0, 0.0, 0.0);
    float trans = 1.0;
    float stepLen = 0.6;
    float t = Hash21(fragCoord) * stepLen;
    for (int i = 0; i < 36; ++i)
    {
        float3 q = ro + rd * t;
        float2 field = NebulaField(q);
        float d = field.x;
        if (d > 0.002 || field.y > 0.002)
        {
            float hue = smoothstep(0.4, 0.6, Noise3(q * 0.06 + 7.0));
            float3 emis = lerp(hydrogen, oxygen, hue) * 1.4 + ColorAccent.rgb * d * d * 0.5;

            // Gas glows brighter around the embedded stars
            float starLight = exp(-length(q - s0) * 0.45) + exp(-length(q - s1) * 0.45) + exp(-length(q - s2) * 0.45);
            emis += starCol * starLight * 1.2;

            col += emis * d * pulse * trans * stepLen * 0.6;
            trans *= exp(-(d * 0.45 + field.y * 0.9) * stepLen);
            if (trans < 0.03) break;
        }
        t += stepLen * (1.0 + t * 0.035);
    }

    col += SpaceBackground(rd, pixelAngle) * trans;

    // Glowing dust motes drifting through the gas, dancing with the music
    col += DancingMotes(ro, rd, 40.0, float3(1.0, 0.75, 0.9), float3(0.6, 0.95, 1.0), pixelAngle) * (0.5 + 0.5 * trans);

    // Star cores (flare on the beat)
    float corePulse = 1.0 + BeatPulse * 0.8 * SafetyDamping;
    for (int k = 0; k < 3; ++k)
    {
        float3 sp = (k == 0) ? s0 : ((k == 1) ? s1 : s2);
        float3 v = sp - ro;
        float tc = dot(v, rd);
        if (tc > 0.0)
        {
            float dm = length(v - rd * tc) / tc;
            col += starCol * (0.00004 / (dm * dm + 0.00002) + exp(-dm * 25.0) * 0.4) * corePulse * (0.35 + 0.65 * trans);
        }
    }
    return col;
}

// -----------------------------------------------------------------------------
// 5. Destination 3: Pulsar (beams spin locked to the detected BPM)
// -----------------------------------------------------------------------------
float3 PulsarBeam(float3 ro, float3 rd, float3 axis, float pulse)
{
    float2 ca = RayAxisClosest(ro, rd, float3(0.0, 0.0, 0.0), axis, 40.0);
    float width = 0.05 + ca.y * 0.07;
    float g = exp(-Sq(ca.x / width)) * exp(-ca.y * 0.05);
    float core = exp(-Sq(ca.x / (width * 0.25))) * exp(-ca.y * 0.08);
    return (ColorSecondary.rgb * g * 0.9 + float3(0.9, 0.95, 1.0) * core * 0.7) * pulse;
}

float3 RenderPulsar(float2 uv, float p, float focal, float dolly, float pixelAngle)
{
    float az = 0.4 + p * 0.03;
    float dist = 15.0 - p * 0.06;
    float3 ro = float3(sin(az) * dist, 2.2 + sin(p * 0.05), cos(az) * dist);
    float3 ta = float3(0.0, 0.3, 0.0);
    ro += normalize(ta - ro) * dolly;
    float3 rd = CameraRay(ro, ta, uv, focal);

    // Magnetic axis tilted ~35 degrees from the spin axis, half a turn per beat
    float3 spin = normalize(float3(0.12, 1.0, 0.08));
    float3 e1 = normalize(cross(spin, float3(0.0, 0.0, 1.0)));
    float3 e2 = cross(spin, e1);
    float phase = CustomParam7 * 3.14159265;
    float3 beamDir = normalize(spin * cos(0.62) + (e1 * cos(phase) + e2 * sin(phase)) * sin(0.62));

    float3 col = SpaceBackground(rd, pixelAngle);

    // Supernova remnant: wispy filaments on a shell around the system
    float2 shell = SphereHit(ro, rd, float3(0.0, 0.0, 0.0), 30.0);
    float3 sp = ro + rd * shell.y;
    float wisps = pow(saturate(1.0 - abs(Fbm3(sp * 0.12 + 5.0) - 0.5) * 9.0), 4.0);
    float3 remnant = lerp(float3(1.0, 0.3, 0.12), float3(0.15, 0.85, 0.7), smoothstep(0.35, 0.65, Noise3(sp * 0.05)));
    col += remnant * wisps * 0.3 * (0.7 + AudioLowMid * 0.5);

    // Equatorial wind torus with a beat ripple and a drop shockwave
    float dn = dot(rd, spin);
    if (abs(dn) > 0.0001)
    {
        float tt = -dot(ro, spin) / dn;
        if (tt > 0.0)
        {
            float3 q = ro + rd * tt;
            float r = length(q);
            float torus = exp(-Sq((r - 3.2) / 0.9)) * (0.4 + 0.9 * Fbm3(q * 0.8 + Time * 0.05));
            float ripple = exp(-Sq((r - 3.0 - (1.0 - BeatPulse) * 7.0) / 0.35)) * BeatPulse;
            float shock = exp(-Sq((r - 2.0 - (1.0 - DropIntensity) * 18.0) / 0.6)) * DropIntensity;
            float3 torusCol = lerp(ColorSecondary.rgb, float3(0.5, 0.35, 1.0), 0.45);
            col += torusCol * (torus * (0.6 + AudioLowMid * 0.6) + (ripple * 0.8 + shock * 1.2) * SafetyDamping) * 0.6;
        }
    }

    // Faint polar jets along the spin axis
    float2 jetA = RayAxisClosest(ro, rd, float3(0.0, 0.0, 0.0), spin, 12.0);
    float2 jetB = RayAxisClosest(ro, rd, float3(0.0, 0.0, 0.0), -spin, 12.0);
    float jets = exp(-Sq(jetA.x / (0.1 + jetA.y * 0.12))) * exp(-jetA.y * 0.2) + exp(-Sq(jetB.x / (0.1 + jetB.y * 0.12))) * exp(-jetB.y * 0.2);
    col += float3(0.55, 0.35, 1.0) * jets * 0.35;

    float beamPulse = 0.9 + AudioKick * 1.1 + BeatPulse * 0.5 * SafetyDamping;
    col += PulsarBeam(ro, rd, beamDir, beamPulse) + PulsarBeam(ro, rd, -beamDir, beamPulse);

    // Neutron star core
    float tc = max(dot(-ro, rd), 0.0);
    float dm = length(-ro - rd * tc);
    col += float3(0.7, 0.85, 1.0) * (0.004 / (dm * dm + 0.002)) * (1.0 + BeatPulse * 0.6 * SafetyDamping);
    col += ColorSecondary.rgb * exp(-dm * 1.5) * 0.3;
    return col;
}

// -----------------------------------------------------------------------------
// 6. Destination 4: Aurora World (icy night-side surface, aurora = spectrum)
// -----------------------------------------------------------------------------
float3 AuroraCurtains(float3 ro, float3 rd, int layers)
{
    float3 col = float3(0.0, 0.0, 0.0);
    if (rd.y <= 0.01) return col;

    for (int i = 0; i < 28; ++i)
    {
        if (i >= layers) break;
        float fi = float(i) * (28.0 / float(layers));
        float h = 2.5 + fi * 0.35;
        float t = (h - ro.y) / rd.y;
        float3 q = ro + rd * t;

        // Vertical curtains: the same ridged xz pattern stacked through the height layers
        float2 flow = q.xz * 0.075 + float2(Time * 0.012, Time * 0.02);
        float n = Noise3(float3(flow * 3.0, Time * 0.04)) + 0.5 * Noise3(float3(flow * 7.0 + 3.0, Time * 0.06));
        float ridge = pow(saturate(1.0 - abs(n / 1.5 * 2.0 - 1.0)), 9.0);

        // Spectrum painted across the sky: left = bass, right = highs; louder bands rise higher
        float az = atan2(q.x - ro.x, q.z - ro.z);
        float spec = GetAudioFFT(0.02 + saturate(az * 0.4 + 0.5) * 0.5);
        float heightFade = exp(-fi / (3.0 + spec * 16.0)) * (0.6 + 0.4 * exp(-fi * 0.6));

        float3 c = lerp(float3(0.15, 1.0, 0.45), float3(0.55, 0.15, 0.85), saturate(fi / 18.0));
        col += c * ridge * heightFade * (0.3 + spec * 1.3) * exp(-t * 0.01) * 0.22;
    }
    return col * (28.0 / float(layers));
}

float3 AuroraSky(float3 ro, float3 rd, float pixelAngle, int layers)
{
    float3 col = SpaceBackground(rd, pixelAngle) * 0.8;

    // Ice giant hanging over the horizon, lit as a thin crescent
    float3 planetDir = normalize(float3(-0.6, 0.32, 1.0));
    float3 planetC = planetDir * 400.0;
    float2 hp = SphereHit(float3(0.0, 0.0, 0.0), rd, planetC, 50.0);
    if (hp.x > 0.0)
    {
        float3 pn = normalize(rd * hp.x - planetC);
        float3 sunDir = normalize(planetDir + float3(0.9, -0.5, 0.0)); // sun behind the planet: thin crescent
        float lit = smoothstep(-0.1, 0.25, dot(pn, sunDir));
        float bands = 0.6 + 0.4 * Noise3(float3(pn.y * 14.0, pn.x * 2.0, 1.0));
        col = float3(0.25, 0.65, 0.75) * bands * lit * 0.8 + float3(0.01, 0.015, 0.025);
        col += float3(0.3, 0.7, 1.0) * pow(1.0 - saturate(dot(pn, -rd)), 3.0) * 0.15;
    }

    col += AuroraCurtains(ro, rd, layers);
    return col;
}

float3 RenderAurora(float2 uv, float p, float focal, float dolly, float pixelAngle)
{
    float3 ro = float3(sin(p * 0.02) * 2.0, 1.0 + sin(p * 0.07) * 0.15, p * 0.9 + dolly);
    float3 ta = ro + float3(sin(p * 0.015) * 0.4, 0.42, 1.0);
    float3 rd = CameraRay(ro, ta, uv, focal);

    // Mountain silhouette on the horizon
    float az = atan2(rd.x, rd.z);
    float elev = rd.y / max(length(rd.xz), 0.0001);
    float mountain = 0.012 + 0.07 * Sq(Fbm3(float3(az * 3.0, 1.0, 2.0))) + 0.02 * Noise3(float3(az * 25.0, 4.0, 1.0));

    float3 col;
    if (rd.y >= 0.0 && elev > mountain)
    {
        col = AuroraSky(ro, rd, pixelAngle, 28);
    }
    else if (rd.y >= 0.0)
    {
        // Mountains catch a faint aurora rim
        col = float3(0.004, 0.006, 0.012) + float3(0.1, 0.6, 0.35) * exp(-(mountain - elev) * 60.0) * 0.12;
    }
    else
    {
        // Glassy ice: reflect the sky with a slight ripple
        float t = -ro.y / rd.y;
        float3 q = ro + rd * t;
        float ice = Fbm3(float3(q.xz * 0.5, 0.0));
        float cracks = pow(saturate(1.0 - abs(Noise3(float3(q.xz * 1.5, 2.0)) * 2.0 - 1.0)), 12.0);
        float3 rr = normalize(float3(rd.x + (ice - 0.5) * 0.04, -rd.y, rd.z + (ice - 0.5) * 0.04));
        float fresnel = 0.25 + 0.6 * pow(1.0 - abs(rd.y), 4.0);
        col = float3(0.01, 0.018, 0.03) * (0.6 + ice) + AuroraSky(q, rr, pixelAngle, 14) * fresnel;
        col += float3(0.4, 0.8, 1.0) * cracks * 0.05 * (0.6 + AudioTreble);
        col = lerp(col, float3(0.02, 0.05, 0.05), 1.0 - exp(-t * 0.02));
    }
    return col;
}

// -----------------------------------------------------------------------------
// 7. Destination 5: Spiral Galaxy (beat ripple runs out through the arms)
// -----------------------------------------------------------------------------
float3 RenderGalaxy(float2 uv, float p, float focal, float dolly, float pixelAngle)
{
    float az = 0.8 + p * 0.025;
    float elev = 0.55 + p * 0.006;
    float dist = 25.0 - p * 0.15;
    float3 ro = float3(cos(elev) * sin(az), sin(elev), cos(elev) * cos(az)) * dist;
    float3 ta = float3(0.0, -1.0, 0.0);
    ro += normalize(ta - ro) * dolly;
    float3 rd = CameraRay(ro, ta, uv, focal);

    float3 col = SpaceBackground(rd, pixelAngle) * 0.8;
    float rot = Time * 0.035 + EnergyAccumulator * 0.002;

    if (abs(rd.y) > 0.0001)
    {
        float t = -ro.y / rd.y;
        if (t > 0.0)
        {
            float2 d2 = (ro + rd * t).xz;
            float r = length(d2);
            float theta = atan2(d2.y, d2.x);
            float spiral = theta + rot - log(r + 0.4) * 2.6;
            float arms = pow(0.5 + 0.5 * cos(2.0 * spiral), 3.0);

            // Texture co-rotates with the arms
            float2 rq = Rotate2D(d2, rot);
            float clumps = Fbm3(float3(rq * 0.9, 1.3));
            float disk = exp(-r / 4.5) * smoothstep(16.0, 9.0, r);
            float dust = pow(0.5 + 0.5 * cos(2.0 * (spiral + 0.38)), 6.0) * smoothstep(1.5, 4.0, r);

            float3 armCol = lerp(float3(0.55, 0.7, 1.0), float3(1.0, 0.95, 0.9), exp(-r * 0.25));
            float spec = GetAudioFFT(0.02 + saturate(r / 14.0) * 0.5);
            float3 em = armCol * disk * (0.25 + arms * (1.2 + clumps * 1.5)) * (0.75 + spec * 0.8);

            // Pink star-forming knots along the arms
            float knots = smoothstep(0.62, 0.75, Noise3(float3(rq * 3.5, 4.0))) * arms;
            em += float3(1.0, 0.3, 0.55) * knots * disk * 2.0 * (0.7 + AudioHighMid * 0.8);
            em *= 1.0 - dust * 0.75;

            float ripple = exp(-Sq((r - (1.0 - BeatPulse) * 14.0) / 0.6)) * BeatPulse * SafetyDamping;
            em += ColorSecondary.rgb * ripple * exp(-r / 8.0) * 0.6;

            float path = 1.0 / max(abs(rd.y), 0.25);
            col += em * path * 0.6;
        }
    }

    // Bulge: warm volumetric glow around the core
    float tc = max(dot(-ro, rd), 0.0);
    float dm = length(-ro - rd * tc);
    col += float3(1.0, 0.82, 0.55) * (exp(-dm * 0.9) * 1.2 + exp(-dm * 0.25) * 0.12) * (0.8 + AudioSubBass * 0.6);
    return col;
}

// -----------------------------------------------------------------------------
// 8. Destination 6: Event Horizon (lensing black hole, kick-driven disk)
// -----------------------------------------------------------------------------
float4 AccretionDisk(float3 xc, float3 v, float r)
{
    float tRot = Time * 0.5 + EnergyAccumulator * 0.04;
    float omega = 3.0 * pow(r, -1.5);
    float2 pr = Rotate2D(xc.xz, -omega * tRot);
    float n = Fbm3(float3(pr * 1.6, r * 0.5));
    float streaks = Noise3(float3(r * 9.0, n * 4.0, 1.0));

    float temp = saturate((9.0 - r) / 6.4);
    float3 c = lerp(float3(0.75, 0.18, 0.06), lerp(float3(1.0, 0.55, 0.18), float3(1.0, 0.95, 0.85), smoothstep(0.55, 1.0, temp)), smoothstep(0.0, 0.6, temp));
    float dens = (0.35 + 0.9 * n * streaks) * smoothstep(2.6, 3.2, r) * (1.0 - smoothstep(7.5, 9.0, r));

    // Doppler beaming: the side orbiting toward the camera is brighter
    float3 tangent = normalize(float3(-xc.z, 0.0, xc.x));
    float beaming = pow(saturate(1.0 + 0.55 * dot(tangent, -v)), 3.0);
    float pulse = 0.8 + AudioKick * 0.8 + BeatPulse * 0.35 * SafetyDamping;
    return float4(c * beaming * pulse * 2.2, saturate(dens * 0.9));
}

float3 RenderBlackHole(float2 uv, float p, float focal, float dolly, float pixelAngle)
{
    float az = 0.2 + p * 0.02;
    float dist = 16.0 - p * 0.08;
    float elev = 0.12 + sin(p * 0.04) * 0.05;
    float3 ro = float3(sin(az) * cos(elev), sin(elev), cos(az) * cos(elev)) * dist;
    float3 ta = float3(0.0, 0.0, 0.0);
    ro += normalize(ta - ro) * dolly;
    float3 rd = CameraRay(ro, ta, uv, focal);

    // Bend the ray around a Schwarzschild hole (r_s = 1, photon sphere at 1.5)
    float3 x = ro;
    float3 v = rd;
    float3 hv = cross(x, v);
    float h2 = dot(hv, hv);
    float3 col = float3(0.0, 0.0, 0.0);
    float alpha = 0.0;
    float ring = 0.0;
    bool captured = false;

    for (int i = 0; i < 140; ++i)
    {
        float r = length(x);
        if (r < 1.0)
        {
            captured = true;
            break;
        }
        float dt = clamp(0.06 * r, 0.03, 0.8);
        float3 xPrev = x;
        v += -1.5 * h2 * x / pow(r, 5.0) * dt;
        x += v * dt;

        if (xPrev.y * x.y < 0.0)
        {
            float3 xc = lerp(xPrev, x, xPrev.y / (xPrev.y - x.y));
            float rc = length(xc.xz);
            if (rc > 2.4 && rc < 9.5)
            {
                float4 dc = AccretionDisk(xc, normalize(v), rc);
                col += (1.0 - alpha) * dc.rgb * dc.a;
                alpha += (1.0 - alpha) * dc.a;
            }
        }

        ring += exp(-abs(r - 1.5) * 10.0) * dt;
        if (alpha > 0.98) break;
        if (r > 40.0 && dot(x, v) > 0.0) break;
    }

    if (!captured) col += (1.0 - alpha) * SpaceBackground(normalize(v), pixelAngle);
    col += float3(1.0, 0.7, 0.4) * ring * 0.08 * (0.8 + AudioSubBass * 0.8) * (1.0 - alpha);
    return col;
}

// -----------------------------------------------------------------------------
// 9. Hyperspace Jump (between destinations)
// -----------------------------------------------------------------------------
float3 Hyperspace(float2 uv, float amount)
{
    float r = length(uv);
    float a = atan2(uv.y, uv.x) / 6.2831853 + 0.5;
    float px = 2.0 / Resolution.y;
    float3 col = float3(0.0, 0.0, 0.0);

    for (int layer = 0; layer < 3; ++layer)
    {
        float lanes = 150.0 + float(layer) * 120.0;
        float lane = floor(a * lanes);
        float h1 = Hash11(lane * 3.71 + float(layer) * 17.3);
        float h2 = Hash11(lane * 1.37 + float(layer) * 5.1 + 2.0);

        float lateral = abs(frac(a * lanes) - 0.5) * (6.2831853 / lanes) * r;
        float lineMask = saturate(1.0 - lateral / (px * 1.2));

        // Streak heads race outward; tails stretch as the jump builds
        float cycle = frac(h2 + Time * (0.6 + h1 * 1.2));
        float rHead = cycle * cycle * 2.4;
        float len = 0.02 + amount * amount * 1.4 * rHead;
        float inStreak = step(rHead - len, r) * step(r, rHead);
        float tail = inStreak * pow(saturate(1.0 - (rHead - r) / len), 1.5);

        float3 sc = lerp(float3(0.75, 0.85, 1.0), ColorSecondary.rgb, h1 * 0.6);
        col += sc * tail * lineMask * step(0.25, h1) * (0.8 + 0.6 * h2);
    }

    // Tunnel haze plus a soft white bloom at the moment of the jump
    float3 haze = lerp(ColorSecondary.rgb, float3(0.5, 0.3, 1.0), 0.5) * exp(-r * 1.6) * 0.35;
    float bloom = pow(amount, 6.0) * exp(-r * 1.2) * 0.9 * SafetyDamping;
    return (col * 1.6 * (0.8 + AudioKick * 0.4) + haze) * amount + float3(0.9, 0.95, 1.0) * bloom;
}

// -----------------------------------------------------------------------------
// 10. Main Pixel Shader
// -----------------------------------------------------------------------------
float4 PSMain(VSOutput input) : SV_Target
{
    float2 uv = input.UV * 2.0 - 1.0;
    uv.x *= AspectRatio;

    // Leg length and jump timing must match Control::SpaceVoyage (kLegSeconds / kWarpSeconds)
    const float kLegSeconds = 15.0;
    float clock = CustomParam6;
    float leg = floor(clock / kLegSeconds);
    float p = clock - leg * kLegSeconds;
    int dest = int(fmod(leg, 7.0) + 0.5);

    // Jump out over the last 3 s of a leg, arrive over the first 1.5 s of the next
    float warpOut = smoothstep(kLegSeconds - 3.0, kLegSeconds - 0.2, p);
    float warpIn = 1.0 - smoothstep(0.0, 1.5, p);
    float warp = max(warpOut, warpIn);
    float dolly = warpOut * warpOut * 7.0 - warpIn * warpIn * 7.0;
    float focal = 1.6 + warp * 0.8;
    float pixelAngle = 2.0 / (Resolution.y * focal);

    float3 col;
    if (dest == 0) col = RenderRingedGiant(uv, p, focal, dolly, pixelAngle);
    else if (dest == 1) col = RenderAsteroidBelt(uv, p, focal, dolly, pixelAngle);
    else if (dest == 2) col = RenderNebula(uv, p, focal, dolly, pixelAngle, input.Position.xy);
    else if (dest == 3) col = RenderPulsar(uv, p, focal, dolly, pixelAngle);
    else if (dest == 4) col = RenderAurora(uv, p, focal, dolly, pixelAngle);
    else if (dest == 5) col = RenderGalaxy(uv, p, focal, dolly, pixelAngle);
    else col = RenderBlackHole(uv, p, focal, dolly, pixelAngle);

    if (warp > 0.001)
    {
        col = col * (1.0 - warp * 0.75) + Hyperspace(uv, warp);
    }

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
